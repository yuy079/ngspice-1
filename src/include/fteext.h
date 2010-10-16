/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1986 Wayne A. Christopher, U. C. Berkeley CAD Group
Modified: 1999 Paolo Nenzi - 2000 AlansFixes
**********/

/*
 * Definitions for all external symbols in FTE.
 */

#ifndef FTEext_h
#define FTEext_h

#include <config.h>

/* needed to find out what the interface structures look like */
#include "typedefs.h"
#include "ifsim.h"
#include "dvec.h"
#include "plot.h"
#include "cpdefs.h"
#include "ftedefs.h"
#include "fteinp.h"

/* aspice.c */

extern void ft_checkkids(void);

/* breakpoint.c */

extern bool ft_bpcheck(struct plot *runplot, int iteration);
extern void dbfree(struct dbcomm *db);


/* breakp2.c */

extern int ft_getSaves(struct save_info **);


/* circuits.c */

extern struct circ *ft_curckt;
extern struct circ *ft_circuits;
extern struct subcirc *ft_subcircuits;
extern void ft_newcirc(struct circ *ckt);

/* clip.c */

extern bool clip_line(int *pX1, int *pY1, int *pX2, int *pY2, int l, int b, int r, int t);
extern bool clip_to_circle(int *x1, int *y1, int *x2, int *y2, int cx, int cy, int rad);

/* cmath1.c */

extern bool cx_degrees;
extern void *cx_mag(void *, short int , int , int *, short int *);
extern void *cx_ph(void *, short int , int , int *, short int *);
extern void *cx_j(void *, short int , int , int *, short int *);
extern void *cx_real(void *, short int , int , int *, short int *);
extern void *cx_imag(void *, short int , int , int *, short int *);
extern void *cx_pos(void *, short int , int , int *, short int *);
extern void *cx_db(void *, short int , int , int *, short int *);
extern void *cx_log(void *, short int , int , int *, short int *);
extern void *cx_ln(void *, short int , int , int *, short int *);
extern void *cx_exp(void *, short int , int , int *, short int *);
extern void *cx_sqrt(void *, short int , int , int *, short int *);
extern void *cx_sin(void *, short int , int , int *, short int *);
extern void *cx_cos(void *, short int , int , int *, short int *);

/* cmath3.c */

extern void *cx_divide(void *, void *, short int , short int , int);
extern void *cx_comma(void *, void *, short int , short int , int);
extern void *cx_power(void *, void *, short int , short int , int);
extern void *cx_eq(void *, void *, short int , short int , int);
extern void *cx_gt(void *, void *, short int , short int , int);
extern void *cx_lt(void *, void *, short int , short int , int);
extern void *cx_ge(void *, void *, short int , short int , int);
extern void *cx_le(void *, void *, short int , short int , int);
extern void *cx_ne(void *, void *, short int , short int , int);

/* cmath4.c */

extern void *cx_and(void *, void *, short int , short int , int);
extern void *cx_or(void *, void *, short int , short int , int);
extern void *cx_not(void *, short int , int , int *, short int *);

extern void *cx_interpolate(void *, short int , int , int *, short int *, struct plot *, struct plot *, int );
extern void *cx_deriv(void *, short int , int , int *, short int *, struct plot *, struct plot *, int );
extern void *cx_group_delay(void *, short int , int , int *, short int *, struct plot *, struct plot *, int );


/* define.c */

extern struct pnode *ft_substdef(const char *name, struct pnode *args);
extern void ft_pnode(struct pnode *pn);

/* dotcards.c */

extern bool ft_acctprint;
extern bool ft_noacctprint;
extern bool ft_noinitprint;
extern bool ft_listprint;
extern bool ft_nopage;
extern bool ft_nomod;
extern bool ft_nodesprint;
extern bool ft_optsprint;
extern int ft_cktcoms(bool terse);
extern void ft_dotsaves(void);
extern int ft_savedotargs(void);

/* error.c */

extern void fperror(char *mess, int code);
extern void ft_sperror(int code, char *mess);
extern char ErrorMessage[];
extern void internalerror(char *); 
extern void externalerror(char *); 



/* evaluate.c */

extern struct dvec *ft_evaluate(struct pnode *node);

/* ginterface.c 

extern bool gi_init();
extern bool gi_endpause;
extern bool gi_rottext;
extern int gi_fntheight;
extern int gi_fntwidth;
extern int gi_maxx;
extern int gi_maxy;
extern int gi_nolst;
extern int gi_nocolors;
extern int gi_package;
extern void gi_arc();
extern void gi_clearscreen();
extern void gi_close();
extern void gi_drawline();
extern void gi_redraw();
extern void gi_setcolor();
extern void gi_resetcolor();
extern void gi_setlinestyle();
extern void gi_text();
extern void gi_update();
*/

/* graf.c */

extern bool gr_gmode;
extern bool gr_hmode;
extern double gr_xrange[2];
extern double gr_yrange[2];
extern int gr_xmargin;
extern int gr_ymargin;
extern int gr_xcenter;
extern int gr_ycenter;
extern int gr_radius;
extern bool gr_circular;

/* inp.c */

void inp_dodeck(struct line *deck, char *tt, wordlist *end, bool reuse, 
		struct line *options, char *filename);
