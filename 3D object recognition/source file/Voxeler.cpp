#include "StdAfx.h"
#include "Voxeler.h"
#include "BoundingBox.h"
#include <stack>
//#include <MarchingCubes.h>


Voxeler::Voxeler( TriMesh * src_mesh, double voxel_size, bool verbose /*= false*/ )
{
  /*      this->mesh = new TriMesh(*src_mesh);
        this->voxelSize = voxel_size;
        this->isVerbose = verbose;
        this->isReadyDraw = false;

        if(mesh == NULL)
                return;

        if(isVerbose) printf("Computing voxels..");

       // mesh->assignFaceArray();

        // For each face in mesh

		for(int i=0;i<mesh->faces.size();i++)
        {
			FaceBounds fb = findFaceBounds( mesh->faces[i] );

                for(int x = fb.minX; x <= fb.maxX; x++)
                {
                        for(int y = fb.minY; y <= fb.maxY; y++)
                        {
                                for(int z = fb.minZ; z <= fb.maxZ; z++)
                                {
                                        Voxel v(x,y,z);

                                        if(isVoxelIntersects(v,  mesh->faces[i]) && !kd.has(x,y,z)){
                                                kd.insert3(v.x, v.y, v.z, voxels.size());
                                                voxels.push_back( v );
                                        }
                                }
                        }
                }
        }
        
        if(isVerbose) printf(".voxel count = %d.\n", (int)voxels.size());

        // Inner / outer computation
       // fillInsideOut(innerVoxels, outerVoxels);
		//fillOuter(outerVoxels);
        
        if(isVerbose) printf("done.");
		computeBounds();
		*/
}

void Voxeler::computeBounds()
{
	/*	minVox = Voxel(voxels[0].x, voxels[0].y, voxels[0].z);
        maxVox = Voxel(voxels[0].x, voxels[0].y, voxels[0].z);

        for(int i = 1; i < (int)voxels.size(); i++)
        {
                Voxel v = voxels[i];
				//printf("\n.v.x = %d,   v.y = %d,  v.z = %d,.\n", v.x,v.y,v.z);
				//system("pause");
                minVox.toMin(v);
                maxVox.toMax(v);
        }

		printf("\n.minVox.x = %d,   minVox.y = %d,  minVox.z = %d,.\n", minVox.x,minVox.y,minVox.z);
		*/
}

FaceBounds Voxeler::findFaceBounds( TriMesh::Face f )
{
        FaceBounds fb;

        double minx = 0.0, miny = 0.0, minz = 0.0;
        double maxx = 0.0, maxy = 0.0, maxz = 0.0;

        minx = maxx = mesh->vertices[f[0]][0];
        miny = maxy = mesh->vertices[f[0]][1];
        minz = maxz = mesh->vertices[f[0]][2];

        for(int v = 0; v < 3; v++)
        {
			point p = mesh->vertices[f[v]];

                if (((double)p[0])< minx) minx = ((double)p[0]);
                if (((double)p[0]) > maxx) maxx = ((double)p[0]);

                if (((double)p[1]) < miny) miny = ((double)p[1]);
                if (((double)p[1]) > maxy) maxy = ((double)p[1]);

                if (((double)p[2]) < minz) minz = ((double)p[2]);
                if (((double)p[2]) > maxz) maxz = ((double)p[2]);
        }

		//int mnx=floor(minx / voxelSize);
        fb.minX = (int)floor(minx / voxelSize);
        fb.minY = (int)floor(miny / voxelSize);
        fb.minZ = (int)floor(minz / voxelSize);

        fb.maxX = ceil(maxx / voxelSize);
        fb.maxY = ceil(maxy / voxelSize);
        fb.maxZ = ceil(maxz / voxelSize);
		//printf("\n.v.x = %d,   v.y = %d,  v.z = %d,.\n", fb.minX ,fb.minY, fb.minZ);
		//system("pause");*/
        return fb;
}

bool Voxeler::isVoxelIntersects( const Voxel& v, TriMesh::Face f )
{
      /*  vecd3 center = vecd3(v.x * voxelSize, v.y * voxelSize, v.z * voxelSize);

        double s = voxelSize * 0.5;

        BoundingBox b(center, s,s,s);

       // std::vector<Vec3d> f_vec =  mesh->facePoints(f);
		vecd3 v1=vecd3((double)mesh->vertices[f[0]][0],(double)mesh->vertices[f[0]][1],(double)mesh->vertices[f[0]][2]);
		vecd3 v2=vecd3((double)mesh->vertices[f[1]][0],(double)mesh->vertices[f[1]][1],(double)mesh->vertices[f[1]][2]);
		vecd3 v3=vecd3((double)mesh->vertices[f[2]][0],(double)mesh->vertices[f[2]][1],(double)mesh->vertices[f[2]][2]);
        return b.containsTriangle(v1,v2, v3);*/
        return true;
}

