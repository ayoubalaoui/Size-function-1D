#pragma once

#ifndef TRIMESH_ALGO_H_
#define TRIMESH_ALGO_H_

#include "Vec.h"
#include "XForm.h"
#include "TriMesh.h"
#include "SizeElement.h"


// Optimally re-triangulate a mesh by doing edge flips
extern void edgeflip(TriMesh *mesh);

// Flip the order of vertices in each face.  Turns the mesh inside out.
extern void faceflip(TriMesh *mesh);

// One iteration of umbrella-operator smoothing
extern void umbrella(TriMesh *mesh, float stepsize);

// Taubin lambda/mu mesh smoothing
extern void lmsmooth(TriMesh *mesh, int niters);

// Remove the indicated vertices from the TriMesh.
extern void remove_vertices(TriMesh *mesh, const std::vector<bool> &toremove);

// Remove vertices that aren't referenced by any face
extern void remove_unused_vertices(TriMesh *mesh);

// Remove faces as indicated by toremove.  Should probably be
// followed by a call to remove_unused_vertices()
extern void remove_faces(TriMesh *mesh, const std::vector<bool> &toremove);

extern void removeNAN(TriMesh *mesh);
// Remove long, skinny faces.  Should probably be followed by a
// call to remove_unused_vertices()
extern void remove_sliver_faces(TriMesh *mesh);

// Remap vertices according to the given table
extern void remap_verts(TriMesh *mesh, const std::vector<int> &remap_table);

// Reorder vertices in a mesh according to the order in which
// they are referenced by the tstrips or faces.
extern void reorder_verts(TriMesh *mesh);

// Perform one iteration of subdivision on a mesh.
enum { SUBDIV_PLANAR, SUBDIV_LOOP, SUBDIV_LOOP_ORIG, SUBDIV_LOOP_NEW,
       SUBDIV_BUTTERFLY, SUBDIV_BUTTERFLY_MODIFIED };
extern void subdiv(TriMesh *mesh, int scheme = SUBDIV_LOOP);

// Smooth the mesh geometry
extern void smooth_mesh(TriMesh *themesh, float sigma);

// Bilateral smoothing
extern void bilateral_smooth_mesh(TriMesh *themesh, float sigma1, float sigma2);

// Diffuse the normals across the mesh
extern void diffuse_normals(TriMesh *themesh, float sigma);

// Diffuse the curvatures across the mesh
extern void diffuse_curv(TriMesh *themesh, float sigma);

// Diffuse the curvature derivatives across the mesh
extern void diffuse_dcurv(TriMesh *themesh, float sigma);

// Given a curvature tensor, find principal directions and curvatures
extern void diagonalize_curv(const vec &old_u, const vec &old_v,
			     float ku, float kuv, float kv,
			     const vec &new_norm,
			     vec &pdir1, vec &pdir2, float &k1, float &k2);

// Reproject a curvature tensor from the basis spanned by old_u and old_v
// (which are assumed to be unit-length and perpendicular) to the
// new_u, new_v basis.
extern void proj_curv(const vec &old_u, const vec &old_v,
		      float old_ku, float old_kuv, float old_kv,
		      const vec &new_u, const vec &new_v,
		      float &new_ku, float &new_kuv, float &new_kv);

// Like the above, but for dcurv
extern void proj_dcurv(const vec &old_u, const vec &old_v,
		       const Vec<4> old_dcurv,
		       const vec &new_u, const vec &new_v,
		       Vec<4> &new_dcurv);

// Create an offset surface from a mesh
extern void inflate(TriMesh *mesh, float amount);

// Transform the mesh by the given matrix
extern void apply_xform(TriMesh *mesh, const xform &xf);

// Translate the mesh
extern void trans(TriMesh *mesh, const vec &transvec);

// Rotate the mesh by r radians
extern void rot(TriMesh *mesh, float r, const vec &axis);

// Rotate the mesh by r radians in a center
extern void rot(TriMesh *mesh, float r,const vec &center, const vec &axis);

// Scale the mesh - isotropic
extern void scale(TriMesh *mesh, float s);

