// MeshViewer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include "TriMesh.h"
#include "TriMesh_algo.h"
#include "XForm.h"
#include "GLCamera.h"
#include "ICP.h"
#include "make_mesh.h"
#include <GL/openglut.h>
#include <string>
#include "timestamp.h"
#include <omp.h>

#include <eigen3\Eigen\Dense>
using std::string;


// Globals
vector<TriMesh *> meshes;
vector<xform> xforms;
vector<bool> visible;
vector<string> xffilenames;

TriMesh::BSphere global_bsph;
xform global_xf;
GLCamera camera;

int current_mesh = -1;

bool draw_edges = false;
bool draw_2side = false;
bool draw_shiny = true;
bool draw_lit = true;
bool draw_falsecolor = false;
bool draw_index = false;
bool white_bg = false;


// Make some mesh current
void set_current(int i)
{
	if (i >= 0 && i < meshes.size() && visible[i])
		current_mesh = i;
	else
		current_mesh = -1;
	camera.stopspin();
}


// Change visiblility of a mesh
void toggle_vis(int i)
{
	if (i >= 0 && i < meshes.size())
		visible[i] = !visible[i];
	if (current_mesh == i && !visible[i])
		set_current(-1);
}


// Signal a redraw
void need_redraw()
{
	glutPostRedisplay();
}


// Clear the screen
void cls()
{
	glDisable(GL_DITHER);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_NORMALIZE);
	glDisable(GL_LIGHTING);
	glDisable(GL_NORMALIZE);
	glDisable(GL_COLOR_MATERIAL);
	if (white_bg)
		glClearColor(1, 1, 1, 0);
	else
		glClearColor(0.08, 0.08, 0.08, 0);
	glClearDepth(1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}


// Set up lights and materials
void setup_lighting(int id)
{
	Color c(0.2,0.7,0.36);
	if (draw_falsecolor)
		c = Color::hsv(-3.88 * id, 0.6 + 0.2 * sin(0.42 * id), 1.0);
		//c = Color::hsv(0, 0.6 + 0.2 * sin(0), 1.0);
		//c = Color::red();
	glColor3fv(c);

	if (!draw_lit || meshes[id]->normals.empty()) {
		glDisable(GL_LIGHTING);
		return;
	}

	GLfloat mat_specular[4] = { 0.18, 0.18, 0.18, 0.18 };
	if (!draw_shiny) {
		mat_specular[0] = mat_specular[1] =
		mat_specular[2] = mat_specular[3] = 0.0f;
	}
	GLfloat mat_shininess[] = { 64 };
	GLfloat global_ambient[] = { 0.02, 0.02, 0.05, 0.05 };
	GLfloat light0_ambient[] = { 0, 0, 0, 0 };
	GLfloat light0_diffuse[] = { 0.85, 0.85, 0.8, 0.85 };
	if (current_mesh >= 0 && id != current_mesh) {
		light0_diffuse[0] *= 0.5f;
		light0_diffuse[1] *= 0.5f;
		light0_diffuse[2] *= 0.5f;
	}
	GLfloat light1_diffuse[] = { -0.01, -0.01, -0.03, -0.03 };
	GLfloat light0_specular[] = { 0.85, 0.85, 0.85, 0.85 };
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, draw_2side);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_NORMALIZE);
}


// Draw triangle strips.  They are stored as length followed by values.
void draw_tstrips(const TriMesh *themesh)
{
	const int *t = &themesh->tstrips[0];
	const int *end = t + themesh->tstrips.size();
	while (likely(t < end)) {
		int striplen = *t++;
		glDrawElements(GL_TRIANGLE_STRIP, striplen, GL_UNSIGNED_INT, t);
		t += striplen;
	}
}


// Draw the mesh
void draw_mesh(int i)
{
	const TriMesh *themesh = meshes[i];

	glPushMatrix();
	glMultMatrixd(xforms[i]);
	//glPushMatrix();
	//glScalef(0.5f,0.5f,0.5f);

	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);

	if (draw_2side) {
		glDisable(GL_CULL_FACE);
	} else {
		glCullFace(GL_BACK);
		glEnable(GL_CULL_FACE);
	}

	// Vertices
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT,
			sizeof(themesh->vertices[0]),
			&themesh->vertices[0][0]);

	// Normals
	if (!themesh->normals.empty() && !draw_index) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT,
				sizeof(themesh->normals[0]),
				&themesh->normals[0][0]);
	} else {
		glDisableClientState(GL_NORMAL_ARRAY);
	}

	// Colors
	if (!themesh->colors.empty() && !draw_falsecolor && !draw_index) {
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(3, GL_FLOAT,
			       sizeof(themesh->colors[0]),
			       &themesh->colors[0][0]);
	} else {
		glDisableClientState(GL_COLOR_ARRAY);
	}

	// Main drawing pass
	if (themesh->tstrips.empty()) {
		// No triangles - draw as points
		glPointSize(1);
		glDrawArrays(GL_POINTS, 0, themesh->vertices.size());
		glPopMatrix();
		return;
	}

	if (draw_edges) {
		glPolygonOffset(10.0f, 10.0f);
		glEnable(GL_POLYGON_OFFSET_FILL);
	}

	draw_tstrips(themesh);
	glDisable(GL_POLYGON_OFFSET_FILL);

	// Edge drawing pass
	if (draw_edges) {
		glPolygonMode(GL_FRONT, GL_LINE);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisable(GL_COLOR_MATERIAL);
		GLfloat global_ambient[] = { 0.2, 0.2, 0.2, 1.0 };
		GLfloat light0_diffuse[] = { 0.8, 0.8, 0.8, 0.0 };
		GLfloat light1_diffuse[] = { -0.2, -0.2, -0.2, 0.0 };
		GLfloat light0_specular[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
		glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
		GLfloat mat_diffuse[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mat_diffuse);
		glColor3f(0, 0, 1); // Used iff unlit
		draw_tstrips(themesh);
		glPolygonMode(GL_FRONT, GL_LINE);
	}
	//glutWireSphere(1.0, 32, 16);
	glPopMatrix();
}


// Draw the scene
void redraw()
{
	timestamp t = now();
	camera.setupGL(global_xf * global_bsph.center, global_bsph.r);
	//point center(0.0f,0.0f,0.0f);
	//camera.setupGL(center,50.0f);
	glPushMatrix();
	glMultMatrixd(global_xf);
	cls();
	for (int i = 0; i < meshes.size(); i++) {
		if (!visible[i])
			continue;
		setup_lighting(i);
		draw_mesh(i);
		/*glPushMatrix();
		glMultMatrixd(xforms[i]);
		glutWireSphere(1.0, 32, 16);
		glPopMatrix();*/

	}

	glPopMatrix();

	//glTranslatef (0.0, 0.0, 0.0);
	//glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient_color);
	//glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	//glMaterialfv(GL_FRONT, GL_SPECULAR, no_mat);
	//glMaterialfv(GL_FRONT, GL_SHININESS, no_shininess);
	//glMaterialfv(GL_FRONT, GL_EMISSION, mat_emission);
	//glutWireSphere(1.0, 32, 16);
	glutSwapBuffers();
	printf("\r                        \r%.1f msec.", 1000.0f * (now() - t));
	fflush(stdout);
}


// Update global bounding sphere.
void update_bsph()
{
	point boxmin(1e38, 1e38, 1e38);
	point boxmax(-1e38, -1e38, -1e38);
	bool some_vis = false;
	for (int i = 0; i < meshes.size(); i++) {
		if (!visible[i])
			continue;
		some_vis = true;
		point c = xforms[i] * meshes[i]->bsphere.center;
		float r = meshes[i]->bsphere.r;
		for (int j = 0; j < 3; j++) {
			boxmin[j] = min(boxmin[j], c[j]-r);
			boxmax[j] = max(boxmax[j], c[j]+r);
		}
	}
	if (!some_vis)
		return;
	point &gc = global_bsph.center;
	float &gr = global_bsph.r;
	gc = 0.5f * (boxmin + boxmax);
	gr = 0.0f;
	for (int i = 0; i < meshes.size(); i++) {
		if (!visible[i])
			continue;
		point c = xforms[i] * meshes[i]->bsphere.center;
		float r = meshes[i]->bsphere.r;
		gr = max(gr, dist(c, gc) + r);
	}
}


// Set the view...
void resetview()
{
	camera.stopspin();
	for (int i = 0; i < meshes.size(); i++)
		if (!xforms[i].read(xffilenames[i]))
			xforms[i] = xform();
	update_bsph();
	global_xf = xform::trans(0, 0, -5.0f * global_bsph.r) *
		    xform::trans(-global_bsph.center);

	// Special case for 1 mesh
	if (meshes.size() == 1 && xforms[0].read(xffilenames[0])) {
		global_xf = xforms[0];
		xforms[0] = xform();
	}
}


// Save the current image to a PPM file.
// Uses the next available filename matching filenamepattern
void dump_image()
{
	// Find first non-used filename
	const char filenamepattern[] = "D:\\img1.ppm";
	int imgnum = 0;
	FILE *f;
	while (1) {
		//char filename[1024];
		//sprintf(filename, filenamepattern, imgnum++);
		f = fopen(filenamepattern, "rb");
		if (!f) {
			f = fopen(filenamepattern, "wb");
			printf("\n\nSaving image %s... ", filenamepattern);
			fflush(stdout);
			break;
		}
		fclose(f);
	}

	// Read pixels
	GLint V[4];
	glGetIntegerv(GL_VIEWPORT, V);
	GLint width = V[2], height = V[3];
	char *buf = new char[width*height*3];
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(V[0], V[1], width, height, GL_RGB, GL_UNSIGNED_BYTE, buf);

	// Flip top-to-bottom
	for (int i = 0; i < height/2; i++) {
		char *row1 = buf + 3 * width * i;
		char *row2 = buf + 3 * width * (height - 1 - i);
		for (int j = 0; j < 3 * width; j++)
			swap(row1[j], row2[j]);
	}

	// Write out file
	fprintf(f, "P6\n#\n%d %d\n255\n", width, height);
	fwrite(buf, width*height*3, 1, f);
	fclose(f);
	delete [] buf;

	printf("Done.\n\n");
}


// Save all transforms
void save_xforms()
{
	if (xforms.size() == 1) {
		printf("Writing %s\n", xffilenames[0].c_str());
		global_xf.write(xffilenames[0]);
		return;
	}
	for (int i = 0; i < xforms.size(); i++) {
		printf("Writing %s\n", xffilenames[i].c_str());
		xforms[i].write(xffilenames[i]);
	}
}


// ICP
void do_icp(int n)
{
	camera.stopspin();

	if (current_mesh < 0 || current_mesh >= meshes.size())
		return;
	if (n < 0 || n >= meshes.size())
		return;
	if (!visible[n] || !visible[current_mesh] || n == current_mesh)
		return;
	ICP(meshes[n], meshes[current_mesh], xforms[n], xforms[current_mesh], 2);
	update_bsph();
	need_redraw();
}


// Handle mouse button and motion events
static unsigned buttonstate = 0;

