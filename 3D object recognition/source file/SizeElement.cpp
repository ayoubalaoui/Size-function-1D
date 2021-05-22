#include "StdAfx.h"

#include "SizeElement.h"
#include <iostream>

SizeElement::SizeElement(double min,double max)
{
	Max  = max;
	Min = min;
	value = -1.0;
}
SizeElement::SizeElement(double min,double max,double val)
{
	Max  = max;
	Min = min;
	value =val;
}

SizeElement::SizeElement()
{
	Max  = 0.0;
	Min = 0.0;
	value =-1.0;
}

SizeElement::~SizeElement(void)
{
}
void SizeElement::displaySizeElement(){
	std::cout<<"Min = "<<Min<<" ,Max = "<<Max<<" , value = "<<value<< endl; 
}
SizeElement& SizeElement::operator =(SizeElement &Q)
{
	if(this!=&Q)
	{
		Max  = Q.Max;
		Min = Q.Min;
		value = Q.value;
	}
	return *this ;
}
SizeElement& SizeElement::operator =(const SizeElement &Q)
{
	if(this!=&Q)
	{
		Max  = Q.Max;
		Min = Q.Min;
		value = Q.value;
	}
	return *this ;
}

SizeElement::SizeElement(SizeElement &Q)
{
		Max  = Q.Max;
		Min = Q.Min;
		value = Q.value;
}
SizeElement::SizeElement(const SizeElement &Q)
{
		Max  = Q.Max;
		Min = Q.Min;
		value = Q.value;
}
