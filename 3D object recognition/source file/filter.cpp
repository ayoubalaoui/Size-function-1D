#include "StdAfx.h"

#include "filter.h"

#include <omp.h>
#include <stdio.h>
#include "TriMesh.h"
#include "TriMesh_algo.h"
#include "lineqn.h"
#include <numeric>
#include "SizeElement.h"

//#include "splineInterpolator.h"
#include <stack>

using namespace std;


// Quick 'n dirty portable random number generator
static inline float tinyrnd()
{
	static unsigned trand = 0;
	trand = 1664525u * trand + 1013904223u;
	return (float) trand / 4294967296.0f;
}


// Create an offset surface from a mesh.  Dumb - just moves along the
// normal by the given distance, making no attempt to avoid self-intersection.
// Eventually, this could/should be extended to use the method in
//  Peng, J., Kristjansson, D., and Zorin, D.
//  "Interactive Modeling of Topologically Complex Geometric Detail"
//  Proc. SIGGRAPH, 2004.
void inflate(TriMesh *mesh, float amount)
{
	mesh->need_normals();

	TriMesh::dprintf("Creating offset surface... ");
	int nv = mesh->vertices.size();
#pragma omp parallel for
	for (int i = 0; i < nv; i++)
		mesh->vertices[i] += amount * mesh->normals[i];
	TriMesh::dprintf("Done.\n");
	mesh->bbox.valid = false;
	mesh->bsphere.valid = false;
}


// Transform the mesh by the given matrix
void apply_xform(TriMesh *mesh, const xform &xf)
{
	int nv = mesh->vertices.size();
	for (int i = 0; i < nv; i++)
		mesh->vertices[i] = xf * mesh->vertices[i];
	if (!mesh->normals.empty()) {
		xform nxf = norm_xf(xf);
		for (int i = 0; i < nv; i++) {
			mesh->normals[i] = nxf * mesh->normals[i];
			normalize(mesh->normals[i]);
		}
	}
}


// Translate the mesh
void trans(TriMesh *mesh, const vec &transvec)
{
	apply_xform(mesh, xform::trans(transvec));
}


// Rotate the mesh by r radians
void rot(TriMesh *mesh, float r, const vec &axis)
{
	apply_xform(mesh, xform::rot(r, axis));
}
void rot(TriMesh *mesh, float r,const vec &center, const vec &axis)
{
	trans(mesh,-center);
	//apply_xform(mesh, xform::trans(-center)*xform::rot(r, axis)*xform::trans(center));
	rot(mesh,r,axis);
	trans(mesh,center);
}


void align(TriMesh *mesh,const vec &axis){
	//point center = mesh_center_of_mass(mesh);
	//trans(mesh,-center);
	//to X
	//float theta = atan2f(axis[0],axis[1]);
	//float phi = atan2f(axis[2],sqrtf(axis[1]*axis[1]+axis[0]*axis[0]));
	//apply_xform(mesh,xform::rot(phi,0.0,1.0,0.0)*xform::rot(theta,0.0,0.0,1.0));
	//to Z
	float theta = atan2f(axis[0],axis[2]);
	float phi = asin(axis[1]/len2(axis));
	apply_xform(mesh,xform::rot(phi,1.0,0.0,0.0)*xform::rot(theta,0.0,1.0,0.0));
	//to Y
	//float theta = atan2f(axis[1],axis[2]);
	//float phi = asinf(axis[0]/len2(axis));
	//apply_xform(mesh,xform::rot(phi,0.0,0.0,1.0)*xform::rot(theta,1.0,0.0,0.0));
	//trans(mesh,center);
}


void alignInv(TriMesh *mesh,const vec &axis){
	point center = mesh_center_of_mass(mesh);
	trans(mesh,-center);
	float theta = atan2f(axis[1],axis[0]);
	float phi = atan2f(sqrtf(axis[1]*axis[1]+axis[0]*axis[0]),axis[2]);
	apply_xform(mesh,xform::rot(-theta,0.0,0.0,1.0)*xform::rot(-phi,0.0,1.0,0.0));
	//apply_xform(mesh,xform::rot(-theta,0.0,0.0,1.0)*xform::rot(-phi,0.0,1.0,0.0));
	trans(mesh,center);
}
point getMaxPoint(point p,TriMesh *mesh){
	point MaxP = mesh->vertices[0];
	float max = dist(p,MaxP);
	int nv = mesh->vertices.size();
	for(int i=1;i<nv;i++){
		float tmp = dist(p,mesh->vertices[i]);
		if(tmp>max){
			max = tmp;
			MaxP = mesh->vertices[i];
		}
	}
	return MaxP;
}

float getMaxDistance(point p,TriMesh *mesh){
	point MaxP = mesh->vertices[0];
	float max = dist(p,MaxP);
	int nv = mesh->vertices.size();
	for(int i=1;i<nv;i++){
		float tmp = dist(p,mesh->vertices[i]);
		if(tmp>max){
			max = tmp;
			MaxP = mesh->vertices[i];
		}
	}
	return max;
}

// Scale the mesh - isotropic
void scale(TriMesh *mesh, float s)
{
	apply_xform(mesh, xform::scale(s));
}

// Scale the mesh - anisotropic in X, Y, Z
void scale(TriMesh *mesh, float sx, float sy, float sz)
{
	apply_xform(mesh, xform::scale(sx, sy, sz));
}


// Scale the mesh - anisotropic in an arbitrary direction
void scale(TriMesh *mesh, float s, const vec &d)
{
	apply_xform(mesh, xform::scale(s, d));
}


// Clip mesh to the given bounding box
void clip(TriMesh *mesh, const TriMesh::BBox &b)
{
	int nv = mesh->vertices.size();
	vector<bool> toremove(nv, false);
	for (int i = 0; i < nv; i++)
		if (mesh->vertices[i][0] < b.min[0] ||
		    mesh->vertices[i][0] > b.max[0] ||
		    mesh->vertices[i][1] < b.min[1] ||
		    mesh->vertices[i][1] > b.max[1] ||
		    mesh->vertices[i][2] < b.min[2] ||
		    mesh->vertices[i][2] > b.max[2])
			toremove[i] = true;

	remove_vertices(mesh, toremove);
}


// Find center of mass of a bunch of points
point point_center_of_mass(const vector<point> &pts)
{
	point com = accumulate(pts.begin(), pts.end(), point());
	return com / (float) pts.size();
}


// Find (area-weighted) center of mass of a mesh
point mesh_center_of_mass(TriMesh *mesh)
{
	if (mesh->faces.empty() && mesh->tstrips.empty())
		return point_center_of_mass(mesh->vertices);

	point com;
	float totwt = 0;

	mesh->need_faces();
	int nf = mesh->faces.size();
	for (int i = 0; i < nf; i++) {
		const point &v0 = mesh->vertices[mesh->faces[i][0]];
		const point &v1 = mesh->vertices[mesh->faces[i][1]];
		const point &v2 = mesh->vertices[mesh->faces[i][2]];

		point face_com = (v0+v1+v2) / 3.0f;
		float wt = len(trinorm(v0,v1,v2));
		com += wt * face_com;
		totwt += wt;
	}
	return com / totwt;
}


// Compute covariance of a bunch of points
void point_covariance(const vector<point> &pts, float C[3][3])
{
	for (int j = 0; j < 3; j++)
		for (int k = 0; k < 3; k++)
			C[j][k] = 0.0f;

	int n = pts.size();
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < 3; j++)
			for (int k = j; k < 3; k++)
				C[j][k] += pts[i][j] * pts[i][k];
	}

	/*for (int j = 0; j < 3; j++)
		for (int k = j; k < 3; k++)
			C[j][k] /= pts.size();*/

	C[1][0] = C[0][1];
	C[2][0] = C[0][2];
	C[2][1] = C[1][2];
}

void directions_curvature_covariance(TriMesh *mesh, float C[3][3])
{
	mesh->need_curvatures();
	mesh->need_normals();
	int nv = mesh->vertices.size();
	vector<vec> dir12(nv);
#pragma omp parallel for
	for(int i=0;i<nv;i++){
		//dir12[i] = ((mesh->pdir1[i]+mesh->pdir2[i])/2.0f) CROSS mesh->normals[i];
		dir12[i] = ((mesh->pdir1[i]+mesh->pdir2[i])/2.0f);
		//dir12[i] = mesh->pdir2[i] CROSS mesh->normals[i];
		//dir12[i] = mesh->normals[i];
		//normalize(dir12[i]);
	}
	point_covariance(dir12,C);
	dir12.clear();
	/*for (int j = 0; j < 3; j++)
		for (int k = 0; k < 3; k++)
			C[j][k] = 0.0f;

	int n = dir12.size();
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < 3; j++)
			for (int k = j; k < 3; k++)
				C[j][k] += dir12[i][j] * dir12[i][k];
	}

	for (int j = 0; j < 3; j++)
		for (int k = j; k < 3; k++)
			C[j][k] /= dir12.size();

	C[1][0] = C[0][1];
	C[2][0] = C[0][2];
	C[2][1] = C[1][2];*/
}


