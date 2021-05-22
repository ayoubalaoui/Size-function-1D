#pragma once

#include "Vec.h"

struct Voxel{ 
        int x, y, z;
        int flag;

        Voxel(){ x = y = z = flag = 0; }
        Voxel(int X, int Y, int Z) : x(X), y(Y), z(Z){ flag = 1; } 

        // Operators
        operator const vecd3() const{ return vecd3(x,y,z); }

        Voxel & operator+= (const Voxel & other){
                x += other.x;   y += other.y;   z += other.z;
                return *this;
        }

        Voxel operator+(const Voxel & other) const{
                return Voxel(*this) += other;
        }

        // Useful for bounds
        inline void toMax(const Voxel & v){ x = max(x, v.x); y = max(y, v.y); z = max(z, v.z); }
        inline void toMin(const Voxel & v){ x = min(x, v.x); y = min(y, v.y); z = min(z, v.z); }
};

struct FaceBounds { int minX, minY, minZ; int maxX, maxY, maxZ;};