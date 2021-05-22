#include "StdAfx.h"
#include "Shape2D.h"
#include <stdlib.h>
#include <math.h>

Shape2D::Shape2D() {
	clear();
}
Shape2D::Shape2D(const string &fileName) {

	clear();
	load(fileName);
}
Shape2D::Shape2D(const vector<vec2> &list) {

	clear();
	Plist.assign(list.begin(),list.end());
}
void Shape2D::clear(){
	fname ="";
	nb_inv = 0;
	nb_invd =0;
	tab.clear();
	tab1.clear();
	Plist.clear();
	Pnlist.clear();
	PnDlist.clear();
	PnDDlist.clear();
	DelList.clear();
	Ku.clear();
}

void Shape2D::load(const string &fileName){
	clear();
	fname = fileName;
	FILE *f = fopen(fname.c_str(),"r");
	if(f==NULL){
		cout<<"Erreur de lecture du fichier " << fname.c_str() << endl;
		exit(-1);
	}
	int x,y;
	int nPoints;
	char c;
	fscanf(f,"%c\t%d",&c,&nPoints);
	//cout<< nPoints<< endl;

	Plist.resize(nPoints);
	for(int i=0;i<nPoints;i++){
		fscanf(f,"%d %d",&x,&y);
		Plist[i][0] = x;
		Plist[i][1] = y;
	}
	fclose(f);
	//Plist.pop_back();
	//cout<< fname.c_str() <<" Chargé "<< endl;
}
void Shape2D::display(){
	int nb = Plist.size();
	for(int i=0;i<nb;i++){
		cout<<"("<<Plist[i][0]<<","<<Plist[i][1]<<")"<<endl;
	}
}
void Shape2D::displayDerivee_1ere(){
	int nb = PnDlist.size();
	cout<<"Affichage de la dérivée 1ère"<<endl;
	for(int i=0;i<nb;i++){
		cout<<"("<<PnDlist[i][0]<<","<<PnDlist[i][1]<<")"<<endl;
	}
}
void Shape2D::displayDerivee_2nd(){
	int nb = PnDDlist.size();
	cout<<"Affichage de la dérivée seconde"<<endl;
	for(int i=0;i<nb;i++){
		cout<<"("<<PnDDlist[i][0]<<","<<PnDDlist[i][1]<<")"<<endl;
	}
}
void Shape2D::displayProcessedShape(){
	int nb = Plist.size();
	for(int i=0;i<nb;i++){
		cout<<"("<<Plist[i][0]<<","<<Pnlist[i][1]<<")"<<endl;
	}
}
Shape2D::Shape2D(Shape2D & Q){
	clear();
	fname = Q.fname;
	Plist.assign(Q.Plist.begin(),Q.Plist.end());
	if(Q.DelList.size()>0) DelList.assign(Q.DelList.begin(),Q.DelList.end());
	if(Q.Ku.size()>0) Ku.assign(Q.Ku.begin(),Q.Ku.end());
	if(Q.tab.size()>0) tab.assign(Q.tab.begin(),Q.tab.end());
	if(Q.tab1.size()>0) tab1.assign(Q.tab1.begin(),Q.tab1.end());
	if(Q.Pnlist.size()>0) Pnlist.assign(Q.Pnlist.begin(),Q.Pnlist.end());
	if(Q.PnDlist.size()>0) PnDlist.assign(Q.PnDlist.begin(),Q.PnDlist.end());
	if(Q.PnDDlist.size()>0) PnDDlist.assign(Q.PnDDlist.begin(),Q.PnDDlist.end());
}
Shape2D & Shape2D::operator =(const Shape2D &Q){
	if(this != &Q){
		clear();
		fname = Q.fname;
		Plist.assign(Q.Plist.begin(),Q.Plist.end());
		if(Q.DelList.size()>0) DelList.assign(Q.DelList.begin(),Q.DelList.end());
		if(Q.Ku.size()>0) Ku.assign(Q.Ku.begin(),Q.Ku.end());
		if(Q.tab.size()>0) tab.assign(Q.tab.begin(),Q.tab.end());
		if(Q.tab1.size()>0) tab1.assign(Q.tab1.begin(),Q.tab1.end());
		if(Q.Pnlist.size()>0) Pnlist.assign(Q.Pnlist.begin(),Q.Pnlist.end());
		if(Q.PnDlist.size()>0) PnDlist.assign(Q.PnDlist.begin(),Q.PnDlist.end());
		if(Q.PnDDlist.size()>0) PnDDlist.assign(Q.PnDDlist.begin(),Q.PnDDlist.end());
	}
	return *this;
}
float Shape2D::Gaussian(float p, float sigma){
	//float ret=(float)(-1.0*pow((double)p,2.0));
	//ret=ret/(2.*pow(sigma,2));
	//ret=exp(ret);
	//ret=ret/sigma;
	float res = (float)(-1.0*pow((double)p,2.0));
	res /= 2.0*sigma*sigma;
	res = exp(res);
	res *= (float)(1.0/(sqrt(2.0*3.14159265358979323846)*sigma));
	return (res);
}


