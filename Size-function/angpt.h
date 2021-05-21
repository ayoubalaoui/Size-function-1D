#ifndef ANGPT_H
#define ANGPT_H

#include <stdlib.h>
#include <float.h>

/* If you change the typeval, you have to change also MAX_SIZE_TYPEVAL */
#ifndef MAX_SIZE_TYPEVAL
	typedef float SIZE_TYPEVAL;
	#define MAX_SIZE_TYPEVAL FLT_MAX
#endif

/* STRUCTRES */
struct ang_pt{
	SIZE_TYPEVAL			x;
	SIZE_TYPEVAL			y;
	struct ang_pt	*next;
};

typedef struct ang_pt ang_pt;

/* PROTOTYPES */
void ins_ang_pt (ang_pt **,ang_pt *);
ang_pt *new_ang_pt (SIZE_TYPEVAL,SIZE_TYPEVAL);
void del_ang_pt (ang_pt **);
void destroy_all_ang_pt(ang_pt **);
void verbose_write_ang_pt(ang_pt,char *);
void write_ang_pt(ang_pt,char *);
ang_pt *read_ang_pt(char *);
ang_pt *read_ang_pt(const char *);

#endif
