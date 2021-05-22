#pragma once
#include "Vec.h"
#include <vector>

class BoundingBox
{

public:
    vecd3 center;
    vecd3 vmax, vmin;
    double xExtent, yExtent, zExtent;

    BoundingBox();
    BoundingBox(const vecd3& c, double x, double y, double z);
    BoundingBox(const vecd3& fromMin, const vecd3& toMax);
    BoundingBox& operator= (const BoundingBox& other);
    ~BoundingBox(void);

    bool contains(const vecd3& point) const;

    bool containsTriangle(const vecd3& tv1, const vecd3& tv2, const vecd3& tv3) const;
    bool intersectsBoundingBox(const BoundingBox& bb) const;

    std::vector<vecd3> getCorners();

    vecd3 Center();
    void Offset(double s);
    void Offset(vecd3 delta);
    double Diag();
};

/* AABB-triangle overlap test code                      */
/* by Tomas Akenine-Möller                              */
#define X 0
#define Y 1
#define Z 2

#define FINDMINMAX(x0,x1,x2,min,max) \
        min = max = x0;   \
        if(x1<min) min=x1;\
        if(x1>max) max=x1;\
        if(x2<min) min=x2;\
        if(x2>max) max=x2;

static inline int planeBoxOverlap(const vecd3& normal, const vecd3& vert, const vecd3& maxbox)
{
    vecd3 vmin, vmax;

    for (int q = X; q <= Z; q++)
    {
        double v = vert[q];
        if (normal[q] > 0.0)
        {
            vmin[q] = -maxbox[q] - v;
            vmax[q] = maxbox[q] - v;
        }
        else
        {
            vmin[q] = maxbox[q] - v;
            vmax[q] = -maxbox[q] - v;
        }
    }
    if ((normal DOT vmin) > 0.0) return 0;
    if ((normal DOT vmax) >= 0.0) return 1;

    return 0;
}
/*======================== X-tests ========================*/
#define AXISTEST_X01(a, b, fa, fb)                         \
        p0 = a*v0[Y] - b*v0[Z];                            \
        p2 = a*v2[Y] - b*v2[Z];                            \
        if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
        rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];   \
        if(min>rad || max<-rad) return 0;
#define AXISTEST_X2(a, b, fa, fb)                          \
        p0 = a*v0[Y] - b*v0[Z];                            \
        p1 = a*v1[Y] - b*v1[Z];                            \
        if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
        rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];   \
        if(min>rad || max<-rad) return 0;
/*======================== Y-tests ========================*/
#define AXISTEST_Y02(a, b, fa, fb)                         \
        p0 = -a*v0[X] + b*v0[Z];                           \
        p2 = -a*v2[X] + b*v2[Z];                           \
        if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
        rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];   \
        if(min>rad || max<-rad) return 0;
#define AXISTEST_Y1(a, b, fa, fb)                          \
        p0 = -a*v0[X] + b*v0[Z];                           \
        p1 = -a*v1[X] + b*v1[Z];                           \
        if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
        rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];   \
        if(min>rad || max<-rad) return 0;
/*======================== Z-tests ========================*/
#define AXISTEST_Z12(a, b, fa, fb)                         \
        p1 = a*v1[X] - b*v1[Y];                            \
        p2 = a*v2[X] - b*v2[Y];                            \
        if(p2<p1) {min=p2; max=p1;} else {min=p1; max=p2;} \
        rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];   \
        if(min>rad || max<-rad) return 0;
#define AXISTEST_Z0(a, b, fa, fb)                          \
        p0 = a*v0[X] - b*v0[Y];                            \
        p1 = a*v1[X] - b*v1[Y];                            \
        if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
        rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];   \
        if(min>rad || max<-rad) return 0;

#undef X
#undef Y
#undef Z