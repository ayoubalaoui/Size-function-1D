/* INCLUDE */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "grp2ang.h"

#ifndef DEF_STRUCTURES
#define DEF_STRUCTURES

/* TYPES and STRUCTURES DEFINITIONS */
typedef unsigned char BYTE;

struct IMG_EDGE{
	int	*row;
	int	*col;
	int	n_pt;
};

typedef struct IMG_EDGE IMG_EDGE;



/* Default values for variables which can be changed calling the program */
#define MF_TYPE "edge"		/* type of measuring function */
#define DIST_TYPE "haus"	/* distance between size functions */
#define MAX_EDGE_PT 560		/* maximum number of edge points */

/* CONSTANTS */
#define DIST(x1,y1,x2,y2) (sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2)))
#define SQR_DIST(x1,y1,x2,y2) ((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2))
#define MAXIMUM(a,b) (a>b ? a : b)

#define PI 3.14159265359

#define STR_MSG 200	/* maximum length for message strings */
#define STR_PATH 100	/* maximum length for file path strings */
#define STR_NAME 40	/* maximum length for single name strings */

#endif

