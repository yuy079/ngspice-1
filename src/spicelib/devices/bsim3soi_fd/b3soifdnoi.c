/**********
Copyright 1999 Regents of the University of California.  All rights reserved.
Author: Weidong Liu and Pin Su         Feb 1999
Author: 1998 Samuel Fung, Dennis Sinitsky and Stephen Tang
File: b3soifdnoi.c          98/5/01
Modified by Paolo Nenzi 2002
**********/

/*
 * Revision 2.1  99/9/27 Pin Su 
 * BSIMFD2.1 release
 */

#include <ngspice/ngspice.h>
#include "b3soifddef.h"
#include <ngspice/cktdefs.h>
#include <ngspice/iferrmsg.h>
#include <ngspice/noisedef.h>
#include <ngspice/suffix.h>
#include <ngspice/const.h>  /* jwan */

/*
 * B3SOIFDnoise (mode, operation, firstModel, ckt, data, OnDens)
 *    This routine names and evaluates all of the noise sources
 *    associated with MOSFET's.  It starts with the model *firstModel and
 *    traverses all of its insts.  It then proceeds to any other models
 *    on the linked list.  The total output noise density generated by
 *    all of the MOSFET's is summed with the variable "OnDens".
 */

/*
 Channel thermal and flicker noises are calculated based on the value
 of model->B3SOIFDnoiMod.
 If model->B3SOIFDnoiMod = 1,
    Channel thermal noise = SPICE2 model
    Flicker noise         = SPICE2 model
 If model->B3SOIFDnoiMod = 2,
    Channel thermal noise = B3SOIFD model
    Flicker noise         = B3SOIFD model
 If model->B3SOIFDnoiMod = 3,
    Channel thermal noise = SPICE2 model
    Flicker noise         = B3SOIFD model
 If model->B3SOIFDnoiMod = 4,
    Channel thermal noise = B3SOIFD model
    Flicker noise         = SPICE2 model
 */


static double
B3SOIFDStrongInversionNoiseEval(double vgs, double vds, B3SOIFDmodel *model, 
                                B3SOIFDinstance *here, double freq, 
				double temp)
{
struct b3soifdSizeDependParam *pParam;
double cd, esat, DelClm, EffFreq, N0, Nl, Vgst;
double T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, Ssi;

    pParam = here->pParam;
    cd = fabs(here->B3SOIFDcd) * here->B3SOIFDm;
    if (vds > here->B3SOIFDvdsat)
    {   esat = 2.0 * pParam->B3SOIFDvsattemp / here->B3SOIFDueff;
	T0 = ((((vds - here->B3SOIFDvdsat) / pParam->B3SOIFDlitl) + model->B3SOIFDem)
	   / esat);
        DelClm = pParam->B3SOIFDlitl * log (MAX(T0, N_MINLOG));
    }
    else 
        DelClm = 0.0;
    EffFreq = pow(freq, model->B3SOIFDef);
    T1 = CHARGE * CHARGE * 8.62e-5 * cd * temp * here->B3SOIFDueff;
    T2 = 1.0e8 * EffFreq * model->B3SOIFDcox
       * pParam->B3SOIFDleff * pParam->B3SOIFDleff;
    Vgst = vgs - here->B3SOIFDvon;
    N0 = model->B3SOIFDcox * Vgst / CHARGE;
    if (N0 < 0.0)
	N0 = 0.0;
    Nl = model->B3SOIFDcox * (Vgst - MIN(vds, here->B3SOIFDvdsat)) / CHARGE;
    if (Nl < 0.0)
	Nl = 0.0;

    T3 = model->B3SOIFDoxideTrapDensityA
       * log(MAX(((N0 + 2.0e14) / (Nl + 2.0e14)), N_MINLOG));
    T4 = model->B3SOIFDoxideTrapDensityB * (N0 - Nl);
    T5 = model->B3SOIFDoxideTrapDensityC * 0.5 * (N0 * N0 - Nl * Nl);

    T6 = 8.62e-5 * temp * cd * cd;
    T7 = 1.0e8 * EffFreq * pParam->B3SOIFDleff
       * pParam->B3SOIFDleff * pParam->B3SOIFDweff * here->B3SOIFDm;
    T8 = model->B3SOIFDoxideTrapDensityA + model->B3SOIFDoxideTrapDensityB * Nl
       + model->B3SOIFDoxideTrapDensityC * Nl * Nl;
    T9 = (Nl + 2.0e14) * (Nl + 2.0e14);

    Ssi = T1 / T2 * (T3 + T4 + T5) + T6 / T7 * DelClm * T8 / T9;

    return Ssi;
}

