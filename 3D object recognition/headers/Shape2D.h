#pragma once

#ifndef SHAPE2D_H_
#define SHAPE2D_H_

#include <string>
#include <vector>
#include "Vec.h"
#include "Del.h"

using namespace std;

class Shape2D {

	string fname;
	vector<vec2> Plist;
	vector<vec2> Pnlist;
	vector<vec2> PnDlist;
	vector<vec2> PnDDlist;
	vector<Del> DelList;
	int nb_inv,nb_invd;
	vector<int> tab1,tab;
	vector<float> Ku;
	void clear();
	float Gaussian(float p, float sigma);

public:
	Shape2D();
	Shape2D(const string &fileName);
	Shape2D(const vector<vec2> &);
	Shape2D(Shape2D & );
	Shape2D & operator =(const Shape2D &);
	void load(const string &fileName);
	void display();
	void displayProcessedShape();
	void smooth(float);

	void calcul_derivee();
	void displayDerivee_1ere();
	void displayDerivee_2nd();
	float d_orientation(int,int,int);
	float d_orientationN(int,int,int);
	float d_orientation(int);
	void descripteur();
	void computeKu();
	int min_courbure(int);
	int max_courbure(int);
	int max_abs_courbure(int,int);
	void delBimbo(int,int);
	float Distance(const Shape2D &b);
	float Distance(const Del &b);
	float Distance_token(const Del &, const Del&);
	void saveDelBimboDescriptor(const string &fileName);
	void displayDelBimboDescriptor();
	void normalize();
	virtual ~Shape2D();
};

#endif /* SHAPE2D_H_ */