void Shape2D::smooth(float sigma){
	PnDDlist.clear();
	int k = Plist.size();
	Pnlist.clear();
	Pnlist.resize(k);
	float tf1,tf2,tf;
	int j,i,ok;
	for(j=0;j<k;j++){
		tf1=0.0;
		tf2=0.0;
		for(i=-(k/2-1);i<k/2;i++){
			ok=j+1-i;
			if(ok<0) ok=ok+k; else ok%=k;
			tf=Gaussian((int)fabsf((float)i),sigma);
			tf1 +=Plist[ok][0]*tf;
			tf2 +=Plist[ok][1]*tf;
		}
		Pnlist[j][0] = tf1;
		Pnlist[j][1] = tf2;
		//cout<<Pnlist[j][0]<<"\t"<<Pnlist[j][1]<<endl;
	}
}
void Shape2D::calcul_derivee(){
	int k = Pnlist.size();
	PnDlist.clear();
	PnDDlist.clear();
	PnDDlist.resize(k);
	PnDlist.resize(k);

	//Calcul derivée première
	//cout<<"Calcul derivée première"<<endl;
	PnDlist[0][0] = (Pnlist[1][0]-Pnlist[k-1][0]);
	PnDlist[0][1] = (Pnlist[1][1]-Pnlist[k-1][1]) ;
	//cout<<PnDlist[0][0]<<"\t"<<PnDlist[0][1]<<endl;
	for(int i=1;i<k;i++){
		PnDlist[i][0] =(Pnlist[(i+1)%k][0]-Pnlist[i-1][0]);
		PnDlist[i][1] =(Pnlist[(i+1)%k][1]-Pnlist[i-1][1]);
		//cout<<PnDlist[i][0]<<"\t"<<PnDlist[i][1]<<endl;
	}

	//Calcul derivée seconde
	//cout<<"Calcul derivée seconde"<<endl;
	PnDDlist[0][0] = Pnlist[1][0]+Pnlist[k-1][0]-2*Pnlist[0][0];
	PnDDlist[0][1] = Pnlist[1][1]+Pnlist[k-1][1]-2*Pnlist[0][1];
	//cout<<PnDDlist[0][0]<<"\t"<<PnDDlist[0][1]<<endl;
	for(int i=1;i<k;i++){
		PnDDlist[i][0] = Pnlist[(i+1)%k][0]+Pnlist[i-1][0]-2*Pnlist[i][0];
		PnDDlist[i][1] = Pnlist[(i+1)%k][1]+Pnlist[i-1][1]-2*Pnlist[i][1];
		//cout<<PnDDlist[i][0]<<"\t"<<PnDDlist[i][1]<<endl;
	}
}

