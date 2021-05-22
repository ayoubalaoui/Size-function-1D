#include "StdAfx.h"
#include "TriMesh_algo.h"

#include <stdio.h>
#include "TriMesh.h"

void save_part(const TriMesh *mesh,vector<bool> toProcess,char *out){
	TriMesh * tmp = new TriMesh(*mesh);

	remove_vertices(tmp,toProcess);
	int k = tmp->faces.size();
	if(k==0) {
		printf("\nProblem k=%d\n",k);
		system("pause");

		return;}
	remove_unused_vertices(tmp);

	tmp->write(out);

	delete tmp;
}
void split_mesh_2_regions_X(const TriMesh *mesh,char *out){
	int n = (int)mesh->vertices.size();

	vector<bool> toProcess1(n,true);
	vector<bool> toProcess2(n,true);

	vector<int> cpt(2,0);
//#pragma omp parallel for
	for(int i=0;i<n;i++){
		const point & p = mesh->vertices[i];
		if(p[0]>=0.0f ){
			toProcess1[i] = false;
			cpt[0]++;
		}
		if(p[0]<=0.0f ){
			toProcess2[i] = false;
			cpt[1]++;
		}
	}

	char * pname = new char[1024];

	sprintf(pname,"%s_1.off",out);

	if(cpt[0]>0) save_part(mesh,toProcess1,pname);
	toProcess1.clear();

	sprintf(pname,"%s_2.off",out);

	if(cpt[1]>0) save_part(mesh,toProcess2,pname);
	toProcess2.clear();

	cpt.clear();

	delete pname;
}
void split_mesh_2_regions_Y(const TriMesh *mesh,char *out){
	int n = (int)mesh->vertices.size();

	vector<bool> toProcess1(n,true);
	vector<bool> toProcess2(n,true);

	vector<int> cpt(2,0);
//#pragma omp parallel for
	for(int i=0;i<n;i++){
		const point & p = mesh->vertices[i];
		if(p[1]>=0.0f ){
			toProcess1[i] = false;
			cpt[0]++;
		}
		if(p[1]<=0.0f ){
			toProcess2[i] = false;
			cpt[1]++;
		}
	}

	char * pname = new char[1024];

	sprintf(pname,"%s_1.off",out);

	if(cpt[0]>0) save_part(mesh,toProcess1,pname);
	toProcess1.clear();

	sprintf(pname,"%s_2.off",out);

	if(cpt[1]>0) save_part(mesh,toProcess2,pname);
	toProcess2.clear();

	cpt.clear();

	delete pname;
}
void split_mesh_2_regions_Z(const TriMesh *mesh,char *out){
	int n = (int)mesh->vertices.size();

	vector<bool> toProcess1(n,true);
	vector<bool> toProcess2(n,true);

	vector<int> cpt(2,0);
//#pragma omp parallel for
	for(int i=0;i<n;i++){
		const point & p = mesh->vertices[i];
		if(p[2]>=0.0f ){
			toProcess1[i] = false;
			cpt[0]++;
		}
		if(p[2]<=0.0f ){
			toProcess2[i] = false;
			cpt[1]++;
		}
	}

	char * pname = new char[1024];

	sprintf(pname,"%s_1.off",out);

	if(cpt[0]>0) save_part(mesh,toProcess1,pname);
	toProcess1.clear();

	sprintf(pname,"%s_2.off",out);

	if(cpt[1]>0) save_part(mesh,toProcess2,pname);
	toProcess2.clear();

	cpt.clear();

	delete pname;
}
void split_mesh_4_regions_Z(const TriMesh *mesh,char *out){
	int n = (int)mesh->vertices.size();

	vector<bool> toProcess1(n,true);
	vector<bool> toProcess2(n,true);
	vector<bool> toProcess3(n,true);
	vector<bool> toProcess4(n,true);

	vector<int> cpt(4,0);
#pragma omp parallel for
	for(int i=0;i<n;i++){
		const point & p = mesh->vertices[i];
		if(p[0]>=0.0f && p[1]<=0.0f ){
			toProcess1[i] = false;
			cpt[0]++;
		}
		else if(p[0]>=0.0f && p[1]>=0.0f){
			toProcess2[i] = false;
			cpt[1]++;
		}
		else if(p[0]<=0.0f && p[1]>=0.0f){
			toProcess3[i] = false;
			cpt[2]++;
		}
		else if(p[0]<=0.0f && p[1]<=0.0f){
			toProcess4[i] = false;
			cpt[3]++;
		}
	}

	char * pname = new char[1024];

	sprintf(pname,"%s_1.off",out);

	if(cpt[0]>0) save_part(mesh,toProcess1,pname);
	toProcess1.clear();

	sprintf(pname,"%s_2.off",out);

	if(cpt[1]>0) save_part(mesh,toProcess2,pname);
	toProcess2.clear();

	sprintf(pname,"%s_3.off",out);

	if(cpt[2]>0) save_part(mesh,toProcess3,pname);
	toProcess3.clear();

	sprintf(pname,"%s_4.off",out);

	if(cpt[3]>0) save_part(mesh,toProcess4,pname);
	toProcess4.clear();


	cpt.clear();

	delete pname;

}

