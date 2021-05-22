#pragma once

#include <cmath>
#include <list>
#include <vector>
#include <iterator>
#include "Vec.h"

#define DEFAULT_PRECISION 200

using namespace std;


class spline_curve
{
public:
  spline_curve(const vector<vec2> & _which);


  spline_curve(const spline_curve & Q);
 

  ~spline_curve()
  {
	  curve_point.clear();
	  plist.clear();
  }

  void compute();
  void Inc_Precision() { this->precision *= 2; }
  void Dec_Precision() { this->precision = (int)ceil((float)this->precision/2.0f); }

  void destroy();

  vector<vec2> curve_point;
  int nb_curve_point;
  
  vector<vec2> plist;
  int nb_point;

  int precision;
};
