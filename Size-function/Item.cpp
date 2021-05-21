#include "StdAfx.h"
#include "Item.h"

#include <iostream>
Item::Item(const string &s)
{
	path = s;
	value =0.0;
	id = -1;
}
Item::Item(const string &s,double val)
{
	path = s;
	value =val;
}Item::Item(const string &s,double val,int i)
{
	path = s;
	value =val;
	id = i;
}
Item::Item()
{
	path = "";
	value =0.0f;
}

Item::~Item(void)
{
	path.clear();
}
void Item::displayItem(){
	std::cout<<"s = "<<path.c_str()<<" , value = "<<value<< endl; 
}
Item& Item::operator =(Item &Q)
{
	if(this!=&Q)
	{
		path = Q.path;
		value = Q.value;
	}
	return *this ;
}
Item& Item::operator =(const Item &Q)
{
	if(this!=&Q)
	{
		path = Q.path;
		value = Q.value;
	}
	return *this ;
}

Item::Item(Item &Q)
{
		path = Q.path;
		value = Q.value;
}
Item::Item(const Item &Q)
{
		path = Q.path;
		value = Q.value;
}
