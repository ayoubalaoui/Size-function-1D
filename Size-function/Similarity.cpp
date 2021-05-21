#include "StdAfx.h"
#include "Similarity.h"
#include "hausdorff.h"
#include "CornersGroup2.h"
#include <iostream>

#include <omp.h>
#include <vector>
#include <algorithm>

Similarity::Similarity(const string &s,int N)
{
	path = s;
	bN = N;
	bN *=100;
	
	ItemList.clear();
	//load_2D_3D_Descriptors();
}
Similarity::Similarity(void)
{
	path = "";
	bN = 0;
	bN *=100;
	ItemList.clear();
}
void Similarity::display(){
	int n = ItemList.size();
	for(int i=0;i<n;i++){
		std::cout<<"N° "<<i<<"->"<<ItemList[i].getString().c_str()<<" value="<<ItemList[i].getValue()<< endl;
	}
}
double Similarity::distance_SF(int i,int j){
	int k1 = bN +i;
	int k2 = bN +j;
	/*char *pnamec1_1 = new char[1024];
	char *pnamec1_2 = new char[1024];
	char *pnamec1_3 = new char[1024];
	char *pnamec2_1 = new char[1024];
	char *pnamec2_2 = new char[1024];
	char *pnamec2_3 = new char[1024];
	sprintf(pnamec1_1, "%s\\m%d\\m%d_Size_Function_3_PCA_Plans\\m%d_Size_Function_3_PCA_1.sf", path.c_str(),k1,k1,k1);
	sprintf(pnamec1_2, "%s\\m%d\\m%d_Size_Function_3_PCA_Plans\\m%d_Size_Function_3_PCA_2.sf", path.c_str(),k1,k1,k1);
	sprintf(pnamec1_3, "%s\\m%d\\m%d_Size_Function_3_PCA_Plans\\m%d_Size_Function_3_PCA_3.sf", path.c_str(),k1,k1,k1);
	std::vector<std::string> fList1;
	fList1.clear();
	fList1.push_back(pnamec1_1);
	fList1.push_back(pnamec1_2);
	fList1.push_back(pnamec1_3);
	CornersGroup2 c1(fList1);
	fList1.clear();
	sprintf(pnamec2_1, "%s\\m%d\\m%d_Size_Function_3_PCA_Plans\\m%d_Size_Function_3_PCA_1.sf", path.c_str(),k2,k2,k2);
	sprintf(pnamec2_2, "%s\\m%d\\m%d_Size_Function_3_PCA_Plans\\m%d_Size_Function_3_PCA_2.sf", path.c_str(),k2,k2,k2);
	sprintf(pnamec2_3, "%s\\m%d\\m%d_Size_Function_3_PCA_Plans\\m%d_Size_Function_3_PCA_3.sf", path.c_str(),k2,k2,k2);

	fList1.clear();
	fList1.push_back(pnamec2_1);
	fList1.push_back(pnamec2_2);
	fList1.push_back(pnamec2_3);
	CornersGroup2 c2(fList1);
	fList1.clear();
	delete pnamec1_1;
	delete pnamec1_2;
	delete pnamec1_3;
	delete pnamec2_1;
	delete pnamec2_2;
	delete pnamec2_3;
	double val = c1.Pseudo_DistanceSF(c2);
	c1.destroy();
	c2.destroy();*/

	std::vector<std::string> fList1;
	for(int d = 1; d <= 8; d++){
		char fname [1024];
		sprintf(fname,"%s\\m%d\\m%d_Size_Function_8_Plans\\m%d_Size_Function_%d.sf",path.c_str(),k1,k1,k1,d);
		FILE *f = fopen(fname,"r");
		if(f==NULL) continue;
		fclose(f);
		fList1.push_back(fname);
	}

	CornersGroup c1(fList1);
	fList1.clear();
	for(int d = 1; d <= 8; d++){
		char fname [1024];
		sprintf(fname,"%s\\m%d\\m%d_Size_Function_8_Plans\\m%d_Size_Function_%d.sf",path.c_str(),k2,k2,k2,d);
		FILE *f = fopen(fname,"r");
		if(f==NULL) continue;
		fclose(f);
		fList1.push_back(fname);
	}

	CornersGroup c2(fList1);
	fList1.clear();
	double val = c1.DistanceSF(c2);
	c1.destroy();
	c2.destroy();

	return val;
	//return SFList[i].DistanceSF(SFList[j]);
}

void Similarity::QuerySF(int r){
	ItemList.clear();
	ItemList.resize(457);
#pragma omp parallel for
	for(int i=0;i<457;i++){
		char fname[10];
		sprintf(fname, "m%d", i);
		//printf("\nPour %s",fname);
		double val = distance_SF(i,r);
		printf("\nval = %f pour %s",val,fname);
		ItemList[i] = Item(fname,val);
	}
	sort(ItemList.begin(),ItemList.end(),Item::compareItems);
}

void Similarity::load_size_function(){
	SFList.clear();
	SFList.resize(100);
#pragma omp parallel for
	for(int j=0;j<100;j++){
		int i = bN +j;
		char pname1[1024],pname2[1024],pname3[1024];
		sprintf(pname1, "%s\\m%d\\m%d_Size_Function_3_PCA_Plans\\m%d_Size_Function_3_PCA_1.sf", path.c_str(),i,i,i);
		sprintf(pname2, "%s\\m%d\\m%d_Size_Function_3_PCA_Plans\\m%d_Size_Function_3_PCA_2.sf", path.c_str(),i,i,i);
		sprintf(pname3, "%s\\m%d\\m%d_Size_Function_3_PCA_Plans\\m%d_Size_Function_3_PCA_3.sf", path.c_str(),i,i,i);
		//std::string path = pname1;
		std::vector<std::string> fList;
		fList.clear();
		fList.push_back(pname1);
		fList.push_back(pname2);
		fList.push_back(pname3);
		SFList[j] = CornersGroup(fList);
		fList.clear();
		//SFList[j].loadFromDirectory(pname1);
		std::cout<<"\nChargement Fini pour "<<pname1<<endl;
	}
	std::cout<<"\nChargement Fini "<<endl;
}
Similarity::~Similarity(void)
{
	SFList.clear();
}