// Compute covariance of faces (area-weighted) in a mesh
void mesh_covariance(TriMesh *mesh, float C[3][3])
{
	if (mesh->faces.empty() && mesh->tstrips.empty()) {
		point_covariance(mesh->vertices, C);
		return;
	}

	mesh->need_faces();

	for (int j = 0; j < 3; j++)
		for (int k = 0; k < 3; k++)
			C[j][k] = 0.0f;

	float totarea = 0.0f;
	const vector<point> &p = mesh->vertices;
	int n = mesh->faces.size();
	for (int i = 0; i < n; i++) {
		const TriMesh::Face &f = mesh->faces[i];
		point c = (p[f[0]] + p[f[1]] + p[f[2]]) / 3.0f;
		float area = len(trinorm(p[f[0]], p[f[1]], p[f[2]]));
		totarea += area;

		// Covariance of triangle relative to centroid
		float vweight = area / 12.0f;
		//float vweight = area ;
		for (int v = 0; v < 3; v++) {
			point pc = p[f[v]] - c;
			for (int j = 0; j < 3; j++)
				for (int k = j; k < 3; k++)
					C[j][k] += vweight * pc[j] * pc[k];
		}

		// Covariance of centroid
		for (int j = 0; j < 3; j++)
			for (int k = j; k < 3; k++)
				C[j][k] += area * c[j] * c[k];
	}

	for (int j = 0; j < 3; j++)
		for (int k = j; k < 3; k++)
			C[j][k] /= totarea;

	C[1][0] = C[0][1];
	C[2][0] = C[0][2];
	C[2][1] = C[1][2];
}



// Scale the mesh so that mean squared distance from center of mass is 1
void normalize_variance(TriMesh *mesh)
{
	point com = mesh_center_of_mass(mesh);
	trans(mesh, -com);

	float C[3][3];
	mesh_covariance(mesh, C);

	float s = 1.0f / sqrt(C[0][0] + C[1][1] + C[2][2]);
	scale(mesh, s);

	trans(mesh, com);
}


// Rotate model so that first principal axis is along +X (using
// forward weighting), and the second is along +Y
void pca_rotate(TriMesh *mesh)
{
	point com = mesh_center_of_mass(mesh);
	trans(mesh, -com);

	float C[3][3];
	mesh_covariance(mesh, C);
	float e[3];
	eigdc<float,3>(C, e);

	// Sorted in order from smallest to largest, so grab third column
	vec first(C[0][2], C[1][2], C[2][2]);
	int npos = 0;
	int nv = mesh->vertices.size();
	for (int i = 0; i < nv; i++)
		if ((mesh->vertices[i] DOT first) > 0.0f)
			npos++;
	if (npos < nv/2)
		first = -first;

	vec second(C[0][1], C[1][1], C[2][1]);
	npos = 0;
	for (int i = 0; i < nv; i++)
		if ((mesh->vertices[i] DOT second) > 0.0f)
			npos++;
	if (npos < nv/2)
		second = -second;

	vec third = first CROSS second;

	xform xf;
	xf[0] = first[0];  xf[1] = first[1];  xf[2] = first[2];
	xf[4] = second[0]; xf[5] = second[1]; xf[6] = second[2];
	xf[8] = third[0];  xf[9] = third[1];  xf[10] = third[2];

	invert(xf);
	apply_xform(mesh, xf);

	trans(mesh, com);
}

void pca_Normalisation(TriMesh *mesh){
	mesh->need_bbox();
	point com = mesh_center_of_mass(mesh);
	//point com =(mesh->bbox.max+mesh->bbox.min)*.5f;
	trans(mesh, -com);
	point center(0.0f,0.0f,0.0f);
	float s = 1.0f/(getMaxDistance(center,mesh));
	scale(mesh,s);
	//trans(mesh, -com);
	float C[3][3];
	mesh_covariance(mesh, C);
	float e[3];
	eigdc<float,3>(C, e);

	// Sorted in order from smallest to largest, so grab third column
	vec first(C[0][2], C[1][2], C[2][2]);
	int npos = 0;
	int nv = mesh->vertices.size();
	for (int i = 0; i < nv; i++)
		if ((mesh->vertices[i] DOT first) > 0.0f)
			npos++;
	if (npos < nv/2)
		first = -first;

	vec second(C[0][1], C[1][1], C[2][1]);
	npos = 0;
	for (int i = 0; i < nv; i++)
		if ((mesh->vertices[i] DOT second) > 0.0f)
			npos++;
	if (npos < nv/2)
		second = -second;

	vec third = first CROSS second;

	xform xf;
	xf[0] = first[0];  xf[1] = first[1];  xf[2] = first[2];
	xf[4] = second[0]; xf[5] = second[1]; xf[6] = second[2];
	xf[8] = third[0];  xf[9] = third[1];  xf[10] = third[2];

	invert(xf);
	apply_xform(mesh, xf);
	//trans(mesh, com);
}
void pca_Normalisation_vertices(TriMesh *mesh){
	point com = point_center_of_mass(mesh->vertices);
	trans(mesh, -com);
	point center(0.0f,0.0f,0.0f);
	float s = 1.0f/(getMaxDistance(center,mesh));
	scale(mesh,s);
	//trans(mesh, -com);
	float C[3][3];
	point_covariance(mesh->vertices, C);
	float e[3];
	eigdc<float,3>(C, e);

	// Sorted in order from smallest to largest, so grab third column
	vec first(C[0][2], C[1][2], C[2][2]);
	int npos = 0;
	int nv = mesh->vertices.size();
	for (int i = 0; i < nv; i++)
		if ((mesh->vertices[i] DOT first) > 0.0f)
			npos++;
	if (npos < nv/2)
		first = -first;

	vec second(C[0][1], C[1][1], C[2][1]);
	npos = 0;
	for (int i = 0; i < nv; i++)
		if ((mesh->vertices[i] DOT second) > 0.0f)
			npos++;
	if (npos < nv/2)
		second = -second;

	vec third = first CROSS second;

	xform xf;
	xf[0] = first[0];  xf[1] = first[1];  xf[2] = first[2];
	xf[4] = second[0]; xf[5] = second[1]; xf[6] = second[2];
	xf[8] = third[0];  xf[9] = third[1];  xf[10] = third[2];

	invert(xf);
	apply_xform(mesh, xf);
	//trans(mesh, com);
}
void center_and_scale_Unit_Sphere(TriMesh *mesh){
	if(!mesh->bbox.valid) mesh->need_bbox();
	point p = mesh->bbox.center();
	float r = getMaxDistance(p,mesh);
	trans(mesh,-p);
	scale(mesh,1.0f/r);
	//apply_xform(mesh,xform::rot(-0.6f,0.0f,0.0f,1.0f));

	//trans(mesh,p);

}
void pca_curvature_direction(TriMesh *mesh){
	point com = mesh_center_of_mass(mesh);
	trans(mesh, -com);
	point center(0.0f,0.0f,0.0f);
	float s = 1.0f/(getMaxDistance(center,mesh));
	scale(mesh,s);
	//trans(mesh, -com);
	float C[3][3];
	directions_curvature_covariance(mesh, C);
	float e[3];
	eigdc<float,3>(C, e);

	// Sorted in order from smallest to largest, so grab third column
	vec first(C[0][2], C[1][2], C[2][2]);
	
	int npos = 0;
	int nv = mesh->vertices.size();
	for (int i = 0; i < nv; i++)
		if ((mesh->vertices[i] DOT first) > 0.0f)
			npos++;
	if (npos < nv/2)
		first = -first;

	vec second(C[0][1], C[1][1], C[2][1]);
	npos = 0;
	for (int i = 0; i < nv; i++)
		if ((mesh->vertices[i] DOT second) > 0.0f)
			npos++;
	if (npos < nv/2)
		second = -second;

	vec third = first CROSS second;

	//vec third(C[0][0],C[1][0],C[2][0]);

	xform xf;
	xf[0] = first[0];  xf[1] = first[1];  xf[2] = first[2];
	xf[4] = second[0]; xf[5] = second[1]; xf[6] = second[2];
	xf[8] = third[0];  xf[9] = third[1];  xf[10] = third[2];

	invert(xf);
	apply_xform(mesh, xf);
	//trans(mesh, com);
}
vec *getPCAAxis(TriMesh *mesh){
	TriMesh *tmp = mesh;
	point com = mesh_center_of_mass(tmp);
	trans(tmp, -com);
	point center(0.0f,0.0f,0.0f);
	float s = 1.0f/(getMaxDistance(center,tmp));
	scale(tmp,s);
	//trans(mesh, -com);
	float C[3][3];
	mesh_covariance(tmp, C);
	float e[3];
	eigdc<float,3>(C, e);

	vec first(C[0][2], C[1][2], C[2][2]);
	vec second(C[0][1], C[1][1], C[2][1]);
	vec third = first CROSS second;

	xform xf;
	xf[0] = first[0];  xf[1] = first[1];  xf[2] = first[2];
	xf[4] = second[0]; xf[5] = second[1]; xf[6] = second[2];
	xf[8] = third[0];  xf[9] = third[1];  xf[10] = third[2];

	//invert(xf);
	vec v1(xf[0],xf[1],xf[2]);
	vec v2(xf[4],xf[5],xf[6]);
	vec v3(xf[8],xf[9],xf[10]);
	vec *V = new vec[3];
	V[0] = v1;
	V[1] = v2;
	V[2] = v3;
	//scale(mesh,((float)1.0f/s));
	//trans(tmp, com);
	return V;
	//apply_xform(tmp, xf);
}
vec *getPCAAxisVertices(TriMesh *mesh){
	TriMesh *tmp = mesh;
	tmp->need_bbox();
	//point com = mesh_center_of_mass(tmp);
	point com = tmp->bbox.center();
	trans(tmp, -com);
	point center(0.0f,0.0f,0.0f);
	float s = 1.0f/(getMaxDistance(center,tmp));
	scale(tmp,s);
	//trans(tmp, com);
	float C[3][3];
	point_covariance(tmp->vertices, C);
	float e[3];
	eigdc<float,3>(C, e);

	vec first(C[0][2], C[1][2], C[2][2]);
	vec second(C[0][1], C[1][1], C[2][1]);
	vec third = first CROSS second;

	xform xf;
	xf[0] = first[0];  xf[1] = first[1];  xf[2] = first[2];
	xf[4] = second[0]; xf[5] = second[1]; xf[6] = second[2];
	xf[8] = third[0];  xf[9] = third[1];  xf[10] = third[2];

	//invert(xf);
	vec v1(xf[0],xf[1],xf[2]);
	vec v2(xf[4],xf[5],xf[6]);
	vec v3(xf[8],xf[9],xf[10]);
	vec *V = new vec[3];
	V[0] = v1;
	V[1] = v2;
	V[2] = v3;
	//scale(mesh,((float)1.0f/s));
	//trans(tmp, com);
	return V;
	//apply_xform(tmp, xf);
}
vecD2 *segmentsIntersect(vecD2 A,vecD2 B, vecD2 C, vecD2 D){
	double x1 = A[0];
	double y1 = A[1];
	double x2 = B[0];
	double y2 = B[1];
	double x3 = C[0];
	double y3 = C[1];
	double x4 = D[0];
	double y4 = D[1];

	double d = (x1-x2)*(y3-y4) - (y1-y2)*(x3-x4);
	if(d==0.0) return NULL;
	double xi = ((x3-x4)*(x1*y2-y1*x2)-(x1-x2)*(x3*y4-y3*x4))/d;
	double yi = ((y3-y4)*(x1*y2-y1*x2)-(y1-y2)*(x3*y4-y3*x4))/d;
	
	vecD2 *P = new vecD2(xi,yi);
	if (xi < min(x1,x2) || xi > max(x1,x2)) return NULL;
	if (xi < min(x3,x4) || xi > max(x3,x4)) return NULL;

	return P;
}
bool isSegmentIntersected(vecD2 A,vecD2 B, vecD2 C, vecD2 D){
	double x1 = A[0];
	double y1 = A[1];
	double x2 = B[0];
	double y2 = B[1];
	double x3 = C[0];
	double y3 = C[1];
	double x4 = D[0];
	double y4 = D[1];

	double d = (x1-x2)*(y3-y4) - (y1-y2)*(x3-x4);
	if(d==0.0) return false;
	double xi = ((x3-x4)*(x1*y2-y1*x2)-(x1-x2)*(x3*y4-y3*x4))/d;
	double yi = ((y3-y4)*(x1*y2-y1*x2)-(y1-y2)*(x3*y4-y3*x4))/d;
	if (xi < min(x1,x2) || xi > max(x1,x2)) return false;
	if (xi < min(x3,x4) || xi > max(x3,x4)) return false;

	return true;
}

