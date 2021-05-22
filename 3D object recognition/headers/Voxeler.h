#pragma once
#include "TriMesh.h"
#include "KDTree2.h"
#include "Voxel.h"
#include <map>
/*
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/IO/BinaryHelper.hh>
#include <OpenMesh\Core\Mesh\TriMesh_ArrayKernelT.hh>


#include <IsoEx/Grids/ScalarGridT.hh>
#include <IsoEx/Extractors/MarchingCubesT.hh>


using namespace IsoEx;
using namespace OpenMesh;
using namespace OpenMesh::IO;

*/
class Voxeler
{
private:
        TriMesh * mesh;
        KDTree2 kd;
	//	typedef ScalarGridT< float > ScalarGrid;

        // Special voxels
        KDTree2 outerVoxels, innerVoxels;

public:
        Voxeler( TriMesh * src_mesh, double voxel_size, bool verbose = false);
		~Voxeler(void);

        FaceBounds findFaceBounds( TriMesh::Face f );
        bool isVoxelIntersects( const Voxel & v, TriMesh::Face f );
        
        void computeBounds();

        // Grow larger by one voxel
        void grow();

        // Find inside and outside of mesh surface
        std::vector< Voxel > fillOther();
        void fillInsideOut(KDTree2 & inside, KDTree2 & outside);
        void fillOuter(KDTree2 & outside);

		void saveRepairedMesh(char *);

        // Intersection
        std::vector<Voxel> Intersects(Voxeler * other);
        std::map<int, Voxel> around(point p);

        KDTree2 corner_kd;
        std::vector< point > corners;
        std::vector< std::vector<int> > cornerIndices;
        std::vector< int > cornerCorrespond;
        std::vector< point > getCorners(int vid);
        int getClosestVoxel(vecd3 point);
        int getEnclosingVoxel( vecd3 point );

        std::vector< point > getVoxelCenters();

        std::vector< Voxel > voxels;
        int getVoxelIndex(Voxel v);
        unsigned int d1, d2;
        bool isVerbose;
        bool isReadyDraw;

        double voxelSize;

        Voxel minVox;
        Voxel maxVox;
};
