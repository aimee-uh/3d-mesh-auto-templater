// trimesh.cpp
// 
// A class for manipulating triangle meshes.
//
// by Brett Allen
//
// *** For license information, please see license.txt. ***


#include <iostream>
#include <fstream>
#include <cstring>
#include "trimesh.h"
#include "ply.h"
#include "vec.h"
#include "float.h"

using namespace std;


// externals from trimesh.h
Vec3d TEMP_VEC;
double TEMP_DOUBLE;

// color correction
const double DIV_BY = 255.0; // 150; 


#define EPSILON 0.000000001


// Hierarchical Bounding Box class for raycasting
class HBBox {
public:
	double mBox[6];
	HBBox *mChildren[2];
	intVec mVertices;

	HBBox() {
		mChildren[0] = NULL;
		mChildren[1] = NULL;
	}

	~HBBox() {
		if (mChildren[0])
			delete mChildren[0];
		if (mChildren[1])
			delete mChildren[1];
	}

	// from Graphics Gems, vol 1 (p 736)
	//  - modified to include intersections that occur in the 
	//    negative direction as well (this may have broken the 
	//    algorithm)
	bool rayIntersect(const Vec3d &origin, const Vec3d &dir) {
		const int RIGHT = 0;
		const int LEFT = 1;
		const int MIDDLE = 2;

		bool inside = true;
		char quadrant[3];
		register int i;
		int whichPlane;
		Vec3d maxT;
		Vec3d candidatePlane;
		Vec3d coord;

		// find candidate planes
		for (i = 0; i < 3; i++) {
			if (origin[i] < mBox[i*2]) {
				quadrant[i] = LEFT;
				candidatePlane[i] = mBox[i*2];
				inside = false;
			}
			else if (origin[i] > mBox[i*2+1]) {
				quadrant[i] = RIGHT;
				candidatePlane[i] = mBox[i*2+1];
				inside = false;
			}
			else
				quadrant[i] = MIDDLE;
		}
		
		if (inside) {
			return true;
		}

		// calculate T distances to candidate planes
		for (i = 0; i < 3; i++) {
			if (quadrant[i] != MIDDLE && dir[i] != 0.)
				maxT[i] = (candidatePlane[i] - origin[i]) / dir[i];
			else
				maxT[i] = 0;//-1.;
		}

		// get largest of the maxTs for final choice of intersection
		whichPlane = 0;
		if (fabs(maxT[whichPlane]) < fabs(maxT[1]))
			whichPlane = 1;
		if (fabs(maxT[whichPlane]) < fabs(maxT[2]))
			whichPlane = 2;

		// check final candidate actually inside box
//		if (maxT[whichPlane] < -EPSILON)
//			return false;
		for (i = 0; i < 3; i++) {
			if (whichPlane != i) {
				coord[i] = origin[i] + maxT[whichPlane] * dir[i];
				if (coord[i] < mBox[i*2] || coord[i] > mBox[i*2+1]) {
					return false;
				}
			}
		}
		return true;
		//return maxT[whichPlane];
	}
};

static double dMax(double a, double b) {
	if (a > b)
		return a;
	return b;
}

static double dMin(double a, double b) {
	if (a < b)
		return a;
	return b;
}

void bbUnion(double *b0, double *b1, double *u) {
	u[0] = dMin(b0[0], b1[0]);
	u[1] = dMax(b0[1], b1[1]);
	u[2] = dMin(b0[2], b1[2]);
	u[3] = dMax(b0[3], b1[3]);
	u[4] = dMin(b0[4], b1[4]);
	u[5] = dMax(b0[5], b1[5]);
}

double bbDistance(Vec3d pt, double *box) {
	Vec3d dist;

	if (pt[0] < box[0])
		dist[0] = pt[0] - box[0];
	else if (pt[0] > box[1])
		dist[0] = pt[0] - box[1];

	if (pt[1] < box[2])
		dist[1] = pt[1] - box[2];
	else if (pt[1] > box[3])
		dist[1] = pt[1] - box[3];

	if (pt[2] < box[4])
		dist[2] = pt[2] - box[4];
	else if (pt[2] > box[5])
		dist[2] = pt[2] - box[5];

	return dist.length();
}

inline double sqr(double x) {
	return x*x;
}

// the following snipped is from
// http://www.acm.org/jgt/papers/MollerTrumbore97/code.html
// modified to take "Vec3d"s as parameters
// ====================================================
#define ANGLE_EPSILON 0.2
#define CROSS(dest,v1,v2) \
          dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
          dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
          dest[2]=v1[0]*v2[1]-v1[1]*v2[0];
#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])
#define SUB(dest,v1,v2) \
          dest[0]=v1[0]-v2[0]; \
          dest[1]=v1[1]-v2[1]; \
          dest[2]=v1[2]-v2[2]; 