void doubleclick(int button, int x, int y)
{
	// Render and read back ID reference image
	camera.setupGL(global_xf * global_bsph.center, global_bsph.r);
	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glClearColor(1,1,1,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	draw_index = true;
	glPushMatrix();
	glMultMatrixd(global_xf);

	for (int i = 0; i < meshes.size(); i++) {
		if (!visible[i])
			continue;
		glColor3ub((i >> 16) & 0xff,
			   (i >> 8)  & 0xff,
			    i        & 0xff);
		draw_mesh(i);
	}
	glPopMatrix();
	draw_index = false;
	GLint V[4];
	glGetIntegerv(GL_VIEWPORT, V);
	y = int(V[1] + V[3]) - 1 - y;
	unsigned char pix[3];
	glReadPixels(x, y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pix);
	int n = (pix[0] << 16) + (pix[1] << 8) + pix[2];

	if (button == 0 || buttonstate == (1 << 0)) {
		// Double left click - select a mesh
		set_current(n);
	} else if (button == 2 || buttonstate == (1 << 2)) {
		// Double right click - ICP current to clicked-on
		do_icp(n);
	}
}

void mousemotionfunc(int x, int y)
{
	static const Mouse::button physical_to_logical_map[] = {
		Mouse::NONE, Mouse::ROTATE, Mouse::MOVEXY, Mouse::MOVEZ,
		Mouse::MOVEZ, Mouse::MOVEXY, Mouse::MOVEXY, Mouse::MOVEXY,
	};

	Mouse::button b = Mouse::NONE;
	if (buttonstate & (1 << 3))
		b = Mouse::WHEELUP;
	else if (buttonstate & (1 << 4))
		b = Mouse::WHEELDOWN;
	else if (buttonstate & (1 << 30))
		b = Mouse::LIGHT;
	else
		b = physical_to_logical_map[buttonstate & 7];

	if (current_mesh < 0) {
		camera.mouse(x, y, b,
			     global_xf * global_bsph.center, global_bsph.r,
			     global_xf);
	} else {
		xform tmp_xf = global_xf * xforms[current_mesh];
		camera.mouse(x, y, b,
			     tmp_xf * meshes[current_mesh]->bsphere.center,
			     meshes[current_mesh]->bsphere.r,
			     tmp_xf);
		xforms[current_mesh] = inv(global_xf) * tmp_xf;
		update_bsph();
	}
	if (b != Mouse::NONE)
		need_redraw();
}

void mousebuttonfunc(int button, int state, int x, int y)
{
	static timestamp last_click_time;
	static unsigned last_click_buttonstate = 0;
	static float doubleclick_threshold = 0.25f;

	if (glutGetModifiers() & GLUT_ACTIVE_CTRL)
		buttonstate |= (1 << 30);
	else
		buttonstate &= ~(1 << 30);

	if (state == GLUT_DOWN) {
		buttonstate |= (1 << button);
		if (buttonstate == last_click_buttonstate &&
		    now() - last_click_time < doubleclick_threshold) {
			doubleclick(button, x, y);
			last_click_buttonstate = 0;
		} else {
			last_click_time = now();
			last_click_buttonstate = buttonstate;
		}
	} else {
		buttonstate &= ~(1 << button);
	}

	mousemotionfunc(x, y);
}


// Idle callback
void idle()
{
	xform tmp_xf = global_xf;
	if (current_mesh >= 0)
		tmp_xf = global_xf * xforms[current_mesh];

	if (camera.autospin(tmp_xf))
		need_redraw();
	else
		usleep(10000);

	if (current_mesh >= 0) {
		xforms[current_mesh] = inv(global_xf) * tmp_xf;
		update_bsph();
	} else {
		global_xf = tmp_xf;
	}
}


// Keyboard
#define Ctrl (1-'a')
void keyboardfunc(unsigned char key, int x, int y)
{
	switch (key) {
		case ' ':
			if (current_mesh < 0)
				resetview();
			else
				set_current(-1);
			break;
		case '@': // Shift-2
			draw_2side = !draw_2side; break;
		case 'e':
			draw_edges = !draw_edges; break;
		case 'f':
			draw_falsecolor = !draw_falsecolor; break;
		case 'l':
			draw_lit = !draw_lit; break;
		case 's':
			draw_shiny = !draw_shiny; break;
		case 'w':
			white_bg = !white_bg; break;
		case 'I':
			dump_image(); break;
		case Ctrl+'x':
			save_xforms();
			break;
		case '\033': // Esc
		case Ctrl+'q':
		case 'Q':
		case 'q':
			exit(0);
		default:
			if (key >= '1' && key <= '9') {
				int m = key - '1';
				toggle_vis(m);
			}
	}
	need_redraw();
}


void usage(const char *myname)
{
	fprintf(stderr, "Usage: %s infile...\n", myname);
	exit(1);
}
void colorbynormals(TriMesh *mesh)
{
	mesh->need_normals();
	int nv = mesh->vertices.size();
	mesh->colors.resize(nv);
	for (int i = 0; i < nv; i++) {
		mesh->colors[i] = Color(0.5f, 0.5f, 0.5f) +
			0.5f * mesh->normals[i];
	}
}


// Apply a solid color to the mesh
void solidcolor(TriMesh *mesh, const char *col)
{
	unsigned c;
	sscanf(col, "%x", &c);
	int r = (c >> 16) & 0xff;
	int g = (c >> 8)  & 0xff;
	int b =  c        & 0xff;
	Color cc = Color(r,g,b);
	int nv = mesh->vertices.size();
	mesh->colors.resize(nv);
	for (int i = 0; i < nv; i++)
		mesh->colors[i] = cc;
}
/*void glutWireSphere(GLdouble radius, GLint slices, GLint stacks)
{
	GLUquadricObj *quadObj = gluNewQuadric();
	gluQuadricDrawStyle(quadObj, GLU_LINE);
	gluQuadricNormals(quadObj, GLU_SMOOTH);
	gluSphere(quadObj, radius, slices, stacks);
}*/
void init_mesh(TriMesh *themesh){
	themesh->need_normals();
	themesh->need_tstrips();
	themesh->need_across_edge();
	themesh->need_adjacentfaces();
	themesh->need_bbox();
	
	clip(themesh,themesh->bbox);
	themesh->need_bsphere();
	center_and_scale_Unit_Sphere(themesh);

}
void verify(){
		vector<int> modelsList;
	modelsList.clear();
	/*modelsList.push_back(1);
	//Part I
	for(int i=3;i<13;i++)
		modelsList.push_back(i);

	for(int i=14;i<16;i++)
		modelsList.push_back(i);

	for(int i=17;i<20;i++)
		modelsList.push_back(i);

	for(int i=21;i<23;i++)
		modelsList.push_back(i);

	for(int i=25;i<40;i++)
		modelsList.push_back(i);

	for(int i=42;i<45;i++)
		modelsList.push_back(i);

	for(int i=3862;i<3896;i++)
		modelsList.push_back(i);

	for(int i=3897;i<4143;i++)
		modelsList.push_back(i);

	for(int i=4144;i<4285;i++)
		modelsList.push_back(i);

	for(int i=4286;i<4289;i++)
		modelsList.push_back(i);
		modelsList.push_back(4290);

	for(int i=4292;i<4400;i++)
		modelsList.push_back(i);
	for(int i=4401;i<4943;i++)
		modelsList.push_back(i);

	for(int i=4944;i<5041;i++)
		modelsList.push_back(i);

	for(int i=5042;i<5240;i++)
		modelsList.push_back(i);

	for(int i=5241;i<5306;i++)
		modelsList.push_back(i);

	for(int i=5307;i<5472;i++)
		modelsList.push_back(i);

	for(int i=5474;i<5789;i++)
		modelsList.push_back(i);

	for(int i=7000;i<7306;i++)
		modelsList.push_back(i);
	
	// Part II
	for(int i=743;i<854;i++)
		modelsList.push_back(i);
		
	for(int i=875;i<990;i++)
		modelsList.push_back(i);
	*/
	for(int i=1300;i<1565;i++)
		modelsList.push_back(i);

	for(int i=0;i<modelsList.size();i++){
		int j = modelsList[i];
		//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.off",j, j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.ply",j, j);
;		char *mname = new char[1024],*fname= new char[1024];
		char *commandLine= new char[1024];
		char *command= new char[1024];
		//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.off",j, j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.ply",j, j);
		sprintf(mname, "C:\\cygwin\\Benchmark\\%d\\m%d.ply",j,j);
		sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d.vri",j,j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.off", j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.ply",j, j);
		FILE *f=fopen(mname,"r");
		if(f!=NULL) continue;
		else if(f==NULL){
			printf("\nthe file %s doesn't exist\n",mname);
			system("pause");
		}

		//TriMesh *mesh = TriMesh::read(mname);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//orient(mesh);
		//orient(mesh);
		//edgeflip(mesh);
		//mesh->need_tstrips();
		//reorder_verts(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//erode(mesh);
		//holes_filling(mesh);
		//remove_unused_vertices(mesh);
		//selectCompByBiggestSize(mesh);
		//remove_unused_vertices(mesh);

		//erode(mesh);
		/*remove_unused_vertices(mesh);
		pca_Normalisation(mesh);*/
		//selectCompByBiggestSize(mesh);
		//pca_Normalisation(mesh);
		//getPCAAxisVertices(mesh);
		//perform_Delaunay_Triangulation(mesh,fname);
		//spectral_embedding_connectivity(mesh);
		//mesh->write(fname);
		printf("\n%s verifie avec succès",fname);
		delete command;
		delete commandLine;
		delete fname;
		delete mname;

		//delete mesh;
	}

}
void ply2vri(){

		vector<int> modelsList;
	modelsList.clear();
/*	modelsList.push_back(1);
	//Part I
	for(int i=3;i<13;i++)
		modelsList.push_back(i);

	for(int i=14;i<16;i++)
		modelsList.push_back(i);

	for(int i=17;i<20;i++)
		modelsList.push_back(i);

	for(int i=21;i<23;i++)
		modelsList.push_back(i);

	for(int i=25;i<40;i++)
		modelsList.push_back(i);

	for(int i=42;i<45;i++)
		modelsList.push_back(i);

	for(int i=3862;i<3896;i++)
		modelsList.push_back(i);

	for(int i=3897;i<4143;i++)
		modelsList.push_back(i);

	for(int i=4144;i<4285;i++)
		modelsList.push_back(i);

	for(int i=4286;i<4289;i++)
		modelsList.push_back(i);
		modelsList.push_back(4290);

	for(int i=4292;i<4400;i++)
		modelsList.push_back(i);
	for(int i=4401;i<4943;i++)
		modelsList.push_back(i);

	for(int i=4944;i<5041;i++)
		modelsList.push_back(i);

	for(int i=5042;i<5240;i++)
		modelsList.push_back(i);

	for(int i=5241;i<5306;i++)
		modelsList.push_back(i);

	for(int i=5307;i<5472;i++)
		modelsList.push_back(i);

	for(int i=5474;i<5789;i++)
		modelsList.push_back(i);

	for(int i=7000;i<7306;i++)
		modelsList.push_back(i);
	
	// Part II
	for(int i=743;i<854;i++)
		modelsList.push_back(i);
		
	for(int i=875;i<1074;i++)
		modelsList.push_back(i);
	for(int i=1074;i<1101;i++)
		modelsList.push_back(i);		
		
	for(int i=1102;i<1300;i++)
		modelsList.push_back(i);
		
	for(int i=1300;i<1565;i++)
		modelsList.push_back(i);
	*/
	for(int i=1565;i<1762;i++)
		modelsList.push_back(i);


	omp_set_num_threads(6);
#pragma omp parallel for
	for(int i=0;i<modelsList.size();i++){
		int j = modelsList[i];
		//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.off",j, j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.ply",j, j);
;		char *mname = new char[1024],*fname= new char[1024];
		char *commandLine= new char[1024];
		char *command= new char[1024];
		//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.off",j, j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.ply",j, j);
		sprintf(mname, "C:\\cygwin\\Benchmark\\%d\\m%d.ply",j,j);
		sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d.vri",j,j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.off", j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.ply",j, j);
		FILE *f=fopen(fname,"r");
		if(f!=NULL) {
			fclose(f);
			printf("\n\nHello %s\n\n",fname);
			continue;
		
		}
		//fclose(f);
		sprintf(command,"C:\\cygwin\\home\\alam1702\\ply2vri\\ply2vri.exe");
		sprintf(commandLine,"%s %s %s", command,mname,fname);
		//TriMesh *mesh = TriMesh::read(mname);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//orient(mesh);
		//orient(mesh);
		//edgeflip(mesh);
		//mesh->need_tstrips();
		//reorder_verts(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//erode(mesh);
		//holes_filling(mesh);
		//remove_unused_vertices(mesh);
		//selectCompByBiggestSize(mesh);
		//remove_unused_vertices(mesh);

		//erode(mesh);
		/*remove_unused_vertices(mesh);
		pca_Normalisation(mesh);*/
		//selectCompByBiggestSize(mesh);
		//pca_Normalisation(mesh);
		//getPCAAxisVertices(mesh);
		//perform_Delaunay_Triangulation(mesh,fname);
		//spectral_embedding_connectivity(mesh);
		//mesh->write(fname);
		system(commandLine);
		printf("\n%s Enregistré avec succès",mname);
		delete command;
		delete commandLine;
		delete fname;
		delete mname;

		//delete mesh;
	}

}
void test_mesh_repair(){
	char *mname = new char[1024],*fname= new char[1024];
		char *commandLine= new char[1024];
		char *command= new char[1024];
		//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.off",j, j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.ply",j, j);
		sprintf(mname, "C:\\cygwin\\home\\alam1702\\PolyMender_testmodels\\bunny.ply");
		sprintf(fname, "C:\\cygwin\\home\\alam1702\\PolyMender_testmodels\\bunnyClosed.ply");
		sprintf(command,"C:\\cygwin\\home\\alam1702\\PolyMender\\PolyMender-qd.exe");
		sprintf(commandLine,"%s %s 7 0.9 %s", command,mname,fname);
				system(commandLine);
		printf("\n%s Enregistré avec succès",mname);

		delete command;
		delete commandLine;
		delete fname;
		delete mname;

}
void mesh_repair_(){

	vector<int> modelsList;
	modelsList.clear();

	
	modelsList.push_back(1);

	//Part I
	for(int i=3;i<13;i++)
		modelsList.push_back(i);

	for(int i=14;i<16;i++)
		modelsList.push_back(i);

	for(int i=17;i<20;i++)
		modelsList.push_back(i);

	for(int i=21;i<23;i++)
		modelsList.push_back(i);
	
	for(int i=25;i<40;i++)
		modelsList.push_back(i);

	for(int i=42;i<45;i++)
		modelsList.push_back(i);
		
	for(int i=3862;i<3896;i++)
		modelsList.push_back(i);

	for(int i=3897;i<4143;i++)
		modelsList.push_back(i);

	for(int i=4144;i<4285;i++)
		modelsList.push_back(i);

	for(int i=4286;i<4289;i++)
		modelsList.push_back(i);
		modelsList.push_back(4290);

	for(int i=4292;i<4400;i++)
		modelsList.push_back(i);
	for(int i=4401;i<4943;i++)
		modelsList.push_back(i);

	for(int i=4944;i<5041;i++)
		modelsList.push_back(i);

	for(int i=5042;i<5240;i++)
		modelsList.push_back(i);

	for(int i=5241;i<5306;i++)
		modelsList.push_back(i);

	for(int i=5307;i<5472;i++)
		modelsList.push_back(i);

	for(int i=5474;i<5789;i++)
		modelsList.push_back(i);

	for(int i=7000;i<7306;i++)
		modelsList.push_back(i);
		
	
	// Part II
	for(int i=743;i<875;i++)
		modelsList.push_back(i);
	for(int i=557;i<726;i++)
		modelsList.push_back(i);
		
	for(int i=875;i<1074;i++)
		modelsList.push_back(i);
	for(int i=1074;i<1101;i++)
		modelsList.push_back(i);		
		
	for(int i=1102;i<1300;i++)
		modelsList.push_back(i);
		
	for(int i=1300;i<1565;i++)
		modelsList.push_back(i);
	
	for(int i=1565;i<1613;i++)
		modelsList.push_back(i);
	for(int i=1614;i<1633;i++)
		modelsList.push_back(i);
	for(int i=1634;i<1762;i++)
		modelsList.push_back(i);
	
	//*/
	//omp_set_num_threads(8);
//#pragma omp parallel for
	for(int i=0;i<modelsList.size();i++){
		int j = modelsList[i];
		//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.off",j, j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.ply",j, j);
;		char *mname = new char[1024],*fname= new char[1024];
		char *commandLine= new char[1024];
		char *command= new char[1024];
		//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.off",j, j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.ply",j, j);
		sprintf(mname, "C:\\cygwin\\Benchmark\\%d\\m%d.off",j,j);
		sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d_fixed.ply",j,j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.off", j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.ply",j, j);
		/*FILE *f=fopen(fname,"r");
		if(f!=NULL) {
			fclose(f);
			printf("\n\nHello %s\n\n",fname);
			continue;
		
		}*/
		//fclose(f);
		sprintf(command,"C:\\cygwin\\MeshFix_v1.0\\MeshFix\\meshfix.exe");
		sprintf(commandLine,"%s %s", command,mname);
		//sprintf(commandLine,"%s --in %s --out %s --maxDepth 9 --curvature 0.99 --fullCaseTable", command,mname,fname);
		//TriMesh *mesh = TriMesh::read(mname);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//orient(mesh);
		//orient(mesh);
		//edgeflip(mesh);
		//mesh->need_tstrips();
		//reorder_verts(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//erode(mesh);
		//holes_filling(mesh);
		//remove_unused_vertices(mesh);
		//selectCompByBiggestSize(mesh);
		//remove_unused_vertices(mesh);

		//erode(mesh);
		/*remove_unused_vertices(mesh);
		pca_Normalisation(mesh);*/
		//selectCompByBiggestSize(mesh);
		//pca_Normalisation(mesh);
		//getPCAAxisVertices(mesh);
		//perform_Delaunay_Triangulation(mesh,fname);
		//spectral_embedding_connectivity(mesh);
		//mesh->write(fname);
		system(commandLine);
		printf("\n%s Enregistré avec succès",mname);
		delete command;
		delete commandLine;
		delete fname;
		delete mname;

		//delete mesh;
	}

}
void generatePlyFile(){
	//omp_set_num_threads(6);

	int j=0;
#pragma omp parallel for
	for(int i=0;i<457;i++){
		j++;
		char mname[1024],fname[1024];
		//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.off",j, j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.ply",j, j);
		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000.ply",i,i);
		sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_7000_fixed.ply",i+100,i+100);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.off", j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.ply",j, j);
		
		TriMesh *mesh = TriMesh::read(mname);
		//erode(mesh);
		remove_unused_vertices(mesh);
		//orient(mesh);
		//orient(mesh);
		//edgeflip(mesh);
		//mesh->need_tstrips();
		//reorder_verts(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//erode(mesh);
		//holes_filling(mesh);
		//remove_unused_vertices(mesh);
		//selectCompByBiggestSize(mesh);
		//remove_unused_vertices(mesh);

		//erode(mesh);
		/*remove_unused_vertices(mesh);
		pca_Normalisation(mesh);*/
		//selectCompByBiggestSize(mesh);
		//pca_Normalisation(mesh);
		//getPCAAxisVertices(mesh);
		//perform_Delaunay_Triangulation(mesh,fname);
		//spectral_embedding_connectivity(mesh);
		mesh->write(fname);
		printf("\n%s Enregistré avec succès",fname);
		delete mesh;
	}
	/*for(int i=30;i<31;i++){
		int j=0+i;
		char mname[1024],fname[1024];
		//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_deci_3000.ply", j,j);
		sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_deci_3000.off",j, j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.off", j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.ply",j, j);
		
		TriMesh *mesh = TriMesh::read(mname);
		//erode(mesh);
		remove_unused_vertices(mesh);
		//orient(mesh);
		//edgeflip(mesh);
		//mesh->need_tstrips();
		//reorder_verts(mesh);
		//erode(mesh);
		remove_unused_vertices(mesh);
		//erode(mesh);
		holes_filling(mesh);
		remove_unused_vertices(mesh);
		selectCompByBiggestSize(mesh);
		//erode(mesh);
		remove_unused_vertices(mesh);
		pca_Normalisation(mesh);
		//selectCompByBiggestSize(mesh);
		//pca_Normalisation(mesh);
		//getPCAAxisVertices(mesh);
		//perform_Delaunay_Triangulation(mesh,fname);
		//spectral_embedding_connectivity(mesh);
		mesh->write(fname);
		printf("\n%s Enregistré avec succès",fname);
		delete mesh;
	}*/
}

void generatePlyFile_2(){
	//omp_set_num_threads(6);

	int j=1730;
//#pragma omp parallel for
	for(int i=340;i<371;i++){
		char mname[1024],fname[1024];
		//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.off",j, j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.ply",j, j);
		j++;
		sprintf(mname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\3\\m%d\\m%d.off",i,i);
		sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d.ply",j,j);
		
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.off", j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.ply",j, j);
		
		TriMesh *mesh = TriMesh::read(mname);
		//erode(mesh);
		/*remove_unused_vertices(mesh);
		//orient(mesh);
		//orient(mesh);
		//edgeflip(mesh);
		//mesh->need_tstrips();
		//reorder_verts(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//erode(mesh);
		/*holes_filling(mesh);

		remove_unused_vertices(mesh);
		selectCompByBiggestSize(mesh);

		remove_unused_vertices(mesh);

		//erode(mesh);
		/*remove_unused_vertices(mesh);
		pca_Normalisation(mesh);*/
		//selectCompByBiggestSize(mesh);
		//pca_Normalisation(mesh);
		//getPCAAxisVertices(mesh);
		//perform_Delaunay_Triangulation(mesh,fname);
		//spectral_embedding_connectivity(mesh);
		mesh->write(fname);
		printf("\n%s Enregistré avec succès",fname);
		delete mesh;
	//	j++;

	}
	/*for(int i=30;i<31;i++){
		int j=0+i;
		char mname[1024],fname[1024];
		//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_deci_3000.ply", j,j);
		sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_deci_3000.off",j, j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.off", j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.ply",j, j);
		
		TriMesh *mesh = TriMesh::read(mname);
		//erode(mesh);
		remove_unused_vertices(mesh);
		//orient(mesh);
		//edgeflip(mesh);
		//mesh->need_tstrips();
		//reorder_verts(mesh);
		//erode(mesh);
		remove_unused_vertices(mesh);
		//erode(mesh);
		holes_filling(mesh);
		remove_unused_vertices(mesh);
		selectCompByBiggestSize(mesh);
		//erode(mesh);
		remove_unused_vertices(mesh);
		pca_Normalisation(mesh);
		//selectCompByBiggestSize(mesh);
		//pca_Normalisation(mesh);
		//getPCAAxisVertices(mesh);
		//perform_Delaunay_Triangulation(mesh,fname);
		//spectral_embedding_connectivity(mesh);
		mesh->write(fname);
		printf("\n%s Enregistré avec succès",fname);
		delete mesh;
	}*/
}
void generatePlyFile_3(){
	//omp_set_num_threads(6);
	vector<int> modelsList;
	modelsList.clear();
	modelsList.push_back(1);
	for(int i=3;i<13;i++)
		modelsList.push_back(i);

	for(int i=14;i<16;i++)
		modelsList.push_back(i);

	for(int i=17;i<20;i++)
		modelsList.push_back(i);

	for(int i=21;i<23;i++)
		modelsList.push_back(i);

	for(int i=25;i<40;i++)
		modelsList.push_back(i);

	for(int i=42;i<45;i++)
		modelsList.push_back(i);

	for(int i=3862;i<3896;i++)
		modelsList.push_back(i);

	for(int i=3897;i<4400;i++)
		modelsList.push_back(i);

	for(int i=4401;i<4143;i++)
		modelsList.push_back(i);
	for(int i=4144;i<4285;i++)
		modelsList.push_back(i);

	for(int i=4286;i<4289;i++)
		modelsList.push_back(i);
		modelsList.push_back(4290);

	for(int i=4292;i<4400;i++)
		modelsList.push_back(i);
	for(int i=4401;i<4943;i++)
		modelsList.push_back(i);

	for(int i=4944;i<5041;i++)
		modelsList.push_back(i);

	for(int i=5042;i<5240;i++)
		modelsList.push_back(i);

	for(int i=5241;i<5306;i++)
		modelsList.push_back(i);

	for(int i=5307;i<5472;i++)
		modelsList.push_back(i);

	for(int i=5474;i<5789;i++)
		modelsList.push_back(i);

	for(int i=7000;i<7306;i++)
		modelsList.push_back(i);


	//int j=0;
//#pragma omp parallel for
	for(int i=0;i<modelsList.size();i++){
		int j = modelsList[i];
		char mname[1024],fname[1024];
		//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.off",j, j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.ply",j, j);
		sprintf(mname, "C:\\Dossier Ayoub\\3D\\Benchmark\\Benchmark\\%d\\%d.obj",j,j);
		sprintf(fname, "C:\\Dossier Ayoub\\3D\\Benchmark\\Benchmark\\%d\\m%d.ply",j,j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.off", j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.ply",j, j);
		
		TriMesh *mesh = TriMesh::read(mname);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//orient(mesh);
		//orient(mesh);
		//edgeflip(mesh);
		//mesh->need_tstrips();
		//reorder_verts(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//erode(mesh);
		//holes_filling(mesh);

//		remove_unused_vertices(mesh);
	//	selectCompByBiggestSize(mesh);

		//remove_unused_vertices(mesh);

		//erode(mesh);
		//selectCompByBiggestSize(mesh);
		//pca_Normalisation(mesh);
		//getPCAAxisVertices(mesh);
		//perform_Delaunay_Triangulation(mesh,fname);
		//spectral_embedding_connectivity(mesh);
		mesh->write(fname);
		printf("\n%s Enregistré avec succès",fname);
		delete mesh;
	}
	/*for(int i=30;i<31;i++){
		int j=0+i;
		char mname[1024],fname[1024];
		//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_deci_3000.ply", j,j);
		sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_deci_3000.off",j, j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.off", j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.ply",j, j);
		
		TriMesh *mesh = TriMesh::read(mname);
		//erode(mesh);
		remove_unused_vertices(mesh);
		//orient(mesh);
		//edgeflip(mesh);
		//mesh->need_tstrips();
		//reorder_verts(mesh);
		//erode(mesh);
		remove_unused_vertices(mesh);
		//erode(mesh);
		holes_filling(mesh);
		remove_unused_vertices(mesh);
		selectCompByBiggestSize(mesh);
		//erode(mesh);
		remove_unused_vertices(mesh);
		pca_Normalisation(mesh);
		//selectCompByBiggestSize(mesh);
		//pca_Normalisation(mesh);
		//getPCAAxisVertices(mesh);
		//perform_Delaunay_Triangulation(mesh,fname);
		//spectral_embedding_connectivity(mesh);
		mesh->write(fname);
		printf("\n%s Enregistré avec succès",fname);
		delete mesh;
	}*/
}


void saveVerticesAndNormals(vector<point> vertices, vector<vec> normals,char *outFile){
	FILE *f = fopen(outFile,"w");
	for(int i=0;i<vertices.size();i++){
		fprintf(f,"%f\t%f\t%f\t%f\t%f\t%f\n",vertices[i][0],vertices[i][1],vertices[i][2],normals[i][0],normals[i][1],normals[i][2]);
	}
	fclose(f);
	printf("\n%s successfully saved\n",outFile);

}

#include "Voxeler.h"

void volumetric_mesh_repairing(){
	vector<int> modelsList;
	modelsList.clear();
	modelsList.push_back(1);
	//Part I
	/*for(int i=3;i<13;i++)
		modelsList.push_back(i);*/
	for(int k=0;k<modelsList.size();k++){
		int j = modelsList[k];
		char *mname = new char[1024],*fname=new char[1024];
		//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.off",j, j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.ply",j, j);
		sprintf(mname, "C:\\cygwin\\Benchmark\\%d\\m%d.ply",j,j);
		sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d_recons_128.ply",j,j);

	
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.off", j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.ply",j, j);

		TriMesh *mesh = TriMesh::read(mname);
		//pca_Normalisation(mesh);
		mesh->need_bbox();
		double val = len(mesh->bbox.max-mesh->bbox.min);
		trans(mesh,-mesh->bbox.min);

		Voxeler *voxeler = new Voxeler(mesh,val/128.0,true);
		voxeler->saveRepairedMesh(fname);
		delete voxeler;
		//mesh->write(fname);
		printf("\n%s Enregistré avec succès",fname);
		delete mesh;
		delete fname;
		delete mname;
	}


}