int
B3SOIFDnoise (int mode, int operation, GENmodel *inModel, CKTcircuit *ckt, 
              Ndata *data, double *OnDens)
{
#define job ((NOISEAN*)ckt->CKTcurJob)

B3SOIFDmodel *model = (B3SOIFDmodel *)inModel;
B3SOIFDinstance *here;
struct b3soifdSizeDependParam *pParam;
char name[N_MXVLNTH];
double tempOnoise;
double tempInoise;
double noizDens[B3SOIFDNSRCS];
double lnNdens[B3SOIFDNSRCS];

double vgs, vds, Slimit;
double T1, T10, T11;
double Ssi, Swi;

int i;

    /* define the names of the noise sources */
    static char *B3SOIFDnNames[B3SOIFDNSRCS] =
    {   /* Note that we have to keep the order */
	".rd",              /* noise due to rd */
			    /* consistent with the index definitions */
	".rs",              /* noise due to rs */
			    /* in B3SOIFDdefs.h */
	".id",              /* noise due to id */
	".1overf",          /* flicker (1/f) noise */
        ".fb", 		    /* noise due to floating body */
	""                  /* total transistor noise */
    };

    for (; model != NULL; model = model->B3SOIFDnextModel)
    {    for (here = model->B3SOIFDinstances; here != NULL;
	      here = here->B3SOIFDnextInstance)
	 {   
	 
              if (here->B3SOIFDowner != ARCHme)
	              continue;
	 
	      pParam = here->pParam;
	      switch (operation)
	      {  case N_OPEN:
		     /* see if we have to to produce a summary report */
		     /* if so, name all the noise generators */

		      if (job->NStpsSm != 0)
		      {   switch (mode)
			  {  case N_DENS:
			          for (i = 0; i < B3SOIFDNSRCS; i++)
				  {    (void) sprintf(name, "onoise.%s%s",
					              here->B3SOIFDname,
						      B3SOIFDnNames[i]);
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
			          for (i = 0; i < B3SOIFDNSRCS; i++)
				  {    (void) sprintf(name, "onoise_total.%s%s",
						      here->B3SOIFDname,
						      B3SOIFDnNames[i]);
                                       data->namelist = TREALLOC(IFuid, data->namelist, data->numPlots + 1);
                                       if (!data->namelist)
					   return(E_NOMEM);
		                       SPfrontEnd->IFnewUid (ckt,
			                  &(data->namelist[data->numPlots++]),
			                  NULL, name, UID_OTHER,
					  NULL);
				       /* we've added one more plot */

			               (void) sprintf(name, "inoise_total.%s%s",
						      here->B3SOIFDname,
						      B3SOIFDnNames[i]);
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
		              NevalSrc(&noizDens[B3SOIFDRDNOIZ],
				       &lnNdens[B3SOIFDRDNOIZ], ckt, THERMNOISE,
				       here->B3SOIFDdNodePrime, here->B3SOIFDdNode,
				       here->B3SOIFDdrainConductance * here->B3SOIFDm);

		              NevalSrc(&noizDens[B3SOIFDRSNOIZ],
				       &lnNdens[B3SOIFDRSNOIZ], ckt, THERMNOISE,
				       here->B3SOIFDsNodePrime, here->B3SOIFDsNode,
				       here->B3SOIFDsourceConductance * here->B3SOIFDm);

                              switch( model->B3SOIFDnoiMod )
			      {  case 1:
			         case 3:
			              NevalSrc(&noizDens[B3SOIFDIDNOIZ],
				               &lnNdens[B3SOIFDIDNOIZ], ckt, 
					       THERMNOISE, here->B3SOIFDdNodePrime,
				               here->B3SOIFDsNodePrime,
                                               (2.0 / 3.0 * fabs(here->B3SOIFDm * 
					       (here->B3SOIFDgm
				               + here->B3SOIFDgds
					       + here->B3SOIFDgmbs))));
				      break;
			         case 2:
			         case 4:
		                      NevalSrc(&noizDens[B3SOIFDIDNOIZ],
				               &lnNdens[B3SOIFDIDNOIZ], ckt,
					       THERMNOISE, here->B3SOIFDdNodePrime,
                                               here->B3SOIFDsNodePrime,
					       (here->B3SOIFDueff
					       * fabs((here->B3SOIFDqinv * here->B3SOIFDm)
					       / (pParam->B3SOIFDleff
					       * pParam->B3SOIFDleff))));
				      break;
			      }
		              NevalSrc(&noizDens[B3SOIFDFLNOIZ], NULL,
				       ckt, N_GAIN, here->B3SOIFDdNodePrime,
				       here->B3SOIFDsNodePrime, (double) 0.0);

                              switch( model->B3SOIFDnoiMod )
			      {  case 1:
			         case 4:
			              noizDens[B3SOIFDFLNOIZ] *= model->B3SOIFDkf
					    * exp(model->B3SOIFDaf
					    * log(MAX(fabs(here->B3SOIFDcd * here->B3SOIFDm),
					    N_MINLOG)))
					    / (pow(data->freq, model->B3SOIFDef)
					    * pParam->B3SOIFDleff
				            * pParam->B3SOIFDleff
					    * model->B3SOIFDcox);
				      break;
			         case 2:
			         case 3:
			              vgs = *(ckt->CKTstates[0] + here->B3SOIFDvgs);
		                      vds = *(ckt->CKTstates[0] + here->B3SOIFDvds);
			              if (vds < 0.0)
			              {   vds = -vds;
				          vgs = vgs + vds;
			              }
                                      if (vgs >= here->B3SOIFDvon + 0.1)
			              {   Ssi = B3SOIFDStrongInversionNoiseEval(vgs,
					      vds, model, here, data->freq,
					      ckt->CKTtemp);
                                          noizDens[B3SOIFDFLNOIZ] *= Ssi;
			              }
                                      else 
			              {   pParam = here->pParam;
				          T10 = model->B3SOIFDoxideTrapDensityA
					      * 8.62e-5 * ckt->CKTtemp;
		                          T11 = pParam->B3SOIFDweff * here->B3SOIFDm
					      * pParam->B3SOIFDleff
				              * pow(data->freq, model->B3SOIFDef)
				              * 4.0e36;
		                          Swi = T10 / T11 * here->B3SOIFDcd * here->B3SOIFDm
				              * here->B3SOIFDcd * here->B3SOIFDm;
                                          Slimit = B3SOIFDStrongInversionNoiseEval(
				               here->B3SOIFDvon + 0.1, vds, model,
					       here, data->freq, ckt->CKTtemp);
				          T1 = Swi + Slimit;
				          if (T1 > 0.0)
                                              noizDens[B3SOIFDFLNOIZ] *= (Slimit
								    * Swi) / T1; 
				          else
                                              noizDens[B3SOIFDFLNOIZ] *= 0.0;
			              }
				      break;
			      }

		              lnNdens[B3SOIFDFLNOIZ] =
				     log(MAX(noizDens[B3SOIFDFLNOIZ], N_MINLOG));

			      /* Low frequency excess noise due to FBE */
                              noizDens[B3SOIFDFBNOIZ] = 0.0;

		              noizDens[B3SOIFDTOTNOIZ] = noizDens[B3SOIFDRDNOIZ]
						     + noizDens[B3SOIFDRSNOIZ]
						     + noizDens[B3SOIFDIDNOIZ]
						     + noizDens[B3SOIFDFLNOIZ]
						     + noizDens[B3SOIFDFBNOIZ];
		              lnNdens[B3SOIFDTOTNOIZ] = 
				     log(MAX(noizDens[B3SOIFDTOTNOIZ], N_MINLOG));

		              *OnDens += noizDens[B3SOIFDTOTNOIZ];

		              if (data->delFreq == 0.0)
			      {   /* if we haven't done any previous 
				     integration, we need to initialize our
				     "history" variables.
				    */

			          for (i = 0; i < B3SOIFDNSRCS; i++)
				  {    here->B3SOIFDnVar[LNLSTDENS][i] =
					     lnNdens[i];
			          }

			          /* clear out our integration variables
				     if it's the first pass
				   */
			          if (data->freq ==
				      job->NstartFreq)
				  {   for (i = 0; i < B3SOIFDNSRCS; i++)
				      {    here->B3SOIFDnVar[OUTNOIZ][i] = 0.0;
				           here->B3SOIFDnVar[INNOIZ][i] = 0.0;
			              }
			          }
		              }
			      else
			      {   /* data->delFreq != 0.0,
				     we have to integrate.
				   */
			          for (i = 0; i < B3SOIFDNSRCS; i++)
				  {    if (i != B3SOIFDTOTNOIZ)
				       {   tempOnoise = Nintegrate(noizDens[i],
						lnNdens[i],
				                here->B3SOIFDnVar[LNLSTDENS][i],
						data);
				           tempInoise = Nintegrate(noizDens[i]
						* data->GainSqInv, lnNdens[i]
						+ data->lnGainInv,
				                here->B3SOIFDnVar[LNLSTDENS][i]
						+ data->lnGainInv, data);
				           here->B3SOIFDnVar[LNLSTDENS][i] =
						lnNdens[i];
				           data->outNoiz += tempOnoise;
				           data->inNoise += tempInoise;
				           if (job->NStpsSm != 0)
					   {   here->B3SOIFDnVar[OUTNOIZ][i]
						     += tempOnoise;
				               here->B3SOIFDnVar[OUTNOIZ][B3SOIFDTOTNOIZ]
						     += tempOnoise;
				               here->B3SOIFDnVar[INNOIZ][i]
						     += tempInoise;
				               here->B3SOIFDnVar[INNOIZ][B3SOIFDTOTNOIZ]
						     += tempInoise;
                                           }
			               }
			          }
		              }
		              if (data->prtSummary)
			      {   for (i = 0; i < B3SOIFDNSRCS; i++)
				  {    /* print a summary report */
			               data->outpVector[data->outNumber++]
					     = noizDens[i];
			          }
		              }
		              break;
		         case INT_NOIZ:
			      /* already calculated, just output */
		              if (job->NStpsSm != 0)
			      {   for (i = 0; i < B3SOIFDNSRCS; i++)
				  {    data->outpVector[data->outNumber++]
					     = here->B3SOIFDnVar[OUTNOIZ][i];
			               data->outpVector[data->outNumber++]
					     = here->B3SOIFDnVar[INNOIZ][i];
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