// Scale the mesh - anisotropic in X, Y, Z
extern void scale(TriMesh *mesh, float sx, float sy, float sz);

// Scale the mesh - anisotropic in an arbitrary direction
extern void scale(TriMesh *mesh, float s, const vec &d);

// Clip mesh to the given bounding box
extern void clip(TriMesh *mesh, const TriMesh::BBox &b);

// Find center of mass of a bunch of points
extern point point_center_of_mass(const vector<point> &pts);

// Find (area-weighted) center of mass of a mesh
extern point mesh_center_of_mass(TriMesh *mesh);

// Compute covariance of a bunch of points
extern void point_covariance(const vector<point> &pts, float C[3][3]);

// Compute covariance of faces (area-weighted) in a mesh
extern void mesh_covariance(TriMesh *mesh, float C[3][3]);

// Scale the mesh so that mean squared distance from center of mass is 1
extern void normalize_variance(TriMesh *mesh);

// Rotate model so that first principal axis is along +X (using
// forward weighting), and the second is along +Y
extern void pca_rotate(TriMesh *mesh);

// As above, but only rotate by 90/180/etc. degrees w.r.t. original
extern void pca_snap(TriMesh *mesh);

// Apply PCA normalisation to assure invariance to translation, Scaling and Rotation
extern void pca_Normalisation(TriMesh *mesh);

// Apply PCA normalisation to assure invariance to translation, Scaling and Rotation
extern void pca_Normalisation_vertices(TriMesh *mesh);

extern void pca_curvature_direction(TriMesh *mesh);

extern void directions_curvature_covariance(TriMesh *mesh, float C[3][3]);

// Flip faces so that orientation among touching faces is consistent
extern void orient(TriMesh *mesh);

// Remove boundary vertices (and faces that touch them)
extern void erode(TriMesh *mesh);

// Add a bit of noise to the mesh
extern void noisify(TriMesh *mesh, float amount);

// Get the three PCA's principal axis
extern vec *getPCAAxis(TriMesh *mesh);
// Get the three PCA's principal axis
extern vec *getPCAAxisVertices(TriMesh *mesh);

// Get the three PCA's principal axis according to curvature's directions
extern vec *getPCAAxisCurvature(TriMesh *mesh);

extern float getMaxDistance(point p,TriMesh *mesh);

extern void measureFunction_PCA_AXIS(TriMesh *mesh,vector<SizeElement> &SizeElementsListX,
									 vector<SizeElement> &SizeElementsListY,vector<SizeElement> &SizeElementsListZ);

extern void  measureFunction_PCA_AXIS(TriMesh *mesh,char * fnameX,char * fnameY,char * fnameZ);

extern void saveMeasureFunctionGraph(vector<double> valuesList,vector< vector<int> > neighbors,char *fname);

extern void  measureFunction_MAJOR_8_PLANS(TriMesh *mesh,char * fname);

extern void  measureFunction_MAJOR_6_PLANS(TriMesh *mesh,char * fname);

extern void  measureFunction_MAJOR_4_Regions(TriMesh *mesh,char * fname);

extern void  measureFunction_MAJOR_4_Regions_X(TriMesh *mesh,char * fname);

extern void  measureFunction_MAJOR_4_Regions_Y(TriMesh *mesh,char * fname);

extern void  measureFunction_Bounding_Sphere(TriMesh *mesh,char * fname);

extern void measure_Function_Laplacian_Beltrami_Embedding(TriMesh * mesh, char *in,char *out);

extern void  measureFunction_curvature(TriMesh *mesh,char * fname);

extern void saveSizeElements(vector<SizeElement> SizeElementsList, const string &fname);

extern void size_Function_Sphere(TriMesh * mesh, int order,char *fname);

extern void measure_Function_Biasoti_et_al(TriMesh * mesh, char *in,char *out);

extern void measure_Function_Biasoti_et_al_2008(TriMesh * mesh, char *in,char *out);

extern void measure_Function_Our_geodesic(TriMesh * mesh, char *in,char *out);

extern void measure_Function_Our_Eccentric_Transform(TriMesh * mesh, char *in,char *out);

extern void spectral_embedding_connectivity(TriMesh * mesh);