float Shape2D::d_orientation(int i, int ind, int j){
	float F = (Pnlist[ind][0]-Pnlist[i][0])*(Pnlist[j][0]-Pnlist[ind][0])+
			(Pnlist[ind][1]-Pnlist[i][1])*(Pnlist[j][1]-Pnlist[ind][1]);
	float G =(Pnlist[ind][0]-Pnlist[i][0])*(Pnlist[j][1]-Pnlist[ind][1])-
			(Pnlist[ind][1]-Pnlist[i][1])*(Pnlist[j][0]-Pnlist[ind][0]);
	return atan(F/G);
}
float Shape2D::d_orientation(int i){
	float teta,x,y;
	double pi = 3.1415926535;
	x=Pnlist[DelList[i].debut][0]+Pnlist[DelList[i].fin][0]-2*Pnlist[DelList[i].indice_d][0];
	y=Pnlist[DelList[i].debut][1]+Pnlist[DelList[i].fin][1]-2*Pnlist[DelList[i].indice_d][1];

	teta =atan2(fabs(y),fabs(x));
	if(x<0 && y>0) teta = pi-teta ;
	else if(x<0 && y<0) teta = pi+teta ;
	else if(x>0 && y<0) teta = 2*pi-teta ;
	return teta ;
}
float Shape2D::Distance_token(const Del &a, const Del &b){
	float d1,d2 ;
	d1=fabs(a.courbure-b.courbure);
	d2=fabs(a.orientation-b.orientation);
	return (d1+d2)/2 ;
}
int Shape2D::max_courbure(int i){
	int ind=i ;
	float mk =Ku[tab[i]];
	for(int j=tab[i]; j<tab[i+1];j++){
		if(mk<Ku[j]){
			mk=Ku[j];
			ind=j;
		}
	}
	return ind ;
}
int Shape2D::min_courbure(int i){
	int ind=i ;
	float mk =Ku[tab[i]];
	for(int j=tab[i]; j<tab[i+1];j++){
		if(mk>Ku[j]){
			mk=Ku[j];
			ind=j;
		}
	}
	return ind ;
}
float Shape2D::Distance(const Shape2D &b){
	float resultat;
	double d,S1=0 ;
	if(nb_invd>=b.nb_invd){
		for(int i=0;i<b.nb_invd;i++){
			d = Distance_token(b.DelList[i],DelList[0]) ;
			for(int j=1;j<nb_invd;j++){
				if(d>Distance_token(b.DelList[i],DelList[j]))
					d=Distance_token(b.DelList[i],DelList[j]);
			S1+=d;
			}
		}
		resultat=S1/b.nb_invd;
	}
	else{
		for(int i=0;i<nb_invd;i++){
			d = Distance_token(DelList[i],b.DelList[0]) ;
			for(int j=1;j<b.nb_invd;j++)
				if(d>Distance_token(DelList[i],b.DelList[j]))
					d=Distance_token(DelList[i],b.DelList[j]);
			S1+=d;
		}
		 resultat =S1/nb_invd ;
	}
	return resultat;
}
float Shape2D::Distance(const Del &b){
	//float resultat;
	double d = Distance_token(b,DelList[0]);
	int nb = DelList.size();
	for(int i=1;i<nb;i++){
		d = min(d,(double)Distance_token(DelList[i],b));
	}
	return (float)d;
}
void Shape2D::computeKu(){
	int k = Plist.size();
	Ku.clear();
	Ku.resize(k);
	for(int ii=0;ii<k;ii++){
		Ku[ii]=(PnDlist[ii][0]*PnDDlist[ii][1]-PnDDlist[ii][0]*PnDlist[ii][1])/pow((pow(PnDlist[ii][0],2)+pow(PnDlist[ii][1],2)),3/2);
		//cout<<"Ku["<<ii<<"]="<<Ku[ii]<<endl;
	}
}

