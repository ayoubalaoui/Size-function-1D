#include "StdAfx.h"
#include "recallPrecision.h"

#include <vector>
#include "Similarity.h"
#include <stdlib.h>
#include <iostream>

using namespace std;

bool contain(vector<int> list,int k){
	int l = list.size();
	for(int i=0;i<l;i++)
		if(list[i]==k) return true;
	return false;
}
int getID(vector<vector<int>> list,int k){
	//int id =-1;
	int size = list.size();
	for(int i=0;i<size;i++){
		if(contain(list[i],k)) return i;
	}
	return -1;
}
void initClasses1(vector<vector<int>> &classes){
	vector<int> C1; C1.clear();
	for(int i=0;i<=4;i++) C1.push_back(i);
	classes.push_back(C1);

	vector<int> C2; C2.clear();
	for(int i=5;i<=8;i++) C2.push_back(i);
	classes.push_back(C2);

	vector<int> C3; C3.clear();
	for(int i=9;i<=12;i++) C3.push_back(i);
	classes.push_back(C3);

	vector<int> C4; C4.clear();
	for(int i=13;i<=17;i++) C4.push_back(i);
	classes.push_back(C4);

	vector<int> C5; C5.clear();
	for(int j=18;j<=45;j++) C5.push_back(j);
	classes.push_back(C5);

	vector<int> C6; C6.clear();
	for(int j=46;j<=66;j++) C6.push_back(j);
	classes.push_back(C6);
	
	vector<int> C7; C7.clear();
	for(int j=67;j<=99;j++) C7.push_back(j);
	classes.push_back(C7);
}
void initClasses2(vector<vector<int>> &classes){
	vector<int> C1; C1.clear();
	for(int i=0;i<=7;i++) C1.push_back(i);
	classes.push_back(C1);

	vector<int> C2; C2.clear();
	for(int i=8;i<=15;i++) C2.push_back(i);
	classes.push_back(C2);

	vector<int> C3; C3.clear();
	for(int i=16;i<=26;i++) C3.push_back(i);
	classes.push_back(C3);

	vector<int> C4; C4.clear();
	for(int i=27;i<=52;i++) C4.push_back(i);
	classes.push_back(C4);

	vector<int> C5; C5.clear();
	for(int j=53;j<=85;j++) C5.push_back(j);
	classes.push_back(C5);

	/*vector<int> C6; C6.clear();
	for(int j=46;j<=66;j++) C6.push_back(j);
	classes.push_back(C6);*/
	
	vector<int> C7; C7.clear();
	for(int j=86;j<=99;j++) C7.push_back(j);
	classes.push_back(C7);
}


