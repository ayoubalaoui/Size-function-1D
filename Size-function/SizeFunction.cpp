#include "StdAfx.h"
#include "SizeFunction.h"
#include "angpt.h"
#include "spline_curve.h"


SizeFunction::SizeFunction(void)
{
	descripteur.clear();
}

SizeFunction::SizeFunction(const string &f)
{
	descripteur.clear();
	loadFile(f);
}
SizeFunction::~SizeFunction(){
	descripteur.clear();
}
SizeFunction::SizeFunction(SizeFunction &sf){
	descripteur.clear();
	descripteur.assign(sf.descripteur.begin(),sf.descripteur.end());
}
SizeFunction::SizeFunction(const SizeFunction &sf){
	descripteur.clear();
	descripteur.assign(sf.descripteur.begin(),sf.descripteur.end());
}
void SizeFunction::displaySF(){
	int n = descripteur.size();
	for(int i=0;i<n;i++){
		printf("\n%f     %f",descripteur[i][0],descripteur[i][1]);
	}
}
double Distance(vec2 A,vec2 B){
	if(A[1]==MAX_SIZE_TYPEVAL ||  B[1]==MAX_SIZE_TYPEVAL) return (double)min(max(fabsf(A[0]-B[0]),0.0f),MAX_SIZE_TYPEVAL);

	return (double)min(max(fabsf(A[0]-B[0]),fabsf(A[1]-B[1])),max(fabsf(A[1]-A[0])*0.5f,fabsf(B[1]-B[0])*0.5f));
}
double SizeFunction::DistanceSF(SizeFunction b){
	int n1 = (int)descripteur.size();
	int n2 = (int)b.descripteur.size();
	double sum = 0.0f;
	if(n1==n2){
		double mx = Distance(descripteur[0],b.descripteur[0]);
		for(int i=1;i<n1;i++){
			mx = max(mx,Distance(descripteur[i],b.descripteur[i]));
		}
		sum = mx;
	}
	else if(n1>n2){
		double mx = Distance(descripteur[0],b.descripteur[0]);
		for(int i=1;i<n2;i++){
			mx = max(mx,Distance(descripteur[i],b.descripteur[i]));
		}
		for(int i=n2;i<n1;i++){
			if(descripteur[i][1] !=MAX_SIZE_TYPEVAL){
				float d = -descripteur[i][0]-descripteur[i][1];
				//printf("\n d = %f\n",d);
				float x = -d/2.0f;
				float y = -d/2.0f;
				mx = max(mx,Distance(descripteur[i],vec2(x,y)));
			}
		}
		sum = mx;
	}
	else{
		double mx = Distance(descripteur[0],b.descripteur[0]);
		for(int i=1;i<n1;i++){
			mx = max(mx,Distance(descripteur[i],b.descripteur[i]));
		}
		for(int i=n1;i<n2;i++){
			if(b.descripteur[i][1] !=MAX_SIZE_TYPEVAL){
				float d = -b.descripteur[i][0]-b.descripteur[i][1];
				//printf("\nnnnnnnnnn b.descripteur[i][0] = %f\n",b.descripteur[i][0]);
				//printf("\nnnnnnnnnn d = %f\n",d);
				float x = -d/2.0f;
				float y = -d/2.0f;
				mx = max(mx,Distance(b.descripteur[i],vec2(x,y)));
			}
		}
		sum = mx;
	}
	return (double)sum;
}
void SizeFunction::destroy(){
	descripteur.clear();
}
SizeFunction & SizeFunction::operator =(const SizeFunction &b){
	if(this!=&b){
		descripteur.clear();
		descripteur.assign(b.descripteur.begin(),b.descripteur.end());
	}
	return *this;
}
SizeFunction & SizeFunction::operator =(SizeFunction &b){
	if(this!=&b){
		descripteur.clear();
		descripteur.assign(b.descripteur.begin(),b.descripteur.end());
	}
	return *this;
}

void SizeFunction::sampleAndSave(const std::string &fname){
	if(descripteur.size()<1) return;
	vector<vec2> tmp;
	tmp.clear();
	
	//tmp.assign(descripteur.begin()+1,descripteur.end());
	tmp.push_back(vec2(descripteur[0][0],0.0f));
	for(int i=1;i<descripteur.size();i++){
		tmp.push_back(descripteur[i]);
	}
	spline_curve sp(tmp);
	tmp.clear();
	sp.compute();
	FILE *f = fopen(fname.c_str(),"w");
	if(f==NULL){
		cout<<"Erreur d'ecriture du fichier " << fname.c_str() << endl;
		exit(-1);
	}
	
	int k = (int)sp.curve_point.size();
	fprintf(f,"# %d\n",k);

	for(int i=0;i<k;i++){
		fprintf(f,"%f\t%f\n",sp.curve_point[i][0],sp.curve_point[i][1]);
	}
	fclose(f);
	sp.destroy();
	std::cout<<"Well done for : "<<fname.c_str()<<endl;
}
void SizeFunction::loadFile(const std::string &fname){
	FILE *f = fopen(fname.c_str(),"r");
	if(f==NULL){
		cout<<"Erreur de lecture du fichier " << fname.c_str() << endl;
		exit(-1);
	}
	/*int n;
	fscanf(f,"#\t%d\n",&n);
	//printf("\n%f %s",sigma,fileName.c_str());
	//printf("\n%d",n);
	descripteur.resize(n);
	for(int i=0;i<n;i++){
		fscanf(f,"%f\t%f\n",&descripteur[i][0],&descripteur[i][1]);
	}
	*/
	char	chkchr;

	int	k,l;
	float x,y;
	k = l = 0;

	while (feof(f) == 0)
	{
		fscanf(f,"%c",&chkchr);
		if (chkchr == 'p')
			fscanf(f,"%d %f %f\n",&k,&x,&y);
		else if (chkchr == 'l')
		{
			fscanf(f,"%d %f\n",&l,&x);
			y = MAX_SIZE_TYPEVAL;
		}
		else
		{
			fprintf(stderr,"I can't read neither 'l' nor 'p' at the begin of line\n");
			exit(2);
		}
		descripteur.push_back(vec2(x,y));
		//ins_ang_pt (&size,new_ang_pt(x,y));
	}

	fclose(f);
}
