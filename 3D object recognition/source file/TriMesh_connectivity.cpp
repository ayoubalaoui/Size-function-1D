#include "StdAfx.h"
#include "TriMesh_connectivity.h"

#include <stdio.h>
#include "TriMesh.h"
#include "TriMesh_algo.h"
#include <vector>
#include <stack>
#include <algorithm>

#define NO_COMP -1
#define FOR_EACH_ADJACENT_FACE(mesh,v,f) \
	for (int f_ind = 0, f = mesh->adjacentfaces[v][0]; \
	     (f_ind < mesh->adjacentfaces[v].size()) && \
	     ((f = mesh->adjacentfaces[v][f_ind]) || 1); \
	     f_ind++)

using std::find;
using std::stack;


// Find the direct neighbors of each vertex
void TriMesh::need_neighbors()
{
	if (!neighbors.empty())
		return;
	need_faces();

	dprintf("Finding vertex neighbors... ");
	int nv = vertices.size(), nf = faces.size();

	vector<int> numneighbors(nv);
	for (int i = 0; i < nf; i++) {
		numneighbors[faces[i][0]]++;
		numneighbors[faces[i][1]]++;
		numneighbors[faces[i][2]]++;
	}

	neighbors.resize(nv);
	for (int i = 0; i < nv; i++)
		neighbors[i].reserve(numneighbors[i]+2); // Slop for boundaries

	for (int i = 0; i < nf; i++) {
		for (int j = 0; j < 3; j++) {
			vector<int> &me = neighbors[faces[i][j]];
			int n1 = faces[i][(j+1)%3];
			int n2 = faces[i][(j+2)%3];
			if (find(me.begin(), me.end(), n1) == me.end())
				me.push_back(n1);
			if (find(me.begin(), me.end(), n2) == me.end())
				me.push_back(n2);
		}
	}

	dprintf("Done.\n");
}


// Find the faces touching each vertex
void TriMesh::need_adjacentfaces()
{
	if (!adjacentfaces.empty())
		return;
	need_faces();

	dprintf("Finding vertex to triangle maps... ");
	int nv = vertices.size(), nf = faces.size();

	vector<int> numadjacentfaces(nv);
	for (int i = 0; i < nf; i++) {
		numadjacentfaces[faces[i][0]]++;
		numadjacentfaces[faces[i][1]]++;
		numadjacentfaces[faces[i][2]]++;
	}

	adjacentfaces.resize(vertices.size());
	for (int i = 0; i < nv; i++)
		adjacentfaces[i].reserve(numadjacentfaces[i]);

	for (int i = 0; i < nf; i++) {
		for (int j = 0; j < 3; j++)
			adjacentfaces[faces[i][j]].push_back(i);
	}

	dprintf("Done.\n");
}


// Find the face across each edge from each other face (-1 on boundary)
// If topology is bad, not necessarily what one would expect...
void TriMesh::need_across_edge()
{
	if (!across_edge.empty())
		return;
	need_adjacentfaces();

	dprintf("Finding across-edge maps... ");

	int nf = faces.size();
	across_edge.resize(nf, Face(-1,-1,-1));
#pragma parallel for
	for (int i = 0; i < nf; i++) {
		for (int j = 0; j < 3; j++) {
			if (across_edge[i][j] != -1)
				continue;
			int v1 = faces[i][(j+1)%3];
			int v2 = faces[i][(j+2)%3];
			const vector<int> &a1 = adjacentfaces[v1];
			const vector<int> &a2 = adjacentfaces[v2];
			for (int k1 = 0; k1 < a1.size(); k1++) {
				int other = a1[k1];
				if (other == i)
					continue;
				vector<int>::const_iterator it =
					find(a2.begin(), a2.end(), other);
				if (it == a2.end())
					continue;
				int ind = (faces[other].indexof(v1)+1)%3;
				if (faces[other][(ind+1)%3] != v2)
					continue;
				across_edge[i][j] = other;
				across_edge[other][ind] = i;
				break;
			}
		}
	}

	dprintf("Done.\n");
}