/*void Shape2D::descripteur(){
	int k = Plist.size();
	Ku.clear();
	Ku.resize(k);
	tab1.clear();
	//tab1.resize(k);
	nb_inv =0;
	nb_invd =0;
	//smooth(sigma);
	calcul_derivee();
	//calcul_derivee();
	computeKu();
	//for(int ii=0;ii<k;ii++)
		//Ku[ii]=(PnDlist[ii][0]*PnDDlist[ii][1]-PnDDlist[ii][0]*PnDlist[ii][1])/pow((pow(PnDlist[ii][0],2)+pow(PnDlist[ii][1],2)),3/2);
	if(Ku[1]*Ku[k-1]<0){
		//tab1[nb_inv] =0 ;
		tab1.push_back(0);
		nb_inv++;
	}
	for(int i=1;i<k;i++){
		if(Ku[i-1]*Ku[(i+1)%k]<0){
			//tab1[nb_inv] =i ;
			tab1.push_back(i);
			nb_inv++;
		}
	}
	tab.clear();
	//tab.resize(nb_inv);
	DelList.clear();
	//DelList.resize((nb_inv/2)+1);
	int nb=0;
	if(Ku[1]*Ku[k-1]<0 && Ku[0]*Ku[k-2]<0){
		for(int j=0;j<nb_inv;j++)
			if(j%2==0){
				//tab[nb]=tab1[j];
				//nb++;
				tab.push_back(tab1[j]);
			}
	}
	else{
		for(int j=0;j<nb_inv;j++)
			if(j%2!=0){
				//tab[nb]=tab1[j];
				//nb++;
				tab.push_back(tab1[j]);
			}
	}
	//Chercher les descripteurs
	int ind;
	nb = tab.size();
	for(int l=0;l<nb-1;l++){
		if(Ku[tab[l]]<Ku[tab[l]+1]){
			ind=max_courbure(l);
			if(ind!=tab[l] &&ind!=tab[(l+1)%k]){
				//DelList[nb_invd].indice_d  = ind ;
				//DelList[nb_invd].courbure = Ku[ind];
				//DelList[nb_invd].orientation=d_orientation(tab[l],ind ,tab[(l+1)%k]);
				Del del;
				del.indice_d  = ind ;
				del.debut = tab[l];
				del.fin = tab[(l+1)%k];
				del.courbure = Ku[ind];
				DelList.push_back(del);
				del.orientation=d_orientation(nb_invd);
				nb_invd++;
			}
		}
	}
	if((tab[nb-1]+1)%k)
	{
		float mk=Ku[tab[nb-1]];
		ind=tab[nb-1];
		if(Ku[tab[nb-1]]<Ku[tab[nb-1]+1])
		{
				for(int j=tab[nb-1];j<k;j++)
				{
						if(mk<Ku[j])
						{
							mk=Ku[j];
							ind=j;
						}
				}
				for(int m=0; m<tab[0];m++)
				{
						if(mk<Ku[m])
						{
							mk=Ku[m];
							ind=m;
						}
				}
				if(ind!=tab[nb-1] && ind!=tab[0])
				{
					//DelList[nb_invd].indice_d  = ind ;
					//DelList[nb_invd].courbure = Ku[ind];
					//DelList[nb_invd].orientation=d_orientation(tab[nb-1],ind ,tab[0]);
					Del del;
					del.indice_d  = ind ;
					del.courbure = Ku[ind];
					del.debut = tab[nb-1];
					del.fin = tab[0];
					DelList.push_back(del);
					del.orientation=d_orientation(nb_invd);
					nb_invd++;
				}
		}
	}
}*/
void Shape2D::descripteur(){
	int n = Plist.size();
	Ku.clear();
	Ku.resize(n);
	tab.clear();
	DelList.clear();
	//tab1.resize(k);
	nb_inv =0;
	nb_invd =0;
	//smooth(sigma);
	calcul_derivee();
	//calcul_derivee();
	computeKu();
	int id = 0;
	for(int i=0;i<n;i++){
		if(Ku[i]==0.0) tab.push_back(i);
	}
	for(int i=0;i<tab.size()-1;i++){
		delBimbo(tab[i],tab[i+1]);
	}
	delBimbo(tab[tab.size()-1],tab[0]);
}
int Shape2D::max_abs_courbure(int begin,int end){
	int ind=begin ;
	float mk =fabsf(Ku[begin]);
	for(int j=begin; j<end;j++){
		float tmp = fabsf(Ku[j]);
		if(mk<tmp){
			mk=tmp;
			ind=j;
		}
	}
	return ind ;
}
void Shape2D::delBimbo(int begin,int end){
	int ind;
	if(end<=begin){
		int a = max_abs_courbure(begin,0);
		int b = max_abs_courbure(0,end);
		ind = (a>b)?a:b;
	}
	else ind = max_abs_courbure(begin,end);
	Del del;
	del.indice_d  = ind ;
	del.debut = begin;
	del.fin = end;
	del.courbure = Ku[ind];
	del.orientation=d_orientation(begin,end,ind);
	DelList.push_back(del);
}

