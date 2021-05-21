#include "size.h"

#define SQUAREOF2 1.4142136

struct Point{
	SIZE_TYPEVAL	x;
	SIZE_TYPEVAL	y;
};

typedef struct Point Point;


/* PROTOTYPES */
float hausdorff(ang_pt *,ang_pt *);