bool connected(const TriMesh *in, int f1, int f2, bool conn_vert)
{
	int f10=in->faces[f1][0], f11=in->faces[f1][1], f12=in->faces[f1][2];
	int f20=in->faces[f2][0], f21=in->faces[f2][1], f22=in->faces[f2][2];

	if (conn_vert)
		return f10 == f20 || f10 == f21 || f10 == f22 ||
		       f11 == f20 || f11 == f21 || f11 == f22 ||
		       f12 == f20 || f12 == f21 || f12 == f22;
	else
		return (f10 == f20 && (f11 == f22 || f12 == f21)) ||
		       (f10 == f21 && (f11 == f20 || f12 == f22)) ||
		       (f10 == f22 && (f11 == f21 || f12 == f20)) ||
		       (f11 == f20 && f12 == f22) ||
		       (f11 == f21 && f12 == f20) ||
		       (f11 == f22 && f12 == f21);
}
// Helper function for find_comps, below.  Finds and marks all the faces
// connected to f.
void find_connected(const TriMesh *in,
		    vector<int> &comps, vector<int> &compsizes,
		    int f, int whichcomponent)
{
	stack<int> s;
	s.push(f);
	while (!s.empty()) {
		int currface = s.top();
		s.pop();
		for (int i = 0; i < 3; i++) {
			int vert = in->faces[currface][i];
			FOR_EACH_ADJACENT_FACE(in, vert, adjface) {
				if (comps[adjface] != NO_COMP ||
				    !connected(in, adjface, currface,false))
					continue;
				comps[adjface] = whichcomponent;
				compsizes[whichcomponent]++;
				s.push(adjface);
			}
		}
	}
}


// Find the connected components of TriMesh "in".
// Outputs:
//  comps is a vector that gives a mapping from each face to its
//   associated connected component.
//  compsizes holds the size of each connected component
void find_comps(TriMesh *in, vector<int> &comps, vector<int> &compsizes)
{
	if (in->vertices.empty())
		return;
	if (in->faces.empty())
		return;
	in->need_adjacentfaces();

	int nf = in->faces.size();
	comps.clear();
	comps.reserve(nf);
	comps.resize(nf, NO_COMP);
	compsizes.clear();

	for (int i = 0; i < nf; i++) {
		if (comps[i] != NO_COMP)
			continue;
		int comp = compsizes.size();
		comps[i] = comp;
		compsizes.push_back(1);
		find_connected(in, comps, compsizes, i, comp);
	}
}

// Helper class for comparing two integers by finding the elements at those
// indices within some array and comparing them
template <class Array>
class CompareArrayElements {
private:
	const Array &a;
public:
	CompareArrayElements(const Array &_a) : a(_a)
		{}
	bool operator () (int i1, int i2) const
	{
		return (a[i1] > a[i2]);
	}
};


// Sorts the connected components from largest to smallest.  Renumbers the
// elements of compsizes to reflect this new numbering.
void sort_comps(vector<int> &comps, vector<int> &compsizes)
{
	if (compsizes.size() < 1)
		return;

	int i;
	vector<int> comp_pointers(compsizes.size());
	for (i = 0; i < comp_pointers.size(); i++)
		comp_pointers[i] = i;

	sort(comp_pointers.begin(), comp_pointers.end(),
	     CompareArrayElements< vector<int> >(compsizes));

	vector<int> remap_table(comp_pointers.size());
	for (i = 0; i < comp_pointers.size(); i++)
		remap_table[comp_pointers[i]] = i;
	for (i = 0; i < comps.size(); i++)
		comps[i] = remap_table[comps[i]];

	vector<int> newcompsizes(compsizes.size());
	for (i = 0; i < compsizes.size(); i++)
		newcompsizes[i] = compsizes[comp_pointers[i]];
	compsizes = newcompsizes;
}

// Select a particular connected component, and delete all other vertices from
// the mesh.
void select_comp(TriMesh *in, const vector<int> &comps, int whichcc)
{
	int numfaces = in->faces.size();
	vector<bool> toremove(numfaces, false);
	for (int i = 0; i < in->faces.size(); i++) {
		if (comps[i] != whichcc)
			toremove[i] = true;
	}

	int k = in->faces.size();
	if(k==0) return;
	remove_faces(in, toremove);
	k = in->faces.size();
	if(k==0) return;
	//printf("\nproblem is here %d\n\n",k);
	remove_unused_vertices(in);
}

// Select a particular connected component, and delete all other vertices from
// the mesh.
void select_TheTwoBiggestcomps(TriMesh *in, const vector<int> &comps)
{
	int numfaces = in->faces.size();
	vector<bool> toremove(numfaces, false);
	for (int i = 0; i < in->faces.size(); i++) {
			if (comps[i] != 0 && comps[i] != 1)
				toremove[i] = true;
	}

	int k = in->faces.size();
	if(k==0) return;
	remove_faces(in, toremove);
	k = in->faces.size();
	if(k==0) return;
	//printf("\nproblem is here %d\n\n",k);
	remove_unused_vertices(in);
}