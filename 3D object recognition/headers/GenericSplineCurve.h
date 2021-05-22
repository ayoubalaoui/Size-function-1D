#pragma once

#include <cmath>
#include <list>
#include <vector>
#include <iterator>
#include "Vec.h"

#define DEFAULT_PRECISION 200


using namespace std;
class GenericSplineCurve
{
public:
    GenericSplineCurve(const vector<Vec<4,float>>& _which);


    GenericSplineCurve(const GenericSplineCurve& Q);


    ~GenericSplineCurve()
    {
        curve_point.clear();
        plist.clear();
    }

    void compute();
    void Inc_Precision() { this->precision *= 2; }
    void Dec_Precision() { this->precision = (int)ceil((float)this->precision / 2.0f); }

    void destroy();

    vector<Vec<4, float>> curve_point;
    int nb_curve_point;

    vector<Vec<4, float>> plist;
    int nb_point;

    int precision;
    float spline_uniform(float t, float x);
};