void split_mesh_4_regions_X(const TriMesh *mesh,char *out){
	int n = (int)mesh->vertices.size();

	vector<bool> toProcess1(n,true);
	vector<bool> toProcess2(n,true);
	vector<bool> toProcess3(n,true);
	vector<bool> toProcess4(n,true);

	vector<int> cpt(4,0);
#pragma omp parallel for
	for(int i=0;i<n;i++){
		const point & p = mesh->vertices[i];
		if(p[2]>=0.0f && p[1]<=0.0f ){
			toProcess1[i] = false;
			cpt[0]++;
		}
		else if(p[2]>=0.0f && p[1]>=0.0f){
			toProcess2[i] = false;
			cpt[1]++;
		}
		else if(p[2]<=0.0f && p[1]>=0.0f){
			toProcess3[i] = false;
			cpt[2]++;
		}
		else if(p[2]<=0.0f && p[1]<=0.0f){
			toProcess4[i] = false;
			cpt[3]++;
		}
	}

	char * pname = new char[1024];

	sprintf(pname,"%s_1.off",out);

	if(cpt[0]>0) {
		printf("\n1");
		//system("pause");
		save_part(mesh,toProcess1,pname);}
	toProcess1.clear();

	sprintf(pname,"%s_2.off",out);

	if(cpt[1]>0) {
		printf("\n2");
		//system("pause");
		save_part(mesh,toProcess2,pname);
	}
	toProcess2.clear();

	sprintf(pname,"%s_3.off",out);

	if(cpt[2]>0) save_part(mesh,toProcess3,pname);
	toProcess3.clear();

	sprintf(pname,"%s_4.off",out);

	if(cpt[3]>0) save_part(mesh,toProcess4,pname);
	toProcess4.clear();

	cpt.clear();

	delete pname;
}

void split_mesh_4_regions_Y(const TriMesh *mesh,char *out){
	int n = (int)mesh->vertices.size();

	vector<bool> toProcess1(n,true);
	vector<bool> toProcess2(n,true);
	vector<bool> toProcess3(n,true);
	vector<bool> toProcess4(n,true);

	vector<int> cpt(4,0);
#pragma omp parallel for
	for(int i=0;i<n;i++){
		const point & p = mesh->vertices[i];
		if(p[2]>=0.0f && p[0]<=0.0f ){
			toProcess1[i] = false;
			cpt[0]++;
		}
		else if(p[2]>=0.0f && p[0]>=0.0f){
			toProcess2[i] = false;
			cpt[1]++;
		}
		else if(p[2]<=0.0f && p[0]>=0.0f){
			toProcess3[i] = false;
			cpt[2]++;
		}
		else if(p[2]<=0.0f && p[0]<=0.0f){
			toProcess4[i] = false;
			cpt[3]++;
		}
	}

	char * pname = new char[1024];

	sprintf(pname,"%s_1.off",out);

	if(cpt[0]>0) save_part(mesh,toProcess1,pname);
	toProcess1.clear();

	sprintf(pname,"%s_2.off",out);

	if(cpt[1]>0) save_part(mesh,toProcess2,pname);
	toProcess2.clear();

	sprintf(pname,"%s_3.off",out);

	if(cpt[2]>0) save_part(mesh,toProcess3,pname);
	toProcess3.clear();

	sprintf(pname,"%s_4.off",out);

	if(cpt[3]>0) save_part(mesh,toProcess4,pname);
	toProcess4.clear();


	cpt.clear();

	delete pname;
}