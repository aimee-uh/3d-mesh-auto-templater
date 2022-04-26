#ifndef MARKERS_H
#define MARKERS_H

#include <iostream>
#include <vector>
using namespace std;

#include "vec.h"
#include "mat.h"
#include "quatnorm.h"

class SkelTransform;
class TriMesh;

const int mkrPOS = 0;
const int mkrPT = 1;
const int mkrBARY = 2;

// ------------- MarkerData ------------

class Marker {
public:
	char name[40];
	Vec3d pos;
	Vec3d color;
	
	int index;
	int kind;
	int baryVerts[3];
	Vec3d baryPos;
	TriMesh *mesh;

	Marker();
	Marker(Vec3d pt);
	Vec3d curPos();
	void convert(int newMode);

	//void drawGL();
};

class MarkerSet {
public:
	vector<Marker> markers;

	MarkerSet();

	void init(int numM);
	void zap();

	int numMarkers();
	void setColor(const Vec3d color);

	Vec3d v(int marker);
	Vec3d &c(int marker);

	//void drawGL();

	void load(const char *fname);
	void save(const char *fname);

	void loadFromLandmarks(const char *fname);
	void loadText(const char *fname);
	void loadFromMkr(const char *fname);
	void loadNames(const char *fname);
};

#endif