extern void measure_Function_Our_Example(TriMesh * mesh,char *out);

extern void measure_Function_Our_Example_2(TriMesh * mesh,char *out);

extern void measure_Function_Our_Geodesic_1D(TriMesh * mesh, char *in,char *out);

extern void measure_Function_Our_Heat_diffusion_Distance(TriMesh * mesh, char *in,char *out);

extern void measure_Function_Our_geodesic_3D(TriMesh * mesh, char *in,char *out);

extern void compute_rabin_2010_descriptor(TriMesh * mesh, char *in,char *out);

extern void partSizeFunction(TriMesh *mesh,vector<bool> toProcess,char * fname);

extern void measure_Function_Our_geodesic_fct_1D(TriMesh * mesh, char *in,char *out);

extern void measure_Function_Our_geodesic_FCT_1D_eccentricity(TriMesh * mesh, char *in,char *out);

extern void measure_Function_Our_Geodesic_Eccentricity_2D(TriMesh *mesh, char *in,char *out);

extern void measure_Function_Our_Convex_Concav_Eccentricity_2D(TriMesh * mesh, char *in,char *out);

extern void compute_Geodesic_Eccentricity_descriptor(TriMesh *mesh, char *in,char *out);

extern void measure_Function_Our_geodesic_2D_Mapping(TriMesh *mesh, char *in,char *out);

extern void split_mesh_4_regions_Z(const TriMesh *mesh,char *out);

extern void split_mesh_4_regions_X(const TriMesh *mesh,char *out);

extern void split_mesh_4_regions_Y(const TriMesh *mesh,char *out);

extern void split_mesh_2_regions_X(const TriMesh *mesh,char *out);

extern void split_mesh_2_regions_Y(const TriMesh *mesh,char *out);

extern void split_mesh_2_regions_Z(const TriMesh *mesh,char *out);

extern void selectCompByBiggestSize(TriMesh *mesh);

extern void selectTheTwoBiggestSize(TriMesh *mesh);

extern void select_TheTwoBiggestcomps(TriMesh *in, const vector<int> &comps);

// Align the mesh to a given axis
extern void align(TriMesh *mesh,const vec &axis);

// Inverse Alignment the mesh to an Axis
extern void alignInv(TriMesh *mesh,const vec &axis);


extern void crunch(TriMesh *in, TriMesh *out, float voxelsize);

extern void mesh_check(TriMesh *in);

extern void holes_filling(TriMesh *mesh);

// Find the connected components of TriMesh "in".
// Outputs:
//  comps is a vector that gives a mapping from each face to its
//   associated connected component.
//  compsizes holds the size of each connected component
extern void find_comps(TriMesh *in, vector<int> &comps, vector<int> &compsizes);

// Sorts the connected components from largest to smallest.  Renumbers the
// elements of compsizes to reflect this new numbering.
extern void sort_comps(vector<int> &comps, vector<int> &compsizes);// Sorts the connected components from largest to smallest.  Renumbers the


// Select a particular connected component, and delete all other vertices from
// the mesh.
extern void select_comp(TriMesh *in, const vector<int> &comps, int whichcc);


// Performs the Delaunay Triangulation algorithm.
//extern void perform_Delaunay_Triangulation(TriMesh *in, char *outFile);

extern void Curvatures_decriptor_Ayoub(TriMesh* mesh, const std::string& fileName);
extern void Curvatures_decriptor_moussa(TriMesh* mesh, const std::string& fileName);

extern void Curvatures_decriptor_Ayoub_4_features(TriMesh* mesh, const std::string& fileName);

extern void measure_Function_Our_Geodesic_Eccentricity__generate_Measure_2D(TriMesh* mesh, char* in, char* out);

extern void measure_Function_Our_Geodesic_Eccentricity__generate_Measure_2D_from_1D(TriMesh* mesh, char* in, char* out);

extern void measure_Function_Our_Geodesic_Eccentricity__generate_Measure_1D_from_2D_Gaussian(TriMesh* mesh, char* in, char* out);



void center_and_scale_Unit_Sphere(TriMesh *mesh);

#endif /* TRIMESH_ALGO_H_ */
