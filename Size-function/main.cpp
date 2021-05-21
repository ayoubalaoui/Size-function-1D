
// SizeFunction.cpp : définit le point d'entrée pour l'application console.
//

#include "stdafx.h"
#include "angpt.h"
#include "deltagrp.h"
#include "grp2ang.h"
#include <omp.h>
#include "recallPrecision.h"
#include "SizeFunction.h"
#include "timestamp.h"

#include <vector>
#include <string>

using namespace std;


/*
* inputMeasures: the name of files where the graphs of measure functions are stored
* outputSizeFunctions: the name of files where to store size functions graphs
*/

void compute_size_function(vector<string> inputMeasures, vector<string> outputSizeFunctions) {
	int n = inputMeasures.size();

	for (int i = 0; i < n; i++) {

		char* fname = new char[1024];
		sprintf(fname, inputMeasures[i].c_str());
		Graph	G = read_file_graph(fname);

		ang_pt* ang = size_deltared_graph(G);

		destroy_graph(G);
		sprintf(fname, outputSizeFunctions[i].c_str());
		write_ang_pt(*ang, fname);
		destroy_all_ang_pt(&ang);
		delete fname;

	}
}



int _tmain(int argc, _TCHAR* argv[])
{

	return 0;
}