//#define TEST_CULL
int
intersect_triangle(Vec3d &orig, Vec3d &dir,
                   Vec3d &vert0, Vec3d &vert1, Vec3d &vert2,
                   double *t, double *u, double *v, bool *away)
{
   double edge1[3], edge2[3], tvec[3], pvec[3], qvec[3];
   double det,inv_det;

   if (away)
	   *away = false;

   // find vectors for two edges sharing vert0 
   SUB(edge1, vert1, vert0);
   SUB(edge2, vert2, vert0);

   // begin calculating determinant - also used to calculate U parameter
   CROSS(pvec, dir, edge2);

   // if determinant is near zero, ray lies in plane of triangle 
   det = DOT(edge1, pvec);

#ifdef TEST_CULL           // define TEST_CULL if culling is desired
   if (det / (
	   sqrt(sqr(edge1[0])+sqr(edge1[1])+sqr(edge1[2])) *
	   sqrt(sqr(pvec[0])+sqr(pvec[1])+sqr(pvec[2]))) < EPSILON)
      return 0;

   // calculate distance from vert0 to ray origin 
   SUB(tvec, orig, vert0);

   // calculate U parameter and test bounds
   *u = DOT(tvec, pvec);
   if (*u < 0.0 || *u > det)
      return 0;

   // prepare to test V parameter
   CROSS(qvec, tvec, edge1);

    // calculate V parameter and test bounds
   *v = DOT(dir, qvec);
   if (*v < 0.0 || *u + *v > det)
      return 0;

   // calculate t, scale parameters, ray intersects triangle
   *t = DOT(edge2, qvec);
   inv_det = 1.0 / det;
   *t *= inv_det;
   *u *= inv_det;
   *v *= inv_det;
   
#else                    // the non-culling branch
   if (det > -EPSILON && det < EPSILON)
     return 0;

   //	double foo=det/ (
//	   sqrt(sqr(edge1[0])+sqr(edge1[1])+sqr(edge1[2])) *
//	   sqrt(sqr(pvec[0]) +sqr(pvec[1]) +sqr(pvec[2])));
//	if (foo > -ANGLE_EPSILON && foo < ANGLE_EPSILON)
//		return 0;

   double foo[3];
   if (away) {
	   CROSS(foo, edge1, edge2);
	   if (DOT(foo, dir) > 0)
		   *away = false;
	   else 
		   *away = true;
	   /*
		if (det / (
			sqrt(sqr(edge1[0])+sqr(edge1[1])+sqr(edge1[2])) *
			sqrt(sqr(pvec[0]) +sqr(pvec[1]) +sqr(pvec[2]))) < 0)
			*away = true;
		else
			*away = false;
			*/
   }
   
   inv_det = 1.0 / det;

   // calculate distance from vert0 to ray origin
   SUB(tvec, orig, vert0);

   // calculate U parameter and test bounds
   *u = DOT(tvec, pvec) * inv_det;
   if (*u < 0.0 - EPSILON || *u > 1.0 + EPSILON)
     return 0;

   // prepare to test V parameter
   CROSS(qvec, tvec, edge1);

   // calculate V parameter and test bounds
   *v = DOT(dir, qvec) * inv_det;
   if (*v < 0.0 - EPSILON || *u + *v > 1.0 + EPSILON)
     return 0;

   // calculate t, ray intersects triangle
   *t = DOT(edge2, qvec) * inv_det;
#endif
   return 1;
}
// ====================================================

/*
static int intersect_triangle(Vec3d &p, Vec3d &v,
                   Vec3d &a, Vec3d &b, Vec3d &c,
                   double *t, double *uu, double *vv) {
	// the following were parameters
	Vec3d bary;
	Vec3d n;
	
	// Intersect ray r with the triangle abc.  If it hits returns true,
	// and put the parameter in t and the barycentric coordinates of the
	// intersection in bary.
	// Uses the algorithm and notation from _Graphic Gems 5_, p. 232.
	//
	// Calculates and returns the normal of the triangle too.
    
	Vec3d ab = b - a;
	Vec3d ac = c - a;
	Vec3d ap = p - a;
  
	n=ab^ac;
  
	// there exists some bad triangles such that two vertices coincide
	// check this before normalize
	if (n.iszero()) return false;
	n.normalize();

	double vdotn = v*n;
	if( -vdotn < 1e-8 )
		return false;
    
	*t = - (ap*n)/vdotn;

	if( *t < 1e-8 )
		return false;

	// find k where k is the index of the component
	// of normal vector with greatest absolute value

	float greatestMag = n[0];
	if(greatestMag < 0)
		greatestMag = -greatestMag;
	int k = 0;
	if(n[1] > greatestMag) {
		k = 1;
		greatestMag = n[1];
	}
	else if(-n[1] > greatestMag) {
		k = 1;
		greatestMag = -n[1];
	}
	if(n[2] > greatestMag) {
		k = 2;
		greatestMag = n[2];
	}
	else if(-n[2] > greatestMag) {
		k = 2;
		greatestMag = -n[2];
	}

	Vec3d am = ap + *t * v;

	double tmp = (ab^ac)[k];
	bary[1] = (am^ac)[k] / tmp;
	if(bary[1] < 0 || bary[1] > 1)
		return false;
	bary[2] = (ab^am)[k] / tmp;
	if(bary[2] < 0 || bary[2] > 1)
		return false;
  
	bary[0] = 1 - bary[1] - bary[2];
	if(bary[0] < 0)
		return false;

	return true;
}
*/

bool pointToTriDist(Vec3d orig, Vec3d *verts, double maxDist,
					double &dist, Vec3d &pt, Vec3d &bary, int *which) {

	Vec3d delta;
	if (verts[0] == verts[1] || verts[0] == verts[2])
		return false;
	Vec3d norm = -(verts[1] - verts[0]) ^ (verts[2] - verts[0]);
	norm.normalize();
	double baryDenom = ((verts[1] - verts[0]) ^ (verts[2] - verts[0])) * norm;
#ifdef WIN32
	if (!_finite(baryDenom))
		return false;
#else
	if (!finite(baryDenom))
		return false;
#endif

	// project current voxel's position onto the triangle's plane
	dist = (verts[0] - orig) * norm;
	if (fabs(dist) >= fabs(maxDist))
		return false;
	Vec3d intersection = orig + dist * norm;

	// now check barycentric coordinates of triangle
	bary[0] = ((verts[2] - verts[1]) ^ (intersection - verts[1])) * norm / baryDenom;
	bary[1] = ((verts[0] - verts[2]) ^ (intersection - verts[2])) * norm / baryDenom;
	bary[2] = 1.0 - bary[1] - bary[0];
	
	// if we're inside the triangle, we're done
	if (bary[0] > -EPSILON && bary[1] > -EPSILON && bary[2] > -EPSILON) {
		which[0] = 0;
		which[1] = 1;
		which[2] = 2;
		pt = bary[0] * verts[0] + bary[1] * verts[1] + bary[2] * verts[2];
	}
	// otherwise, we need to check against edges or points
	else {
		double param;
		int dVert0, dVert1 = -1;
		if (bary[0] < 0) {
			param = (orig - verts[1]) * (verts[2] - verts[1]) / (verts[2] - verts[1]).length2();
			if (param < -EPSILON) {
				dVert0 = 1;
			}
			else if (param > 1+EPSILON) {
				dVert0 = 2;
			}
			else {
				dVert0 = 1;
				dVert1 = 2;
			}
		}
		if (bary[1] < 0 && dVert1 == -1) {
			param = (orig - verts[0]) * (verts[2] - verts[0]) / (verts[2] - verts[0]).length2();
			if (param < -EPSILON) {
				dVert0 = 0;
			}
			else if (param > 1+EPSILON) {
				dVert0 = 2;
			}
			else {
				dVert0 = 0;
				dVert1 = 2;
			}
		}
		if (bary[2] < 0 && dVert1 == -1) {
			param = (orig - verts[0]) * (verts[1] - verts[0]) / (verts[1] - verts[0]).length2();
			if (param < -EPSILON) {
				dVert0 = 0;
			}
			else if (param > 1+EPSILON) {
				dVert0 = 1;
			}
			else {
				dVert0 = 0;
				dVert1 = 1;
			}
		}

		if (dVert1 == -1) {
			// closest point is a vertex
			delta = orig - verts[dVert0];
			dist = delta.length();

			which[0] = dVert0;
			which[1] = -1;
			which[2] = -1;
			pt = verts[dVert0];
			bary[0] = 1.0;
			bary[1] = 0.0;
			bary[2] = 0.0;
		}
		else {
			// closest point is an edge
			pt = (verts[dVert0] + (verts[dVert1] - verts[dVert0]) * param);
			dist = (orig - pt).length();

			which[0] = dVert0;
			which[1] = dVert1;
			which[2] = -1;
			bary[0] = param;
			bary[1] = 1.0 - param;
			bary[2] = 0.0;
		}
	}

	return (fabs(dist) < fabs(maxDist));
}


