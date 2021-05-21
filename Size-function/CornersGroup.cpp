#include "StdAfx.h"
#include "CornersGroup.h"
#include "dirProcessing.h"
#include "hausdorff.h"
CornersGroup::CornersGroup(void)
{
	descripteur.clear();
}

CornersGroup::CornersGroup(std::vector<std::string> fList ){
	//vector<std::string> fList = getFilesFromDir(f);
	int k = (int)fList.size();
	descripteur.resize(k);
	
	for(int i=0;i<k;i++){
		descripteur[i] = read_ang_pt(fList[i].c_str());
		//read_ang_pt(fList[i].c_str());
	}
	
}
/*void CornersGroup::loadFromDirectory(const std::string &f){
	vector<std::string> fList = getFilesFromDir(f);
	int k = (int)fList.size();
	descripteur.resize(k);
	//printf("\n k = %d",k);
	
	for(int i=0;i<k;i++){
		descripteur[i] = read_ang_pt(fList[i].c_str());
	}
}*/

CornersGroup::~CornersGroup(void)
{
	destroy();
}
void CornersGroup::destroy()
{
	int k = descripteur.size();
	for(int i=0;i<k;i++)
		destroy_all_ang_pt(&descripteur[i]);
	descripteur.clear();
}

CornersGroup::CornersGroup(CornersGroup &sf){
	descripteur.clear();
	descripteur.assign(sf.descripteur.begin(),sf.descripteur.end());
}
CornersGroup::CornersGroup(const CornersGroup &sf){
	descripteur.clear();
	descripteur.assign(sf.descripteur.begin(),sf.descripteur.end());
}

double CornersGroup::DistanceSF(const CornersGroup &b){
	int k1 = descripteur.size();
	int k2 = b.descripteur.size();


	//printf("\n k1 = %d",k1);
	//printf("\n k2 = %d",k2);
	double S1 = 0.0,d, res=0.0;
	
		if(k1>=k2){
		for(int i=0;i<k2;i++){
			d = hausdorff(b.descripteur[i],descripteur[0]) ;
			//printf("\nval %f",d);
			for(int j=1;j<k1;j++){
				if(d>hausdorff(b.descripteur[i],descripteur[j])){
					d=hausdorff(b.descripteur[i],descripteur[j]);

				}
			}
			S1+=d;
		}
		res=S1/k2;
	}
	else{
		for(int i=0;i<k1;i++){
			d = hausdorff(descripteur[i],b.descripteur[0]) ;
			for(int j=1;j<k2;j++)
				if(d>hausdorff(descripteur[i],b.descripteur[j])){
					d=hausdorff(descripteur[i],b.descripteur[j]);

				}
			S1+=d;
		}
		 res =S1/k1 ;
	}
	return res;

}

CornersGroup & CornersGroup::operator =(const CornersGroup &b){
	if(this!=&b){
		descripteur.clear();
		descripteur.assign(b.descripteur.begin(),b.descripteur.end());
	}
	return *this;
}
CornersGroup & CornersGroup::operator =(CornersGroup &b){
	if(this!=&b){
		descripteur.clear();
		descripteur.assign(b.descripteur.begin(),b.descripteur.end());
	}
	return *this;
}