void generateRecallPrecisionSF(){
	vector<double> recall(100,0.0);
	vector<double> precision(100,0.0);
	vector<double> recallS(100,0.0);
	vector<double> precisionS(100,0.0);
	vector<vector<int>> classes; classes.clear();

	initClasses1(classes);

	const char *path = "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11";
	Similarity b(path,11);
	//b.load_size_function();
	for(int i=0;i<100;i++){
		int cpt=0;
		int req = i;
		int ic = getID(classes,i);
		int nc = classes[ic].size();
		b.QuerySF(i);
		//std::cout<<"Salut\n"<<endl;
		for(int j=0;j<100;j++){
			string s = b.ItemList[j].getString();
			//std::cout<<s<<endl;
			int res = atoi(s.substr(1,s.size()).c_str());
			if(contain(classes[ic],res)) cpt++;
			precision[j] = (double)cpt/((double)(j+1));
			recall[j] = (double)cpt/(double)nc;
		}
#pragma omp parallel for
		for(int j=0;j<100;j++){
			precisionS[j] += precision[j];
			recallS[j] += recall[j];
		}
		std::cout<<"\nRéussite pour \n"<<i<< endl;
	}
	FILE *f = fopen("C:\\Dossier Ayoub\\3D\\psb_v1\\curves\\recall_Size_Function_PCA_Pseudo_1_haus.txt","w");
	if(f==NULL){
		cout<<"Erreur d'écriture du fichier " << endl;
		exit(-1);
	}
	for(int i=0;i<100;i++){
		fprintf(f,"%lf\t%lf\n",recallS[i]/100.0,precisionS[i]/100.0);
	}
	fclose(f);
}
void initClassesMcGill(vector<vector<int>> &classes){
	vector<int> C1; C1.clear();
	for(int i=0;i<=29;i++) C1.push_back(i);
	classes.push_back(C1);

	vector<int> C2; C2.clear();
	for(int i=30;i<=59;i++) C2.push_back(i);
	classes.push_back(C2);

	vector<int> C3; C3.clear();
	for(int i=60;i<=79;i++) C3.push_back(i);
	classes.push_back(C3);

	vector<int> C4; C4.clear();
	for(int i=80;i<=108;i++) C4.push_back(i);
	classes.push_back(C4);

	vector<int> C5; C5.clear();
	for(int j=109;j<=133;j++) C5.push_back(j);
	classes.push_back(C5);

	vector<int> C6; C6.clear();
	for(int j=134;j<=153;j++) C6.push_back(j);
	classes.push_back(C6);
	
	vector<int> C7; C7.clear();
	for(int j=154;j<=178;j++) C7.push_back(j);
	classes.push_back(C7);

	vector<int> C8; C8.clear();
	for(int j=179;j<=203;j++) C8.push_back(j);
	classes.push_back(C8);

	vector<int> C9; C9.clear();
	for(int j=204;j<=234;j++) C9.push_back(j);
	classes.push_back(C9);

	vector<int> C10; C10.clear();
	for(int j=235;j<=254;j++) C10.push_back(j);
	classes.push_back(C10);

	vector<int> C11; C11.clear();
	for(int j=255;j<=280;j++) C11.push_back(j);
	classes.push_back(C11);

	vector<int> C12; C12.clear();
	for(int j=281;j<=301;j++) C12.push_back(j);
	classes.push_back(C12);

	vector<int> C13; C13.clear();
	for(int j=302;j<=324;j++) C13.push_back(j);
	classes.push_back(C13);

	vector<int> C14; C14.clear();
	for(int j=325;j<=349;j++) C14.push_back(j);
	classes.push_back(C14);

	vector<int> C15; C15.clear();
	for(int j=350;j<=368;j++) C15.push_back(j);
	classes.push_back(C15);

	vector<int> C16; C16.clear();
	for(int j=369;j<=403;j++) C16.push_back(j);
	classes.push_back(C16);

	vector<int> C17; C17.clear();
	for(int j=404;j<=434;j++) C17.push_back(j);
	classes.push_back(C17);

	vector<int> C18; C18.clear();
	for(int j=435;j<=456;j++) C18.push_back(j);
	classes.push_back(C18);


}
void generateRecallPrecisionMcGill(){
	vector<double> recall(457,0.0);
	vector<double> precision(457,0.0);
	vector<double> recallS(457,0.0);
	vector<double> precisionS(457,0.0);
	vector<vector<int>> classes; classes.clear();

	initClasses1(classes);

	const char *path = "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB";
	Similarity b(path,0);
	//b.load_size_function();
	for(int i=0;i<457;i++){
		int cpt=0;
		int req = i;
		int ic = getID(classes,i);
		int nc = classes[ic].size();
		b.QuerySF(i);
		//std::cout<<"Salut\n"<<endl;
		for(int j=0;j<457;j++){
			string s = b.ItemList[j].getString();
			//std::cout<<s<<endl;
			int res = atoi(s.substr(1,s.size()).c_str());
			if(contain(classes[ic],res)) cpt++;
			precision[j] = (double)cpt/((double)(j+1));
			recall[j] = (double)cpt/(double)nc;
		}
#pragma omp parallel for
		for(int j=0;j<457;j++){
			precisionS[j] += precision[j];
			recallS[j] += recall[j];
		}
		std::cout<<"\nRéussite pour \n"<<i<< endl;
	}
	FILE *f = fopen("C:\\Dossier Ayoub\\3D\\psb_v1\\curves\\recall_Size_Function_PCA_Pseudo_McGill_haus.txt","w");
	if(f==NULL){
		cout<<"Erreur d'écriture du fichier " << endl;
		exit(-1);
	}
	for(int i=0;i<457;i++){
		fprintf(f,"%lf\t%lf\n",recallS[i]/457.0,precisionS[i]/457.0);
	}
	fclose(f);
}