bool TriMeshInterface::init(int pts, int tris) {
	bool ret = setNumPts(pts);
	ret = ret & setNumTris(tris);
	return ret;
}


TriMesh::TriMesh() {
	hasPtColors = true;
	hasPtConfs = true;
	hasTriColors = false;
	addedTris = 0;
	addedPts = 0;

	m_hbb = NULL;
	xClip = 1e6;
	showColor = false;
	solidColor = Vec3d(0.8, 0.8, 0.8);
	alpha = 1;
	closestRestrictNormal = false;
}

TriMesh::TriMesh(TriMesh *t) {
	hasPtColors = t->hasPtColors;
	hasPtConfs = t->hasPtConfs;
	hasTriColors = t->hasTriColors;
	addedTris = t->addedTris;
	addedPts = t->addedPts;

	m_hbb = NULL;

	init(t->numPts(), t->numTris());

	int i;
	// points
	copyPoints(t);
	// triangle normals
	for (i=0; i < t->triNormals.size(); i++)
		triNormals[i] = triNormals[i];
	// vertex normals
	for (i=0; i < t->ptNormals.size(); i++)
		ptNormals[i] = t->ptNormals[i];
	// triangle indices
	for (i=0; i < t->tris.size(); i++)
		tris[i] = t->tris[i];
}

TriMesh::~TriMesh() {
	if (m_hbb)
		delete m_hbb;
}

// TriMeshInterface methods -------------------------------

bool TriMesh::init(int pts, int tris) {
	if (m_hbb)
		delete m_hbb;
	m_hbb = NULL;

	return TriMeshInterface::init(pts, tris);
}

int TriMesh::numPts() {
	return pts.size();
}

bool TriMesh::setNumPts(int size) {
	pts.resize(size);
	ptNormals.resize(size);
	if (hasPtColors)
		ptColors.resize(size);
	if (hasPtConfs)
		ptConfs.resize(size);

	addedPts = 0;

	return true;
}

void TriMesh::addPoint(const Vec3d &pt) {
	// assume either:
	//  (1) the number of points was initially set using setNumPts
	//  (2) we're pushing onto the end of the vector
	if (addedPts < numPts()) {
		pts[addedPts] = pt;
	}
	else {
		pts.push_back(pt);
		ptNormals.push_back(Vec3d());
		if (hasPtColors)
			ptColors.push_back(Vec3d(1,1,1));
		if (hasPtConfs)
			ptConfs.push_back(1);
	}
	addedPts++;
}

Vec3d &TriMesh::getPt(int index) {
	return pts[index];
}

Vec3d &TriMesh::getPtColor(int index) {
	if (hasPtColors)
		return ptColors[index];
	return TEMP_VEC;
}

Vec3d &TriMesh::getPtNormal(int index) {
	return ptNormals[index];
}

double &TriMesh::getPtConf(int index) {
	if (hasPtConfs)
		return ptConfs[index];
	return TEMP_DOUBLE;
}

int TriMesh::numTris() {
	return tris.size() / 3;
}

bool TriMesh::setNumTris(int size) {
	tris.resize(size*3);
	triNormals.resize(size);
	if (hasTriColors)
		triColors.resize(size);

	addedTris = 0;

	return true;
}

void TriMesh::addTri(int v0, int v1, int v2) {
	// assume either:
	//  (1) the number of triangles was initially set using setNumTris
	//  (2) we're pushing onto the end of the vector
	if (addedTris < numTris()) { 
		tris[addedTris*3 + 0] = v0;
		tris[addedTris*3 + 1] = v1;
		tris[addedTris*3 + 2] = v2;
	}
	else {
		tris.push_back(v0);
		tris.push_back(v1);
		tris.push_back(v2);
		triNormals.push_back(Vec3d());
		if (hasTriColors)
			triColors.push_back(Vec3d(1,1,1));
	}
	addedTris++;
}

int &TriMesh::getTri(int face, int vert) {
	return tris[face*3 + vert];
}

Vec3d &TriMesh::getTriColor(int face) {
	if (hasTriColors)
		return triColors[face];
	return TEMP_VEC;
}

Vec3d &TriMesh::getTriNormal(int face) {
	return triNormals[face];
}

bool TriMesh::gsPtColors(int action) {
	if (action == 0) {
		hasPtColors = false;
		return true;
	}
	else if (action == 1) {
		hasPtColors = true;
		ptColors.resize(numPts());
		return true;
	}
	else {
		return hasPtColors;
	}
}

bool TriMesh::gsPtNormals(int action) {
	if (action == 0) {
		return false;
	}
	else if (action == 1) {
		return true;
	}
	else {
		return true;
	}
}

bool TriMesh::gsPtConfs(int action) {
	if (action == 0) {
		hasPtConfs = false;
		return true;
	}
	else if (action == 1) {
		hasPtConfs = true;
		ptConfs.resize(numPts());
		return true;
	}
	else {
		return hasPtConfs;
	}
}

bool TriMesh::gsTriColors(int action) {
	if (action == 0) {
		hasTriColors = false;
		return true;
	}
	else if (action == 1) {
		hasTriColors = true;
		triColors.resize(numTris());
		return true;
	}
	else {
		return hasTriColors;
	}
}

