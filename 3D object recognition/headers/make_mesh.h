#pragma once



#ifndef MAKE_MESH_H_
#define MAKE_MESH_H_

#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "TriMesh.h"
#include "TriMesh_algo.h"
#include "XForm.h"
#include "noise3d.h"
#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif
using namespace std;


static inline void mkpoint(TriMesh *mesh, float x, float y, float z)
{
	mesh->vertices.push_back(point(x,y,z));
}

static inline void mkface(TriMesh *mesh, int v1, int v2, int v3)
{
	mesh->faces.push_back(TriMesh::Face(v1, v2, v3));
}

static inline void mkquad(TriMesh *mesh, int ll, int lr, int ul, int ur)
{
	mkface(mesh, ll, lr, ur);
	mkface(mesh, ll, ur, ul);
}

static inline void tess4(TriMesh *mesh, int v1, int v2, int v3, int v4)
{
	point c = 0.25f * (mesh->vertices[v1] + mesh->vertices[v2] +
			   mesh->vertices[v3] + mesh->vertices[v4]);
	mkpoint(mesh, c[0], c[1], c[2]);
	int ci = mesh->vertices.size() - 1;
	mkface(mesh, ci, v1, v2);
	mkface(mesh, ci, v2, v3);
	mkface(mesh, ci, v3, v4);
	mkface(mesh, ci, v4, v1);
}

static inline void tess5(TriMesh *mesh, int v1, int v2, int v3, int v4, int v5)
{
	point c = 0.2f * (mesh->vertices[v1] + mesh->vertices[v2] +
			  mesh->vertices[v3] + mesh->vertices[v4] +
			  mesh->vertices[v5]);
	mkpoint(mesh, c[0], c[1], c[2]);
	int ci = mesh->vertices.size() - 1;
	mkface(mesh, ci, v1, v2);
	mkface(mesh, ci, v2, v3);
	mkface(mesh, ci, v3, v4);
	mkface(mesh, ci, v4, v5);
	mkface(mesh, ci, v5, v1);
}


extern TriMesh *make_plane(int tess_x, int tess_y);

extern TriMesh *make_bump(int tess, float sigma);

extern TriMesh *make_wave(int tess, float omega);

extern TriMesh *make_frac(int tess);

extern TriMesh *make_cube(int tess);

extern TriMesh *make_disc(int tess_th, int tess_r);

extern TriMesh *make_cyl(int tess_th, int tess_h, float);

extern TriMesh *make_ccyl(int tess_th, int tess_h, float);

extern TriMesh *make_cone(int tess_th, int tess_r, float);

extern TriMesh *make_ccone(int tess_th, int tess_r, float);

extern TriMesh *make_torus(int tess_th, int tess_ph, float);

extern TriMesh *make_knot(int tess_th, int tess_ph, float);

extern TriMesh *make_klein(int tess_th, int tess_ph);

extern TriMesh *make_helix(int tess_th, int tess_ph, float turns, float r );

extern TriMesh *make_sphere_polar(int tess_ph, int tess_th);

extern TriMesh *make_platonic(int nfaces);

extern TriMesh *make_sphere_subdiv(int nfaces, int nsubdiv);

extern TriMesh *make_rd();

extern TriMesh *make_rt();




#endif /* MAKE_MESH_H_ */