vec ClosestPtPointTriangle(vec p, vec a, vec b, vec c){
	// Check if P in vertex region outside A
	vec ab = b - a;
	vec ac = c - a;
	vec ap = p - a;
	float d1 = ab DOT ap;
	float d2 = ac DOT ap;
	if (d1 <= 0.0f && d2 <= 0.0f) return a; // barycentric coordinates (1,0,0)
	// Check if P in vertex region outside B
	vec bp = p - b;
	float d3 = ab DOT bp;
	float d4 = ac DOT bp;
	if (d3 >= 0.0f && d4 <= d3) return b; // barycentric coordinates (0,1,0)
	// Check if P in edge region of AB, if so return projection of P onto AB
	float vc = d1*d4 - d3*d2;
	if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
	float v = d1 / (d1 - d3);
	return a + v * ab; // barycentric coordinates (1-v,v,0)
	}
	// Check if P in vertex region outside C
	vec cp = p - c;
	float d5 = ab DOT cp;
	float d6 = ac DOT cp;
	if (d6 >= 0.0f && d5 <= d6) return c; // barycentric coordinates (0,0,1)

	// Check if P in edge region of AC, if so return projection of P onto AC
	float vb = d5*d2 - d1*d6;
	if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
	float w = d2 / (d2 - d6);
	return a + w * ac; // barycentric coordinates (1-w,0,w)
	}
	// Check if P in edge region of BC, if so return projection of P onto BC
	float va = d3*d6 - d5*d4;
	if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
	float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
	return b + w * (c - b); // barycentric coordinates (0,1-w,w)
	}
	// P inside face region. Compute Q through its barycentric coordinates (u,v,w)
	float denom = 1.0f / (va + vb + vc);
	float v = vb * denom;
	float w = vc * denom;
	return a + ab * v + ac * w; // = u*a + v*b + w*c, u = va * denom = 1.0f - v - w
}
bool triangleSphereOverlap(vec p1,vec p2,vec p3, vec C,float r){
	//vec n = (p2-p1) CROSS (p3-p1);
	//float d = - n DOT p1;
	//float l = (n DOT C) + d;
	vec p = ClosestPtPointTriangle(C, p1, p2, p3);
	//printf("\n(%f,%f,%f)",p[0],p[1],p[2]);
	// Sphere and triangle intersect if the (squared) distance from sphere
// center to point p is less than the (squared) sphere radius
	vec v = p - C;
	/*if((v DOT v) <= r * r){
		printf("\n d= %f",sqrtf(v DOT v));
		printf("\n r= %f",r);
	}*/
	return (v DOT v) < r * r;
	/*printf("\n l= %f",l);
	printf("\n d= %f",d);
	printf("\n r= %f",r);
	if(fabsf(l)<r) return true;
	return false;*/
}
void intersection_Segment_set_of_Segments(vecD2 A,vecD2 B, vector<double> setOfSegments,vector<vecD2> &results){
	results.clear();
	int n = setOfSegments.size();
	for(int i=0;i<n-1;i++){
		vecD2 *P = segmentsIntersect(A,B,vecD2((double)i,setOfSegments[i]),vecD2((double)(i+1),setOfSegments[i+1]));
		if(P!=NULL) {
			results.push_back(*P);
		}
	}
}
bool isInsideRectangel(vecD2 Pi, vecD2 A, vecD2 B){
	return ( (Pi[0]>=A[0] && B[0]>Pi[0]) && (Pi[1]>=A[1] && B[1]>Pi[1]) ) ;
}
bool belongTo(int id,vector<int> ls){
	int n = ls.size();
	for(int i=0;i<n;i++)
		if(ls[i]==id) return true;
	return false;
}
int getNext(int indice, vector<int> domain,vector<bool> &marked){
	int i = indice;
	while(belongTo(i,domain)){
		//printf("\nOui !!!!!!!!!");
		marked[i] = true;
		i++;
	}
	return i;
}
int regionsCount(vector<vecD2> setOfSegments, vector<int> intersectionPointsMin,vector<int> intersectionPoints,double ymin,double ymax, 
				 vector<bool> &marked,vector<bool>isIntersectionPoint){
	int count = 0;
	int n = setOfSegments.size();
	vecD2 Pi(setOfSegments[0][1],setOfSegments[0][1]);
	int id=0;
	int idd;
	if(!marked[0] && !isIntersectionPoint[0] && Pi[1]>=ymin && Pi[1]<=ymax+0.0009){
		int idMin = intersectionPoints.size();
		//printf("\nidMin = %d",idMin);

		int idMax = intersectionPoints[0];
		id = min(idMin,idMax);
		//printf("\nidMax = %d",idMax);

		if( isInsideRectangel(Pi,vecD2(0.0,ymin),vecD2((double)id, ymax+0.0009)) ){
			count++;
			for(int i=0;i<id;i++){
				Pi[0] = setOfSegments[i][0];
				Pi[1] = setOfSegments[i][1];
				if( isInsideRectangel(Pi,vecD2(0.0,ymin),setOfSegments[id]) ) marked[i] = true;
			}
			marked[id] = true;
		}
	}
	//printf("\n%d",count);

	int indice;
	for(int i=0;i<n;i++){
		if(isIntersectionPoint[i]){
			indice = i;
			//break;
		}
	}
	
	//printf("\nindice = %d ",indice);
	
	vecD2 Pif(setOfSegments[indice][0],setOfSegments[indice][1]);
	//printf("\nPif = %lf",Pif[1]);
	//if(isIntersectionPoint[indice]) 				printf("\nYeah it is !!!!!!!!!!!!!");

	if(!marked[indice] &&Pif[1]>=ymin && Pif[1]<=ymax+0.0009){
			if(indice<n-1){
				printf("\nOui");
				Pif[0] = setOfSegments[indice+1][0];
				Pif[1] = setOfSegments[indice+1][1];
				//printf("\nS = %lf",setOfSegments[indice+1][1]);
				//printf("\nymax = %lf",ymax);
				//if( isInsideRectangel(Pif,vecD2(setOfSegments[indice][0],ymin),vecD2((double)(n-1), ymax)) ){
				if( Pif[1]>=ymin && Pif[1]<=ymax+0.0009 ){
					count++;
					for(int i=indice;i<n;i++){
						marked[i] = true;
					}
				}
			}
			else marked[indice] = true;
	}
	int i = id;
	//printf("\nid = %d",id);
	//printf("\nindice = %d",indice);
	while(i<indice){
		if(marked[i] && !isIntersectionPoint[i]){ 
			i++;
			//continue;
		}
		else if(isIntersectionPoint[i]){
			int j= i+1;
			if(belongTo(i,intersectionPointsMin)){
				//printf("\nj = %d",j); 
				int k = getNext(j,intersectionPointsMin,marked);
				j = k;
				//printf("\nk = %d",k); 
				//vecD2 P(setOfSegments[j][0],setOfSegments[j][1]);
				if( !marked[j] && setOfSegments[j][1]>=ymin && setOfSegments[j][1]<= ymax+0.0009 ){ 
					count++;
				//printf("\nOui");
					printf("\nx = %lf, y = %lf",setOfSegments[j][0],setOfSegments[j][1]);
					while(j<indice && !belongTo(j,intersectionPointsMin)){
				//printf("\nOui");
						if( setOfSegments[j][1]>=ymin && setOfSegments[j][1]<= ymax+0.0009 ) marked[j] = true;
						j++;
					}
				}
			}
			else if(belongTo(i,intersectionPoints)){
				int k = getNext(j,intersectionPoints,marked);
				j = k;
				vecD2 P(setOfSegments[j][0],setOfSegments[j][1]);
				//printf("\nOui");
				if(!marked[j]&& setOfSegments[j][1]>=ymin && setOfSegments[j][1]<= ymax+0.0009 ){ 
					count++;
					while(j<indice && !belongTo(j,intersectionPoints)){
						//P[0] = setOfSegments[j][0];
						//P[1] = setOfSegments[j][1];
						if( setOfSegments[j][1]>=ymin && setOfSegments[j][1]<= ymax+0.0009 ) marked[j] = true;
						j++;
					}
				}
			}
			i = j;
		}
		else i++;
	}
	//printf("\nCount = %d",count);*/

	return count;
}
void size_Function_extraction(vector<double> valuesList,vector<SizeElement> &SizeElementsList,int order){
	vector<vecD2> setOfSegments;
	setOfSegments.clear();
	SizeElementsList.clear();
	vector<vector<int>> domainsList(order+1);
	for(int i=0;i<=order;i++){
		domainsList[i].clear();
	}
	int n = valuesList.size();
	int k=0;
	//vector<vecD2> vectList;
	//vectList.clear();
	vector<bool> added(n,false);
	for(int i=0;i<n-1;i++){
		//printf("\nVal = %lf",valuesList[i]);

		setOfSegments.push_back(vecD2(((double)i)/((double)n-1),valuesList[i]));
		//isIntersectionPoint.push_back(false);

		for(int j=0;j<=order;j++){
			double u = 1.0*(((double)j)/((double)order));
					//printf("\nU = %lf, j= %d",u,j);
			vecD2 A((double)i/((double)n-1),valuesList[i]);
			vecD2 B((double)(i+1)/((double)n-1),valuesList[i+1]);
			vecD2 C((double)0,u);
			vecD2 D((double)1,u);
			if(isSegmentIntersected(A,B,C,D)){
				vecD2 *P = segmentsIntersect(A,B,C,D);
				if(A== *P){
					//isIntersectionPoint[isIntersectionPoint.size()-1] = true;
										//printf("\nU = %lf, j= %d",u);
					//domainsList[j].push_back(k);
					//k++;
					delete P;
					//break;
				}
				else if(B== *P){
					if(i+1==n-1){
						setOfSegments.push_back(vecD2(B[0],B[1]));
						//isIntersectionPoint.push_back(true);
						//domainsList[j].push_back(isIntersectionPoint.size()-1);
										//	printf("\nU = %lf, j= %d",u);
					}
					//k++;
					delete P;
					//break;
				}
				else{
									//printf("\nU = %lf, j= %d",u,j);
					setOfSegments.push_back(vecD2((*P)[0],(*P)[1]));
					//printf("\n%lf",setOfSegments[setOfSegments.size()-1][1]);
					//printf("\nsssssssssssssssssssssssssssssssssssssssssss");
					delete P;
					//isIntersectionPoint[isIntersectionPoint.size()-1] = false;
					//isIntersectionPoint.push_back(true);
					//domainsList[j].push_back(isIntersectionPoint.size()-1);
					//k++;
					//k++;
					//break;
				}
			}
			else{
				//k = isIntersectionPoint.size();
				continue;			
			}
		}
	}
	setOfSegments.push_back(vecD2(1.0,valuesList[n-1]));
	//printf("\nk = %d",k);
	//printf("\nNb = %d",isIntersectionPoint.size());
	//printf("\nNbs = %d",setOfSegments.size());

	int l =setOfSegments.size();
	double min = setOfSegments[0][1];
	double max = setOfSegments[0][1];
	for(int i=1;i<l;i++){
		if(setOfSegments[i][1]<min) min = setOfSegments[i][1];
		if(setOfSegments[i][1]>max) max = setOfSegments[i][1];
	}
	for(int i=0;i<l;i++){
		setOfSegments[i][1] = (setOfSegments[i][1] - min)/(max - min);
	}

	/*FILE *f = fopen("D:\\size.txt","w");
	if(f==NULL){
		std::cout<<"Erreur d'écriture du fichier " << endl;
		exit(-1);
	}
	//fprintf(f,"#\t%d\n",k);
	for(int i=0;i<l;i++){
		fprintf(f,"%lf\t%lf\n",setOfSegments[i][0],setOfSegments[i][1]);
	}
	fclose(f);
	std::cout<<"Fin d'écriture du fichier " << endl;

	/*FILE *fr = fopen("D:\\size.txt","r");
	if(fr==NULL){
		std::cout<<"Erreur d'écriture du fichier " << endl;
		exit(-1);
	}
	for(int i=0;i<l;i++){
		fscanf(fr,"%lf\t%lf\n",&setOfSegments[i][0],&setOfSegments[i][1]);
		//printf("%lf\t%lf\n",setOfSegments[i][0],setOfSegments[i][1]);
	}
	fclose(fr);
	//std::cout<<"Fin d'écriture du fichier " << endl;
	//fprintf(f,"#\t%d\n",k);*/
	vector<bool> isIntersectionPoint(l,false);
	for(int i=0;i<=order;i++){
		double u = (double)(((double)i)/((double)order));
		for(int j=0;j<setOfSegments.size();j++){
			//printf("%d\n",domainsList[i].size());
			if(setOfSegments[j][1]>=u && setOfSegments[j][1]<=u+0.00009){
				domainsList[i].push_back(j);
				isIntersectionPoint[j] = true;
			}
		}
		//printf("\n%lf",u);
	}
	vector<bool> marked(isIntersectionPoint.size(),false);
/*	for(int i=0;i<=order;i++){
		printf("\nNiveau %d\n",i);
		for(int j=0;j<domainsList[i].size();j++){
			printf("%d\t",domainsList[i][j]);
		}
	}*/
	for(int i=0;i<order;i++){
		double ymin = (double)(((double)i)/((double)order));
		double ymax = (double)(((double)(i+1))/((double)order));
		int count = regionsCount(setOfSegments,domainsList[i],domainsList[i+1],ymin,ymax,marked,isIntersectionPoint);
		printf("\nCount = %d",count);

		SizeElementsList.push_back(SizeElement(ymin,ymax,(double)count));
	}	
	
	isIntersectionPoint.clear();
	marked.clear();
	setOfSegments.clear();
	for(int i=0;i<=order;i++){
		domainsList[i].clear();
	}
	domainsList.clear();
}
void measureFunction_PCA_AXIS(TriMesh *mesh,vector<SizeElement> &SizeElementsListX,vector<SizeElement> &SizeElementsListY,vector<SizeElement> &SizeElementsListZ){
	vec *V = getPCAAxisVertices(mesh);
	vec P1 = fabs(V[0]) ;
	vec P2 = fabs(V[1]) ;
	vec P3 = fabs(V[2]) ;
	vec N1 = P1 CROSS P2;
	vec N2 = P1 CROSS P3;
	vec N3 = P2 CROSS P3;
	int n = mesh->vertices.size();
	vector<double> valuesListX(n);
	vector<double> valuesListY(n);
	vector<double> valuesListZ(n);

	for(int i=0;i<n;i++){
		const vec &tmp = mesh->vertices[i];
		double val1 =0.0;
		double val2 =0.0;
		double val3 =0.0;
		float landa1 = -(tmp DOT N1)/len2(N1);
		float landa2 = -(tmp DOT N2)/len2(N2);
		float landa3 = -(tmp DOT N3)/len2(N3);
		if(landa1>0.0f) val1 = len(tmp);
		if(landa2>0.0f) val2 = len(tmp);
		if(landa2>0.0f) val3 = len(tmp);
		valuesListX[i] = val1;
		valuesListY[i] = val2;
		valuesListZ[i] = val3;
	}
	//Measures functions' Normalization
	double min = valuesListX[0];
	double max = valuesListX[0];
	for(int i=1;i<n;i++){
		if(valuesListX[i]<min) min = valuesListX[i];
		if(valuesListX[i]>max) max = valuesListX[i];
	}
	for(int i=0;i<n;i++){
		valuesListX[i] = (valuesListX[i] - min)/(max - min);
	}
	min = valuesListY[0];
	max = valuesListY[0];
	for(int i=1;i<n;i++){
		if(valuesListY[i]<min) min = valuesListY[i];
		if(valuesListY[i]>max) max = valuesListY[i];
	}
	for(int i=0;i<n;i++){
		valuesListY[i] = (valuesListY[i] - min)/(max - min);
	}
	min = valuesListZ[0];
	max = valuesListZ[0];
	for(int i=1;i<n;i++){
		if(valuesListZ[i]<min) min = valuesListZ[i];
		if(valuesListZ[i]>max) max = valuesListZ[i];
	}
	for(int i=0;i<n;i++){
		valuesListZ[i] = (valuesListZ[i] - min)/(max - min);
	}
	//size_Function_extraction(valuesListX,SizeElementsListX,10);
	//size_Function_extraction(valuesListY,SizeElementsListY,10);
	//size_Function_extraction(valuesListZ,SizeElementsListZ,10);
	/*FILE *f = fopen("D:\\size.txt","w");
	if(f==NULL){
		std::cout<<"Erreur d'écriture du fichier " << endl;
		exit(-1);
	}
	int k =valuesListX.size();
	//fprintf(f,"#\t%d\n",k);
	for(int i=0;i<k;i++){
		fprintf(f,"%lf\t%lf\n",(double)(((double)(i))/((double)(k-1))),valuesListX[i]);
	}
	fclose(f);
	std::cout<<"Fin d'écriture du fichier " << endl;

	valuesListX.clear();
	valuesListY.clear();
	valuesListZ.clear();*/
}
void measureFunction_PCA_AXIS(TriMesh *mesh,char * fnameX,char * fnameY,char * fnameZ){
	//void saveMeasureFunctionGraph(vector<double> ,vector< vector<int> > ,char *);
	mesh->need_neighbors();
	vec V = fabs(getPCAAxisVertices(mesh)[0]);
	align(mesh,V);

	int n = mesh->vertices.size();
	vector<double> valuesListX(n);
	vector<double> valuesListY(n);
	vector<double> valuesListZ(n);
#pragma omp parallel for
	for(int i=0;i<n;i++){
		const vec &tmp = mesh->vertices[i];
		valuesListX[i] = tmp[0];
		valuesListY[i] = tmp[1];
		valuesListZ[i] = tmp[2];
	}
	//Measures functions' Normalization
	double min = valuesListX[0];
	double max = valuesListX[0];
	for(int i=1;i<n;i++){
		if(valuesListX[i]<min) min = valuesListX[i];
		if(valuesListX[i]>max) max = valuesListX[i];
	}
#pragma omp parallel for
	for(int i=0;i<n;i++){
		valuesListX[i] = (valuesListX[i] - min)/(max - min);
	}

	min = valuesListY[0];
	max = valuesListY[0];
	for(int i=1;i<n;i++){
		if(valuesListY[i]<min) min = valuesListY[i];
		if(valuesListY[i]>max) max = valuesListY[i];
	}
#pragma omp parallel for
	for(int i=0;i<n;i++){
		valuesListY[i] = (valuesListY[i] - min)/(max - min);
	}
	min = valuesListZ[0];
	max = valuesListZ[0];
	for(int i=1;i<n;i++){
		if(valuesListZ[i]<min) min = valuesListZ[i];
		if(valuesListZ[i]>max) max = valuesListZ[i];
	}
#pragma omp parallel for
	for(int i=0;i<n;i++){
		valuesListZ[i] = (valuesListZ[i] - min)/(max - min);
	}
	saveMeasureFunctionGraph(valuesListX,mesh->neighbors,fnameX);
	saveMeasureFunctionGraph(valuesListY,mesh->neighbors,fnameY);
	saveMeasureFunctionGraph(valuesListZ,mesh->neighbors,fnameZ);
	valuesListX.clear();
	valuesListY.clear();
	valuesListZ.clear();

}



