#include "StdAfx.h"
#include "spline_curve.h"


float spline_uniform(float t, float x) // fonction allure Gauss, maximum en x = t
{
  t -= 2.0f;

  if ((x < t+1) && (x >= t)) return pow(x - t,3)/6.0f;
  else if ((x < t+2) && (x >= t+1)) return 1.0f/6.0f * (3*t*t*t-3*t*t*(3*x-4)+3*t*(3*x*x-8*x+4)-3*x*x*x+12*x*(x-1)+4);
  else if ((x < t+3) && (x >= t+2)) return -1.0f/6.0f * (3*t*t*t-3*t*t*(3*x-8)+3*t*(3*x*x-16*x+20)-3*x*x*x+24*x*x-60*x+44);
  else if ((x < t+4) && (x >= t+3)) return 1.0f/6.0f * (t*t*t - 3*t*t*(x-4)+3*t*(x*x-8*x+16)-x*x*x+12*x*x-48*x+64);
  else return 0.0f;
}
 spline_curve::spline_curve(const vector<vec2> & _which)
  {
	  this->plist.clear();
	  plist.assign(_which.begin(),_which.end());
	  this->curve_point.clear();
    this->nb_curve_point = 0;
    this->nb_point = this->plist.size();
    this->precision = DEFAULT_PRECISION;
  }
  spline_curve::spline_curve(const spline_curve & Q) // non testé
  {
	  if(Q.plist.size()>0) plist.assign(Q.plist.begin(),Q.plist.end());
	  if(Q.curve_point.size()>0) curve_point.assign(Q.curve_point.begin(),Q.curve_point.end());
	  precision = Q.precision;
	  nb_curve_point = Q.nb_curve_point;
	  nb_point = Q.nb_point;
  }
  void spline_curve::destroy(){
	  curve_point.clear();
	  plist.clear();
  }
void spline_curve::compute()
  {
	  curve_point.clear();
	  this->curve_point.resize(this->precision+1);
    this->nb_curve_point = this->precision+1;

    float tx, ty, tb, u=1.0f, p = ((float)this->nb_point-3.0f)/(float)this->precision;

    for (int i = 0; i<this->nb_curve_point; i++)
      {
	tx = 0;
	ty = 0;
	for (int j=1; j<=this->nb_point; j++) // sommer tous les points est très lourd (puisque pour la plupart, spline_uniform(...) retourne 0), mais tellement plus simple. A optimiser donc...
	  {
	    tb = spline_uniform(j-1,u);
	    tx += tb*this->plist[j-1][0];
	    ty += tb*this->plist[j-1][1];
	  }
	tb = 0;
	u += p;
	this->curve_point[i][0] = tx;
	this->curve_point[i][1] = ty;
      }

 
  }
