#pragma once

#include <vector>
#include "Vec.h"
using namespace std;


void detectCorners(char *inPut,char *outPut);

class SizeFunction
{
	vector<vec2> descripteur;
public:
	SizeFunction(void);
	SizeFunction(const string &f);
	virtual ~SizeFunction(void);
	void loadFile(const string &f);
	void sampleAndSave(const string &f);
	void displaySF();
	void destroy();
	double DistanceSF(SizeFunction b);
	SizeFunction(SizeFunction& );
	SizeFunction(const SizeFunction& );
	SizeFunction& operator=(SizeFunction &);
	SizeFunction& operator=(const SizeFunction &);
};
