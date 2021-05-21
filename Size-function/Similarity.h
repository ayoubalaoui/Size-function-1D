#pragma once

#include <vector>
#include "Item.h"
#include "CornersGroup.h"

using namespace std;
class Similarity
{
	string path;
	int bN;
	vector<CornersGroup> SFList;
public:
	Similarity(const string &,int);
	void load_size_function();
	double distance_SF(int,int);
	void QuerySF(int r);
	void display();
	Similarity(void);
	~Similarity(void);
	vector<Item> ItemList;
};