float Shape2D::d_orientationN(int i,int j,int k){
	vec3 A(Pnlist[i][0],Pnlist[i][1],0.0);
	vec3 B(Pnlist[j][0],Pnlist[j][1],0.0);
	vec3 u = B - A;
	vec3 C = B + A;
	C *=2.0;
	vec3 v = vec3(Pnlist[k][0],Pnlist[k][1],0.0) - C;

	vec3 prodVec = u CROSS v;
	float normU = sqrt(u DOT u);
	float normV = sqrt(v DOT v);
	float normProdVec = sqrt(prodVec DOT prodVec);
	float angle = (normU==0 || normV==0)?0.0:asinf(normProdVec/(normU*normV));
	return angle;
}
void Shape2D::saveDelBimboDescriptor(const string &fileName){
	FILE *f = fopen(fileName.c_str(),"w");
	if(f==NULL){
		cout<<"Impossible d'écrire dans le fichier <"<<fileName.c_str()<<">"<<endl;
	}
	int k = DelList.size();
	fprintf(f,"#\t%d\n",k);

	for(int i=0;i<k;i++){
		fprintf(f,"%d\t%d\t%d\t%f\t%f\n",DelList[i].debut,DelList[i].fin,DelList[i].indice_d,DelList[i].courbure,DelList[i].orientation);
	}
	fclose(f);
	//cout<<"Enregistrement réussi pour <"<<fileName.c_str()<<">"<<endl;
}
void Shape2D::displayDelBimboDescriptor(){
	int k = DelList.size();
	cout<<k<<endl;
	for(int i=0;i<k;i++){
		cout<<"Debut= "<<DelList[i].debut<<" Fin= "<<DelList[i].fin<<" Indice ="<<DelList[i].indice_d<<" Courbure ="<<DelList[i].courbure<<" Orientation ="<<DelList[i].orientation<<endl;
	}
}
void Shape2D::normalize(){
	int k = Plist.size();
	vec2 center(0.0,0.0);
	for(int i=0;i<k;i++){
		center += Plist[i];
	}
	center /= 1.0*((float)k);
	for(int i=0;i<k;i++){
		Plist[i] -= center;
	}
	vec2 org(0.0,0.0);
	float max = dist(org,Plist[0]);
	int ind = 0;
	float d=0;
	for(int i=1;i<k;i++){
		d = dist(org,Plist[i]);
		if(max<d){
			max = d;
			ind = i;
		}
	}
	float theta = atan2(Plist[ind][1],Plist[ind][0]);
	float x,y;
	for(int i=0;i<k;i++){
		x = Plist[i][0]; y = Plist[i][1];
		Plist[i][0] = x*cos(-theta) - y*sin(-theta);
		Plist[i][1] = x*sin(-theta) + y*cos(-theta);
	}
	//cout<<"Fin Normalisation de "<<fname.c_str()<<endl;
	//Pnlist.assign(Plist.begin(),Plist.end());
}
Shape2D::~Shape2D() {
	clear();
}