std::vector<Voxel> Voxeler::fillOther()
{
        std::vector<Voxel> filled;

      /*  for(int x = minVox.x - 1; x <= maxVox.x + 1; x++){
                for(int y = minVox.y - 1; y <= maxVox.y + 1; y++){
                        for(int z = minVox.z - 1; z <= maxVox.z + 1; z++){
                                if(!kd.has(x,y,z))
                                        filled.push_back(Voxel(x,y,z));
                        }
                }
        }
		*/
        return filled;
}

void Voxeler::fillInsideOut(KDTree2 & inside, KDTree2 & outside)
{
        /*printf("Computing inside, outside..");

        fillOuter(outside);

        // Compute inner as complement of outside
        for(int x = minVox.x - 1; x <= maxVox.x + 1; x++){
                for(int y = minVox.y - 1; y <= maxVox.y + 1; y++){
                        for(int z = minVox.z - 1; z <= maxVox.z + 1; z++){
                                if(!kd.has(x,y,z) && !outside.has(x,y,z)){
                                        inside.insert3(x,y,z,1);
                                }
                        }
                }
        }*/
}

void Voxeler::fillOuter(KDTree2 & outside)
{
    /*    std::stack<Voxel> stack;

        stack.push(maxVox + Voxel(1,1,1));

        while(!stack.empty())
        {
                // Get next square
                Voxel c = stack.top(); // get current voxel
                stack.pop();

                // Base case:
                if( !kd.has(c.x, c.y, c.z) && !outside.has(c.x, c.y, c.z) )
                {
                        // Otherwise, add it to set of outside voxels
                        outside.insert3(c.x, c.y, c.z, 1);

                        // Visit neighbors
                        if(c.x < maxVox.x + 1) stack.push( c + Voxel( 1, 0, 0) );
                        if(c.y < maxVox.y + 1) stack.push( c + Voxel( 0, 1, 0) );
                        if(c.z < maxVox.z + 1) stack.push( c + Voxel( 0, 0, 1) );

                        if(c.x > minVox.x - 1) stack.push( c + Voxel(-1, 0, 0) );
                        if(c.y > minVox.y - 1) stack.push( c + Voxel( 0,-1, 0) );
                        if(c.z > minVox.z - 1) stack.push( c + Voxel( 0, 0,-1) );
                }
        }*/
}

void Voxeler::saveRepairedMesh(char *inFile){
	/*mesh->need_bbox();
	 typedef OpenMesh::TriMesh_ArrayKernelT<OpenMesh::DefaultTraits> MyMesh;
	 Grid::Vec3 bbmin =Grid::Vec3(mesh->bbox.min[0],mesh->bbox.min[1],mesh->bbox.min[2]);
	 Grid::Vec3 bbmax =Grid::Vec3(mesh->bbox.max[0],mesh->bbox.max[1],mesh->bbox.max[2]);
	 int volres= (int)(2.0/voxelSize);
	
	 ScalarGrid grid( bbmin,
                   Grid::Vec3( (double)(bbmax[0]-bbmin[0]), 0.0f, 0.0f ),
                   Grid::Vec3( 0.0f, (double)(bbmax[1]-bbmin[1]), 0.0f ),
                   Grid::Vec3( 0.0f, 0.0f, (double)(bbmax[2]-bbmin[2]) ),
                   volres, volres, volres );*/
	
	/*	int volres= (int)(2.0/voxelSize);

		MarchingCubes *mc = new MarchingCubes(volres+2,volres+2,volres+2) ;
		mc->init_all() ;

		for(int i=0;i<volres+2;i++)
		 for(int j=0;j<volres+2;j++)
#pragma omp parallel for
			 for(int k=0;k<volres+1;k++){
				mc->set_data(-1.0f,i,j,k);
			 }

		KDTree2  outside;

	   std::stack<Voxel> stack;

        stack.push(maxVox + Voxel(1,1,1));
        while(!stack.empty())
        {
                // Get next square
                Voxel c = stack.top(); // get current voxel
                stack.pop();

                // Base case:
                if( !kd.has(c.x, c.y, c.z) && !outside.has(c.x, c.y, c.z) )
                {
                        // Otherwise, add it to set of outside voxels
                        outside.insert3(c.x, c.y, c.z, 1);
						//printf("\n i = %d, j=%d, k=%d\n",c.x, c.y, c.z);
						mc->set_data(1.0f,c.x,c.y,c.z);

                        // Visit neighbors
                        if(c.x < maxVox.x + 1) stack.push( c + Voxel( 1, 0, 0) );
                        if(c.y < maxVox.y + 1) stack.push( c + Voxel( 0, 1, 0) );
                        if(c.z < maxVox.z + 1) stack.push( c + Voxel( 0, 0, 1) );

                        if(c.x > minVox.x ) stack.push( c + Voxel(-1, 0, 0) );
                        if(c.y > minVox.y ) stack.push( c + Voxel( 0,-1, 0) );
                        if(c.z > minVox.z ) stack.push( c + Voxel( 0, 0,-1) );
                }
        }

	std::cerr << "Extracting the Iso-surface ... ";


	mc->run() ;
	mc->clean_temps() ;

	mc->writePLY(inFile) ;
	mc->clean_all() ;
	delete mc;

	*/
}


