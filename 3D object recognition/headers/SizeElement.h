#pragma once

#include <string>

using namespace std;

class SizeElement
{
	double Min, Max;
	double value;
public:
	SizeElement();
	SizeElement(double,double);
	SizeElement(double,double,double );
	SizeElement(SizeElement& );
	SizeElement(const SizeElement& );
	void displaySizeElement();
	double getMin(){return Min;}
	double getMax(){return Max;}
	void setValue(double val){value = val;}
	double getValue(){return value;}
	SizeElement& operator=(SizeElement &);
	SizeElement& operator=(const SizeElement &);
	virtual ~SizeElement();
};