void saveSizeElements(vector<SizeElement> SizeElementsList, const string &fname){
	FILE *f = fopen(fname.c_str(),"w");
	if(f==NULL){
		std::cout<<"Erreur d'écriture du fichier " << fname.c_str() << endl;
		exit(-1);
	}
	int k =SizeElementsList.size();
	fprintf(f,"#\t%d\n",k);
	for(int i=0;i<k;i++){
		fprintf(f,"%lf\t%lf\n",SizeElementsList[i].getMin(),SizeElementsList[i].getMax());
	}
	fclose(f);
	std::cout<<"Fin d'écriture du fichier " << fname.c_str() << endl;
}
bool triangleInsideSphere(vec v1,vec v2,vec v3, vec C, float r){
	if(triangleSphereOverlap(v1,v2,v3,C,r)) return false;
	float d1 = dist(C,v1);
	float d2 = dist(C,v2);
	float d3 = dist(C,v3);
	float mx = max(d1,max(d2,d3));
	if(mx<=r) return true;
	return false;
}
bool triangleOutsideSphere(vec v1,vec v2,vec v3, vec C, float r){
	if(triangleSphereOverlap(v1,v2,v3,C,r)) return false;
	if(triangleInsideSphere(v1,v2,v3,C,r)) return false;
	return true;
}
int zoneCount(TriMesh * mesh,vector<int> interMin,vector<int> interMax, vector<bool> isOverlap,vector<bool> &marked, float rmin, float rmax
			  ){
	int count = 0;
	int n = mesh->faces.size();
	//printf("\nNeed aD = %d",mesh->across_edge.size());
	vector<bool> isCrossedMin(n,false);
	vector<bool> isCrossedMax(n,false);
		for(int i=0;i<n;i++){
			if(marked[i] && !isOverlap[i]) continue;
			else if(isOverlap[i] && ((belongTo(i,interMax) && !isCrossedMax[i]) || (belongTo(i,interMin) && !isCrossedMin[i]))){
					count++;
					stack<int> s;
					s.push(i);
					while(!s.empty()){
						int id = s.top(); 
						s.pop();
						if((!marked[id] && ( (triangleInsideSphere(mesh->vertices[mesh->faces[id][0]],mesh->vertices[mesh->faces[id][1]],
							mesh->vertices[mesh->faces[id][2]],vec(0.0f,0.0f,0.0f),rmax) &&
							(triangleOutsideSphere(mesh->vertices[mesh->faces[id][0]],mesh->vertices[mesh->faces[id][1]],
							mesh->vertices[mesh->faces[id][2]],vec(0.0f,0.0f,0.0f),rmin)
							) )
					))){
							int iF1 = mesh->across_edge[id][0];
							int iF2 = mesh->across_edge[id][1];
							int iF3 = mesh->across_edge[id][2];
							marked[id] = true;
							if(iF1 != -1) s.push(iF1);
							if(iF2 != -1) s.push(iF2);
							if(iF3 != -1) s.push(iF3);
						}
						else if( isOverlap[id] && belongTo(id,interMax) && !isCrossedMax[id])
						{
							int iF1 = mesh->across_edge[id][0];
							int iF2 = mesh->across_edge[id][1];
							int iF3 = mesh->across_edge[id][2];
							marked[id] = true;
							isCrossedMax[id] = true;
							if(iF1 != -1) s.push(iF1);
							if(iF2 != -1) s.push(iF2);
							if(iF3 != -1) s.push(iF3);
						}
						else if( isOverlap[id] && belongTo(id,interMin) && !isCrossedMin[id])
						{
							int iF1 = mesh->across_edge[id][0];
							int iF2 = mesh->across_edge[id][1];
							int iF3 = mesh->across_edge[id][2];
							marked[id] = true;
							isCrossedMin[id] = true;
							if(iF1 != -1) s.push(iF1);
							if(iF2 != -1) s.push(iF2);
							if(iF3 != -1) s.push(iF3);
						}
					}
				
			}
			else if(triangleInsideSphere(mesh->vertices[mesh->faces[i][0]],mesh->vertices[mesh->faces[i][1]],
					mesh->vertices[mesh->faces[i][2]],vec(0.0f,0.0f,0.0f),rmax) && triangleOutsideSphere(mesh->vertices[mesh->faces[i][0]],mesh->vertices[mesh->faces[i][1]],
					mesh->vertices[mesh->faces[i][2]],vec(0.0f,0.0f,0.0f),rmin) && !marked[i]){
				count++;
				stack<int> s;
				s.push(i);
					while(!s.empty()){
						int id = s.top(); 
						s.pop();
						if((!marked[id] && ( (triangleInsideSphere(mesh->vertices[mesh->faces[id][0]],mesh->vertices[mesh->faces[id][1]],
							mesh->vertices[mesh->faces[id][2]],vec(0.0f,0.0f,0.0f),rmax) &&
							(triangleOutsideSphere(mesh->vertices[mesh->faces[id][0]],mesh->vertices[mesh->faces[id][1]],
							mesh->vertices[mesh->faces[id][2]],vec(0.0f,0.0f,0.0f),rmin)
							) )
					))){
							int iF1 = mesh->across_edge[id][0];
							int iF2 = mesh->across_edge[id][1];
							int iF3 = mesh->across_edge[id][2];
							marked[id] = true;
							isCrossedMax[id] = true;
							isCrossedMin[id] = true;
							if(iF1 != -1) s.push(iF1);
							if(iF2 != -1) s.push(iF2);
							if(iF3 != -1) s.push(iF3);
						}
						else if( isOverlap[id] && belongTo(id,interMax) && !isCrossedMax[id])
						{
							int iF1 = mesh->across_edge[id][0];
							int iF2 = mesh->across_edge[id][1];
							int iF3 = mesh->across_edge[id][2];
							marked[id] = true;
							isCrossedMax[id] = true;
							if(iF1 != -1) s.push(iF1);
							if(iF2 != -1) s.push(iF2);
							if(iF3 != -1) s.push(iF3);
						}
						else if( isOverlap[id] && belongTo(id,interMin) && !isCrossedMin[id])
						{
							int iF1 = mesh->across_edge[id][0];
							int iF2 = mesh->across_edge[id][1];
							int iF3 = mesh->across_edge[id][2];
							marked[id] = true;
							isCrossedMin[id] = true;
							if(iF1 != -1) s.push(iF1);
							if(iF2 != -1) s.push(iF2);
							if(iF3 != -1) s.push(iF3);
						}
					}
			}
		}
	//}
	isCrossedMin.clear();
	isCrossedMax.clear();
	return count;
}
int zoneCount(TriMesh * mesh, float rmin, float rmax  ){
	int count = 0;
	int n = mesh->faces.size();
	//printf("\nNeed aD = %d",mesh->across_edge.size());
	vector<bool> marked(n,false);
		for(int i=0;i<n;i++){
			if(marked[i] && !triangleSphereOverlap(mesh->vertices[mesh->faces[i][0]],mesh->vertices[mesh->faces[i][1]],
							mesh->vertices[mesh->faces[i][2]],vec(0.0f,0.0f,0.0f),rmin)
							&& !triangleSphereOverlap(mesh->vertices[mesh->faces[i][0]],mesh->vertices[mesh->faces[i][1]],
							mesh->vertices[mesh->faces[i][2]],vec(0.0f,0.0f,0.0f),rmax)) continue;
			else if(triangleSphereOverlap(mesh->vertices[mesh->faces[i][0]],mesh->vertices[mesh->faces[i][1]],
							mesh->vertices[mesh->faces[i][2]],vec(0.0f,0.0f,0.0f),rmin)
							|| triangleSphereOverlap(mesh->vertices[mesh->faces[i][0]],mesh->vertices[mesh->faces[i][1]],
							mesh->vertices[mesh->faces[i][2]],vec(0.0f,0.0f,0.0f),rmax)){
					count++;
					stack<int> s;
					s.push(i);
					while(!s.empty()){
						int id = s.top(); 
						s.pop();
						if(triangleSphereOverlap(mesh->vertices[mesh->faces[id][0]],mesh->vertices[mesh->faces[id][1]],
							mesh->vertices[mesh->faces[id][2]],vec(0.0f,0.0f,0.0f),rmin)
							|| triangleSphereOverlap(mesh->vertices[mesh->faces[id][0]],mesh->vertices[mesh->faces[id][1]],
							mesh->vertices[mesh->faces[id][2]],vec(0.0f,0.0f,0.0f),rmax) ||(!marked[id] && (  (triangleInsideSphere(mesh->vertices[mesh->faces[id][0]],mesh->vertices[mesh->faces[id][1]],
							mesh->vertices[mesh->faces[id][2]],vec(0.0f,0.0f,0.0f),rmax) &&
							(triangleOutsideSphere(mesh->vertices[mesh->faces[id][0]],mesh->vertices[mesh->faces[id][1]],
							mesh->vertices[mesh->faces[id][2]],vec(0.0f,0.0f,0.0f),rmin)
							) )
					))){
							int iF1 = mesh->across_edge[id][0];
							int iF2 = mesh->across_edge[id][1];
							int iF3 = mesh->across_edge[id][2];
							marked[id] = true;
							if(iF1 != -1) s.push(iF1);
							if(iF2 != -1) s.push(iF2);
							if(iF3 != -1) s.push(iF3);
						}
					}
				
			}
			else if(triangleInsideSphere(mesh->vertices[mesh->faces[i][0]],mesh->vertices[mesh->faces[i][1]],
					mesh->vertices[mesh->faces[i][2]],vec(0.0f,0.0f,0.0f),rmax) && triangleOutsideSphere(mesh->vertices[mesh->faces[i][0]],mesh->vertices[mesh->faces[i][1]],
					mesh->vertices[mesh->faces[i][2]],vec(0.0f,0.0f,0.0f),rmin) && !marked[i]){
				count++;
				stack<int> s;
				s.push(i);
					while(!s.empty()){
						int id = s.top(); 
						s.pop();
						if(triangleSphereOverlap(mesh->vertices[mesh->faces[id][0]],mesh->vertices[mesh->faces[id][1]],
							mesh->vertices[mesh->faces[id][2]],vec(0.0f,0.0f,0.0f),rmin)
							|| triangleSphereOverlap(mesh->vertices[mesh->faces[id][0]],mesh->vertices[mesh->faces[id][1]],
							mesh->vertices[mesh->faces[id][2]],vec(0.0f,0.0f,0.0f),rmax) ||(!marked[id] && (  (triangleInsideSphere(mesh->vertices[mesh->faces[id][0]],mesh->vertices[mesh->faces[id][1]],
							mesh->vertices[mesh->faces[id][2]],vec(0.0f,0.0f,0.0f),rmax) &&
							(triangleOutsideSphere(mesh->vertices[mesh->faces[id][0]],mesh->vertices[mesh->faces[id][1]],
							mesh->vertices[mesh->faces[id][2]],vec(0.0f,0.0f,0.0f),rmin)
							) )
					))){
							int iF1 = mesh->across_edge[id][0];
							int iF2 = mesh->across_edge[id][1];
							int iF3 = mesh->across_edge[id][2];
							marked[id] = true;
							if(iF1 != -1) s.push(iF1);
							if(iF2 != -1) s.push(iF2);
							if(iF3 != -1) s.push(iF3);
						}
					}
			}
		}
	//}
	marked.clear();
	return count;
}
bool isVertexbetween2Spheres(point v,  point C1, point C2, float rmin, float rmax){
	float d1 = dist(v,C1);
	float d2 = dist(v,C2);
	return d1>=rmin && d2<rmax;
}
void faces_between_spheres(TriMesh * mesh, vec C1, vec C2, float rmin, float rmax, vector<bool> &toremove){
	//printf("\nrmin = %f",rmin);
	if(rmin==0.0f){
		int nf = mesh->faces.size();
#pragma omp parallel for
		for(int i=0;i<nf;i++)
			if((triangleInsideSphere(mesh->vertices[mesh->faces[i][0]],mesh->vertices[mesh->faces[i][1]],
							mesh->vertices[mesh->faces[i][2]],C2,rmax)) || 
							triangleSphereOverlap(mesh->vertices[mesh->faces[i][0]],mesh->vertices[mesh->faces[i][1]],
				mesh->vertices[mesh->faces[i][2]],C2,rmax))
				//indices.push_back(i);
				toremove[i] = false;

	}
	else{
		int nf = mesh->faces.size();
#pragma omp parallel for
		for(int i=0;i<nf;i++)
			if((triangleInsideSphere(mesh->vertices[mesh->faces[i][0]],mesh->vertices[mesh->faces[i][1]],
							mesh->vertices[mesh->faces[i][2]],C2,rmax) && triangleOutsideSphere(mesh->vertices[mesh->faces[i][0]],
							mesh->vertices[mesh->faces[i][1]], mesh->vertices[mesh->faces[i][2]],C1,rmin)) || 
							triangleSphereOverlap(mesh->vertices[mesh->faces[i][0]],mesh->vertices[mesh->faces[i][1]],
							mesh->vertices[mesh->faces[i][2]],C2,rmax)
							||
							triangleSphereOverlap(mesh->vertices[mesh->faces[i][0]],mesh->vertices[mesh->faces[i][1]],
							mesh->vertices[mesh->faces[i][2]],C1,rmin)
							)
				toremove[i] = false;
	}
}