std::vector<Voxel> Voxeler::Intersects(Voxeler * other)
{
       std::vector<Voxel> intersection;

       /* Voxeler *minVoxeler = this, *maxVoxeler = other;

        // Swap with minimum
        if(other->voxels.size() < this->voxels.size()){
                minVoxeler = other;
                maxVoxeler = this;
        }

        for(int i = 0; i < (int) minVoxeler->voxels.size(); i++)
        {
                Voxel v = minVoxeler->voxels[i];

                if(maxVoxeler->kd.has(v.x, v.y, v.z))
                        intersection.push_back(v);
        }

        return intersection;*/
        return intersection;
}

std::map<int, Voxel> Voxeler::around(point p)
{
        std::map<int, Voxel> result;

      /*  int x = p[0] / voxelSize;
        int y = p[1] / voxelSize;
        int z = p[2] / voxelSize;

        for(int i = -1; i <= 1; i += 1){
                for(int j = -1; j <= 1; j += 1){
                        for(int k = -1; k <= 1; k += 1){
                                Voxel v(x + i, y + j, z + k);

                                vecd3 vpos(v.x, v.y, v.z);

                                if(kd.has(v.x, v.y, v.z)){
                                        result[kd.getData(&vpos[0])] = v;
                                }
                        }
                }
        }
		*/
        return result;
}

void Voxeler::grow()
{
      /*  int N = (int)voxels.size();

        for(int i = 0; i < N; i++)
        {
                Voxel curVoxel = voxels[i];

                for(int x = -1; x <= 1; x += 1){
                        for(int y = -1; y <= 1; y++){
                                for(int z = -1; z <= 1; z++){
                                        Voxel v(curVoxel.x + x, curVoxel.y + y, curVoxel.z + z);

                                        if(!kd.has(v.x,v.y,v.z))
                                        {
                                                kd.insert3(v.x, v.y, v.z, voxels.size());
                                                voxels.push_back( v );
                                        }
                                }
                        }
                }
        }

        printf("\nVoxler grown from (%d) to (%d).\n", N, (int)voxels.size());

        isReadyDraw = false;*/
}

std::vector< point > Voxeler::getCorners( int vid )
{
        std::vector< point > result;

        //for(int i = 0; i < 8; i++)
          //      result.push_back(corners[cornerIndices[vid][i]]);

        return result;
}

int Voxeler::getClosestVoxel( vecd3 point )
{
       // return cornerCorrespond[ corner_kd.getData(&point[0]) ];
        return -1;
}

int Voxeler::getEnclosingVoxel( vecd3 p )
{
        int N = (int)voxels.size();

        double s = voxelSize * 0.5;

        double minDist = DBL_MAX;
        int closestVoxel = -1;

    /*    for(int i = 0; i < N; i++)
        {
                Voxel curVoxel = voxels[i];

                vecd3 voxelCenter(curVoxel.x * s, curVoxel.y * s, curVoxel.z * s);

                double curDist = len(voxelCenter - p);

                if(curDist < minDist){
                        closestVoxel = i;
                        minDist = curDist;
                }
        }
		*/
        return closestVoxel;
}

int Voxeler::getVoxelIndex( Voxel v )
{
      /*  for(int i = 0; i < (int) voxels.size(); i++)
        {
                Voxel w = voxels[i];

                if(w.x == v.x && w.y == v.y && w.z == v.z)
                        return i;
        }
		*/
        return -1;
}

std::vector< point > Voxeler::getVoxelCenters()
{
        std::vector< point > pnts;

        /*for(int i = 0; i < (int) voxels.size(); i++){
                Voxel w = voxels[i];
                pnts.push_back(point(w.x * voxelSize, w.y* voxelSize, w.z* voxelSize) );
        }
		*/
        return pnts;
}

Voxeler::~Voxeler(void)
{
	corner_kd.clear();
	kd.clear();
	outerVoxels.clear();
	innerVoxels.clear();
	corners.clear();
	cornerIndices.clear();
	cornerCorrespond.clear();
	voxels.clear();
	delete mesh;
	voxels.clear();
}