bool TriMesh::gsTriNormals(int action) {
	if (action == 0) {
		return false;
	}
	else if (action == 1) {
		return true;
	}
	else {
		return true;
	}
}

// other methods ------------------------------------------

void TriMesh::resetMesh() {
	pts.clear();
	ptColors.clear();
	ptConfs.clear();
	ptNormals.clear();
	tris.clear();
	triNormals.clear();
	triColors.clear();
	addedPts = 0;
	addedTris = 0;

	if (m_hbb)
		delete m_hbb;
	m_hbb = NULL;
}

bool TriMesh::loadFile(const char *fname) {
	int l = strlen(fname);

	if (fname[l-3] == 'p' && fname[l-2] == 'l' && fname[l-1] == 'y')
		return loadPly(fname, true);

	ifstream is(fname);
	if (!is.good()) {
		return false;
	}

	bool ret = false;

	if (fname[l-3] == 'o' && fname[l-2] == 'b' && fname[l-1] == 'j')
		ret = loadObj(is);
	if (fname[l-3] == 'r' && fname[l-2] == 'a' && fname[l-1] == 'w')
		ret = loadRaw(is);
	if (fname[l-3] == 't' && fname[l-2] == 'x' && fname[l-1] == 't')
		ret = loadTxt(is);
	if (fname[l-3] == 'w' && fname[l-2] == 'r' && fname[l-1] == 'l')
		ret = loadVrml(is);

	is.close();

	return ret;
}

bool TriMesh::saveFile(const char *fname) {
	int l = strlen(fname);

	if (fname[l-3] == 'p' && fname[l-2] == 'l' && fname[l-1] == 'y')
		return savePly(fname);

	ofstream os(fname);
	if (!os.good()) {
		return false;
	}

	bool ret = false;

	if (fname[l-3] == 'o' && fname[l-2] == 'b' && fname[l-1] == 'j')
		ret = saveObj(os);

	os.close();

	return ret;
}

bool TriMesh::loadRaw(istream &in) {
	Vec3d curPt;
	int v0, v1, v2;
	int count = 0;

	while (1) {
		in >> curPt[0];
		if (!in.good())
			return count > 0;
		in >> curPt[1] >> curPt[2];
		v0 = getPointIndex(curPt);
		in >> curPt[0] >> curPt[1] >> curPt[2];
		v1 = getPointIndex(curPt);
		in >> curPt[0] >> curPt[1] >> curPt[2];
		v2 = getPointIndex(curPt);

		addTri(v0, v1, v2);
		count++;
	}
}

bool TriMesh::loadTxt(istream &in) {
	Vec3d curPt;
	int v0, v1, v2, temp;
	int count = 0;
	char tempS[80];

	while (1) {
		in >> curPt[0];
		if (!in.good())
			break;
		// swap y and z
		in >> curPt[2] >> curPt[1];
		pts.push_back(curPt);
		count++;
	}
	if (count == 0)
		return false;

	count = 0;
	in.clear();
	in >> tempS;
	while (1) {
		in >> v0;
		if (!in.good())
			break;
		in >> v1 >> v2 >> temp;
		addTri(v0, v1, v2);
		count++;
	}

	return (count > 0);
}