void vertices_betweeen2Spheres(TriMesh *mesh,vec C1, vec C2, float rmin, float rmax, vector<bool> &toremove){
	int n = mesh->vertices.size();
#pragma omp parallel for
	for(int i=0;i<n;i++){
		if(isVertexbetween2Spheres(mesh->vertices[i],vec(0.0f,0.0f,0.0f),vec(0.0f,0.0f,0.0f),rmin,rmax)){
			toremove[i] = false;
		}
	}
}
int zoneCount(const TriMesh * mesh,vector<bool> toremove){
	TriMesh * tmp = new TriMesh(*mesh);
	tmp->across_edge.clear();
	/*vector<bool> toremove(tmp->faces.size(),true);
	int indCount = indices.size();
	for(int i=0;i<indCount;i++)
		toremove[indices[i]] = false;
		*/
	remove_faces(tmp,toremove);
	if(tmp->faces.empty()) return 0;
	remove_unused_vertices(tmp);
	//tmp->need_across_edge();

	vector<int> comps;
	vector<int> compsizes;
	tmp->need_across_edge();
	find_comps(tmp, comps, compsizes);
	sort_comps(comps, compsizes);
	delete tmp;
	int count = compsizes.size();
	compsizes.clear();
	comps.clear();
	return count;
}
int zoneCount__(const TriMesh * mesh,vector<bool> toremove){
	TriMesh * tmp = new TriMesh(*mesh);
	tmp->across_edge.clear();
	/*vector<bool> toremove(tmp->faces.size(),true);
	int indCount = indices.size();
	for(int i=0;i<indCount;i++)
		toremove[indices[i]] = false;
		*/
	remove_vertices(tmp,toremove);
	if(tmp->faces.empty()) return 0;
	remove_unused_vertices(tmp);
	//tmp->need_across_edge();

	vector<int> comps;
	vector<int> compsizes;
	tmp->need_across_edge();
	find_comps(tmp, comps, compsizes);
	sort_comps(comps, compsizes);
	delete tmp;
	int count = compsizes.size();
	compsizes.clear();
	comps.clear();
	return count;
}

