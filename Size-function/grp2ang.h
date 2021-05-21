#ifndef GRP2ANG_H
#define GRP2ANG_H

#include <stdlib.h>
#include <float.h>

#include "angpt.h"
#include "deltagrp.h"

/* If you change the typeval, you have to change also MAX_SIZE_TYPEVAL */
#ifndef MAX_SIZE_TYPEVAL
	typedef float SIZE_TYPEVAL;
	#define MAX_SIZE_TYPEVAL FLT_MAX
#endif


/* PROTOTYPE FUNCTIONS */
ang_pt *size_deltared_graph(Graph);
ang_pt *size_deltared_P_graph(P_node *);

#endif