bool belongsTo(int k){
	std::vector<int> modelsList;
	modelsList.clear();
	modelsList.push_back(5664);
		for(int i=5445;i<5453;i++)
			modelsList.push_back(i);

		modelsList.push_back(5720);
		modelsList.push_back(5367);
		modelsList.push_back(4104);
		modelsList.push_back(5720);
		modelsList.push_back(4906);
		modelsList.push_back(36);
		modelsList.push_back(37);
		modelsList.push_back(42);//
		modelsList.push_back(44);
		modelsList.push_back(10);
		modelsList.push_back(11);//
		modelsList.push_back(12);
		modelsList.push_back(14);//
		modelsList.push_back(15);//
		modelsList.push_back(21);//
		modelsList.push_back(22);//
		modelsList.push_back(30);//
		modelsList.push_back(39);
		modelsList.push_back(4);//
		modelsList.push_back(6);//

		/*
		modelsList.push_back(5);
		modelsList.push_back(3985);
		modelsList.push_back(4002);
		modelsList.push_back(4065);
		modelsList.push_back(4087);
		modelsList.push_back(954);
		modelsList.push_back(1169);
		modelsList.push_back(1208);
		modelsList.push_back(1461);
		modelsList.push_back(1472);
		modelsList.push_back(1507);
		modelsList.push_back(1520);
		modelsList.push_back(1524);
		modelsList.push_back(1531);
		modelsList.push_back(1547);
		modelsList.push_back(1553);
		modelsList.push_back(1554);
		modelsList.push_back(1607);
		modelsList.push_back(1618);
		modelsList.push_back(1637);
		modelsList.push_back(1638);
		modelsList.push_back(1658);
		modelsList.push_back(4213);
		modelsList.push_back(4215);
		modelsList.push_back(4216);
		modelsList.push_back(4308);
		modelsList.push_back(4341);
		modelsList.push_back(4462);
		modelsList.push_back(4465);
		modelsList.push_back(4468);
		modelsList.push_back(4471);
		modelsList.push_back(4482);
		modelsList.push_back(4483);
		modelsList.push_back(4488);
		modelsList.push_back(4673);
		*/
		for(int i=0;i<modelsList.size();i++)
			if(modelsList[i]==k)
				return true;
	return false;
}
void mesh_repair(){

	vector<int> modelsList;
	modelsList.clear();
	
	for(int i=726;i<743;i++)
		modelsList.push_back(i);

	/*
	modelsList.push_back(1);

	//Part I
	for(int i=3;i<13;i++)
		modelsList.push_back(i);

	for(int i=14;i<16;i++)
		modelsList.push_back(i);

	for(int i=17;i<20;i++)
		modelsList.push_back(i);

	for(int i=21;i<23;i++)
		modelsList.push_back(i);
	
	for(int i=25;i<40;i++)
		modelsList.push_back(i);

	for(int i=42;i<45;i++)
		modelsList.push_back(i);
		
	for(int i=3862;i<3896;i++)
		modelsList.push_back(i);

	for(int i=3897;i<4143;i++)
		modelsList.push_back(i);

	for(int i=4144;i<4285;i++)
		modelsList.push_back(i);

	for(int i=4286;i<4289;i++)
		modelsList.push_back(i);
		modelsList.push_back(4290);

	for(int i=4292;i<4400;i++)
		modelsList.push_back(i);
	for(int i=4401;i<4943;i++)
		modelsList.push_back(i);

	for(int i=4944;i<5041;i++)
		modelsList.push_back(i);

	for(int i=5042;i<5240;i++)
		modelsList.push_back(i);

	for(int i=5241;i<5306;i++)
		modelsList.push_back(i);

	for(int i=5307;i<5472;i++)
		modelsList.push_back(i);

	for(int i=5474;i<5789;i++)
		modelsList.push_back(i);

	for(int i=7000;i<7306;i++)
		modelsList.push_back(i);
		
	
	// Part II
	for(int i=743;i<875;i++)
		modelsList.push_back(i);
	for(int i=557;i<726;i++)
		modelsList.push_back(i);
		
	for(int i=875;i<1074;i++)
		modelsList.push_back(i);
	for(int i=1074;i<1101;i++)
		modelsList.push_back(i);		
		
	for(int i=1102;i<1300;i++)
		modelsList.push_back(i);
		
	for(int i=1300;i<1565;i++)
		modelsList.push_back(i);
	
	for(int i=1565;i<1613;i++)
		modelsList.push_back(i);
	for(int i=1614;i<1633;i++)
		modelsList.push_back(i);
	for(int i=1634;i<1762;i++)
		modelsList.push_back(i);
	
	//*/
	//omp_set_num_threads(8);
//#pragma omp parallel for
	for(int i=0;i<modelsList.size();i++){
		int j = modelsList[i];
		if(!belongsTo(j)){
		//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.off",j, j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.ply",j, j);
;		char *mname = new char[1024],*fname= new char[1024];
		char *commandLine= new char[1024];
		char *command= new char[1024];
		//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.off",j, j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.ply",j, j);
		sprintf(mname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_7000.ply",j,j);
		sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_7000_fixed.off",j,j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.off", j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.ply",j, j);
		/*FILE *f=fopen(fname,"r");
		if(f!=NULL) {
			fclose(f);
			printf("\n\nHello %s\n\n",fname);
			continue;
		
		}*/
		//fclose(f);
		sprintf(command,"C:\\cygwin\\MeshFix_v1.0\\MeshFix\\meshfix.exe");
		sprintf(commandLine,"%s %s -n", command,mname);
		//sprintf(commandLine,"%s --in %s --out %s --maxDepth 9 --curvature 0.99 --fullCaseTable", command,mname,fname);
		//TriMesh *mesh = TriMesh::read(mname);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//orient(mesh);
		//orient(mesh);
		//edgeflip(mesh);
		//mesh->need_tstrips();
		//reorder_verts(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//erode(mesh);
		//holes_filling(mesh);
		//remove_unused_vertices(mesh);
		//selectCompByBiggestSize(mesh);
		//remove_unused_vertices(mesh);

		//erode(mesh);
		/*remove_unused_vertices(mesh);
		pca_Normalisation(mesh);*/
		//selectCompByBiggestSize(mesh);
		//pca_Normalisation(mesh);
		//getPCAAxisVertices(mesh);
		//perform_Delaunay_Triangulation(mesh,fname);
		//spectral_embedding_connectivity(mesh);
		//mesh->write(fname);
		system(commandLine);
		TriMesh *mesh = TriMesh::read(fname);
		sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_7000_fixed.ply",j,j);
		mesh->write(fname);

		printf("\n%s Enregistré avec succès",mname);
		delete command;
		delete commandLine;
		delete fname;
		delete mname;
		}
		//delete mesh;
	}

}


void saveModelsIndices(){
		//omp_set_num_threads(6);
	vector<int> modelsList;
	modelsList.clear();
		modelsList.push_back(1);

	//Part I
	for(int i=3;i<13;i++)
		modelsList.push_back(i);

	for(int i=14;i<16;i++)
		modelsList.push_back(i);

	for(int i=17;i<20;i++)
		modelsList.push_back(i);

	for(int i=21;i<23;i++)
		modelsList.push_back(i);
	
	for(int i=25;i<40;i++)
		modelsList.push_back(i);

	for(int i=42;i<45;i++)
		modelsList.push_back(i);
		
	for(int i=3862;i<3896;i++)
		modelsList.push_back(i);

	for(int i=3897;i<4143;i++)
		modelsList.push_back(i);

	for(int i=4144;i<4285;i++)
		modelsList.push_back(i);

	for(int i=4286;i<4289;i++)
		modelsList.push_back(i);
		modelsList.push_back(4290);

	for(int i=4292;i<4400;i++)
		modelsList.push_back(i);
	for(int i=4401;i<4943;i++)
		modelsList.push_back(i);

	for(int i=4944;i<5041;i++)
		modelsList.push_back(i);

	for(int i=5042;i<5240;i++)
		modelsList.push_back(i);

	for(int i=5241;i<5306;i++)
		modelsList.push_back(i);

	for(int i=5307;i<5472;i++)
		modelsList.push_back(i);

	for(int i=5474;i<5789;i++)
		modelsList.push_back(i);

	for(int i=7000;i<7306;i++)
		modelsList.push_back(i);
	
	for(int i=875;i<892;i++)
		modelsList.push_back(i);
	for(int i=961;i<990;i++)
		modelsList.push_back(i);
	for(int i=557;i<621;i++)
		modelsList.push_back(i);
	for(int i=658;i<668;i++)
		modelsList.push_back(i);
	for(int i=668;i<722;i++)
		modelsList.push_back(i);
	for(int i=743;i<761;i++)
		modelsList.push_back(i);
	for(int i=796;i<854;i++)
		modelsList.push_back(i);
	for(int i=864;i<875;i++)
		modelsList.push_back(i);

	for(int i=100;i<402;i++)
		modelsList.push_back(i);

	for(int i=425;i<535;i++)
		modelsList.push_back(i);

	std::sort(modelsList.begin(),modelsList.end());

	FILE *f = fopen("C:\\cygwin\\modelsIndices.txt","w");
	int j=0;
	for(int i=0;i<modelsList.size();i++){
		if(!belongsTo(modelsList[i])){
			fprintf(f,"%d\n",modelsList[i]);
			j++;
		}
	}
	fclose(f);
	printf("\nModels indices successfully saved %d models \n", j);

}
void readIndices(std::vector<int> &modelsList){
	FILE *f = fopen("C:\\cygwin\\modelsIndices.txt","r");
		
	while(!feof(f)){
		int x;
		fscanf(f,"%d\n",&x);
		modelsList.push_back(x);
	}
	fclose(f);
	printf("\nModels indices successfully read\n");

}

void generatePlyFile_4(){
	//omp_set_num_threads(6);
	vector<int> modelsList;
	modelsList.clear();
	/*	modelsList.push_back(1);

	//Part I
	for(int i=3;i<13;i++)
		modelsList.push_back(i);

	for(int i=14;i<16;i++)
		modelsList.push_back(i);

	for(int i=17;i<20;i++)
		modelsList.push_back(i);

	for(int i=21;i<23;i++)
		modelsList.push_back(i);
	
	for(int i=25;i<40;i++)
		modelsList.push_back(i);

	for(int i=42;i<45;i++)
		modelsList.push_back(i);
		
	for(int i=3862;i<3896;i++)
		modelsList.push_back(i);

	for(int i=3897;i<4143;i++)
		modelsList.push_back(i);

	for(int i=4144;i<4285;i++)
		modelsList.push_back(i);

	for(int i=4286;i<4289;i++)
		modelsList.push_back(i);
		modelsList.push_back(4290);

	for(int i=4292;i<4400;i++)
		modelsList.push_back(i);
	for(int i=4401;i<4943;i++)
		modelsList.push_back(i);

	for(int i=4944;i<5041;i++)
		modelsList.push_back(i);

	for(int i=5042;i<5240;i++)
		modelsList.push_back(i);

	for(int i=5241;i<5306;i++)
		modelsList.push_back(i);

	for(int i=5307;i<5472;i++)
		modelsList.push_back(i);

	for(int i=5474;i<5789;i++)
		modelsList.push_back(i);

	for(int i=7000;i<7306;i++)
		modelsList.push_back(i);
	
	for(int i=875;i<911;i++)
		modelsList.push_back(i);
	for(int i=961;i<990;i++)
		modelsList.push_back(i);
	for(int i=557;i<621;i++)
		modelsList.push_back(i);
	for(int i=658;i<668;i++)
		modelsList.push_back(i);
	for(int i=668;i<722;i++)
		modelsList.push_back(i);
	modelsList.push_back(721);
	for(int i=743;i<761;i++)
		modelsList.push_back(i);
	for(int i=796;i<854;i++)
		modelsList.push_back(i);
	for(int i=864;i<875;i++)
		modelsList.push_back(i);
	*/
	for(int i=100;i<557;i++)
		modelsList.push_back(i);

//#pragma omp parallel for
	for(int k=0;k<modelsList.size();k++){
		int j = modelsList[k];
		if(!belongsTo(j)){

			char mname[1024],fname[1024], vname[1024];
			//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
			//sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.off",j, j);
			//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.ply",j, j);
			sprintf(mname, "C:\\cygwin\\Benchmark\\%d\\m%d_decim_openMesh_500.off",j,j);
			sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d_decim_openMesh_500.off",j,j);

			//sprintf(mname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.off", j,j);
			//sprintf(fname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.ply",j, j);

			TriMesh *mesh = TriMesh::read(mname);
			//subdiv(mesh);
			//subdiv(mesh);
			//subdiv(mesh);
			//orient(mesh);
			//erode(mesh);
			//remove_unused_vertices(mesh);
			//edgeflip(mesh);
			//remove_unused_vertices(mesh);
			//orient(mesh);
		//	mesh->faces.clear();
			//mesh->need_normals();
		//	saveVerticesAndNormals(mesh->vertices,mesh->normals,fname);
			//orient(mesh);
			//edgeflip(mesh);
			//mesh->need_tstrips();
			//reorder_verts(mesh);
			erode(mesh);
			//remove_unused_vertices(mesh);
			selectCompByBiggestSize(mesh);

		holes_filling(mesh);
		//					selectCompByBiggestSize(mesh);

			//	erode(mesh);
		orient(mesh);
			remove_unused_vertices(mesh);
			//for(int i=0;i<2;i++)
				//subdiv(mesh);
			//subdiv(mesh,SUBDIV_BUTTERFLY);

	//		remove_unused_vertices(mesh);
			//selectCompByBiggestSize(mesh);
			//erode(mesh);
				//	selectCompByBiggestSize(mesh);

		//remove_unused_vertices(mesh);

			//erode(mesh);
			//selectCompByBiggestSize(mesh);
			//pca_Normalisation(mesh);
			//getPCAAxisVertices(mesh);
			//perform_Delaunay_Triangulation(mesh,fname);
			//spectral_embedding_connectivity(mesh);
			mesh->write(fname);
			printf("\n%s Enregistré avec succès",fname);
			delete mesh;
		}
	}
	/*for(int i=30;i<31;i++){
		int j=0+i;
		char mname[1024],fname[1024];
		//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_deci_3000.ply", j,j);
		sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_deci_3000.off",j, j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.off", j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.ply",j, j);
		
		TriMesh *mesh = TriMesh::read(mname);
		//erode(mesh);
		remove_unused_vertices(mesh);
		//orient(mesh);
		//edgeflip(mesh);
		//mesh->need_tstrips();
		//reorder_verts(mesh);
		//erode(mesh);
		remove_unused_vertices(mesh);
		//erode(mesh);
		holes_filling(mesh);
		remove_unused_vertices(mesh);
		selectCompByBiggestSize(mesh);
		//erode(mesh);
		remove_unused_vertices(mesh);
		pca_Normalisation(mesh);
		//selectCompByBiggestSize(mesh);
		//pca_Normalisation(mesh);
		//getPCAAxisVertices(mesh);
		//perform_Delaunay_Triangulation(mesh,fname);
		//spectral_embedding_connectivity(mesh);
		mesh->write(fname);
		printf("\n%s Enregistré avec succès",fname);
		delete mesh;
	}*/
	
}
void f(){
//omp_set_num_threads(4);
//#pragma omp parallel for
	for(int i=0;i<100;i++){
		int j=1100+i;
		char *mname = new char[1024],*fname1 = new char[1024],*fname2=new char[1024], *fname3=new char[1024];

		//char *fnamecurv1 = new char[1024],*fnamecurv2 = new char[1024],*fnameLaplace = new char[1024];


		sprintf(mname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.ply",j, j);

		sprintf(fname1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_1.sf", j,j,j);
		sprintf(fname2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		sprintf(fname3, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		delete fname1;
		delete fname2;
		delete fname3;
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete mesh;
	}
}

void generate_regions(){
#pragma omp parallel for
	for(int i=38;i<39;i++){
		int j=0+i;
		char *mname = new char[1024],*fnameX = new char[1024], *fnameY = new char[1024],*fnameZ= new char[1024],*fname3 = new char[1024];

		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_50000_embed.off",j, j);

		sprintf(fnameX, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_50000_embed_2_REGIONS_X\\m%d_Region_mesh", j,j,j);
		sprintf(fnameY, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_50000_embed_2_REGIONS_Y\\m%d_Region_mesh", j,j,j);
		sprintf(fnameZ, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_50000_embed_2_REGIONS_Z\\m%d_Region_mesh", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		pca_Normalisation(mesh);

		split_mesh_2_regions_X(mesh,fnameX);
		split_mesh_2_regions_Y(mesh,fnameY);
		split_mesh_2_regions_Z(mesh,fnameZ);

		
		delete fnameX;
		delete fnameY;
		delete fnameZ;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname3;
		delete mname;

		delete mesh;
	}
}
void generateMeasureFunctions_2(){

		char *mname = new char[1024],*fnameX = new char[1024], *fnameY = new char[1024],*fnameZ= new char[1024],*fname = new char[1024];

		sprintf(mname, "C:\\Dossier Ayoub\\3D\\models_Robert W. Sumner\\models\\cat-poses\\cat-reference_50000.ply");

		sprintf(fname, "C:\\Dossier Ayoub\\3D\\models_Robert W. Sumner\\models\\cat-poses\\cat-reference_Measure_Function_6_Plans\\cat-reference_Measure_Function");
		sprintf(fnameX, "C:\\Dossier Ayoub\\3D\\models_Robert W. Sumner\\models\\cat-poses\\cat-reference_Measure_Function_4_REGIONS_X\\cat-reference_Measure_Function");
		sprintf(fnameY, "C:\\Dossier Ayoub\\3D\\models_Robert W. Sumner\\models\\cat-poses\\cat-reference_Measure_Function_4_REGIONS_Y\\cat-reference_Measure_Function");
		sprintf(fnameZ, "C:\\Dossier Ayoub\\3D\\models_Robert W. Sumner\\models\\cat-poses\\cat-reference_Measure_Function_4_REGIONS_Z\\cat-reference_Measure_Function");
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		//pca_Normalisation(mesh);

		measureFunction_MAJOR_6_PLANS(mesh,fname);
		measureFunction_MAJOR_4_Regions(mesh,fnameZ);
		measureFunction_MAJOR_4_Regions_X(mesh,fnameX);
		measureFunction_MAJOR_4_Regions_Y(mesh,fnameY);

		delete fnameX;
		delete fnameY;
		delete fnameZ;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mname;

		delete mesh;
}
void generateLocalMeasureFunctionsPartialMatching(){
	
#pragma omp parallel for
	for(int i=0;i<275;i++){
		int j=i;
		char *mname = new char[1024],*fname = new char[1024], *fnameX = new char[1024],*fnameY= new char[1024],*fnameZ = new char[1024];



		sprintf(mname, "C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset\\m%d\\m%d_deci_2000_ISOMAP.off",j, j);

		sprintf(fname, "C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset\\m%d\\m%d_deci_2000_New_Measure_Function_6_Plans\\m%d_Measure_Function", j,j,j);
		sprintf(fnameX, "C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset\\m%d\\m%d_deci_2000_New_Measure_Function_4_REGIONS_X\\m%d_Measure_Function", j,j,j);
		sprintf(fnameY, "C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset\\m%d\\m%d_deci_2000_New_Measure_Function_4_REGIONS_Y\\m%d_Measure_Function", j,j,j);
		sprintf(fnameZ, "C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset\\m%d\\m%d_deci_2000_New_Measure_Function_4_REGIONS_Z\\m%d_Measure_Function", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);

		pca_Normalisation(mesh);
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		measureFunction_MAJOR_6_PLANS(mesh,fname);
		measureFunction_MAJOR_4_Regions(mesh,fnameZ);
		measureFunction_MAJOR_4_Regions_X(mesh,fnameX);
		measureFunction_MAJOR_4_Regions_Y(mesh,fnameY);
	
		delete fnameX;
		delete fnameY;
		delete fnameZ;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}






	


}
void generateMeasureFunctions(){
//omp_set_num_threads(5);
	/*
#pragma omp parallel for
	for(int i=350;i<457;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *fname1 = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_75000_embed.off",j, j);

		sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_75000_New_Measure_Function_6_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/
	/*
		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		pca_Normalisation(mesh);
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
/*		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		measureFunction_MAJOR_6_PLANS(mesh,fname);
		delete fname1;
		delete fname2;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
	/*	delete fname;
		delete mesh;
	}
*/
	vector<int> modelsList;
	modelsList.clear();
	readIndices(modelsList);
	vector<float> timeCalcul(modelsList.size());
#pragma omp parallel for
	for(int i=0;i<modelsList.size();i++){
		int j=modelsList[i];
		char *mname = new char[1024],*fname = new char[1024], *fnameX = new char[1024],*fnameY= new char[1024],*fnameZ = new char[1024];



		sprintf(mname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_embed.off",j, j);

		sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_embed_Measure_Function_6_Plans\\m%d_Measure_Function", j,j,j);
		sprintf(fnameX, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_embed_Measure_Function_4_REGIONS_X\\m%d_Measure_Function", j,j,j);
		sprintf(fnameY, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_embed_Measure_Function_4_REGIONS_Y\\m%d_Measure_Function", j,j,j);
		sprintf(fnameZ, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_embed_Measure_Function_4_REGIONS_Z\\m%d_Measure_Function", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		timestamp t1 = now();

		pca_Normalisation(mesh);
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		measureFunction_MAJOR_6_PLANS(mesh,fname);
		measureFunction_MAJOR_4_Regions(mesh,fnameZ);
		measureFunction_MAJOR_4_Regions_X(mesh,fnameX);
		measureFunction_MAJOR_4_Regions_Y(mesh,fnameY);
		timestamp t2 = now();
		timeCalcul[i] = t2-t1;
		delete fnameX;
		delete fnameY;
		delete fnameZ;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}






	float sum = 0.0f;
	for(int i=0;i<timeCalcul.size();i++)
		sum +=timeCalcul[i];
	printf("\n\nLe temps de calcul de Biasotti et al. est:   %f \n",sum/((float)timeCalcul.size())); 





}
void measure_biasoti(){
	//omp_set_num_threads(5);
	vector<int> modelsList;
	modelsList.clear();
	readIndices(modelsList);
	std::sort(modelsList.begin(),modelsList.end());
	vector<float> timeCalcul(modelsList.size());
#pragma omp parallel for
	for(int i=0;i<600;i++){
		int j=modelsList[i];
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\cygwin\\Benchmark\\%d\\m%d_decim_openMesh_1000.off",j, j);

		sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_embed_Measure_Functions_Biasoti_el_al_2008\\m%d_Measure_Function", j,j,j);
		//sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_geodesic_distances_matrix_mesh.txt", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		orient(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		pca_Normalisation(mesh);
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		timestamp t1 = now();
		measure_Function_Biasoti_et_al_2008(mesh,in,fname);
		timestamp t2 = now();
		timeCalcul[i] = t2-t1;
		delete in;
		delete fname2;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}
#pragma omp parallel for
	for(int i=600;i<1500;i++){
		int j=modelsList[i];
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\cygwin\\Benchmark\\%d\\m%d_decim_openMesh_1000.off",j, j);

		sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_embed_Measure_Functions_Biasoti_el_al_2008\\m%d_Measure_Function", j,j,j);
		//sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_geodesic_distances_matrix_mesh.txt", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		pca_Normalisation(mesh);
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		timestamp t1 = now();
		measure_Function_Biasoti_et_al_2008(mesh,in,fname);
		timestamp t2 = now();
		timeCalcul[i] = t2-t1;
		delete in;
		delete fname2;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}
#pragma omp parallel for
	for(int i=1500;i<2000;i++){
		int j=modelsList[i];
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\cygwin\\Benchmark\\%d\\m%d_decim_openMesh_1000.off",j, j);

		sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_embed_Measure_Functions_Biasoti_el_al_2008\\m%d_Measure_Function", j,j,j);
		//sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_geodesic_distances_matrix_mesh.txt", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		pca_Normalisation(mesh);
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		timestamp t1 = now();
		measure_Function_Biasoti_et_al_2008(mesh,in,fname);
		timestamp t2 = now();
		timeCalcul[i] = t2-t1;
		delete in;
		delete fname2;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}
#pragma omp parallel for
	for(int i=2000;i<modelsList.size();i++){
		int j=modelsList[i];
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\cygwin\\Benchmark\\%d\\m%d_decim_openMesh_1000.off",j, j);

		sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_embed_Measure_Functions_Biasoti_el_al_2008\\m%d_Measure_Function", j,j,j);
		//sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_geodesic_distances_matrix_mesh.txt", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		pca_Normalisation(mesh);
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		timestamp t1 = now();
		measure_Function_Biasoti_et_al_2008(mesh,in,fname);
		timestamp t2 = now();
		timeCalcul[i] = t2-t1;
		delete in;
		delete fname2;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}



	float sum = 0.0f;
	for(int i=0;i<457;i++)
		sum +=timeCalcul[i];
	printf("\n\nLe temps de calcul de Biasotti et al. est:   %f \n",sum/((float)modelsList.size())); 
}
void measure_biasoti_Rodola(){
	//omp_set_num_threads(5);
#pragma omp parallel for
	for(int i=0;i<275;i++){
		int j=i;
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset\\m%d\\m%d_deci_2000.ply",j, j);

		sprintf(fname, "C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset\\m%d\\m%d_deci_2000_New_Measure_Function_Biasotti_et_al_2008\\m%d_Measure_Function", j,j,j);
		//sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_geodesic_distances_matrix_mesh.txt", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		orient(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		pca_Normalisation(mesh);
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		timestamp t1 = now();
		measure_Function_Biasoti_et_al_2008(mesh,in,fname);
		timestamp t2 = now();

		delete in;
		delete fname2;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}
}


#include "descriptors_3D_3D.h"

void generate__moments(){
//#pragma omp parallel for
	for(int i=393;i<457;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *out = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000.off",j, j);

		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000_New_Measure_Function_Our_Geodesic_3D\\m%d_Measure_Function", j,j,j);
		sprintf(out, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_geometric_moments.txt", j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		pca_Normalisation(mesh);
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		//measure_Function_Our_geodesic_3D(mesh,in,fname);
		GeometricMomentDescriptor(mesh,5,out);
		delete out;
		delete fname2;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}
}

void generate__rabin_2010_descriptor(){
		vector<float> timeCalcul(457);
#pragma omp parallel for
	for(int i=0;i<457;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *out = new char[1024],*in= new char[1024],*fname3 = new char[1024];

		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_700_v2.ply",j, j);

		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000_New_Measure_Function_Our_Geodesic_3D\\m%d_Measure_Function", j,j,j);
		sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_geodesic_distances_matrix_mesh_700_v2.txt", j,j);
		sprintf(out, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_rabin_2010_descriptor.txt", j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		//pca_Normalisation(mesh);
		timestamp t1 = now();
		compute_rabin_2010_descriptor(mesh,in,out);
		timestamp t2 = now();
		timeCalcul[i]= t2-t1;
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		//measure_Function_Our_geodesic_3D(mesh,in,fname);
		//GeometricMomentDescriptor(mesh,5,out);
		delete out;
		delete in;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}
	float sum = 0.0f;
	for(int i=0;i<457;i++)
		sum +=timeCalcul[i];
	printf("\n\nLe temps de calcul de Ion et al. est:   %f \n",sum/457.0f); 

}
void generate__Ion_descriptor_2008_Rodola(){
	vector<float> timeCalcul(275);
	omp_set_num_threads(4);
#pragma omp parallel for
	for(int i=0;i<275;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *out = new char[1024],*in= new char[1024],*fname3 = new char[1024];

		sprintf(mname, "C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset\\m%d\\m%d_deci_2000.ply",j, j);

		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000_New_Measure_Function_Our_Geodesic_3D\\m%d_Measure_Function", j,j,j);
		sprintf(in, "C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset\\m%d\\m%d_geodesic_distances_matrix_mesh_2000.txt", j,j);
		sprintf(out, "C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset\\m%d\\m%d_Ion_2008_descriptor.txt", j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		//pca_Normalisation(mesh);
		timestamp t1= now();
		compute_Geodesic_Eccentricity_descriptor(mesh,in,out);
		timestamp t2 = now();
		timeCalcul[i] = t2-t1;
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		//measure_Function_Our_geodesic_3D(mesh,in,fname);
		//GeometricMomentDescriptor(mesh,5,out);
		delete out;
		delete in;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}
	float sum = 0.0f;
	for(int i=0;i<275;i++)
		sum +=timeCalcul[i];
	printf("\n\nLe temps de calcul de Ion et al. est:   %f \n",sum/275.0f); 

}
void generate__Projected_area_descriptor_2012_Rodola(){
	vector<float> timeCalcul(275);
	omp_set_num_threads(4);
#pragma omp parallel for
	for(int i=0;i<275;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *out = new char[1024],*in= new char[1024],*fname3 = new char[1024];

		sprintf(mname, "C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset\\m%d\\m%d_deci_2000.ply",j, j);

		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000_New_Measure_Function_Our_Geodesic_3D\\m%d_Measure_Function", j,j,j);
		sprintf(in, "C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset\\m%d\\m%d_geodesic_distances_matrix_mesh_2000.txt", j,j);
		sprintf(out, "C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset\\m%d\\m%d_Ion_2008_descriptor.txt", j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		//pca_Normalisation(mesh);
		timestamp t1= now();
		compute_Geodesic_Eccentricity_descriptor(mesh,in,out);
		timestamp t2 = now();
		timeCalcul[i] = t2-t1;
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		//measure_Function_Our_geodesic_3D(mesh,in,fname);
		//GeometricMomentDescriptor(mesh,5,out);
		delete out;
		delete in;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}
	float sum = 0.0f;
	for(int i=0;i<275;i++)
		sum +=timeCalcul[i];
	printf("\n\nLe temps de calcul de Ion et al. est:   %f \n",sum/275.0f); 

}

void generate__Ion_descriptor_2008(){
	vector<float> timeCalcul(457);
#pragma omp parallel for
	for(int i=0;i<457;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *out = new char[1024],*in= new char[1024],*fname3 = new char[1024];

		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000.off",j, j);

		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000_New_Measure_Function_Our_Geodesic_3D\\m%d_Measure_Function", j,j,j);
		sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_geodesic_distances_matrix_mesh_3000.txt", j,j);
		sprintf(out, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Ion_2008_descriptor.txt", j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		//pca_Normalisation(mesh);
		timestamp t1= now();
		compute_Geodesic_Eccentricity_descriptor(mesh,in,out);
		timestamp t2 = now();
		timeCalcul[i] = t2-t1;
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		//measure_Function_Our_geodesic_3D(mesh,in,fname);
		//GeometricMomentDescriptor(mesh,5,out);
		delete out;
		delete in;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}
	float sum = 0.0f;
	for(int i=0;i<457;i++)
		sum +=timeCalcul[i];
	printf("\n\nLe temps de calcul de Ion et al. est:   %f \n",sum/457.0f); 

}
float sum2D=0.0;
float sum3D=0.0;
void measure_Our_geodesic(){
	//omp_set_num_threads(5);
	vector<float> timeCalcul_2D(11);
	vector<float> timeCalcul_3D(11);
	int k=0;
//#pragma omp parallel for
	for(int i=743;i<753;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\cygwin\\Benchmark\\%d\\m%d_decim_openMesh_1000.off",j, j);

		sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_embed_Measure_Functions_geodesic_diffusion_1D\\m%d_Measure_Function", j,j,j);
		sprintf(in, "C:\\cygwin\\Benchmark\\%d\\m%d_geodesic_distances_matrix_mesh_1000.txt", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		//pca_Normalisation(mesh);
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		timestamp t1= now();
		measure_Function_Our_geodesic_fct_1D(mesh,in,fname);
		timestamp t2= now();
		timeCalcul_2D[k++]= t2-t1;
		//printf("\nFCT 1D geodesic = %f\n", ((float)timeCalcul_2D[i])/((float)1.0f));
		/*timestamp t_1= now();
		measure_Function_Our_geodesic_3D(mesh,in,fname);
		timestamp t_2= now();
		timeCalcul_3D[i]= t_2-t_1;*/
		delete in;
		delete fname2;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}
	for(int i=0;i<10;i++){
		sum2D +=timeCalcul_2D[i];
		//sum3D +=timeCalcul_3D[i];
	}
	printf("\nFCT 1D geodesic = %f\n", ((float)sum2D)/((float)10.0f));

}

void saveMeasureFunctionGraph___(vector<float> valuesList,vector< vector<int> > neighbors,char *fname){
	FILE *f = fopen(fname,"w");
	if(f==NULL){
		std::cout<<"Erreur d'écriture du fichier " << fname << endl;
		exit(-1);
	}
	int k = (int)valuesList.size();
	fprintf(f,"%d\n",k);
	for(int i=0;i<k;i++){
		fprintf(f,"%f\n",valuesList[i]);
		for(int j=0;j<neighbors[i].size();j++){
			fprintf(f,",%d",neighbors[i][j]);
		}
		fprintf(f,"/\n");
	}
	fclose(f);
	std::cout<<"Fin d'écriture du fichier " << fname << endl;
}


void measure_Our_Eccentric_Transform(){
	//omp_set_num_threads(5);
#pragma omp parallel for
	for(int i=0;i<457;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000.off",j, j);

		sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000_New_Measure_Function_Our_Eccentric_Transform\\m%d_Measure_Function", j,j,j);
		sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_geodesic_distances_matrix_mesh_3000.txt", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		pca_Normalisation(mesh);
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		measure_Function_Our_Eccentric_Transform(mesh,in,fname);
		delete in;
		delete fname2;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}
}

void measure_fct_1D_1(){
	//omp_set_num_threads(5);
	vector<int> modelsList;
	modelsList.clear();
	readIndices(modelsList);
	std::sort(modelsList.begin(),modelsList.end());
	vector<float> timeCalcul(modelsList.size());
#pragma omp parallel for
	for(int i=0;i<500;i++){
		int j=modelsList[i];
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_embed.off",j, j);

		sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_embed_Measure_Function_FCT_1D_euclidian\\m%d_Measure_Function.sf", j,j,j);
		//sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Laplacian_Beltrami_heat_diffusion_DCT_3000.txt", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		int n = mesh->vertices.size();
		vector<bool> toProcess(n,false);
		timestamp t1 = now();
		pca_Normalisation(mesh);
		partSizeFunction(mesh,toProcess,fname);
		timestamp t2 = now();
		timeCalcul[i] = t2-t1;
		//measure_Function_Our_Heat_diffusion_Distance(mesh,in,fname);
		toProcess.clear();
		delete in;
		delete fname2;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}
#pragma omp parallel for
	for(int i=500;i<1500;i++){
		int j=modelsList[i];
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_embed.off",j, j);

		sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_embed_Measure_Function_FCT_1D_euclidian\\m%d_Measure_Function.sf", j,j,j);
		//sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Laplacian_Beltrami_heat_diffusion_DCT_3000.txt", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		int n = mesh->vertices.size();
		vector<bool> toProcess(n,false);
		timestamp t1 = now();
		pca_Normalisation(mesh);
		partSizeFunction(mesh,toProcess,fname);
		timestamp t2 = now();
		timeCalcul[i] = t2-t1;
		//measure_Function_Our_Heat_diffusion_Distance(mesh,in,fname);
		toProcess.clear();
		delete in;
		delete fname2;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}
#pragma omp parallel for
	for(int i=1500;i<2400;i++){
		int j=modelsList[i];
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_embed.off",j, j);

		sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_embed_Measure_Function_FCT_1D_euclidian\\m%d_Measure_Function.sf", j,j,j);
		//sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Laplacian_Beltrami_heat_diffusion_DCT_3000.txt", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		int n = mesh->vertices.size();
		vector<bool> toProcess(n,false);
		timestamp t1 = now();
		pca_Normalisation(mesh);
		partSizeFunction(mesh,toProcess,fname);
		timestamp t2 = now();
		timeCalcul[i] = t2-t1;
		//measure_Function_Our_Heat_diffusion_Distance(mesh,in,fname);
		toProcess.clear();
		delete in;
		delete fname2;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}
#pragma omp parallel for
	for(int i=2400;i<3888;i++){
		int j=modelsList[i];
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_embed.off",j, j);

		sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_embed_Measure_Function_FCT_1D_euclidian\\m%d_Measure_Function.sf", j,j,j);
		//sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Laplacian_Beltrami_heat_diffusion_DCT_3000.txt", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		int n = mesh->vertices.size();
		vector<bool> toProcess(n,false);
		timestamp t1 = now();
		pca_Normalisation(mesh);
		partSizeFunction(mesh,toProcess,fname);
		timestamp t2 = now();
		timeCalcul[i] = t2-t1;
		//measure_Function_Our_Heat_diffusion_Distance(mesh,in,fname);
		toProcess.clear();
		delete in;
		delete fname2;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}
		float sum = 0.0f;
	for(int i=0;i<modelsList.size();i++)
		sum +=timeCalcul[i];
	printf("\n\nLe temps de calcul de 1D et al. est:   %f \n",sum/(float)modelsList.size()); 

}
float sum_S=0.0f;
void measure_fct_1D_2(){
	vector<float> timeCalcul(457);
#pragma omp parallel for
	for(int i=0;i<457;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];

		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000.off",j, j);

		sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000_New_Measure_Function_FCT_1D_Eccentricity\\m%d_Measure_Function", j,j,j);
		sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_geodesic_distances_matrix_mesh_3000.txt", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		pca_Normalisation(mesh);
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		int n = mesh->vertices.size();
		//vector<bool> toProcess(n,true);
		//partSizeFunction(mesh,toProcess,fname);
		//vector<bool> toProcess(n,false);
		timestamp t1 = now();
		measure_Function_Our_geodesic_FCT_1D_eccentricity(mesh,in,fname);
		//partSizeFunction(mesh,toProcess,fname);
		timestamp t2 = now();
		timeCalcul[i] = t2-t1;
		//toProcess.clear();
		delete in;
		delete fname2;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}
	//float sum = 0.0f;
	for(int i=0;i<457;i++)
		sum_S +=timeCalcul[i];
	//printf("\n\nLe temps de calcul de Ion et al. est:   %f \n",sum/457.0f); 

}
float sum_exc=0.0;
void measure_1D_3(){
		vector<float> timeCalcul(457);
#pragma omp parallel for
	for(int i=0;i<457;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];

		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000.off",j, j);

		sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_15000_embed_New_Measure_Function_FCT_1D_Eccentricity\\m%d_Measure_Function", j,j,j);
		sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_geodesic_distances_matrix_mesh_3000.txt", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		pca_Normalisation(mesh);
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		int n = mesh->vertices.size();
		//vector<bool> toProcess(n,true);
		//partSizeFunction(mesh,toProcess,fname);
		timestamp t1 = now();

		measure_Function_Our_geodesic_FCT_1D_eccentricity(mesh,in,fname);
		timestamp t2 = now();
		timeCalcul[i] = t2-t1;

		//toProcess.clear();
		delete in;
		delete fname2;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}
	//float sum = 0.0f;
	for(int i=0;i<457;i++)
		sum_exc +=timeCalcul[i];
	//printf("\n\nLe temps de calcul de Ion et al. est:   %f \n",sum/457.0f); 

}
void measure_our_example(){
		
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];


		sprintf(mname, "C:\\Dossier Ayoub\\3D\\Stanford models\\happy_recon\\happy_recon\\cylindre.ply");

		sprintf(fname, "C:\\Dossier Ayoub\\3D\\Stanford models\\happy_recon\\happy_recon\\cylindre\\cylindre_1_");
		//sprintf(amplitude, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Spectral_Amplitude.txt", j,j,j);
		//sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Laplacian_Beltrami_heat_diffusion_DCT_3000.txt", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		pca_Normalisation(mesh);
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		measure_Function_Our_Example_2(mesh,fname);
		delete in;
		delete fname2;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;

}
void measure_Our_Laplacian_Beltrami_Embedding(){
	//omp_set_num_threads(5);
#pragma omp parallel for
	for(int i=0;i<457;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];
		char *amplitude = new char[1024];


		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000.off",j, j);

		sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000_New_Measure_Function_Laplacian_Beltrami_Embedding\\m%d_Measure_Function", j,j,j);
		//sprintf(amplitude, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Spectral_Amplitude.txt", j,j,j);
		sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Laplacian_Beltrami_heat_diffusion_DCT_3000.txt", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		pca_Normalisation(mesh);
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		measure_Function_Laplacian_Beltrami_Embedding(mesh,in,fname);
		delete in;
		delete fname2;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete amplitude;
		delete mesh;
	}
}
void sizeFunction_offline(){
#pragma omp parallel for
	for(int i=40;i<1;i++){
		int j=1100+i;
		char mname[1024], fname[1024]/*,fname1[1024],fname2[1024], fname3[1024]*/;
		sprintf(mname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.ply",j, j);
		sprintf(fname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Size_corners_Sphere.sf",j, j);
		/*sprintf(fname1, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d_Size_Function_3_PCA_Plans\\m%d_Size_Function_X.sf", j,j,j);
		sprintf(fname2, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d_Size_Function_3_PCA_Plans\\m%d_Size_Function_Y.sf", j,j,j);
		sprintf(fname3, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d_Size_Function_3_PCA_Plans\\m%d_Size_Function_Z.sf", j,j,j);
		vector<SizeElement> SizeElementsListX;
		vector<SizeElement> SizeElementsListY;
		vector<SizeElement> SizeElementsListZ;*/
		TriMesh *mesh = TriMesh::read(mname);
		remove_unused_vertices(mesh);
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		pca_Normalisation_vertices(mesh);
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		size_Function_Sphere(mesh,14,fname);
		delete mesh;
	}

}

using namespace Eigen;

void eigenTest(){
 Matrix<double, 2, 2> m;

 m(0,0) = 3;
  m(1,0) = 2.5;
  m(0,1) = -1;
  m(1,1) = m(1,0) + m(0,1);
  
	  std::cout << "Here is the matrix m:\n" << m << std::endl;
  VectorXd v(2);
  v(0) = 4;
  v(1) = v(0) - 1;
  std::cout << "Here is the vector v:\n" << v << std::endl;
}
void generate_measure_function_bounding_sphere(){
#pragma omp parallel for
	for(int i=0;i<457;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *fname1 = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_50000_embed.off",j, j);

		sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_75000_New_Measure_Function_Bounding_Sphere\\m%d_Measure_Function.sf", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		remove_unused_vertices(mesh);
		erode(mesh);
		remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		pca_Normalisation(mesh);
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		measureFunction_Bounding_Sphere(mesh,fname);
		delete fname1;
		delete fname2;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mesh;
	}
}

void generate_regions_2(){

		char *mname = new char[1024],*fnameX = new char[1024], *fnameY = new char[1024],*fnameZ= new char[1024],*fname3 = new char[1024];

		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m410\\m410_sampled_50000.ply");

		sprintf(fnameX, "C:\\Dossier Ayoub\\3D\\models_Robert W. Sumner\\models\\Chacal\\Example_2_REGIONS_X\\Chacal_Example_Region_mesh");
		sprintf(fnameY, "C:\\Dossier Ayoub\\3D\\models_Robert W. Sumner\\models\\Chacal\\Example_2_REGIONS_y\\Chacal_Example_Region_mesh");
		sprintf(fnameZ, "C:\\Dossier Ayoub\\3D\\models_Robert W. Sumner\\models\\Chacal\\Example_2_REGIONS_Z\\Chacal_Example_Region_mesh");
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);

		center_and_scale_Unit_Sphere(mesh);
		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		//remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		//pca_Normalisation(mesh);

		split_mesh_2_regions_X(mesh,fnameX);
		split_mesh_2_regions_Y(mesh,fnameY);
		split_mesh_2_regions_Z(mesh,fnameZ);
		
		
		delete fnameX;
		delete fnameY;
		delete fnameZ;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname3;
		delete mname;

		delete mesh;
}

void generate_google_3D_warehouse(){
//#pragma omp parallel for
	for(int i=0;i<139;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *fname1 = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\Dossier Ayoub\\3D\\Google 3D Warehouse\\Tanks\\m%d.off",j, j);

		sprintf(fname, "C:\\Dossier Ayoub\\3D\\Google 3D Warehouse\\Tanks\\m%d_N.ply", j,j,j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\McGill_3D_Models_Non_Articulated\\m%d\\m%d_Measure_Function_8_Plans\\m%d_Measure_Function", j,j,j);
		//sprintf(fname1, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_2.sf", j,j,j);
		//sprintf(fname2, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);
		//sprintf(fname3, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_Measure_Function_3_PCA_Plans\\m%d_Measure_Function_3.sf", j,j,j);

		/*sprintf(fnamecurv1, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_1.sf", j,j,j);
		sprintf(fnamecurv2, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Curvatures\\m%d_Measure_Function_Curvatures_2.sf", j,j,j);
		sprintf(fnameLaplace, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d_Measure_Function_Laplacian_Beltrami\\m%d_Measure_Function_Laplacian_Beltrami.sf", j,j,j);
		*/

		TriMesh *mesh = TriMesh::read(mname);

		//subdiv(mesh,SUBDIV_LOOP_NEW);
		//subdiv(mesh,SUBDIV_LOOP_ORIG);
		//smooth_mesh(mesh,2.5f);
		//remove_unused_vertices(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//mesh->need_curvatures();
		//init_mesh(mesh);
		//mesh->need_bbox();
		//clip(mesh,mesh->bbox);
		//pca_Normalisation_vertices(mesh);
		
		//subdiv(mesh,SUBDIV_BUTTERFLY);
		pca_Normalisation(mesh);
		/*for(int k=0;k<mesh->vertices.size()/2;k++){
			printf("\nx= %f, y = %f , z= %f",mesh->vertices[k][0],mesh->vertices[k][1],mesh->vertices[k][2]);
		}*/
		//mesh->need_bsphere();
		//center_and_scale_Unit_Sphere(mesh);
		//erode(mesh);
		//remove_unused_vertices(mesh);
		//measureFunction_PCA_AXIS(mesh,SizeElementsListX,SizeElementsListY,SizeElementsListZ);
		//measureFunction_PCA_AXIS(mesh,fname1,fname2,fname3);
		//measureFunction_MAJOR_8_PLANS(mesh,fname);
		//measureFunction_Bounding_Sphere(mesh,fname);
		mesh->write(fname);
		delete fname1;
		delete fname2;
		delete fname3;
		//measureFunction_curvature(mesh,fname);
		/*measureFunction_curvatures(mesh,fnamecurv1,fnamecurv2);
		delete fnamecurv1;
		delete fnamecurv2;
		measureFunction_Laplacian_Beltrami(mesh,fnameLaplace);
		delete fnameLaplace;*/
		delete fname;
		delete mname;
		delete mesh;
	}
}

void measure_test(){
	float M[8][8];
	FILE *f = fopen("C:\\cygwin\\geodesic_n.txt","r");
	if(f==NULL){
		std::cout<<"Erreur de lecture du fichier : C:\\cygwin\\geodesic_n.txt " << endl;
		exit(-1);
	}

	float adj[8]={1,2,3,2,1,2,2,1};
	vector<float> phi(8);
	vector<vector<int>> graph(8);
	graph[0].push_back(1);
	graph[1].push_back(0);
	graph[1].push_back(2);
	graph[2].push_back(1);
	graph[2].push_back(3);
	graph[2].push_back(5);
	graph[3].push_back(2);
	graph[3].push_back(4);
	graph[4].push_back(3);
	graph[5].push_back(2);
	graph[5].push_back(6);
	graph[6].push_back(5);
	graph[6].push_back(7);
	graph[7].push_back(6);
	float sigma = 0.0f;
	for(int i=0;i<8;i++){
		for(int j=0;j<8;j++){
			fscanf(f,"%f",&M[i][j]);
			if(M[i][j]>sigma) sigma = M[i][j];
		}
		fprintf(f,"\n");
	}
	fclose(f);

#pragma omp parallel for
	for(int i=0;i<8;i++){
		float tmp = 0.0f;
		for(int j=0;j<8;j++){
			tmp += exp(-M[i][j]/(sigma))/adj[j];
		}
		phi[i] = tmp;
	}

	float min = phi[0];
	float maxx = phi[0];
	for(int i=1;i<8;i++){
		if(phi[i]<min) min = phi[i];
		if(phi[i]>maxx) maxx = phi[i];
	}

#pragma omp parallel for
	for(int i=0;i<8;i++){
		if(maxx==min) phi[i] = 1.0f;
		else phi[i] = (phi[i])/(maxx);
	}

	printf("Done\n");

	saveMeasureFunctionGraph___(phi, graph,"C:\\cygwin\\geodesic_n_measure.txt");
}

void mesh_volumetric_repair(){
	//for(){
	//}
}
void generate_measure_Function_Our_Geodesic_1D_partial(){
	//omp_set_num_threads(8);
#pragma omp parallel for
	for(int i=0;i<275;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset\\m%d\\m%d_deci_2000.ply",j, j);

		sprintf(fname, "C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset\\m%d\\m%d_deci_2000_New_Measure_Function_FCT_1D_geodesic\\m%d_Measure_Function.sf", j,j,j);
		sprintf(in, "C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset\\m%d\\m%d_geodesic_distances_matrix_mesh_2000.txt", j,j);


		TriMesh *mesh = TriMesh::read(mname);

		//remove_unused_vertices(mesh);
		pca_Normalisation(mesh);
		measure_Function_Our_Geodesic_1D(mesh,in,fname);
		int n = mesh->vertices.size();
		printf("\n n = %d \n",n);

	//	if(n>4000) system("pause");


		delete in;
		delete fname2;
		delete fname3;
		delete fname;
		delete mesh;
	}
}
void generate_measure_Function_Our_Geodesic_1D(){
	//omp_set_num_threads(8);
#pragma omp parallel for
	for(int i=0;i<1;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_uniform_3000_remeshed.off",j, j);

		sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000_New_Measure_Function_FCT_1D_geodesic\\m%d_Measure_Function.sf", j,j,j);
		sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_geodesic_distances_matrix_mesh_3000.txt", j,j);


		TriMesh *mesh = TriMesh::read(mname);

		remove_unused_vertices(mesh);
		//pca_Normalisation(mesh);
		measure_Function_Our_Geodesic_1D(mesh,in,fname);
		int n = mesh->vertices.size();
		printf("\n n = %d \n",n);

	//	if(n>4000) system("pause");


		delete in;
		delete fname2;
		delete fname3;
		delete fname;
		delete mesh;
	}

	/*
#pragma omp parallel for
	for(int i=109;i<134;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000.ply",j, j);

		sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000_New_Measure_Function_FCT_1D_geodesic\\m%d_Measure_Function.sf", j,j,j);
		sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_geodesic_distances_matrix_mesh_3000.txt", j,j);



		TriMesh *mesh = TriMesh::read(mname);

		remove_unused_vertices(mesh);
		//pca_Normalisation(mesh);
		measure_Function_Our_Geodesic_1D(mesh,in,fname);
		int n = mesh->vertices.size();
		printf("\n n = %d \n",n);

	//	if(n>4000) system("pause");


		delete in;
		delete fname2;
		delete fname3;
		delete fname;
		delete mesh;
	}
#pragma omp parallel for
	for(int i=109;i<134;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000.ply",j, j);

		sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000_New_Measure_Function_FCT_1D_geodesic\\m%d_Measure_Function.sf", j,j,j);
		sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_geodesic_distances_matrix_mesh_3000.txt", j,j);



		TriMesh *mesh = TriMesh::read(mname);

		remove_unused_vertices(mesh);
		//pca_Normalisation(mesh);
		measure_Function_Our_Geodesic_1D(mesh,in,fname);
		int n = mesh->vertices.size();
		printf("\n n = %d \n",n);

	//	if(n>4000) system("pause");


		delete in;
		delete fname2;
		delete fname3;
		delete fname;
		delete mesh;
	}
#pragma omp parallel for
	for(int i=134;i<=234;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_uniform_2000_remeshed.off",j, j);

		sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000_New_Measure_Function_FCT_1D_geodesic\\m%d_Measure_Function.sf", j,j,j);
		sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_geodesic_distances_matrix_mesh_2000_remeshed.txt", j,j);


		TriMesh *mesh = TriMesh::read(mname);

		remove_unused_vertices(mesh);
		//pca_Normalisation(mesh);
		measure_Function_Our_Geodesic_1D(mesh,in,fname);
		int n = mesh->vertices.size();
		printf("\n n = %d \n",n);

	//	if(n>4000) system("pause");


		delete in;
		delete fname2;
		delete fname3;
		delete fname;
		delete mesh;
	}
#pragma omp parallel for
	for(int i=281;i<=434;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_uniform_2000_remeshed.off",j, j);

		sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000_New_Measure_Function_FCT_1D_geodesic\\m%d_Measure_Function.sf", j,j,j);
		sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_geodesic_distances_matrix_mesh_2000_remeshed.txt", j,j);


		TriMesh *mesh = TriMesh::read(mname);

		remove_unused_vertices(mesh);
		//pca_Normalisation(mesh);
		measure_Function_Our_Geodesic_1D(mesh,in,fname);
		int n = mesh->vertices.size();
		printf("\n n = %d \n",n);

	//	if(n>4000) system("pause");


		delete in;
		delete fname2;
		delete fname3;
		delete fname;
		delete mesh;
	}
#pragma omp parallel for
	for(int i=435;i<457;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000.ply",j, j);

		sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000_New_Measure_Function_FCT_1D_geodesic\\m%d_Measure_Function.sf", j,j,j);
		sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_geodesic_distances_matrix_mesh_3000.txt", j,j);


		TriMesh *mesh = TriMesh::read(mname);

		remove_unused_vertices(mesh);
		//pca_Normalisation(mesh);
		measure_Function_Our_Geodesic_1D(mesh,in,fname);
		int n = mesh->vertices.size();
		printf("\n n = %d \n",n);

	//	if(n>4000) system("pause");


		delete in;
		delete fname2;
		delete fname3;
		delete fname;
		delete mesh;
	}
/*
#pragma omp parallel for
	for(int i=150;i<300;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000.ply",j, j);

		sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000_New_Measure_Function_FCT_1D_geodesic\\m%d_Measure_Function.sf", j,j,j);
		sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_geodesic_distances_matrix_mesh_3000.txt", j,j);


		TriMesh *mesh = TriMesh::read(mname);

		remove_unused_vertices(mesh);
		//pca_Normalisation(mesh);
		measure_Function_Our_Geodesic_1D(mesh,in,fname);
		int n = mesh->vertices.size();
		printf("\n n = %d \n",n);

		//if(n>4000) system("pause");
		delete in;
		delete fname2;
		delete fname3;
		delete fname;
		delete mesh;
	}

#pragma omp parallel for
	for(int i=300;i<457;i++){
		int j=0+i;
		char *mname = new char[1024],*fname = new char[1024], *in = new char[1024],*fname2= new char[1024],*fname3 = new char[1024];



		sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000.ply",j, j);

		sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_3000_New_Measure_Function_FCT_1D_geodesic\\m%d_Measure_Function.sf", j,j,j);
		sprintf(in, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_geodesic_distances_matrix_mesh_3000.txt", j,j);


		TriMesh *mesh = TriMesh::read(mname);

		remove_unused_vertices(mesh);
		//pca_Normalisation(mesh);
		measure_Function_Our_Geodesic_1D(mesh,in,fname);
		int n = mesh->vertices.size();
		printf("\n n = %d \n",n);

		//if(n>4000) system("pause");
		delete in;
		delete fname2;
		delete fname3;
		delete fname;
		delete mesh;
	}*/
		
}

void removed_isolated_parts(){

//#pragma omp parallel for	
	for(int i=0;i<52;i++){
			//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
			//sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.off",j, j);
			//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.ply",j, j);
			char *mname = new char[1024],*fname= new char[1024];
			char *commandLine= new char[1024];
			char *command= new char[1024];
			int j = i;
			//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
			//sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.off",j, j);
			//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.ply",j, j);
			sprintf(mname, "C:\\Dossier Ayoub\\3D\\Google 3D Warehouse\\Knifes\\m%d_rep_octree.ply",j,j);
			sprintf(fname, "C:\\Dossier Ayoub\\3D\\Google 3D Warehouse\\Knifes\\m%d_rep_octree.obj",j,j);
			//sprintf(mname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.off", j,j);
			//sprintf(fname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.ply",j, j);
			/*FILE *f=fopen(fname,"r");
			if(f!=NULL) {
				fclose(f);
				printf("\n\nHello %s\n\n",fname);
				continue;
		
			}*/
			//fclose(f);
			//sprintf(command,"C:\\cygwin\\MeshFix_v1.0\\MeshFix\\meshfix.exe");
			//sprintf(commandLine,"%s %s -n", command,mname);
			//sprintf(commandLine,"%s --in %s --out %s --maxDepth 9 --curvature 0.99 --fullCaseTable", command,mname,fname);
			//TriMesh *mesh = TriMesh::read(mname);
			//erode(mesh);
			TriMesh * mesh = new TriMesh(*TriMesh::read(mname));
			//remove_unused_vertices(mesh);
			//orient(mesh);
			//orient(mesh);
			//edgeflip(mesh);
			//mesh->need_tstrips();
			//reorder_verts(mesh);
			//erode(mesh);
			//remove_unused_vertices(mesh);
			//erode(mesh);
			//selectCompByBiggestSize(mesh);

			//holes_filling(mesh);
			//remove_unused_vertices(mesh);
			//remove_unused_vertices(mesh);

			//erode(mesh);
			//remove_unused_vertices(mesh);
			//edgeflip(mesh);
			//orient(mesh);
			//pca_Normalisation(mesh);
			removeNAN(mesh);
			selectCompByBiggestSize(mesh);
			remove_unused_vertices(mesh);

			//orient(mesh);

			//pca_Normalisation(mesh);
			//getPCAAxisVertices(mesh);
			//perform_Delaunay_Triangulation(mesh,fname);
			//spectral_embedding_connectivity(mesh);
			//mesh->write(fname);
			//system(commandLine);
			//TriMesh *mesh = TriMesh::read(fname);
			//sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_7000_fixed.ply",j,j);
			mesh->write(fname);

			printf("\n%s Enregistré avec succès",mname);
			delete command;
			delete commandLine;
			delete fname;
			delete mname;
			delete mesh;
	}

}

void convert_to_ply(){
	for(int i=0;i<275;i++){
		//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.off",j, j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.ply",j, j);
		char *mname = new char[1024],*fname= new char[1024];
		char *commandLine= new char[1024];
		char *command= new char[1024];
			int j = i;
			//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
			//sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.off",j, j);
			//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.ply",j, j);
			sprintf(mname, "C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset\\m%d\\m%d_deci_2000.off",j,j);
			sprintf(fname, "C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset\\m%d\\m%d_deci_2000.ply",j,j);
			//sprintf(mname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.off", j,j);
			//sprintf(fname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.ply",j, j);
			/*FILE *f=fopen(fname,"r");
			if(f!=NULL) {
				fclose(f);
				printf("\n\nHello %s\n\n",fname);
				continue;
		
			}*/
			//fclose(f);
			//sprintf(command,"C:\\cygwin\\MeshFix_v1.0\\MeshFix\\meshfix.exe");
			//sprintf(commandLine,"%s %s -n", command,mname);
			//sprintf(commandLine,"%s --in %s --out %s --maxDepth 9 --curvature 0.99 --fullCaseTable", command,mname,fname);
			//TriMesh *mesh = TriMesh::read(mname);
			//erode(mesh);
			TriMesh * mesh = new TriMesh(*TriMesh::read(mname));
			selectCompByBiggestSize(mesh);
			remove_unused_vertices(mesh);
			pca_Normalisation(mesh);

		/*	removeNAN(mesh);

			//orient(mesh);
			//if(mesh->vertices.size()>20000){
			TriMesh *out = new TriMesh;

			float voxelsize = 2.0f * mesh->feature_size();
			crunch(mesh, out, voxelsize);
			//holes_filling(mesh);
			//orient(mesh);
			//edgeflip(mesh);
			//mesh->need_tstrips();
			//reorder_verts(mesh);
			//erode(mesh);
			//remove_unused_vertices(mesh);
			//erode(mesh);
			selectCompByBiggestSize(out);

			//holes_filling(mesh);
			remove_unused_vertices(out);

			//erode(mesh);
			/*remove_unused_vertices(mesh);
			pca_Normalisation(mesh);*/
			//selectCompByBiggestSize(mesh);
			//pca_Normalisation(mesh);
			//getPCAAxisVertices(mesh);
			//perform_Delaunay_Triangulation(mesh,fname);
			//spectral_embedding_connectivity(mesh);
			//mesh->write(fname);
			//system(commandLine);
			//TriMesh *mesh = TriMesh::read(fname);
			//sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_7000_fixed.ply",j,j);
			
			//remove_unused_vertices(out);
			//pca_Normalisation(mesh);
			//orient(out);
			mesh->write(fname);
			delete mesh;
			//}
			//else mesh->write(fname);
			printf("\n%s Enregistré avec succès",fname);
			delete command;
			delete commandLine;
			delete fname;
			delete mname;
			//std::remove(mname);

		//	delete mesh;
	}
}
void prepare_3D_models(){
	for(int i=49;i<52;i++){
		//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
		//sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.off",j, j);
		//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.ply",j, j);
		char *mname = new char[1024],*fname= new char[1024];
		char *commandLine= new char[1024];
		char *command= new char[1024];
			int j = i;
			//sprintf(mname, "D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\11\\m%d\\m%d.off",j, j);
			//sprintf(mname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.off",j, j);
			//sprintf(fname, "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m%d\\m%d_sampled_4000.ply",j, j);
			sprintf(mname, "C:\\Dossier Ayoub\\3D\\Google 3D Warehouse\\Knifes\\m%d_rep_octree.ply",j,j);
			sprintf(fname, "C:\\Dossier Ayoub\\3D\\Google 3D Warehouse\\Knifes\\m%d_rep_octree.ply",j,j);
			//sprintf(mname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.off", j,j);
			//sprintf(fname, "C:\\Dossier Ayoub\\3D\\psb_v1\\benchmark\\db\\11\\m%d\\m%d.ply",j, j);
			/*FILE *f=fopen(fname,"r");
			if(f!=NULL) {
				fclose(f);
				printf("\n\nHello %s\n\n",fname);
				continue;
		
			}*/
			//fclose(f);
			//sprintf(command,"C:\\cygwin\\MeshFix_v1.0\\MeshFix\\meshfix.exe");
			//sprintf(commandLine,"%s %s -n", command,mname);
			//sprintf(commandLine,"%s --in %s --out %s --maxDepth 9 --curvature 0.99 --fullCaseTable", command,mname,fname);
			//TriMesh *mesh = TriMesh::read(mname);
			//erode(mesh);
			TriMesh * mesh = new TriMesh(*TriMesh::read(mname));
			//remove_unused_vertices(mesh);
			removeNAN(mesh);

			//orient(mesh);
			//if(mesh->vertices.size()>20000){
			TriMesh *out = new TriMesh;

			float voxelsize = 2.0f * mesh->feature_size();
			crunch(mesh, out, voxelsize);
			//holes_filling(mesh);
			//orient(mesh);
			//edgeflip(mesh);
			//mesh->need_tstrips();
			//reorder_verts(mesh);
			//erode(mesh);
			//remove_unused_vertices(mesh);
			//erode(mesh);
			selectCompByBiggestSize(out);

			//holes_filling(mesh);
			remove_unused_vertices(out);

			//erode(mesh);
			/*remove_unused_vertices(mesh);
			pca_Normalisation(mesh);*/
			//selectCompByBiggestSize(mesh);
			//pca_Normalisation(mesh);
			//getPCAAxisVertices(mesh);
			//perform_Delaunay_Triangulation(mesh,fname);
			//spectral_embedding_connectivity(mesh);
			//mesh->write(fname);
			//system(commandLine);
			//TriMesh *mesh = TriMesh::read(fname);
			//sprintf(fname, "C:\\cygwin\\Benchmark\\%d\\m%d_sampled_7000_fixed.ply",j,j);
			
			//remove_unused_vertices(out);
			//pca_Normalisation(mesh);
			//orient(out);
			out->write(fname);
			delete out;
			//}
			//else mesh->write(fname);
			printf("\n%s Enregistré avec succès",mname);
			delete command;
			delete commandLine;
			delete fname;
			delete mname;
			//std::remove(mname);

			delete mesh;
	}

}

#include <windows.h>
 typedef std::vector<std::string> stringvec;
 LPWSTR ConvertToLPWSTR( const std::string& s )
{
  LPWSTR ws = new wchar_t[s.size()+1]; // +1 for zero at the end
  copy( s.begin(), s.end(), ws );
  ws[s.size()] = 0; // zero at the end
  return ws;
}
void read_directory(const std::string& name, stringvec& v)
{
    std::string pattern(name);
    pattern.append("\\*");
    WIN32_FIND_DATA data;
    HANDLE hFind;
    if ((hFind = FindFirstFile(ConvertToLPWSTR(pattern), &data)) != INVALID_HANDLE_VALUE) {
        do {
			   //convert from wide char to narrow char array
			char ch[260];
			char DefChar = ' ';
			WideCharToMultiByte(CP_ACP,0,data.cFileName,-1, ch,260,&DefChar, NULL);
            v.push_back(ch);
        } while (FindNextFile(hFind, &data) != 0);
        FindClose(hFind);
    }
}
 template<typename TString>
  inline bool starts_with(const TString& str, const TString& start) {
    if (start.size() > str.size()) return false;
    return str.compare(0, start.size(), start) == 0;
  }
  template<typename TString>
  inline bool ends_with(const TString& str, const TString& end) {
    if (end.size() > str.size()) return false;
    return std::equal(end.rbegin(), end.rend(), str.rbegin());
  }

  bool str_ends_with(const char * str, const char * suffix) {

  if( str == NULL || suffix == NULL )
    return false;

  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);

  if(suffix_len > str_len)
    return false;

  return 0 == strncmp( str + str_len - suffix_len, suffix, suffix_len );
}
  bool str_starts_with(const char * str, const char * suffix) {

  if( str == NULL || suffix == NULL )
    return false;

  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);

  if(suffix_len > str_len)
    return false;

  return 0 == strncmp( str + str_len - suffix_len, suffix, suffix_len );
}
  void save3DObject(const std::string &starts,const std::string &ends,const std::string &path,const std::string &pathOut,stringvec filesList,int &id){
	for(int i=0;i<filesList.size();i++){
		if(starts_with<std::string>(filesList[i],starts) && ends_with<std::string>(filesList[i],ends)){
			TriMesh * mesh = new TriMesh(*TriMesh::read((path+"\\"+filesList[i]).c_str()));
			stringstream ss;
			ss << id;
			string out = pathOut+"\\m"+ss.str()+"\\m"+ss.str()+".off";
			mesh->write(out.c_str());
			printf("\n %s \n",out.c_str());
			id++;
			delete mesh;
		}
	}
  }

  void save3DHolesRatesCuts(const std::string &starts,const std::string &ends,const std::string &path,const std::string &pathOut,stringvec filesList,int &id, bool isCut, bool isHole){
	for(int i=0;i<filesList.size();i++){
		if(starts_with<std::string>(filesList[i],starts) && ends_with<std::string>(filesList[i],ends)){
			
			stringstream ss;
			ss << id;
			string out = pathOut+"\\m"+ss.str()+"\\m"+ss.str()+".holesCuts";
			if(isHole){
				int deb = filesList[i].find('.');
				int fin = filesList[i].rfind('_');
				int res = atoi(filesList[i].substr(deb+1,fin-1).c_str());
				FILE *f = fopen(out.c_str(), "w");
				fprintf(f,"H %d\n",res);
				fprintf(f,"C 0\n");
				fclose(f);
			}
			else if (isCut){
				int deb = filesList[i].find('l');
				int fin = filesList[i].rfind('.');
				int res = atoi(filesList[i].substr(deb+1,fin-1).c_str());
				FILE *f = fopen(out.c_str(), "w");
				fprintf(f,"H 0\n");
				fprintf(f,"C %d\n",res);
				fclose(f);			
			}
			else{
				FILE *f = fopen(out.c_str(), "w");
				fprintf(f,"H 0\n");
				fprintf(f,"C 0\n");
				fclose(f);			
			}
			printf("\n %s \n",out.c_str());
			id++;
		}
	}
  }
int main(int argc, char *argv[])
{
	//glutInitWindowSize(512, 512);
	//glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	//eigenTest();
	//glutInit(&argc, argv);
	
//	generate_measure_Function_Our_Geodesic_1D_partial();
	//generate__Ion_descriptor_2008_Rodola();
//	measure_biasoti_Rodola();

	//generateLocalMeasureFunctionsPartialMatching();
	//convert_to_ply();
	//generate_google_3D_warehouse();
	//measure_biasoti();
//	measure_Our_geodesic();
	stringvec filesListNull,filesListcuts, filesListholes;
	read_directory("C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\null",filesListNull);
	read_directory("C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\cuts",filesListcuts);
	read_directory("C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\holes",filesListholes);
	/*for(int i=0;i<filesList.size();i++)
		printf("%s\n",filesList[i].c_str());
*/
	/*
	int id = 0;
	save3DObject("cat","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\null","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListNull,id);
	save3DObject("cat","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\cuts","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListcuts,id);
	save3DObject("cat","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\holes","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListholes,id);

	save3DObject("centaur","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\null","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListNull,id);
	save3DObject("centaur","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\cuts","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListcuts,id);
	save3DObject("centaur","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\holes","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListholes,id);

	save3DObject("david","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\null","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListNull,id);
	save3DObject("david","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\cuts","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListcuts,id);
	save3DObject("david","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\holes","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListholes,id);

	save3DObject("dog","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\null","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListNull,id);
	save3DObject("dog","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\cuts","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListcuts,id);
	save3DObject("dog","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\holes","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListholes,id);

	save3DObject("horse","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\null","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListNull,id);
	save3DObject("horse","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\cuts","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListcuts,id);
	save3DObject("horse","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\holes","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListholes,id);
	
	save3DObject("michael","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\null","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListNull,id);
	save3DObject("michael","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\cuts","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListcuts,id);
	save3DObject("michael","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\holes","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListholes,id);

	save3DObject("victoria","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\null","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListNull,id);
	save3DObject("victoria","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\cuts","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListcuts,id);
	save3DObject("victoria","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\holes","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListholes,id);
	
	save3DObject("wolf","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\null","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListNull,id);
	save3DObject("wolf","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\cuts","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListcuts,id);
	save3DObject("wolf","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\holes","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListholes,id);
	*/

		
	int id = 0;
	save3DHolesRatesCuts("cat","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\null","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListNull,id,false,false);
	save3DHolesRatesCuts("cat","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\cuts","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListcuts,id,true,false);
	save3DHolesRatesCuts("cat","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\holes","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListholes,id,false,true);

	save3DHolesRatesCuts("centaur","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\null","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListNull,id,false,false);
	save3DHolesRatesCuts("centaur","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\cuts","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListcuts,id,true,false);
	save3DHolesRatesCuts("centaur","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\holes","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListholes,id,false,true);

	save3DHolesRatesCuts("david","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\null","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListNull,id,false,false);
	save3DHolesRatesCuts("david","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\cuts","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListcuts,id,true,false);
	save3DHolesRatesCuts("david","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\holes","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListholes,id,false,true);

	save3DHolesRatesCuts("dog","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\null","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListNull,id,false,false);
	save3DHolesRatesCuts("dog","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\cuts","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListcuts,id,true,false);
	save3DHolesRatesCuts("dog","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\holes","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListholes,id,false,true);

	save3DHolesRatesCuts("horse","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\null","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListNull,id,false,false);
	save3DHolesRatesCuts("horse","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\cuts","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListcuts,id,true,false);
	save3DHolesRatesCuts("horse","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\holes","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListholes,id,false,true);
	
	save3DHolesRatesCuts("michael","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\null","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListNull,id,false,false);
	save3DHolesRatesCuts("michael","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\cuts","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListcuts,id,true,false);
	save3DHolesRatesCuts("michael","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\holes","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListholes,id,false,true);

	save3DHolesRatesCuts("victoria","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\null","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListNull,id,false,false);
	save3DHolesRatesCuts("victoria","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\cuts","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListcuts,id,true,false);
	save3DHolesRatesCuts("victoria","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\holes","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListholes,id,false,true);
	
	save3DHolesRatesCuts("wolf","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\null","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListNull,id,false,false);
	save3DHolesRatesCuts("wolf","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\cuts","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListcuts,id,true,false);
	save3DHolesRatesCuts("wolf","off","C:\\Dossier Ayoub\\3D\\partial_off\\SGP_dataset_off\\selected\\holes","C:\\Dossier Ayoub\\3D\\Partial_Matching_dataset",filesListholes,id,false,true);
	
	//prepare_3D_models();

	//generate_measure_Function_Our_Geodesic_1D();

	//prepare_3D_models();

	//removed_isolated_parts();
//	generate_measure_Our_Our_Geodesic_Eccentricity_2D_Mapping();
	system("pause");
	//TriMesh *mesh =TriMesh::read("C:\\Users\\alam1702\\Desktop\\Mediouni_Cave\\femur6_3000.ply");
	//pca_Normalisation(mesh);
	//holes_filling(mesh);
	//selectTheTwoBiggestSize(mesh);
	//remove_unused_vertices(mesh);
	//mesh->write("C:\\Users\\alam1702\\Desktop\\Mediouni_Cave\\femur6_3000_2_big.ply");
	
//	measure_fct_1D_2();
	//*generate_measure_Our_Convex_Concav_Transform();
	//TriMesh * mesh = make_ccyl(15,15,0.3f);
	//mesh->write("C:\\Dossier Ayoub\\3D\\Stanford models\\happy_recon\\happy_recon\\cynlindre.ply");
	//mesh->write("C:\\Dossier Ayoub\\3D\\Stanford models\\happy_recon\\happy_recon\\cylindre.ply");
	//measure_our_example();
	//delete mesh;

	//measure_fct_1D_1();

	//generate_regions_2();

	//saveModelsIndices();
	//TriMesh *mesh =TriMesh::read("C:\\Users\\alam1702\\Desktop\\Mediouni_Cave\\humanBody_repaired.ply");
	//selectCompByBiggestSize(mesh);
	//remove_unused_vertices(mesh);

	//mesh->need_normals();
	//colorbynormals(mesh);
	//mesh->write("C:\\Users\\alam1702\\Desktop\\Mediouni_Cave\\humanBody_opt.obj");
	//generatePlyFile();

	//volumetric_mesh_repairing();

	//system("pause");
	//verify();
	//mesh_repair();
	//test_mesh_repair();
	//ply2vri();
	//generatePlyFile_4();

//	generate__rabin_2010_descriptor();

	//generate__Ion_descriptor_2008();
		
	//generate__moments();

//	measure_Our_geodesic();

//	generateMeasureFunctions();

//	generate_regions_2();

	//generateMeasureFunctions_2();
//
	//measure_Our_Heat_diffusion();

	//generate_regions();

	//measure_fct_1D_1();

	//measure_fct_1D_2();
	//measure_1D_3();

	printf("\n\nTemps Surfacique : %f", sum_S/457.0f);
	//printf("\n\nTemps Excentricite : %f", sum_exc/457.0f);
	//printf("\n\nTemps fct 2D : %f", sum2D/457.0f);
	//printf("\n\nTemps fct 3D : %f", sum3D/457.0f);
	system("pause");
	//if (argc < 2)
		//usage(argv[0]);

/*	//for (int i = 3; i <=3; i++) {
		const char *filename = "C:\\Dossier Ayoub\\3D\\new_McGill_benchmark\\McGillDBAll_DB\\m11\\m11.ply";
	//	char filename[1024];
	//	sprintf(filename,"%s%d.off","D:\\Ayoub\\Publications\\Implémentation\\benchmark\\db\\1\\m107\\m107_portions\\m107_portion",i);
		//const char *filename ="C:\\Users\\Ayoub\\Desktop\\OGL_Sha_Lan_Sec_Ed\\happy_param.ply";
		TriMesh *themesh = TriMesh::read(filename);
		TriMesh::dprintf("\n%d Vertices %d Faces",themesh->vertices.size(),themesh->faces.size());
		//align(themesh,getPCAAxisVertices(themesh)[0]);
		//init_mesh(themesh);
		//center_and_scale_Unit_Sphere(themesh);
		themesh->need_curvatures();

		/*for(int i=0;i<themesh->curv1.size();i++){
			printf("\ncurv1[%d] = %f, curv2[%d] = %f",i,themesh->curv1[i],i,themesh->curv2[i]);
		}*/
/*		pca_Normalisation_vertices(themesh);
		/*point center = mesh_center_of_mass(themesh);
		//v2
		//rot(themesh,90.0f,center,vec(1.0f,0.0f,0.0f));
		//v3
		//rot(themesh,-90.0f,center,vec(0.0f,1.0f,0.0f));

		//v4
		vec v4(1.0f,1.0f,1.0f);
		normalize(v4);
		//align(themesh,v4);
		//v5
		vec v5(1.0f,1.0f,-1.0f);
		normalize(v5);
		//align(themesh,v5);
		//v6
		vec v6(-1.0f,1.0f,1.0f);
		normalize(v6);
		//align(themesh,v6);
		//v7
		vec v7(-1.0f,1.0f,-1.0f);
		normalize(v7);
		align(themesh,v7);
*/
/*
		meshes.push_back(themesh);

		//TriMesh *themesh1 = make_sphere_subdiv(6,3);
		//if (!themesh)
			//usage(argv[0]);
		//subdiv(themesh,1);
		//subdiv(themesh,SUBDIV_LOOP);
		//erode(themesh1);
		//TriMesh::dprintf("\n%d Vertices %d Faces",themesh1->vertices.size(),themesh1->faces.size());
		//init_mesh(themesh1);
		//pca_Normalisation(themesh1);
		//meshes.push_back(themesh1);
		//colorbynormals(themesh1);
		colorbynormals(themesh);
		//normalize_variance(themesh);
		//point com = mesh_center_of_mass(themesh);
		//trans(themesh,-com);
		//pca_rotate(themesh);
		//pca_Normalisation(themesh);
		//pca_Normalisation(themesh);
		/*vec *V = getPCAAxis(themesh);
		vec v1=V[0];
		vec v2=V[1];vec v3=V[2];

		printf("Vecteur d'alignement 1<%f,%f,%f>\n",v1[0],v1[1],v1[2]);
		printf("Vecteur d'alignement 2<%f,%f,%f>\n",v2[0],v2[1],v2[2]);
		printf("Vecteur d'alignement 3<%f,%f,%f>\n",v3[0],v3[1],v3[2]);
		printf("Alignement\n");
		//pca_Normalisation(themesh);
		vec vv(-1.0f,1.0f,-1.0f);
		normalize(vv);
		align(themesh,vv);
		alignInv(themesh,vv);
		//pca_Normalisation(themesh);
		//point  center = mesh_center_of_mass(themesh);
		//rot(themesh,-90.0f,center,vec(0.0,1.0,0.0));
		//normalize_variance(themesh);
		//pca_snap(themesh);
		//noisify(themesh,0.009);
		//pca_Normalisation(themesh);
		//remove_unused_vertices(themesh);
		//themesh->normals.clear();
		//themesh->need_normals();
		//colorbynormals(themesh);
		//char color[20];
		//int col= i*i;
		//sprintf(color,"%s%d","0xFF54",col);
		//solidcolor(themesh,color);
		//solidcolor(themesh,"0x0790");*/
	/*	string xffilename = xfname(filename);
		xffilenames.push_back(xffilename);
		printf("%s %s\n", filename, xffilename.c_str());
		//xffilenames.push_back(xffilename);
printf("%s %s\n", filename, xffilename.c_str());

		xforms.push_back(xform());
		visible.push_back(true);
//		xforms.push_back(xform());
	//	visible.push_back(true);
	//}

	glutCreateWindow("Test");
	glutDisplayFunc(redraw);
	glutMouseFunc(mousebuttonfunc);
	glutMotionFunc(mousemotionfunc);
	glutKeyboardFunc(keyboardfunc);
	glutIdleFunc(idle);

	white_bg = true;
	draw_edges = false;

	resetview();

	glutMainLoop();*/
}