int fct(TriMesh *mesh, float l,float m){
	vector<bool> toremove(mesh->faces.size(),true);
	faces_between_spheres(mesh,vec(0.0f,0.0f,0.0f),vec(0.0f,0.0f,0.0f),l,m,toremove);
	int k = zoneCount(mesh,toremove);
	toremove.clear();
	return k;
}
int fct__(TriMesh *mesh, float l,float m){
	vector<bool> toremove(mesh->vertices.size(),true);
	vertices_betweeen2Spheres(mesh,vec(0.0f,0.0f,0.0f),vec(0.0f,0.0f,0.0f),l,m,toremove);
	int k = zoneCount__(mesh,toremove);
	toremove.clear();
	return k;
}
void size_Function_Sphere(TriMesh * mesh, int order, char *fname){
	mesh->need_across_edge();
	int nf = mesh->faces.size();
	vector<bool> isOverlap(nf,false);
	vector<bool> marked(nf,false);
	vector<vector<int>> domainList(order+1);
	for(int i=0;i<order;i++){
		domainList[i].clear();
	}
	int cpt =0;
	//printf("\n Nf = %d",nf);
	/*for(int i=0;i<nf;i++){
		for(int j=0;j<=order;j++){
			float u = 1.0f*(((float)j)/((float)order));
			if(triangleSphereOverlap(mesh->vertices[mesh->faces[i][0]],mesh->vertices[mesh->faces[i][1]],
				mesh->vertices[mesh->faces[i][2]],vec(0.0f,0.0f,0.0f),u)){ 
				isOverlap[i] = true; 
				domainList[j].push_back(i);
				//cpt++;
				//printf("\n i = %d",i);
			}
		}
	}*/

	//printf("\nGood !!!!!!!!!!!");
	vector<SizeElement> SizeElementsList;
	SizeElementsList.clear();
	vector<float> values(order);
//#pragma omp parallel for
	for(int i=0;i<order;i++){
		values[i] = (float)(((float)(i+1))/((float)order));
		//printf("\nklp= %f",values[i]);
	}
	/*for(int i=0;i<order;i++){
		float rmin = (float)(((float)(i))/((float)order));
		float rmax = (float)(((float)(i+1))/((float)order));
		//int count = zoneCount(mesh,domainList[i],domainList[i+1],isOverlap,marked,rmin,rmax);
		vector<int> indices;
		indices.clear();
		faces_between_spheres(mesh,vec(0.0f,0.0f,0.0f),vec(0.0f,0.0f,0.0f),rmin,rmax,indices);
		int count = zoneCount(mesh,indices);
		printf("\nCount = %d",count);
		indices.clear();
		SizeElementsList.push_back(SizeElement((double)rmin,(double)rmax,(double)count));
	}*/
	vector<bool> crossed(order,false);
	for(int i=0;i<order;i++){
		for(int j=i+1;j<order;j++){
			printf("\n(%d,%d)\t",i,j);
			vec2 v(values[i],values[j]);
			float eps1 = (values[j]-values[i])/2.5f;
			//printf("v=(%f,%f)\t",v[0],v[1]);
			printf("\neps1= %f",eps1);
			float alpha = eps1;
			float l=v[0]+alpha, m = v[1]-alpha;
			/*printf("\nl= %f, m=%f",l,m);
			 l=v[0]+alpha; m = v[1]+alpha;
			printf("\nl= %f, m=%f",l,m);
			 l=v[0]-alpha; m = v[1]-alpha;
			printf("\nl= %f, m=%f",l,m);
			 l=v[0]-alpha; m = v[1]+alpha;
			printf("\nl= %f, m=%f",l,m);*/
			float k = v[0];
			float eps2 = (-k+sqrtf(k*k+4))/4.5f;
			//printf("\neps2= %f",eps2);
			
			//Test if is a cornerline
			//float l,m;
			if(!crossed[i]){
				l=v[0]+eps2,m=1.0f/eps2;
				//printf("\n(%d,%d)\t",i,j);
				/*vector<bool> toremove1(mesh->faces.size(),true);
				faces_between_spheres(mesh,vec(0.0f,0.0f,0.0f),vec(0.0f,0.0f,0.0f),l,m,toremove1);
				int klp = zoneCount(mesh,toremove1);
				//int klp = zoneCount(mesh,l,m);
				toremove1.clear();*/
				vector<int> K(2);
				vector<vec2> vals(2);
				vals[0] = vec2(v[0]+eps2,1.0f/eps2);
				vals[1] = vec2(v[0]-eps2,1.0f/eps2);
				/*int klp = fct__(mesh,l,m);
				printf("\nklp= %d",klp);
				//printf("\nl= %f, m=%f",l,m);
				vector<bool> toremove2(mesh->faces.size(),true);
				faces_between_spheres(mesh,vec(0.0f,0.0f,0.0f),vec(0.0f,0.0f,0.0f),v[0]-eps2,1.0f/eps2,toremove2);
				int klm = zoneCount(mesh,toremove2);
				//int klm = zoneCount(mesh,v[0]-eps2,1.0f/eps2);
				printf("\nklm= %d",klm);
				toremove2.clear();*/
#pragma omp parallel for
				for(int d=0;d<2;d++){
					K[d] = fct__(mesh,vals[d][0],vals[d][1]);
					printf("\nklp%d = %d\n",d,K[d]);
				}
				crossed[i] = true;
				if((K[0]-K[1])>0) SizeElementsList.push_back(SizeElement((double)v[0],(double)100000.0,(double)0.0));
			}
			//int l,m;
			//Test if it is a cornerpoint
			//float alpha = eps1;
			//while(alpha<eps1){
				
				//float beta = eps1-alpha;
			

				l = v[0]+alpha;
				m = v[1]-alpha;
				vector<int> K(4);
				vector<vec2> vals(4);
				vals[0] = vec2(v[0]+alpha,v[1]-alpha);
				vals[1] = vec2(v[0]+alpha,v[1]+alpha);
				vals[2] = vec2(v[0]-alpha,v[1]-alpha);
				vals[3] = vec2(v[0]-alpha,v[1]+alpha);
#pragma omp parallel for
				for(int d=0;d<4;d++){
					K[d] = fct__(mesh,vals[d][0],vals[d][1]);
					printf("\nk%d = %d\n",d,K[d]);
				}
				//printf("\nl= %f, m=%f",l,m);
				//printf("\nalpha= %f, beta=%f",alpha,beta);
				/*int k1=0;
				if(m>l){
					//printf("\nYes\n");
					if(l<0) l=values[0]/4.0f;
					vector<bool> toremove(mesh->faces.size(),true);
					faces_between_spheres(mesh,vec(0.0f,0.0f,0.0f),vec(0.0f,0.0f,0.0f),l,m,toremove);
					k1 = zoneCount(mesh,toremove);
					//k1 = zoneCount(mesh,l,m);
					printf("\nk1= %d",k1);
					printf("\nl= %f, m=%f",l,m);
					toremove.clear();
				}

				l = v[0]+alpha;
				m = v[1]+alpha;
				int k2=0;
				//printf("\nl= %f, m=%f",l,m);
				if(m>l){
					if(l<0) l=values[0]/4.0f;
					vector<bool> toremove(mesh->faces.size(),true);
					faces_between_spheres(mesh,vec(0.0f,0.0f,0.0f),vec(0.0f,0.0f,0.0f),l,m,toremove);
					k2 = zoneCount(mesh,toremove);
					//k2 = zoneCount(mesh,l,m);
					printf("\nk2= %d",k2);
					printf("\nl= %f, m=%f",l,m);
					toremove.clear();
				}

				l = v[0]-alpha;
				m = v[1]-alpha;
				int k3=0;
				if(m>l){
					if(l<0) l=values[0]/4.0f;
					vector<bool> toremove(mesh->faces.size(),true);
					faces_between_spheres(mesh,vec(0.0f,0.0f,0.0f),vec(0.0f,0.0f,0.0f),l,m,toremove);
					k3 = zoneCount(mesh,toremove);
					//k3 = zoneCount(mesh,l,m);
					printf("\nk3= %d",k3);
					printf("\nl= %f, m=%f",l,m);
					toremove.clear();
				}

				l = v[0]-alpha;
				m = v[1]+alpha;
				int k4=0;
				if(m>l){
					if(l<0) l=values[0]/4.0f;
					vector<bool> toremove(mesh->faces.size(),true);
					faces_between_spheres(mesh,vec(0.0f,0.0f,0.0f),vec(0.0f,0.0f,0.0f),l,m,toremove);
					k4 = zoneCount(mesh,toremove);
					//k4 = zoneCount(mesh,l,m);
					printf("\nk4= %d",k4);
					printf("\nl= %f, m=%f",l,m);
					toremove.clear();
				}*/
				if((K[0]-K[1]-K[2]+K[3])>0){ 
					SizeElementsList.push_back(SizeElement((double)v[0],(double)v[1],(double)0.0));
				}
			
		}
		//printf("\n");
	}
	values.clear();
	crossed.clear();
	int l = SizeElementsList.size();
	printf("\n %d",l);
	//saveSizeElements(SizeElementsList,fname);
	SizeElementsList.clear();
	mesh->vertices.clear();
	mesh->faces.clear();
	mesh->across_edge.clear();
}
void size_Function_Sphere1(TriMesh * mesh, int order, char *fname){
	mesh->need_across_edge();
	int nf = mesh->faces.size();
	vector<bool> isOverlap(nf,false);
	vector<bool> marked(nf,false);
	vector<vector<int>> domainList(order+1);
	for(int i=0;i<order;i++){
		domainList[i].clear();
	}
	int cpt =0;
	//printf("\n Nf = %d",nf);
	for(int i=0;i<nf;i++){
		for(int j=0;j<=order;j++){
			float u = 1.0f*(((float)j)/((float)order));
			if(triangleSphereOverlap(mesh->vertices[mesh->faces[i][0]],mesh->vertices[mesh->faces[i][1]],
				mesh->vertices[mesh->faces[i][2]],vec(0.0f,0.0f,0.0f),u)){ 
				isOverlap[i] = true; 
				domainList[j].push_back(i);
				//cpt++;
				//printf("\n i = %d",i);
			}
		}
	}

	//printf("\nGood !!!!!!!!!!!");
	vector<SizeElement> SizeElementsList;
	SizeElementsList.clear();
	for(int i=0;i<order;i++){
		float rmin = (float)(((float)(i))/((float)order));
		float rmax = (float)(((float)(i+1))/((float)order));
		int count = zoneCount(mesh,domainList[i],domainList[i+1],isOverlap,marked,rmin,rmax);
		//vector<int> indices;
		//indices.clear();
		//faces_between_spheres(mesh,vec(0.0f,0.0f,0.0f),vec(0.0f,0.0f,0.0f),rmin,rmax,indices);
		//int count = zoneCount(mesh,indices);
		printf("\nCount = %d",count);
		//indices.clear();
		SizeElementsList.push_back(SizeElement((double)rmin,(double)rmax,(double)count));
	}	
	saveSizeElements(SizeElementsList,fname);
	SizeElementsList.clear();
	mesh->vertices.clear();
	mesh->faces.clear();
	mesh->across_edge.clear();
}
vec *getPCAAxisCurvature(TriMesh *mesh){
	point com = point_center_of_mass(mesh->vertices);
	trans(mesh, -com);
	point center(0.0f,0.0f,0.0f);
	float s = 1.0f/(getMaxDistance(center,mesh));
	scale(mesh,s);
	//trans(mesh, -com);
	float C[3][3];
	directions_curvature_covariance(mesh, C);
	float e[3];
	eigdc<float,3>(C, e);

	// Sorted in order from smallest to largest, so grab third column
	vec first(C[0][2], C[1][2], C[2][2]);
	

	vec second(C[0][1], C[1][1], C[2][1]);

	vec third = first CROSS second;
	xform xf;
	xf[0] = first[0];  xf[1] = first[1];  xf[2] = first[2];
	xf[4] = second[0]; xf[5] = second[1]; xf[6] = second[2];
	xf[8] = third[0];  xf[9] = third[1];  xf[10] = third[2];

	//invert(xf);
	vec v1(xf[0],xf[1],xf[2]);
	vec v2(xf[4],xf[5],xf[6]);
	vec v3(xf[8],xf[9],xf[10]);
	vec *V = new vec[3];
	V[0] = v1;
	V[1] = v2;
	V[2] = v3;
	//scale(mesh,((float)1.0f/s));
	//trans(tmp, com);
	return V;
	//apply_xform(tmp, xf);
}
// As above, but only rotate by 90/180/etc. degrees w.r.t. original
void pca_snap(TriMesh *mesh)
{
	point com = mesh_center_of_mass(mesh);
	trans(mesh, -com);

	float C[3][3];
	mesh_covariance(mesh, C);
	float e[3];
	eigdc<float,3>(C, e);

	// Sorted in order from smallest to largest, so grab third column
	vec first(C[0][2], C[1][2], C[2][2]);
	int npos = 0;
	int nv = mesh->vertices.size();
	for (int i = 0; i < nv; i++)
		if ((mesh->vertices[i] DOT first) > 0.0f)
			npos++;
	if (npos < nv/2)
		first = -first;
	if (fabs(first[0]) > fabs(first[1])) {
		if (fabs(first[0]) > fabs(first[2])) {
			first[1] = first[2] = 0;
			first[0] /= fabs(first[0]);
		} else {
			first[0] = first[1] = 0;
			first[2] /= fabs(first[2]);
		}
	} else {
		if (fabs(first[1]) > fabs(first[2])) {
			first[0] = first[2] = 0;
			first[1] /= fabs(first[1]);
		} else {
			first[0] = first[1] = 0;
			first[2] /= fabs(first[2]);
		}
	}

	vec second(C[0][1], C[1][1], C[2][1]);
	npos = 0;
	for (int i = 0; i < nv; i++)
		if ((mesh->vertices[i] DOT second) > 0.0f)
			npos++;
	if (npos < nv/2)
		second = -second;
	second -= first * (first DOT second);
	if (fabs(second[0]) > fabs(second[1])) {
		if (fabs(second[0]) > fabs(second[2])) {
			second[1] = second[2] = 0;
			second[0] /= fabs(second[0]);
		} else {
			second[0] = second[1] = 0;
			second[2] /= fabs(second[2]);
		}
	} else {
		if (fabs(second[1]) > fabs(second[2])) {
			second[0] = second[2] = 0;
			second[1] /= fabs(second[1]);
		} else {
			second[0] = second[1] = 0;
			second[2] /= fabs(second[2]);
		}
	}

	vec third = first CROSS second;

	xform xf;
	xf[0] = first[0];  xf[1] = first[1];  xf[2] = first[2];
	xf[4] = second[0]; xf[5] = second[1]; xf[6] = second[2];
	xf[8] = third[0];  xf[9] = third[1];  xf[10] = third[2];

	invert(xf);
	apply_xform(mesh, xf);

	trans(mesh, com);
}


