#include "StdAfx.h"
#include "CornersGroup2.h"

#include "dirProcessing.h"

CornersGroup2::CornersGroup2(void)
{
	descripteur.clear();
}

CornersGroup2::CornersGroup2(std::vector<std::string> fList){
	//vector<std::string> fList = getFilesFromDir(f);
	int k = (int)fList.size();
	descripteur.resize(k);
	for(int i=0;i<k;i++){
		descripteur[i].loadFile(fList[i]);
	}
}

CornersGroup2::~CornersGroup2(void)
{
	descripteur.clear();
}
void CornersGroup2::destroy()
{
	descripteur.clear();
}
CornersGroup2::CornersGroup2(CornersGroup2 &sf){
	descripteur.clear();
	descripteur.assign(sf.descripteur.begin(),sf.descripteur.end());
}
CornersGroup2::CornersGroup2(const CornersGroup2 &sf){
	descripteur.clear();
	descripteur.assign(sf.descripteur.begin(),sf.descripteur.end());
}

double CornersGroup2::Pseudo_DistanceSF(CornersGroup2 b){
	int k1 = descripteur.size();
	int k2 = b.descripteur.size();

	//printf("\n k1 = %d",k1);
	//printf("\n k2 = %d",k2);
	double S1 = 0.0,d, res=0.0;

		if(k1>=k2){
		for(int i=0;i<k2;i++){
			//d = hausdorff(b.descripteur[i],descripteur[0]) ;
			d = b.descripteur[i].DistanceSF(descripteur[0]);
			//printf("\nval %f",d);
			for(int j=1;j<k1;j++){
				if(d>b.descripteur[i].DistanceSF(descripteur[j])){
					d=b.descripteur[i].DistanceSF(descripteur[j]);

				}
			}
			S1+=d;
		}
		res=S1/k2;
	}
	else{
		for(int i=0;i<k1;i++){
			d = descripteur[i].DistanceSF(b.descripteur[0]) ;
			for(int j=1;j<k2;j++)
				if(d>descripteur[i].DistanceSF(b.descripteur[j])){
					d=descripteur[i].DistanceSF(b.descripteur[j]);

				}
			S1+=d;
		}
		 res =S1/k1 ;
	}
	return res;

}

CornersGroup2 & CornersGroup2::operator =(const CornersGroup2 &b){
	if(this!=&b){
		descripteur.clear();
		descripteur.assign(b.descripteur.begin(),b.descripteur.end());
	}
	return *this;
}
CornersGroup2 & CornersGroup2::operator =(CornersGroup2 &b){
	if(this!=&b){
		descripteur.clear();
		descripteur.assign(b.descripteur.begin(),b.descripteur.end());
	}
	return *this;
}