bool TriMesh::loadVrml(istream &in) {
	char s[256];

	int ofs;

	while (!in.eof()) {
		in >> s;

		if (strcmp(s, "IndexedFaceSet") == 0) {
			Vec3d color = Vec3d((rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0);
			ofs = numPts();

			while (!in.eof()) {
				in >> s;

				if (strcmp(s, "[") == 0)
					break;
			}

			while (!in.eof()) {
				Vec3d v;

				in >> s;
				if (s[0] == ']')
					break;
				v[0] = atof(s);
				in >> s;
				v[1] = atof(s);
				in >> s;
				if (s[strlen(s)-1] == ',')
					s[strlen(s)-1] = 0;
				v[2] = atof(s);

				addPoint(v);
				getPtColor(numPts()-1) = color;
			}

			while (!in.eof()) {
				in >> s;

				if (strcmp(s, "[") == 0) 
					break;
			}

			while (!in.eof()) {
				int v[3];

				in >> s;
				if (s[0] == ']')
					break;

				v[0] = atoi(s);
				in >> s;
				v[1] = atoi(s);
				in >> s;
				v[2] = atoi(s);
				in >> s;

				addTri(ofs + v[0], ofs + v[1], ofs + v[2]);
			}
		}
	}

	return true;
}

bool TriMesh::loadObj(istream &in) {
	// wavefrom obj format
	// documented here: http://astronomy.swin.edu.au/~pbourke/geomformats/obj/

	// we'll assume it's a triangle mesh and ignore everything except
	// vertices and faces

	char s[256], *curV;
	vector<int> verts;
	while (!in.eof()) {
		in.getline(s, 256);
		if (s[0] == 'v' && s[1] == ' ') {
			Vec3d curPt;

			curV = strtok(s, " ");
			curV = strtok(NULL, " ");
			curPt[0] = atof(curV);
			curV = strtok(NULL, " ");
			curPt[1] = atof(curV);
			curV = strtok(NULL, " \n");
			curPt[2] = atof(curV);

			addPoint(curPt);
		}
		else if (s[0] == 'f') {
			verts.clear();
			curV = strtok(s, " ");
			curV = strtok(NULL, " ");

			while (curV) {
				int j;
				for (j=0; j < strlen(curV); j++) {
					if (curV[j] == '/') {
						break;
						curV[j] = 0;
					}
				}
				verts.push_back(atoi(curV)-1);
				curV = strtok(NULL, " ");
			}

			int i;
			for (i=2; i < verts.size(); i++)
				addTri(verts[0], verts[i - 1], verts[i]);
		}
	}

	return true;
}

bool TriMesh::saveObj(ostream &out) {
	// wavefrom obj format
	// documented here: http://astronomy.swin.edu.au/~pbourke/geomformats/obj/

	// we'll assume it's a triangle mesh and ignore everything except
	// vertices and faces

	int i;

	out << "g default" << endl;
	for (i=0; i < numPts(); i++) {
		out << "v " << getPt(i) << endl;
	}
	for (i=0; i < numTris(); i++) {
		out << "f " << (getTri(i, 0)+1) << " " << (getTri(i, 2)+1) << " " << (getTri(i, 1)+1) << endl;
	}

	return true;
}

struct PlyVertex {
	float x,y,z;
	float conf;
	unsigned char r, g, b;
};

struct PlyFace {
  unsigned char nverts;    /* number of vertex indices in list */
  int *verts;              /* vertex index list */
};

PlyProperty vert_props[] = { // list of property information for a vertex
  {"x", PLY_FLOAT, PLY_FLOAT, offsetof(PlyVertex,x), 0, 0, 0, 0},
  {"y", PLY_FLOAT, PLY_FLOAT, offsetof(PlyVertex,y), 0, 0, 0, 0},
  {"z", PLY_FLOAT, PLY_FLOAT, offsetof(PlyVertex,z), 0, 0, 0, 0},
  {"confidence", PLY_FLOAT, PLY_FLOAT, offsetof(PlyVertex,conf), 0, 0, 0, 0},
  {"diffuse_red", PLY_UCHAR, PLY_UCHAR, offsetof(PlyVertex,r), 0, 0, 0, 0},
  {"diffuse_green", PLY_UCHAR, PLY_UCHAR, offsetof(PlyVertex,g), 0, 0, 0, 0},
  {"diffuse_blue", PLY_UCHAR, PLY_UCHAR, offsetof(PlyVertex,b), 0, 0, 0, 0},
//  {"red", PLY_UCHAR, PLY_UCHAR, offsetof(PlyVertex,r), 0, 0, 0, 0},
  //{"green", PLY_UCHAR, PLY_UCHAR, offsetof(PlyVertex,g), 0, 0, 0, 0},
  //{"blue", PLY_UCHAR, PLY_UCHAR, offsetof(PlyVertex,b), 0, 0, 0, 0},
};

PlyProperty face_props[] = { /* list of property information for a face */
  {"vertex_indices", PLY_INT, PLY_INT, offsetof(PlyFace,verts),
   1, PLY_UCHAR, PLY_UCHAR, offsetof(PlyFace,nverts)},
};

struct PlyTristrip {
  int nverts;    /* number of vertex indices in list */
  int *verts;              /* vertex index list */
};

PlyProperty tristrips_props[] = { /* list of property information for a tristrip */
  {"vertex_indices", PLY_INT, PLY_INT, offsetof(PlyTristrip,verts),
   1, PLY_INT, PLY_INT, offsetof(PlyTristrip,nverts)},
};

bool TriMesh::loadPly(const char *fn, bool color) {
	bool readConf = true;

	FILE *f = fopen(fn, "rb");
	if (f == NULL) {
		cerr << "couldn't open " << fn << endl;
		return false;
	}

	int nelems;
	char **elemNames;
	PlyFile *ply = ply_read(f, &nelems, &elemNames);
	if (ply == NULL) {
		cerr << "couldn't read ply file " << fn << endl;
		return false;
	}

	resetMesh();

	// go through each kind of element that we learned is in the file 
	// and read them
	char *elemName;
	PlyProperty **plist;
	int numElems, nprops;
	int i, j;
	for (i = 0; i < nelems; i++) {
		// get the description of the first element
		elemName = elemNames[i];
		plist = ply_get_element_description(ply, elemName, &numElems, &nprops);

		// if we're on vertex elements, read them in
		if (strcmp("vertex", elemName) == 0) {
			// set up for getting vertex elements
			ply_get_property (ply, elemName, &vert_props[0]);
			ply_get_property (ply, elemName, &vert_props[1]);
			ply_get_property (ply, elemName, &vert_props[2]);
			if (readConf)
				readConf = (ply_get_property (ply, elemName, &vert_props[3]) != 0);
			if (color) {
				ply_get_property (ply, elemName, &vert_props[4]);
				ply_get_property (ply, elemName, &vert_props[5]);
				ply_get_property (ply, elemName, &vert_props[6]);
			}

			// grab all the vertex elements
			for (j = 0; j < numElems; j++) {
				// grab an element from the file
				PlyVertex vert;
				ply_get_element (ply, &vert);
				addPoint(Vec3d(vert.x, vert.y, vert.z));
//				addPoint(Vec3d(vert.x / 300.0, vert.y / 300.0, vert.z / 300.0));
				if (color)
					getPtColor(addedPts-1) = Vec3d(vert.r / DIV_BY, vert.g / DIV_BY, vert.b / DIV_BY);
				if (readConf)
					getPtConf(addedPts-1) = vert.conf;
		
			}
		}

		// if we're on face elements, read them in
		if (strcmp("face", elemName) == 0) {
			// set up for getting face elements
			ply_get_property (ply, elemName, &face_props[0]);
      
			// grab all the face elements
			for (j = 0; j < numElems; j++) {
				// grab an element from the file
				PlyFace face;
				ply_get_element(ply, &face);

				addTri(face.verts[0], face.verts[1], face.verts[2]);

				free(face.verts);
			}
		}

		// if we're on tristrips elements, read them in
		if (strcmp("tristrips", elemName) == 0) {
			// set up for getting tristrips
			ply_get_property (ply, elemName, &tristrips_props[0]);
      
			// grab all the face elements
			for (j = 0; j < numElems; j++) {
				// grab an element from the file
				PlyTristrip strip;
				ply_get_element(ply, &strip);

				bool clockwise = true;
				for (i=2; i < strip.nverts; i++) {
					if (strip.verts[i] == -1) {
						i += 2;
						clockwise = true;
					}
					else {
						if (clockwise)
							addTri(strip.verts[i-2], strip.verts[i-1], strip.verts[i]);
						else
							addTri(strip.verts[i-2], strip.verts[i], strip.verts[i-1]);
						clockwise = !clockwise;
					}
				}

				free(strip.verts);
			}
		}
	}

	ply_close(ply);

//	cout << "loaded " << curTris.size() << " vertices, " << m_tris.size() << " triangles" << endl;

	return true;
}

bool TriMesh::savePly(const char *fn) {
	int i;

	FILE *f = fopen(fn, "wb");
	if (f == NULL) {
		cerr << "couldn't open " << fn << endl;
		return false;
	}

	char **elemNames = new char*[2];
	elemNames[0] = new char[20];
	strcpy(elemNames[0], "vertex");
	elemNames[1] = new char[20];
	strcpy(elemNames[1], "face");

	PlyFile *ply = ply_write(f, 2, elemNames, PLY_BINARY_LE);
	if (ply == NULL) {
		cerr << "couldn't write ply file " << fn << endl;
		return false;
	}

	ply_describe_element(ply, elemNames[0], numPts(), 7, vert_props);
	ply_describe_element(ply, elemNames[1], numTris(), 1, face_props);
	ply_header_complete(ply);

	// write vertices
	ply_put_element_setup(ply, elemNames[0]);
	PlyVertex vertex;
	for (i=0; i < numPts(); i++) {
		vertex.x = pts[i][0];
		vertex.y = pts[i][1];
		vertex.z = pts[i][2];
		if (hasPtColors) {
			vertex.r = floor(ptColors[i][0]*255+0.5);
			vertex.g = floor(ptColors[i][1]*255+0.5);
			vertex.b = floor(ptColors[i][2]*255+0.5);
		}
		if (hasPtConfs) {
			vertex.conf = ptConfs[i];
		}
		ply_put_element(ply, &vertex);
	}

	// write triangles
	ply_put_element_setup(ply, elemNames[1]);
	PlyFace face;
	face.nverts = 3;
	face.verts = new int[3];
	for (i=0; i < numTris(); i++) {
		face.verts[0] = getTri(i, 0);
		face.verts[1] = getTri(i, 1);
		face.verts[2] = getTri(i, 2);
		ply_put_element(ply, &face);
	}
	delete []face.verts;

	ply_close(ply);

	return true;
}

void TriMesh::makeCylinder(double len, double rad, int lenSegs, int radSegs, double eRad, double zMult) {
	int i, j, k;
	double lenStep = len / (lenSegs-1);
	double radStep = 2.0 * 3.14159265358979 / radSegs;
	double radOfs;
	Vec3d curPt;

	// make the points
	for (i=0; i < lenSegs; i++) {
/*		if (i%2)
			radOfs = radStep/2.0;
		else
*/			radOfs = 0.0;

		double curR, ratio;
		if (eRad > 0) {
			ratio = (double)i / (double)(lenSegs-1.0);
			curR = rad + (eRad - rad) * ratio;
		}
		else
			curR = rad;
		
		for (j=0; j < radSegs; j++) {
			curPt[0] = i * lenStep;
			curPt[1] = curR * cos(j*radStep+radOfs) - (curR - rad);
			curPt[2] = curR * sin(j*radStep+radOfs) * zMult;
			pts.push_back(curPt);
		}
	}

	// make the triangles
	for (i=1; i < lenSegs; i++) {
		for (j=0; j < radSegs; j++) {
			k = (j + 1) % radSegs;
//			if (i%2) {
				addTri(radSegs*i + j, radSegs*(i-1) + k, radSegs*(i-1) + j);
				addTri(radSegs*i + j, radSegs*i + k, radSegs*(i-1) + k);
/*			}
			else {
				addTri(radSegs*i + j, radSegs*i + k, radSegs*(i-1) + j);
				addTri(radSegs*i + k, radSegs*(i-1) + k, radSegs*(i-1) + j);
			}
*/		}
	}
}

void TriMesh::calcNormals() {
	int i;
	Vec3d pt0, pt1, pt2, norm;

	for (i=0; i < numPts(); i++)
		getPtNormal(i) = Vec3d();

	for (i=0; i < numTris(); i++) {
		pt0 = getPt(getTri(i, 0));
		pt1 = getPt(getTri(i, 1));
		pt2 = getPt(getTri(i, 2));

		norm = -(pt1-pt0)^(pt2-pt0);
		norm.normalize();
		getTriNormal(i) = norm;
		getPtNormal(getTri(i, 0)) += norm;
		getPtNormal(getTri(i, 1)) += norm;
		getPtNormal(getTri(i, 2)) += norm;
	}

	for (i=0; i < numPts(); i++)
		getPtNormal(i).normalize();
}

void TriMesh::scale(double xs, double ys, double zs) {
	// scale all vertices
	int i, n = numPts();
	for (i = 0; i < n; i++) {
		getPt(i)[0] *= xs;
		getPt(i)[1] *= ys;
		getPt(i)[2] *= zs;
	}
}

void TriMesh::transform(Mat4d &m) {
	// transform all vertices
	Vec4d p;
	int i, n = numPts();
	for (i = 0; i < n; i++) {
		p[0] = getPt(i)[0];
		p[1] = getPt(i)[1];
		p[2] = getPt(i)[2];
		p[3] = 1.0;
		p = m * p;
		p /= p[3];
		getPt(i)[0] = p[0];
		getPt(i)[1] = p[1];
		getPt(i)[2] = p[2];
	}
}

void TriMesh::translate(double xt, double yt, double zt) {
	// translate all vertices
	int i, n = numPts();
	for (i = 0; i < n; i++) {
		getPt(i)[0] += xt;
		getPt(i)[1] += yt;
		getPt(i)[2] += zt;
	}
}

void TriMesh::copyPoints(TriMesh *source) {
	// copy vertices from another TriMesh
	int i, n = source->numPts();
	setNumPts(n);
	for (i=0; i < n; i++) {
		getPt(i) = source->getPt(i);
	}
}

int TriMesh::getPointIndex(Vec3d pt) {
	/*
	ptVec::iterator pi;
	int i = 0;
	for (pi = m_points.begin(); pi != m_points.end(); pi++) {
		if (*pi == pt)
			return i;
		i++;
	}*/
	// point not found
	pts.push_back(pt);
	return pts.size()-1;
}

void TriMesh::calcTriBounds(double *bb, intVec &tris) {
	// x bounds
	bb[0] = 1e6;
	bb[1] = -1e6;
	// y bounds
	bb[2] = 1e6;
	bb[3] = -1e6;
	// z bounds
	bb[4] = 1e6;
	bb[5] = -1e6;

	double x, y, z;
	int i;
	for (i = 0; i < tris.size(); i++) {
		Vec3d curPt = getPt(tris[i]);
		x = curPt[0];
		y = curPt[1];
		z = curPt[2];
		if (x < bb[0])
			bb[0] = x;
		if (x > bb[1])
			bb[1] = x;
		if (y < bb[2])
			bb[2] = y;
		if (y > bb[3])
			bb[3] = y;
		if (z < bb[4])
			bb[4] = z;
		if (z > bb[5])
			bb[5] = z;
	}
}

void TriMesh::calcPointBounds(double *bb, int *pts, int numPts) {
	int pt, i;

	for (i=0; i < 3; i++) {
		bb[i*2+0] = 1e6;
		bb[i*2+1] = -1e6;
	}

	for (pt = 0; pt < numPts; pt++) {
		for (i=0; i < 3; i++) {
			if (getPt(pts[pt])[i] < bb[i*2+0])
				bb[i*2+0] = getPt(pts[pt])[i];
			if (getPt(pts[pt])[i] > bb[i*2+1])
				bb[i*2+1] = getPt(pts[pt])[i];
		}
	}
}

void TriMesh::calcPointBounds(double *bb) {
	int pt, i;

	for (i=0; i < 3; i++) {
		bb[i*2+0] = 1e6;
		bb[i*2+1] = -1e6;
	}

	for (pt = 0; pt < numPts(); pt++) {
		for (i=0; i < 3; i++) {
			Vec3d curPt = getPt(pt);
			if (curPt[i] < bb[i*2+0])
				bb[i*2+0] = curPt[i];
			if (curPt[i] > bb[i*2+1])
				bb[i*2+1] = curPt[i];
		}
	}
}

void TriMesh::dumpBounds() {
	double bounds[6];
	calcPointBounds(bounds);
	cout << bounds[0] << ", " << bounds[1] << endl;
	cout << bounds[2] << ", " << bounds[3] << endl;
	cout << bounds[4] << ", " << bounds[5] << endl;
}

void TriMesh::calcHBB(int maxSize) {
	if (m_hbb)
		delete m_hbb;

	m_hbb = calcHBBRecursive(maxSize, tris);
}

static double findSplit(vector<int> &tris, vector<Vec3d> &pts, int dim) {
	int i;
	vector<double> toSort;

	for (i=0; i < tris.size(); i += 3) {
		double m = dMax(pts[tris[i + 0]][dim], dMax(pts[tris[i + 1]][dim], pts[tris[i + 2]][dim]));
		toSort.push_back(m);
	}
	sort(toSort.begin(), toSort.end());
	return toSort[toSort.size() / 2];
}

static int cmpTriDim;
static vector<int> *cmpTris;
static vector<Vec3d> *cmpTriPts;

static int cmpTri(const void *a, const void *b) {
	int aInd = *((int*)a);
	int bInd = *((int*)b);
	double aMax = dMax((*cmpTriPts)[(*cmpTris)[aInd + 0]][cmpTriDim], 
		dMax((*cmpTriPts)[(*cmpTris)[aInd + 1]][cmpTriDim], (*cmpTriPts)[(*cmpTris)[aInd + 2]][cmpTriDim]));
	double bMax = dMax((*cmpTriPts)[(*cmpTris)[bInd + 0]][cmpTriDim], 
		dMax((*cmpTriPts)[(*cmpTris)[bInd + 1]][cmpTriDim], (*cmpTriPts)[(*cmpTris)[bInd + 2]][cmpTriDim]));
	
	if (aMax > bMax)
		return 1;
	else if (aMax < bMax)
		return -1;
	else 
		return 0;
}

HBBox *TriMesh::calcHBBRecursive(int maxSize, intVec &tris) {
	int i;

	HBBox *ret = new HBBox();

	if (tris.size() > maxSize) {
		intVec v0, v1;
		int maxDim;
		double split;

		// step 1: determine the largest dimension
		double mySize[6];
		calcTriBounds(mySize, tris);
		double xSize = fabs(mySize[1] - mySize[0]);
		double ySize = fabs(mySize[3] - mySize[2]);
		double zSize = fabs(mySize[5] - mySize[4]);
		if (ySize > xSize) {
			if (zSize > ySize) {
				maxDim = 2;
				split = mySize[4] + zSize / 2.0;
			}
			else {
				maxDim = 1;
				split = mySize[2] + ySize / 2.0;
			}
		}
		else {
			if (zSize > xSize) {
				maxDim = 2;
				split = mySize[4] + zSize / 2.0;
			}
			else {
				maxDim = 0;
				split = mySize[0] + xSize / 2.0;
			}
		}

		int *indices = new int[tris.size() / 3];
		for (i=0; i < tris.size(); i+=3)
			indices[i/3] = i;
		cmpTriDim = maxDim;
		cmpTris = &tris;
		cmpTriPts = &pts;
		qsort(indices, tris.size()/3, sizeof(int), cmpTri);

		for (i=0; i < tris.size() / 6; i++) {
			v0.push_back(tris[indices[i] + 0]);
			v0.push_back(tris[indices[i] + 1]);
			v0.push_back(tris[indices[i] + 2]);
		}
		for (; i < tris.size() / 3; i++) {
			v1.push_back(tris[indices[i] + 0]);
			v1.push_back(tris[indices[i] + 1]);
			v1.push_back(tris[indices[i] + 2]);
		}
		delete []indices;
/*
		for (i = 0; i < tris.size(); i += 3) {
			int whichSide = 0;
			if (m_points[tris[i + 0]][maxDim] > split)
				whichSide++;
			if (m_points[tris[i + 1]][maxDim] > split)
				whichSide++;
			if (m_points[tris[i + 2]][maxDim] > split)
				whichSide++;

			if (whichSide < 2) {
				v0.push_back(tris[i + 0]);
				v0.push_back(tris[i + 1]);
				v0.push_back(tris[i + 2]);
			}
			else {
				v1.push_back(tris[i + 0]);
				v1.push_back(tris[i + 1]);
				v1.push_back(tris[i + 2]);
			}
		}*/
//		cerr << "split: " << v0.size() << "/" << v1.size() << endl;

		if (v0.size() < 3 || v1.size() < 3) {
			for (i=0; i < tris.size(); i++)
				ret->mVertices.push_back(tris[i]);
			calcTriBounds(ret->mBox, tris);
		}
		else {
			ret->mChildren[0] = calcHBBRecursive(maxSize, v0);
			ret->mChildren[1] = calcHBBRecursive(maxSize, v1);
			bbUnion(ret->mChildren[0]->mBox, ret->mChildren[1]->mBox, ret->mBox);
		}
	}
	else {
		for (i=0; i < tris.size(); i++)
			ret->mVertices.push_back(tris[i]);
		calcTriBounds(ret->mBox, tris);
	}
	return ret;
}

bool TriMesh::calcRayIntersection(Vec3d orig, Vec3d dir) {
	tPos = 1e6;
	tNeg = -1e6;
	hitPos = false;
	hitNeg = false;
	rayOrig = orig;
	rayDir = dir;

	if (m_hbb) {
		if (m_hbb->rayIntersect(rayOrig, rayDir)) // || m_hbb->rayIntersect(rayOrig, -rayDir))
			calcRayIntersection(m_hbb);
	}
	else {
		calcRayIntersection(tris);
	}

	return hitPos || hitNeg;
}

void TriMesh::calcRayIntersection(intVec &curTris) {
	int j;
	double u, v;
	double curDist;
	bool hit = false;
	bool curAway;

	for (j=0; j < curTris.size(); j += 3) {
		curDist = 1e6;
		if (intersect_triangle(rayOrig, rayDir,
			   pts[curTris[j + 2]],
			   pts[curTris[j + 1]],
               pts[curTris[j + 0]],
               &curDist, &u, &v, &curAway)) {

			if (curDist < 0) {
				if (curDist > tNeg) {
					tNeg = curDist;
					hitNeg = curAway; //true;
				}
			}
			else {
				if (curDist < tPos) {
					tPos = curDist;
					hitPos = curAway; //true;
				}
			}
		}
	}
}

void TriMesh::calcRayIntersection(HBBox *hbb) {
	if (hbb->mChildren[0] == NULL) {
		calcRayIntersection(hbb->mVertices);
	}
	else {

		if (hbb->mChildren[0]->rayIntersect(rayOrig, rayDir)) { // || hbb->mChildren[0]->rayIntersect(rayOrig, -rayDir)) {
			calcRayIntersection(hbb->mChildren[0]);
		}

		if (hbb->mChildren[1]->rayIntersect(rayOrig, rayDir)) { // || hbb->mChildren[0]->rayIntersect(rayOrig, -rayDir)) {
			calcRayIntersection(hbb->mChildren[1]);
		}
	}
}

bool TriMesh::calcClosestPoint(Vec3d orig, double closest, Vec3d normalMatch) {
	closestDist = closest;
	rayOrig = orig;
	if (m_hbb) {
		return calcClosestPoint(m_hbb);
	}
	else {
		return calcClosestPoint(tris);
	}
}

bool TriMesh::calcClosestPoint(intVec &curTris) {
	int i, j;
	Vec3d verts[3];
	bool found = false;

	for (j=0; j < curTris.size(); j += 3) {
		Vec3d pt, bary;
		double dist;
		int tri[3];

		verts[0] = getPt(curTris[j + 0]);
		verts[1] = getPt(curTris[j + 1]);
		verts[2] = getPt(curTris[j + 2]);

		if (closestRestrictNormal) {
			double mult = 1.0;
			Vec3d norm = -(verts[1] - verts[0]) ^ (verts[2] - verts[0]);
			norm.normalize();
			mult = closestNormalRestriction * norm;
			if (mult <= EPSILON)
				continue;
		}

		if (pointToTriDist(rayOrig, verts, closestDist, dist, pt, bary, tri)) {
			found = true;
			closestDist = dist;
			closestPt = pt;
			closestBary = bary;
			for (i=0; i < 3; i++)
				if (tri[i] == -1)
					closestTri[i] = -1;
				else
					closestTri[i] = curTris[j + tri[i]];
		}
	}

	return found;
}

bool TriMesh::calcClosestPoint(HBBox *hbb) {
	if (hbb->mChildren[0] == NULL) {
		return calcClosestPoint(hbb->mVertices);
	}

	double d1 = bbDistance(rayOrig, hbb->mChildren[0]->mBox);
	double d2 = bbDistance(rayOrig, hbb->mChildren[1]->mBox);
	bool found = false;

	if (d1 < d2) {
		if (d1 < closestDist)
			found |= calcClosestPoint(hbb->mChildren[0]);
		if (d2 < closestDist)
			found |= calcClosestPoint(hbb->mChildren[1]);
	}
	else {
		if (d2 < closestDist)
			found |= calcClosestPoint(hbb->mChildren[1]);
		if (d1 < closestDist)
			found |= calcClosestPoint(hbb->mChildren[0]);
	}

	return found;
}

void TriMesh::blur() {
	int i, j;
	int n = numPts();

	vector<Vec3d> origPts;
	origPts = pts;
	vector<int> counts;
	counts.resize(n);
	for (i=0; i < n; i++) {
		counts[i] = 0;
		getPt(i) = Vec3d();
	}

	for (i=0; i < tris.size(); i += 3) {
		for (j=0; j < 3; j++) {
			int v0 = tris[i + j];
			int v1 = tris[i + (j+1)%3];

			getPt(v0) += origPts[v0] + origPts[v1];
			counts[v0] += 2;
			getPt(v1) += origPts[v0] + origPts[v1];
			counts[v1] += 2;
		}
	}

	for (i=0; i < n; i++) {
		if (counts[i] > 0)
			getPt(i) /= counts[i];
	}
}

/*
void testTrimesh() {
	TriMesh tm;

	tm.m_points.push_back(Vec3d(0, 1, 0.8));
	tm.m_points.push_back(Vec3d(1, -1, 0));
	tm.m_points.push_back(Vec3d(-1, -1, 0));

	tm.m_points.push_back(Vec3d(0, 1, 1.2));
	tm.m_points.push_back(Vec3d(1, -1, 1.5));
	tm.m_points.push_back(Vec3d(-1, -1, 1.5));

	tm.m_tris.push_back(0);
	tm.m_tris.push_back(1);
	tm.m_tris.push_back(2);
	tm.m_tris.push_back(3);
	tm.m_tris.push_back(4);
	tm.m_tris.push_back(5);

	tm.calcHBB(10);

	tm.calcRayIntersection(Vec3d(0, 0, 1), Vec3d(0.1, 0.1, 1));
	cout << tm.hitNeg << " " << tm.tNeg << "; " << tm.hitPos << " " << tm.tPos << endl;
	tm.calcRayIntersection(Vec3d(0, 0, 1), Vec3d(0.1, 0.1, -1));
	cout << tm.hitNeg << " " << tm.tNeg << "; " << tm.hitPos << " " << tm.tPos << endl;
}
*/