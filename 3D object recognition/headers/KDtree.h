#pragma once

#ifndef KDTREE_H_
#define KDTREE_H_

#include <vector>

class KDtree {
private:
	class Node;
	Node *root;
	void build(const float *ptlist, int n);

public:
	// Compatibility function for closest-compatible-point searches
	struct CompatFunc
	{
		virtual bool operator () (const float *p) const = 0;
		virtual ~CompatFunc() {}  // To make the compiler shut up
	};

	// Constructor from an array of points
	KDtree(const float *ptlist, int n)
		{ build(ptlist, n); }
	// Constructor from a vector of points
	template <class T> KDtree(const std::vector<T> &v)
		{ build((const float *) &v[0], v.size()); }
	~KDtree();

	// The queries: returns closest point to a point or a ray,
	// provided it's within sqrt(maxdist2) and is compatible
	const float *closest_to_pt(const float *p,
				   float maxdist2,
				   const CompatFunc *iscompat = NULL) const;
	const float *closest_to_ray(const float *p, const float *dir,
				    float maxdist2,
				    const CompatFunc *iscompat = NULL) const;
};

#endif /* KDTREE_H_ */

