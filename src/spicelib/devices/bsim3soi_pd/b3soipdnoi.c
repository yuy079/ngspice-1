/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1998 Samuel Fung, Dennis Sinitsky and Stephen Tang
File: b3soipdnoi.c          98/5/01
Modified by Hui Wan 02/3/5
Modified by Paolo Nenzi 2002
**********/

/*
 * Revision 2.2.3  02/3/5  Pin Su 
 * BSIMPD2.2.3 release
 */

#include <ngspice/ngspice.h>
#include "b3soipddef.h"
#include <ngspice/cktdefs.h>
#include <ngspice/iferrmsg.h>
#include <ngspice/noisedef.h>
#include <ngspice/suffix.h>
#include <ngspice/const.h>  /* jwan */

/*
 * B3SOIPDnoise (mode, operation, firstModel, ckt, data, OnDens)
 *    This routine names and evaluates all of the noise sources
 *    associated with MOSFET's.  It starts with the model *firstModel and
 *    traverses all of its insts.  It then proceeds to any other models
 *    on the linked list.  The total output noise density generated by
 *    all of the MOSFET's is summed with the variable "OnDens".
 */

/*
 Channel thermal and flicker noises are calculated based on the value
 of model->B3SOIPDnoiMod.
 If model->B3SOIPDnoiMod = 1,
    Channel thermal noise = SPICE2 model
    Flicker noise         = SPICE2 model
 If model->B3SOIPDnoiMod = 2,
    Channel thermal noise = B3SOIPD model
    Flicker noise         = B3SOIPD model
 If model->B3SOIPDnoiMod = 3,
    Channel thermal noise = SPICE2 model
    Flicker noise         = B3SOIPD model
 If model->B3SOIPDnoiMod = 4,
    Channel thermal noise = B3SOIPD model
    Flicker noise         = SPICE2 model
 */


static double
B3SOIPDStrongInversionNoiseEval(double vgs, double vds, B3SOIPDmodel *model, 
                                B3SOIPDinstance *here, double freq, 
				double temp)
{
struct b3soipdSizeDependParam *pParam;
double cd, esat, DelClm, EffFreq, N0, Nl;
double T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, Ssi;

    NG_IGNORE(vgs);

    pParam = here->pParam;
    cd = fabs(here->B3SOIPDcd) * here->B3SOIPDm;

/* v2.2.3 bug fix */
    if(model->B3SOIPDem<=0.0) DelClm = 0.0;
    else {
    	    esat = 2.0 * pParam->B3SOIPDvsattemp / here->B3SOIPDueff;
            T0 = ((((vds - here->B3SOIPDVdseff) / pParam->B3SOIPDlitl)
                + model->B3SOIPDem) / esat);
            DelClm = pParam->B3SOIPDlitl * log (MAX(T0, N_MINLOG));
    }


    EffFreq = pow(freq, model->B3SOIPDef);
    T1 = CHARGE * CHARGE * 8.62e-5 * cd * temp * here->B3SOIPDueff;
    T2 = 1.0e8 * EffFreq * model->B3SOIPDcox
       * pParam->B3SOIPDleff * pParam->B3SOIPDleff;

/* v2.2.3 bug fix */
    N0 = model->B3SOIPDcox * here->B3SOIPDVgsteff / CHARGE; 
    Nl = model->B3SOIPDcox * here->B3SOIPDVgsteff
         * (1.0 - here->B3SOIPDAbovVgst2Vtm * here->B3SOIPDVdseff) / CHARGE; 


    T3 = model->B3SOIPDoxideTrapDensityA
       * log(MAX(((N0 + 2.0e14) / (Nl + 2.0e14)), N_MINLOG));
    T4 = model->B3SOIPDoxideTrapDensityB * (N0 - Nl);
    T5 = model->B3SOIPDoxideTrapDensityC * 0.5 * (N0 * N0 - Nl * Nl);

    T6 = 8.62e-5 * temp * cd * cd;
    T7 = 1.0e8 * EffFreq * pParam->B3SOIPDleff
       * pParam->B3SOIPDleff * pParam->B3SOIPDweff * here->B3SOIPDm;
    T8 = model->B3SOIPDoxideTrapDensityA + model->B3SOIPDoxideTrapDensityB * Nl
       + model->B3SOIPDoxideTrapDensityC * Nl * Nl;
    T9 = (Nl + 2.0e14) * (Nl + 2.0e14);

    Ssi = T1 / T2 * (T3 + T4 + T5) + T6 / T7 * DelClm * T8 / T9;

    return Ssi;
}