extern void inp_source(char *file);
void inp_spsource(FILE *fp, bool comfile, char *filename);
extern void inp_casefix(char *string);
extern void inp_list(FILE *file, struct line *deck, struct line *extras, int type);
extern void inp_readall(FILE *fp, struct line **data, int call_depth, char *dir_name, bool comfile);
extern FILE *inp_pathopen(char *name, char *mode);

/* nutinp.c */

void inp_nutsource(FILE *fp, bool comfile, char *filename);
void nutinp_dodeck(struct line *deck, char *tt, wordlist *end, bool reuse, 
		   struct line *options, char *filename);
extern void nutcom_source(wordlist *wl);

/* interpolate.c */

extern bool ft_interpolate(double *data, double *ndata, double *oscale, int olen, double *nscale, int nlen, int degree);
extern bool ft_polyfit(double *xdata, double *ydata, double *result, int degree, double *scratch);
extern double ft_peval(double x, double *coeffs, int degree);
extern void ft_polyderiv(double *coeffs, int degree);

/* numparse.c */

extern bool ft_strictnumparse;
double * ft_numparse(char **s, bool whole);

/* options.c */

extern bool ft_simdb;
extern bool ft_parsedb;
extern bool ft_evdb;
extern bool ft_vecdb;
extern bool ft_grdb;
extern bool ft_gidb;
extern bool ft_controldb;
extern bool ft_asyncdb;
extern char *ft_setkwords[];
extern struct line *inp_getopts(struct line *deck);
extern struct line *inp_getoptsc(char *in_line, struct line *com_options);
extern bool ft_ngdebug;

/* parse.c */

extern struct func ft_funcs[];
extern struct func func_not;
extern struct func func_uminus;
extern struct pnode * ft_getpnames(wordlist *wl, bool check);
#define free_pnode(ptr)  free_pnode_x(ptr); ptr=NULL
extern void free_pnode_x(struct pnode *t);

/* plotcurve.c */

extern int ft_findpoint(double pt, double *lims, int maxp, int minp, bool islog);
extern double * ft_minmax(struct dvec *v, bool real);
extern void ft_graf(struct dvec *v, struct dvec *xs, bool nostart);

/* rawfile.c */
extern int raw_prec;
extern void raw_write(char *name, struct plot *pl, bool app, bool binary);
extern void spar_write(char *name, struct plot *pl, double val);
extern struct plot *raw_read(char *name);

/* meas.c */
extern void do_measure(char *what, bool chk_only);
extern bool check_autostop(char *what);

/* randnumb.c */
extern void TausSeed(void);
/* resource.c */

extern void ft_ckspace(void);

/* runcoms.c */

extern int ft_dorun(char *file);

extern bool ft_getOutReq(FILE **, struct plot **, bool *, char *, char *);

/* spice.c & nutmeg.c */

extern bool ft_nutmeg;
extern IFsimulator *ft_sim;
extern char *ft_rawfile;
extern int main(int argc, char **argv);

/* spiceif.c & nutmegif.c */

extern bool if_tranparams(struct circ *ci, double *start, double *stop, double *step);
extern char *if_errstring(int code);
extern int if_sens_run(CKTcircuit *t, wordlist *args, INPtables *tab);
extern struct variable *(*if_getparam)(CKTcircuit *ckt, char** name, char* param, int ind, int do_model);
extern struct variable * nutif_getparam(CKTcircuit *ckt, char **name, char *param, int ind, int do_model);
extern struct variable *spif_getparam(CKTcircuit *ckt, char **name, char *param, int ind, int do_model);
extern struct variable *spif_getparam_special(CKTcircuit *ckt, char **name, char *param, int ind, int do_model);
extern void if_setndnames(char *line);
extern void if_setparam_model(CKTcircuit *ckt, char **name, char *val );
extern void if_setparam(CKTcircuit *ckt, char **name, char *param, struct dvec *val, int do_model);
extern struct variable *if_getstat(CKTcircuit *ckt, char *name);

/* typesdef.c */

extern char *ft_typabbrev(int);
extern char *ft_typenames(int);
extern char *ft_plotabbrev(char *);
extern int ft_typnum(char *);

/* vectors.c */

extern bool vec_eq(struct dvec *v1, struct dvec *v2);
extern int plot_num;
extern struct dvec *vec_fromplot(char *word, struct plot *plot);
extern struct dvec *vec_copy(struct dvec *v);
extern struct dvec *vec_get(const char *word);
extern struct dvec *vec_mkfamily(struct dvec *v);
extern struct plot *plot_cur;
extern struct plot *plot_alloc(char *name);
extern struct plot *plot_list;
extern int plotl_changed;
extern void plot_add(struct plot *pl);
#define vec_free(ptr)  vec_free_x(ptr); ptr=NULL
extern void vec_free_x(struct dvec *v);
extern void vec_gc(void);
extern void ft_loadfile(char *file);
extern void vec_new(struct dvec *d);
extern void plot_docoms(wordlist *wl);
extern void vec_remove(char *name);
extern void plot_setcur(char *name);
extern void plot_new(struct plot *pl);
extern char *vec_basename(struct dvec *v);
extern bool plot_prefix(char *pre, char *str);
extern void vec_transpose(struct dvec *v);

/* main.c */
extern bool ft_intrpt;
extern bool ft_setflag;

/* dimens.c */
extern void dimstring(int *data, int length, char *retstring);
extern int atodims(char *p, int *data, int *outlength);
extern void indexstring(int *data, int length, char *retstring);
extern int incindex(int *counts, int numcounts, int *dims, int numdims);

#endif /* FTEext_h */
