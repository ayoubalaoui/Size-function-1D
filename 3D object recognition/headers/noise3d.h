#pragma once

#ifndef NOISE3D_H_
#define NOISE3D_H_

#include <math.h>
#include <algorithm>
#include <vector>
using std::max;
using std::swap;
using std::vector;


// Quick 'n dirty portable random number generator
static inline float tinyrnd()
{
	static unsigned trand = 0;
	trand = 1664525u * trand + 1013904223u;
	return (float) trand / 4294967296.0f;
}


class Noise3D {
public:
	int xsize, ysize, zsize;
	vector<float> r;
	vector<int> p;

	int coord2index(int x, int y, int z) const
	{
		return p[ p[x] + y ] + z;
	}


	Noise3D(int _xsize, int _ysize, int _zsize) :
		xsize(_xsize), ysize(_ysize), zsize(_zsize)
	{
		if (xsize < 2)
			xsize = 2;
		if (ysize < 2)
			ysize = 2;
		if (zsize < 2)
			ysize = 2;

		int i;
		int pxy = max(xsize, ysize);
		for (i = 0; i < pxy; i++)
			p.push_back(i);
		for (i = 0; i < pxy; i++) {
			int j = int(tinyrnd()*pxy);
			swap(p[i], p[j]);
		}
		for (i = pxy; i < pxy+ysize; i++)
			p.push_back(p[i-pxy]);
		for (i = 0; i < pxy + zsize; i++)
			r.push_back(tinyrnd());
	}

	virtual float lookup(float x, float y, float z) const
	{
		x -= floor(x);
		y -= floor(y);
		z -= floor(z);

		int X = int(x*xsize);
		int Y = int(y*ysize);
		int Z = int(z*zsize);
		int X1 = X+1;  if (X1 == xsize) X1 = 0;
		int Y1 = Y+1;  if (Y1 == ysize) Y1 = 0;
		int Z1 = Z+1;  if (Z1 == zsize) Z1 = 0;

		float xf = x*xsize - X,  xf1 = 1.0-xf;
		float yf = y*ysize - Y,  yf1 = 1.0-yf;
		float zf = z*zsize - Z,  zf1 = 1.0-zf;

		return xf1*(yf1*(zf1*r[coord2index(X , Y , Z )] +
				 zf *r[coord2index(X , Y , Z1)]) +
			    yf *(zf1*r[coord2index(X , Y1, Z )] +
				 zf *r[coord2index(X , Y1, Z1)])) +
		       xf *(yf1*(zf1*r[coord2index(X1, Y , Z )] +
				 zf *r[coord2index(X1, Y , Z1)]) +
			    yf *(zf1*r[coord2index(X1, Y1, Z )] +
				 zf *r[coord2index(X1, Y1, Z1)]));
	}

	virtual ~Noise3D() {}
};


#define MAGIC_SCALE 1.5707963f

class PerlinNoise3D : public Noise3D {
	int passes;
	float correction;
public:
	PerlinNoise3D(int _xsize, int _ysize, int _zsize) :
		Noise3D(_xsize, _ysize, _zsize), correction(0)
	{
		passes = int(log((float)xsize)/log(MAGIC_SCALE) + 0.5);
		passes = max(passes, int(log((float)ysize)/log(MAGIC_SCALE) + 0.5));
		passes = max(passes, int(log((float)zsize)/log(MAGIC_SCALE) + 0.5));
		float factor = 1.0;
		for (int pass = 0 ; pass < passes; pass++, factor *= MAGIC_SCALE)
			correction += factor*factor;
		correction = 1.0f / sqrt(correction);
	}

	virtual float lookup(float x, float y, float z) const
	{
		float t = 0;
		float factor = 1.0;
		for (int pass = 0 ; pass < passes; pass++, factor *= MAGIC_SCALE) {
			float r = 1.0f / factor;
			t += Noise3D::lookup(x*r,y*r,z*r) * factor;
		}

		return t * correction;
	}
};


#endif /* NOISE3D_H_ */
