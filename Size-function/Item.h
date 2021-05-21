#pragma once

#include <string>

using namespace std;

class Item
{
	double value;
	string path;
	int id;
public:
	Item();
	Item(const string &s);
	Item(const string &s,double val);
	Item(const string &s,double val,int i);
	Item(Item& );
	Item(const Item& );
	void displayItem();
	string getString(){return path;}
	void setValue(double val){value = val;}
	double getValue(){return value;}
	Item& operator=(Item &);
	Item& operator=(const Item &);
	virtual ~Item();
	static bool compareItems(Item lhs, Item rhs) { return (lhs.value < rhs.value); }
	static bool compareItemsSup(Item lhs, Item rhs) { return (lhs.value > rhs.value); }
};
