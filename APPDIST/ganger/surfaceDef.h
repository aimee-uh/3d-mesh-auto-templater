#ifndef SURFACE_DEF_H
#define SURFACE_DEF_H

#include "vec.h"
#include "solver.h"
#include "edgelist.h"
#include "trimesh_util.h"

class TriMesh;
class MarkerSet;
class MeshEdge;

class LADeformationGoalFunction : public IDifferentiableFunction {
public:
	TriMesh *targetTM, *curMesh;
	vector <int> *curMeshNeigh;
	vector <double> *curMeshNeighWeights;
	Vec3d *origVerts;
	double lastErr;
	double *restAreas;

	vector<int> markerRefs;
	MarkerSet *sMarkers, *tMarkers;

	int capRate, stepCount;

	bool smatchShowError;
	bool globalTrans;
	double surfaceMatchWeight, smoothnessWeight, markerMatchWeight;
	double *neighWeights;

	double curDistance;
	Vec3d curGradient;
	Vec3d curClosestPt;
	double curSurfaceWeight;

	char *vertList;
	EdgeList *edgeList;

	Vecd vars, grad;
	int varsPerVert;

	bool lockShape;
	Vec3d *lockNormals;
	double *conf;

	double bendWeight;
	
	LADeformationGoalFunction(TriMesh *mesh, vector <int>* neigh = NULL);
	~LADeformationGoalFunction();

	void prepareTriMesh(TriMesh *dtm);
	virtual void applyDef(Vecd &variables);
	virtual void zeroDeformation();
	void templateFromCur();

	void updateCurrent(int pt);
	virtual void addGradientVec(int index, Vec3d v);

	virtual double evaluateFunction(Vecd& variables);
	virtual void evaluateGradient(Vecd& variables, Vecd& gradient);
	virtual void solverStep();
};

#endif