// Helper function: return the largest X coord for this face
static float max_x(const TriMesh *mesh, int i)
{
	return max(max(mesh->vertices[mesh->faces[i][0]][0],
		       mesh->vertices[mesh->faces[i][1]][0]),
		       mesh->vertices[mesh->faces[i][2]][0]);
}


// Flip faces so that orientation among touching faces is consistent
void orient(TriMesh *mesh)
{
	mesh->need_faces();
	mesh->tstrips.clear();
	mesh->need_adjacentfaces();

	mesh->flags.clear();
	const unsigned NONE = ~0u;
	mesh->flags.resize(mesh->faces.size(), NONE);

	TriMesh::dprintf("Auto-orienting mesh... ");
	unsigned cc = 0;
	vector<int> cc_farthest;
	for (int i = 0; i < mesh->faces.size(); i++) {
		if (mesh->flags[i] != NONE)
			continue;
		mesh->flags[i] = cc;
		cc_farthest.push_back(i);
		float farthest_val = max_x(mesh, i);

		vector<int> q;
		q.push_back(i);
		while (!q.empty()) {
			int f = q.back();
			q.pop_back();
			for (int j = 0; j < 3; j++) {
				int v0 = mesh->faces[f][j];
				int v1 = mesh->faces[f][(j+1)%3];
				const vector<int> &a = mesh->adjacentfaces[v0];
				for (int k = 0; k < a.size(); k++) {
					int f1 = a[k];
					if (mesh->flags[f1] != NONE)
						continue;
					int i0 = mesh->faces[f1].indexof(v0);
					int i1 = mesh->faces[f1].indexof(v1);
					if (i0 < 0 || i1 < 0)
						continue;
					if (i1 == (i0 + 1) % 3)
						swap(mesh->faces[f1][1],
						     mesh->faces[f1][2]);
					mesh->flags[f1] = cc;
					if (max_x(mesh, f1) > farthest_val) {
						farthest_val = max_x(mesh, f1);
						cc_farthest[cc] = f1;
					}
					q.push_back(f1);
				}
			}
		}
		cc++;
	}

	vector<bool> cc_flip(cc, false);
	for (int i = 0; i < cc; i++) {
		int f = cc_farthest[i];
		const point &v0 = mesh->vertices[mesh->faces[f][0]];
		const point &v1 = mesh->vertices[mesh->faces[f][1]];
		const point &v2 = mesh->vertices[mesh->faces[f][2]];
		int j = 0;
		if (v1[0] > v0[0])
			if (v2[0] > v1[0]) j = 2; else j = 1;
		else
			if (v2[0] > v0[0]) j = 2;
		int v = mesh->faces[f][j];
		const vector<int> &a = mesh->adjacentfaces[v];
		vec n;
		for (int k = 0; k < a.size(); k++) {
			int f1 = a[k];
			const point &v0 = mesh->vertices[mesh->faces[f1][0]];
			const point &v1 = mesh->vertices[mesh->faces[f1][1]];
			const point &v2 = mesh->vertices[mesh->faces[f1][2]];
			n += trinorm(v0, v1, v2);
		}
		if (n[0] < 0.0f)
			cc_flip[i] = true;
	}

	for (int i = 0; i < mesh->faces.size(); i++) {
		if (cc_flip[mesh->flags[i]])
			swap(mesh->faces[i][1], mesh->faces[i][2]);
	}
	TriMesh::dprintf("Done.\n");
}


// Remove boundary vertices (and faces that touch them)
void erode(TriMesh *mesh)
{
	int nv = mesh->vertices.size();
	vector<bool> bdy(nv);
	for (int i = 0; i < nv; i++)
		bdy[i] = mesh->is_bdy(i);
	remove_vertices(mesh, bdy);
}


// Add a bit of noise to the mesh
void noisify(TriMesh *mesh, float amount)
{
	mesh->need_normals();
	mesh->need_neighbors();
	int nv = mesh->vertices.size();
	vector<vec> disp(nv);

	for (int i = 0; i < nv; i++) {
		point &v = mesh->vertices[i];
		// Tangential
		for (int j = 0; j < mesh->neighbors[i].size(); j++) {
			const point &n = mesh->vertices[mesh->neighbors[i][j]];
			float scale = amount / (amount + len(n-v));
			disp[i] += (float) tinyrnd() * scale * (n-v);
		}
		if (mesh->neighbors[i].size())
			disp[i] /= (float) mesh->neighbors[i].size();
		// Normal
		disp[i] += (2.0f * (float) tinyrnd() - 1.0f) *
			   amount * mesh->normals[i];
	}
	for (int i = 0; i < nv; i++)
		mesh->vertices[i] += disp[i];
}