int
B3SOIPDnoise (int mode, int operation, GENmodel *inModel, CKTcircuit *ckt, 
              Ndata *data, double *OnDens)
{
#define job ((NOISEAN*)ckt->CKTcurJob)

B3SOIPDmodel *model = (B3SOIPDmodel *)inModel;
B3SOIPDinstance *here;
struct b3soipdSizeDependParam *pParam;
char name[N_MXVLNTH];
double tempOnoise;
double tempInoise;
double noizDens[B3SOIPDNSRCS];
double lnNdens[B3SOIPDNSRCS];

double vgs, vds, Slimit;
double T1, T10, T11;
double Ssi, Swi;

int i;

    /* define the names of the noise sources */
    static char *B3SOIPDnNames[B3SOIPDNSRCS] =
    {   /* Note that we have to keep the order */
	".rd",              /* noise due to rd */
			    /* consistent with the index definitions */
	".rs",              /* noise due to rs */
			    /* in B3SOIPDdefs.h */
	".id",              /* noise due to id */
	".1overf",          /* flicker (1/f) noise */
        ".fb", 		    /* noise due to floating body */
	""                  /* total transistor noise */
    };

    for (; model != NULL; model = model->B3SOIPDnextModel)
    {    for (here = model->B3SOIPDinstances; here != NULL;
	      here = here->B3SOIPDnextInstance)
	 {    
	 
	      if (here->B3SOIPDowner != ARCHme)
	              continue;
		      
	      pParam = here->pParam;
	      switch (operation)
	      {  case N_OPEN:
		     /* see if we have to to produce a summary report */
		     /* if so, name all the noise generators */

		      if (job->NStpsSm != 0)
		      {   switch (mode)
			  {  case N_DENS:
			          for (i = 0; i < B3SOIPDNSRCS; i++)
				  {    (void) sprintf(name, "onoise.%s%s",
					              here->B3SOIPDname,
						      B3SOIPDnNames[i]);
                                       data->namelist = TREALLOC(IFuid, data->namelist, data->numPlots + 1);
                                       if (!data->namelist)
					   return(E_NOMEM);
		                       SPfrontEnd->IFnewUid (ckt,
			                  &(data->namelist[data->numPlots++]),
			                  NULL, name, UID_OTHER,
					  NULL);
				       /* we've added one more plot */
			          }
			          break;
		             case INT_NOIZ:
			          for (i = 0; i < B3SOIPDNSRCS; i++)
				  {    (void) sprintf(name, "onoise_total.%s%s",
						      here->B3SOIPDname,
						      B3SOIPDnNames[i]);
                                       data->namelist = TREALLOC(IFuid, data->namelist, data->numPlots + 1);
                                       if (!data->namelist)
					   return(E_NOMEM);
		                       SPfrontEnd->IFnewUid (ckt,
			                  &(data->namelist[data->numPlots++]),
			                  NULL, name, UID_OTHER,
					  NULL);
				       /* we've added one more plot */

			               (void) sprintf(name, "inoise_total.%s%s",
						      here->B3SOIPDname,
						      B3SOIPDnNames[i]);
                                       data->namelist = TREALLOC(IFuid, data->namelist, data->numPlots + 1);
                                       if (!data->namelist)
					   return(E_NOMEM);
		                       SPfrontEnd->IFnewUid (ckt,
			                  &(data->namelist[data->numPlots++]),
			                  NULL, name, UID_OTHER,
					  NULL);
				       /* we've added one more plot */
			          }
			          break;
		          }
		      }
		      break;
	         case N_CALC:
		      switch (mode)
		      {  case N_DENS:
		              NevalSrc(&noizDens[B3SOIPDRDNOIZ],
				       &lnNdens[B3SOIPDRDNOIZ], ckt, THERMNOISE,
				       here->B3SOIPDdNodePrime, here->B3SOIPDdNode,
				       here->B3SOIPDdrainConductance * here->B3SOIPDm);

		              NevalSrc(&noizDens[B3SOIPDRSNOIZ],
				       &lnNdens[B3SOIPDRSNOIZ], ckt, THERMNOISE,
				       here->B3SOIPDsNodePrime, here->B3SOIPDsNode,
				       here->B3SOIPDsourceConductance * here->B3SOIPDm);

                              switch( model->B3SOIPDnoiMod )
			      {  case 1:
			         case 3:
			              NevalSrc(&noizDens[B3SOIPDIDNOIZ],
				               &lnNdens[B3SOIPDIDNOIZ], ckt, 
					       THERMNOISE, here->B3SOIPDdNodePrime,
				               here->B3SOIPDsNodePrime,
                                               (2.0 / 3.0 * fabs(here->B3SOIPDm * 
					       (here->B3SOIPDgm
				               + here->B3SOIPDgds
					       + here->B3SOIPDgmbs))));
				      break;
			         case 2:

/* v2.2.3 bug fix */
			         case 4:
		                      NevalSrc(&noizDens[B3SOIPDIDNOIZ],
				               &lnNdens[B3SOIPDIDNOIZ], ckt,
					       THERMNOISE, here->B3SOIPDdNodePrime,
                                               here->B3SOIPDsNodePrime,
					       (here->B3SOIPDueff
					       * fabs((here->B3SOIPDqinv * here->B3SOIPDm)
					       / (pParam->B3SOIPDleff
					       * pParam->B3SOIPDleff+
						 here->B3SOIPDueff*fabs
						 (here->B3SOIPDqinv * here->B3SOIPDm)*
						  (here->B3SOIPDrds / here->B3SOIPDm))))); 
				      break;
			      }
		              NevalSrc(&noizDens[B3SOIPDFLNOIZ], NULL,
				       ckt, N_GAIN, here->B3SOIPDdNodePrime,
				       here->B3SOIPDsNodePrime, (double) 0.0);

                              switch( model->B3SOIPDnoiMod )
			      {  case 1:
			         case 4:
			              noizDens[B3SOIPDFLNOIZ] *= model->B3SOIPDkf
					    * exp(model->B3SOIPDaf
					    * log(MAX(fabs(here->B3SOIPDcd * here->B3SOIPDm),
					    N_MINLOG)))
					    / (pow(data->freq, model->B3SOIPDef)
					    * pParam->B3SOIPDleff
				            * pParam->B3SOIPDleff
					    * model->B3SOIPDcox);
				      break;
			         case 2:
			         case 3:
			              vgs = *(ckt->CKTstates[0] + here->B3SOIPDvgs);
		                      vds = *(ckt->CKTstates[0] + here->B3SOIPDvds);
			              if (vds < 0.0)
			              {   vds = -vds;
				          vgs = vgs + vds;
			              }
                                      if (vgs >= here->B3SOIPDvon + 0.1)
			              {   Ssi = B3SOIPDStrongInversionNoiseEval(vgs,
					      vds, model, here, data->freq,
					      ckt->CKTtemp);
                                          noizDens[B3SOIPDFLNOIZ] *= Ssi;
			              }
                                      else 
			              {   pParam = here->pParam;
				          T10 = model->B3SOIPDoxideTrapDensityA
					      * 8.62e-5 * ckt->CKTtemp;
		                          T11 = pParam->B3SOIPDweff * here->B3SOIPDm
					      * pParam->B3SOIPDleff
				              * pow(data->freq, model->B3SOIPDef)
				              * 4.0e36;
		                          Swi = T10 / T11 * here->B3SOIPDcd * here->B3SOIPDm
				              * here->B3SOIPDcd * here->B3SOIPDm;
                                          Slimit = B3SOIPDStrongInversionNoiseEval(
				               here->B3SOIPDvon + 0.1, vds, model,
					       here, data->freq, ckt->CKTtemp);
				          T1 = Swi + Slimit;
				          if (T1 > 0.0)
                                              noizDens[B3SOIPDFLNOIZ] *= (Slimit
								    * Swi) / T1; 
				          else
                                              noizDens[B3SOIPDFLNOIZ] *= 0.0;
			              }
				      break;
			      }

		              lnNdens[B3SOIPDFLNOIZ] =
				     log(MAX(noizDens[B3SOIPDFLNOIZ], N_MINLOG));

			      /* Low frequency excess noise due to FBE */
		              NevalSrc(&noizDens[B3SOIPDFBNOIZ], &lnNdens[B3SOIPDFBNOIZ],
				          ckt, SHOTNOISE, here->B3SOIPDsNodePrime,
				          here->B3SOIPDbNode, 
                                          2.0 * model->B3SOIPDnoif * here->B3SOIPDibs * here->B3SOIPDm);

		              noizDens[B3SOIPDTOTNOIZ] = noizDens[B3SOIPDRDNOIZ]
						     + noizDens[B3SOIPDRSNOIZ]
						     + noizDens[B3SOIPDIDNOIZ]
						     + noizDens[B3SOIPDFLNOIZ]
						     + noizDens[B3SOIPDFBNOIZ];
		              lnNdens[B3SOIPDTOTNOIZ] = 
				     log(MAX(noizDens[B3SOIPDTOTNOIZ], N_MINLOG));

		              *OnDens += noizDens[B3SOIPDTOTNOIZ];

		              if (data->delFreq == 0.0)
			      {   /* if we haven't done any previous 
				     integration, we need to initialize our
				     "history" variables.
				    */

			          for (i = 0; i < B3SOIPDNSRCS; i++)
				  {    here->B3SOIPDnVar[LNLSTDENS][i] =
					     lnNdens[i];
			          }

			          /* clear out our integration variables
				     if it's the first pass
				   */
			          if (data->freq ==
				      job->NstartFreq)
				  {   for (i = 0; i < B3SOIPDNSRCS; i++)
				      {    here->B3SOIPDnVar[OUTNOIZ][i] = 0.0;
				           here->B3SOIPDnVar[INNOIZ][i] = 0.0;
			              }
			          }
		              }
			      else
			      {   /* data->delFreq != 0.0,
				     we have to integrate.
				   */
			          for (i = 0; i < B3SOIPDNSRCS; i++)
				  {    if (i != B3SOIPDTOTNOIZ)
				       {   tempOnoise = Nintegrate(noizDens[i],
						lnNdens[i],
				                here->B3SOIPDnVar[LNLSTDENS][i],
						data);
				           tempInoise = Nintegrate(noizDens[i]
						* data->GainSqInv, lnNdens[i]
						+ data->lnGainInv,
				                here->B3SOIPDnVar[LNLSTDENS][i]
						+ data->lnGainInv, data);
				           here->B3SOIPDnVar[LNLSTDENS][i] =
						lnNdens[i];
				           data->outNoiz += tempOnoise;
				           data->inNoise += tempInoise;
				           if (job->NStpsSm != 0)
					   {   here->B3SOIPDnVar[OUTNOIZ][i]
						     += tempOnoise;
				               here->B3SOIPDnVar[OUTNOIZ][B3SOIPDTOTNOIZ]
						     += tempOnoise;
				               here->B3SOIPDnVar[INNOIZ][i]
						     += tempInoise;
				               here->B3SOIPDnVar[INNOIZ][B3SOIPDTOTNOIZ]
						     += tempInoise;
                                           }
			               }
			          }
		              }
		              if (data->prtSummary)
			      {   for (i = 0; i < B3SOIPDNSRCS; i++)
				  {    /* print a summary report */
			               data->outpVector[data->outNumber++]
					     = noizDens[i];
			          }
		              }
		              break;
		         case INT_NOIZ:
			      /* already calculated, just output */
		              if (job->NStpsSm != 0)
			      {   for (i = 0; i < B3SOIPDNSRCS; i++)
				  {    data->outpVector[data->outNumber++]
					     = here->B3SOIPDnVar[OUTNOIZ][i];
			               data->outpVector[data->outNumber++]
					     = here->B3SOIPDnVar[INNOIZ][i];
			          }
		              }
		              break;
		      }
		      break;
	         case N_CLOSE:
		      /* do nothing, the main calling routine will close */
		      return (OK);
		      break;   /* the plots */
	      }       /* switch (operation) */
	 }    /* for here */
    }    /* for model */

    return(OK);
}



