#include "BoundingBox.h"


BoundingBox::BoundingBox()
{
    this->center = vecd3(0, 0, 0);

    this->xExtent = 0;
    this->yExtent = 0;
    this->zExtent = 0;
}

BoundingBox::BoundingBox(const vecd3& c, double x, double y, double z)
{
    this->center = c;

    this->xExtent = x;
    this->yExtent = y;
    this->zExtent = z;

    vecd3 corner(x / 2, y / 2, z / 2);

    vmin = center - corner;
    vmax = center + corner;
}

BoundingBox& BoundingBox::operator=(const BoundingBox& other)
{
    this->center = other.center;

    this->xExtent = other.xExtent;
    this->yExtent = other.yExtent;
    this->zExtent = other.zExtent;

    this->vmax = other.vmax;
    this->vmin = other.vmin;

    return *this;
}

BoundingBox::BoundingBox(const vecd3& fromMin, const vecd3& toMax)
{
    vmin = fromMin;
    vmax = toMax;

    this->center = (vmin + vmax) / 2.0;

    this->xExtent = abs(vmax[0] - center[0]);
    this->yExtent = abs(vmax[1] - center[1]);
    this->zExtent = abs(vmax[2] - center[2]);
}

std::vector<vecd3> BoundingBox::getCorners()
{
    std::vector<vecd3> corners;

    vecd3 x = (vecd3(1, 0, 0) * xExtent);
    vecd3 y = (vecd3(0, 1, 0) * yExtent);
    vecd3 z = (vecd3(0, 0, 1) * zExtent);

    vecd3 c = center + x + y + z;
    corners.push_back(c);
    corners.push_back(c - (2.0 * x));
    corners.push_back(c - (y * 2.0));
    corners.push_back(c - (x * 2.0) - (y * 2.0));

    corners.push_back(corners[0] - (z * 2.0));
    corners.push_back(corners[1] - (z * 2.0));
    corners.push_back(corners[2] - (z * 2.0));
    corners.push_back(corners[3] - (z * 2.0));

    return corners;
}

/* AABB-triangle overlap test code                      */
/* by Tomas Akenine-Möller                              */
bool BoundingBox::containsTriangle(const vecd3& tv0, const vecd3& tv1, const vecd3& tv2) const
{
    vecd3 boxcenter(center);
    vecd3 boxhalfsize(xExtent, yExtent, zExtent);

    int X = 0, Y = 1, Z = 2;

    /*    use separating axis theorem to test overlap between triangle and box */
    /*    need to test for overlap in these directions: */
    /*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
    /*       we do not even need to test these) */
    /*    2) normal of the triangle */
    /*    3) crossproduct(edge from tri, {x,y,z}-directin) */
    /*       this gives 3x3=9 more tests */
    vecd3 v0, v1, v2;
    double min, max, p0, p1, p2, rad, fex, fey, fez;
    vecd3 normal, e0, e1, e2;

    /* This is the fastest branch on Sun */
    /* move everything so that the box center is in (0,0,0) */
    v0 = tv0 - boxcenter;
    v1 = tv1 - boxcenter;
    v2 = tv2 - boxcenter;

    /* compute triangle edges */
    e0 = v1 - v0;      /* tri edge 0 */
    e1 = v2 - v1;      /* tri edge 1 */
    e2 = v0 - v2;      /* tri edge 2 */

    /* Bullet 3:  */
    /*  test the 9 tests first (this was faster) */
    fex = fabsf(e0[X]);
    fey = fabsf(e0[Y]);
    fez = fabsf(e0[Z]);
    AXISTEST_X01(e0[Z], e0[Y], fez, fey);
    AXISTEST_Y02(e0[Z], e0[X], fez, fex);
    AXISTEST_Z12(e0[Y], e0[X], fey, fex);
    fex = fabsf(e1[X]);
    fey = fabsf(e1[Y]);
    fez = fabsf(e1[Z]);
    AXISTEST_X01(e1[Z], e1[Y], fez, fey);
    AXISTEST_Y02(e1[Z], e1[X], fez, fex);
    AXISTEST_Z0(e1[Y], e1[X], fey, fex);
    fex = fabsf(e2[X]);
    fey = fabsf(e2[Y]);
    fez = fabsf(e2[Z]);
    AXISTEST_X2(e2[Z], e2[Y], fez, fey);
    AXISTEST_Y1(e2[Z], e2[X], fez, fex);
    AXISTEST_Z12(e2[Y], e2[X], fey, fex);

    /* Bullet 1: */
    /*  first test overlap in the {x,y,z}-directions */
    /*  find min, max of the triangle each direction, and test for overlap in */
    /*  that direction -- this is equivalent to testing a minimal AABB around */
    /*  the triangle against the AABB */
    /* test in X-direction */
    FINDMINMAX(v0[X], v1[X], v2[X], min, max);
    if (min > boxhalfsize[X] || max < -boxhalfsize[X]) return 0;
    /* test in Y-direction */
    FINDMINMAX(v0[Y], v1[Y], v2[Y], min, max);
    if (min > boxhalfsize[Y] || max < -boxhalfsize[Y]) return 0;
    /* test in Z-direction */
    FINDMINMAX(v0[Z], v1[Z], v2[Z], min, max);
    if (min > boxhalfsize[Z] || max < -boxhalfsize[Z]) return 0;

    /* Bullet 2: */
    /*  test if the box intersects the plane of the triangle */
    /*  compute plane equation of triangle: normal*x+d=0 */
    normal = e0 CROSS e1;

    if (!planeBoxOverlap(normal, v0, boxhalfsize)) return 0;
    return 1;   /* box and triangle overlaps */
}

bool BoundingBox::intersectsBoundingBox(const BoundingBox& bb) const
{
    if (center[0] + xExtent < bb.center[0] - bb.xExtent || center[0] - xExtent > bb.center[0] + bb.xExtent)
        return false;
    else if (center[1] + yExtent < bb.center[1] - bb.yExtent || center[1] - yExtent > bb.center[1] + bb.yExtent)
        return false;
    else if (center[2] + zExtent < bb.center[2] - bb.zExtent || center[2] - zExtent > bb.center[2] + bb.zExtent)
        return false;
    else
        return true;
}
vecd3 BoundingBox::Center()
{
    return center;
}

void BoundingBox::Offset(double s)
{
    Offset(vecd3(s, s, s));
}

void BoundingBox::Offset(vecd3 delta)
{
    *this = BoundingBox(vmin - delta, vmax + delta);
}

double BoundingBox::Diag()
{
    return len(vmin - vmax);
}

BoundingBox::~BoundingBox(void)
{
}
