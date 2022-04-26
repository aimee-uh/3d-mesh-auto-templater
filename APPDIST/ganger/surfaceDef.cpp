//#include "ganger.h"
#include <float.h>
#include "surfaceDef.h"
#include "trimesh.h"
#include "trimesh_util.h"
#include "markers.h"
#include "ba.h"
#include "quatnorm.h"

//bool recordOptimization = true;
bool recordOptimization = false;

//#define SPLIT_TRANSFORMATION


// LADeformationGoalFunction function ===============================

LADeformationGoalFunction::LADeformationGoalFunction(TriMesh *mesh, vector<int> *neigh) {
	int i, j;

	curMesh = mesh;
	if (neigh)
		curMeshNeigh = neigh;
	else
		curMeshNeigh = findTMNeighbors(curMesh);

	curMeshNeighWeights = new vector<double>[curMesh->numPts()];
	double avg = 0;
	int count = 0;
	for (i=0; i < curMesh->numPts(); i++) {
		for (j=0; j < (int)curMeshNeigh[i].size(); j++) {
			double dist = (curMesh->getPt(i) - curMesh->getPt(curMeshNeigh[i][j])).length();
			avg += dist;
			count++;
			curMeshNeighWeights[i].push_back(dist);
		}
	}
	avg /= count;
	for (i=0; i < curMesh->numPts(); i++) {
		for (j=0; j < (int)curMeshNeigh[i].size(); j++) {
			if (curMeshNeighWeights[i][j] < 1e-8)
				curMeshNeighWeights[i][j] = avg / 1e-8;
			else
				curMeshNeighWeights[i][j] = avg / curMeshNeighWeights[i][j];
		}
	}

	origVerts = new Vec3d[curMesh->numPts()];
	for (i=0; i < curMesh->numPts(); i++)
		origVerts[i] = curMesh->getPt(i);

	neighWeights = new double[curMesh->numPts()];
	for (i=0; i < curMesh->numPts(); i++) {
		neighWeights[i] = 1;
	}

	lockShape = true;
	smatchShowError = false;
	lockNormals = NULL;

	globalTrans = false;
	surfaceMatchWeight = 0.0;
	smoothnessWeight = 0.0;
	markerMatchWeight = 1.0;

	stepCount = 0;
	capRate = 1;

	vertList = NULL;
	sMarkers = NULL;
	tMarkers = NULL;

	edgeList = NULL;

#ifdef SPLIT_TRANSFORMATION
	varsPerVert = 10;
#else
	varsPerVert = 12;
#endif

	vars.resize(curMesh->numPts() * varsPerVert, true);
	grad.resize(curMesh->numPts() * varsPerVert);

	conf = new double[curMesh->numPts()];
}

LADeformationGoalFunction::~LADeformationGoalFunction() {
	if (vertList)
		delete []vertList;
	if (conf)
		delete []conf;
	if (neighWeights)
		delete []neighWeights;
	if (edgeList)
		delete edgeList;
	if (vertList)
		delete []vertList;
	if (lockNormals)
		delete []lockNormals;
	if (restAreas)
		delete []restAreas;
}

void LADeformationGoalFunction::prepareTriMesh(TriMesh *dtm) {
	targetTM = dtm;

	if (edgeList)
		delete edgeList;
	edgeList = new EdgeList();
	edgeList->buildFromTriMesh(*targetTM);
	if (vertList)
		delete []vertList;
	vertList = new char[edgeList->numVerts];
	edgeList->markVerts(vertList);
}

void LADeformationGoalFunction::applyDef(Vecd &variables) {
	int i;

	if (&variables != &vars)
		vars = variables;

	for (i=0; i < variables.size(); i += varsPerVert) {
		Vec3d cp = origVerts[i/varsPerVert];

#ifdef SPLIT_TRANSFORMATION
		// scale
		Vec3d result = prod(Vec3d(variables[i+4], variables[i+5], variables[i+6]), cp);
		// rotation
		result = QuatNorm(variables[i+0], variables[i+1], variables[i+2], variables[i+3]).toMatrixD() * result;
		// translation
		result[0] += variables[i+7];
		result[1] += variables[i+8];
		result[2] += variables[i+9];

		curMesh->getPt(i/varsPerVert) = result;
#else
		if (globalTrans) {
			curMesh->getPt(i/varsPerVert) = Vec3d( 
				variables[0] * cp[0] + variables[1] * cp[1] + variables[2] * cp[2] + variables[3],
				variables[4] * cp[0] + variables[5] * cp[1] + variables[6] * cp[2] + variables[7],
				variables[8] * cp[0] + variables[9] * cp[1] + variables[10] * cp[2] + variables[11]);
		}
		else {
			curMesh->getPt(i/varsPerVert) = Vec3d( 
				variables[i+0] * cp[0] + variables[i+1] * cp[1] + variables[i+ 2] * cp[2] + variables[i+3],
				variables[i+4] * cp[0] + variables[i+5] * cp[1] + variables[i+ 6] * cp[2] + variables[i+7],
				variables[i+8] * cp[0] + variables[i+9] * cp[1] + variables[i+10] * cp[2] + variables[i+11]);
		}
#endif
	}
	curMesh->calcNormals();
}

void LADeformationGoalFunction::zeroDeformation() {
	if (vars.size() != curMesh->numPts() * varsPerVert) {
		vars.resize(curMesh->numPts() * varsPerVert);
		grad.resize(curMesh->numPts() * varsPerVert);
	}

	vars.zeroElements();

	int i;
	for (i=0; i < curMesh->numPts(); i++) {
#ifdef SPLIT_TRANSFORMATION
		vars[i*varsPerVert + 3] = 1;
		vars[i*varsPerVert + 4] = 1;
		vars[i*varsPerVert + 5] = 1;
		vars[i*varsPerVert + 6] = 1;
#else
		vars[i*varsPerVert + 0] = 1;
		vars[i*varsPerVert + 5] = 1;
		vars[i*varsPerVert + 10] = 1;
#endif
	}
}

void LADeformationGoalFunction::templateFromCur() {
	int i;
	for (i=0; i < curMesh->numPts(); i++)
		origVerts[i] = curMesh->getPt(i);
	zeroDeformation();
}

const double CONF_MULT = 5.0;
const double NORMAL_TOL = cos(90 * DEG_TO_RAD); // was 60

void LADeformationGoalFunction::updateCurrent(int pt) {
	curClosestPt = Vec3d();

	curDistance = 0;
	curSurfaceWeight = 0;
	curGradient = Vec3d();

	targetTM->closestRestrictNormal = true; //IYTIAN 7/9/2020 THIS CONTROLS NORMAL CHECKS
//		targetTM->closestRestrictNormal = false;
	if (lockNormals)
		targetTM->closestNormalRestriction = lockNormals[pt];
	else
		targetTM->closestNormalRestriction = curMesh->getPtNormal(pt);
//		targetTM->closestNormalRestriction.normalize();

//#define CAST_RAYS
	/*
if (pt >= 32*32*76 && pt < 32*32*77) {
//#ifdef CAST_RAYS
	if (targetTM->calcRayIntersection(curMesh->evalPts[pt].dispPos, -curMesh->evalPts[pt].dispNorm)) {
		if (targetTM->hitPos && (!targetTM->hitNeg || targetTM->tPos < fabs(targetTM->tNeg)) && targetTM->tPos < 0.1) {
//				curClosestPt = curMesh->evalPts[pt].dispPos + hitPos*curMesh->evalPts[pt].dispNorm;
			curDistance = targetTM->tPos;
			curGradient = targetTM->tPos * curMesh->evalPts[pt].dispNorm;
		}
		else if (targetTM->hitNeg && -targetTM->tNeg < 0.1) {
//				curClosestPt = curMesh->evalPts[pt].dispPos + hitNeg*curMesh->evalPts[pt].dispNorm;
			curDistance = -targetTM->tNeg;
			curGradient = targetTM->tNeg*curMesh->evalPts[pt].dispNorm;
		}
	}
}
//#else
else {*/
	if (targetTM->calcClosestPoint(curMesh->getPt(pt), 1.10)) {
		curClosestPt = targetTM->closestPt;
//		cout << pt << " " << curClosestPt << endl;

		Vec3d delta = targetTM->closestPt - curMesh->getPt(pt);
		delta.normalize();

		// if closest point is a vertex, check if we need to flip the sign
		if (targetTM->closestTri[1] == -1) {
//				curSurfaceWeight = 1.0 - vertList[targetTM->closestTri[0]];
			curSurfaceWeight = targetTM->getPtConf(targetTM->closestTri[0]);
//				if (curSurfaceWeight < 0 || curSurfaceWeight > 1) {
//					cout << curSurfaceWeight << ": " << targetTM->closestBary[0] << ", " << vertList[targetTM->closestTri[0]] << endl;
//				}

			double dotProd = delta * targetTM->getPtNormal(targetTM->closestTri[0]);
			if (dotProd < 0)
				targetTM->closestDist *= -1;

			// check if this vertex adjoins a hole
			if (vertList[targetTM->closestTri[0]]) {
				curSurfaceWeight = 0;
				return;
			}

			// check normal match
			if (targetTM->closestNormalRestriction * targetTM->getPtNormal(targetTM->closestTri[0]) < NORMAL_TOL) {
				curSurfaceWeight = 0;
				return;
			}
		}
		// if closest point is an edge, check if we need to flip the sign
		else if (targetTM->closestTri[2] == - 1) {
			curSurfaceWeight = targetTM->closestBary[0] * targetTM->getPtConf(targetTM->closestTri[0]) + targetTM->closestBary[1] * targetTM->getPtConf(targetTM->closestTri[1]);
//				curSurfaceWeight = 1.0 - (targetTM->closestBary[0] * vertList[targetTM->closestTri[0]] + targetTM->closestBary[1] * vertList[targetTM->closestTri[1]]);
//				if (curSurfaceWeight < 0 || curSurfaceWeight > 1) {
//					cout << curSurfaceWeight << ": " << targetTM->closestBary[0] << ", " << vertList[targetTM->closestTri[0]] << "; " << targetTM->closestBary[1] << ", " << vertList[targetTM->closestTri[1]] << endl;
//				}

			EdgeInfo *ei = edgeList->findEdge(targetTM->closestTri[0], targetTM->closestTri[1]);
			if (!ei) {
				cout << "error: unknown edge!!" << endl;
				return;
			}
			double dotProd = delta * ei->normal;
			if (dotProd < 0)
				targetTM->closestDist *= -1;

			// check if this edge adjoins a hole
			if (ei->count == 1) {
				curSurfaceWeight = 0;
				return;
			}

			// check normal match
			if (targetTM->closestNormalRestriction * ei->normal < NORMAL_TOL) {
				curSurfaceWeight = 0;
				return;
			}
		}
		else {
			curSurfaceWeight = 
				targetTM->closestBary[0] * targetTM->getPtConf(targetTM->closestTri[0]) + 
				targetTM->closestBary[1] * targetTM->getPtConf(targetTM->closestTri[1]) + 
				targetTM->closestBary[2] * targetTM->getPtConf(targetTM->closestTri[2]);
//				curSurfaceWeight = 1.0 - (targetTM->closestBary[0] * vertList[targetTM->closestTri[0]] + targetTM->closestBary[1] * vertList[targetTM->closestTri[1]] + targetTM->closestBary[2] * vertList[targetTM->closestTri[2]]);
//				if (curSurfaceWeight < 0 || curSurfaceWeight > 1) {
//					cout << curSurfaceWeight << ": " << targetTM->closestBary[0] << ", " << vertList[targetTM->closestTri[0]] << "; " << targetTM->closestBary[1] << ", " << vertList[targetTM->closestTri[1]] << "; " << targetTM->closestBary[2] << ", " << vertList[targetTM->closestTri[2]] << endl;
//				}

			Vec3d verts[3];
			verts[0] = targetTM->getPt(targetTM->closestTri[0]);
			verts[1] = targetTM->getPt(targetTM->closestTri[1]);
			verts[2] = targetTM->getPt(targetTM->closestTri[2]);
			Vec3d norm = -(verts[1] - verts[0]) ^ (verts[2] - verts[0]);
			norm.normalize();

			// check normal match
			if (targetTM->closestNormalRestriction * norm < NORMAL_TOL) {
				curSurfaceWeight = 0;
				return;
			}
		}

		curDistance = targetTM->closestDist;
		curGradient = curMesh->getPt(pt) - targetTM->closestPt;
	}
//}

//		double x = max(0, baMin(1.0, curDistances[pt] / 0.1 + 0.5));
//		defMesh->m_colors[pt] = Vec3d(x, x, x);
}

void LADeformationGoalFunction::addGradientVec(int index, Vec3d v) {
	Vec3d cp = origVerts[index];
	int ofs = index*varsPerVert;

	if (globalTrans)
		ofs = 0;

#ifdef SPLIT_TRANSFORMATION
	int i;
	Mat4d qVal;
	Mat4d qCurDerivs[4];
	QuatNorm(vars[ofs+0], vars[ofs+1], vars[ofs+2], vars[ofs+3]).getMatrices(qVal, qCurDerivs);
	Vec3d scaleVec = Vec3d(vars[ofs+4], vars[ofs+5], vars[ofs+6]);
	Vec3d scalePt = prod(scaleVec, cp);

	grad[ofs+4 + 0] = (qVal * prod(Vec3d(1, 0, 0), cp)) * v;
	grad[ofs+4 + 1] = (qVal * prod(Vec3d(0, 1, 0), cp)) * v;
	grad[ofs+4 + 2] = (qVal * prod(Vec3d(0, 0, 1), cp)) * v;

//	for (i=0; i < 4; i++)
//		grad[ofs + i] = (qCurDerivs[i] * scalePt) * v;

	for (i=0; i < 3; i++)
		grad[ofs+7 + i] = v[i];
#else
		grad[ofs + 0] += cp[0] * v[0];
		grad[ofs + 1] += cp[1] * v[0];
		grad[ofs + 2] += cp[2] * v[0];
	grad[ofs + 3] += v[0];

		grad[ofs + 4] += cp[0] * v[1];
		grad[ofs + 5] += cp[1] * v[1];
		grad[ofs + 6] += cp[2] * v[1];
	grad[ofs + 7] += v[1];

		grad[ofs + 8] += cp[0] * v[2];
		grad[ofs + 9] += cp[1] * v[2];
		grad[ofs + 10] += cp[2] * v[2];
	grad[ofs + 11] += v[2];
#endif
}

double LADeformationGoalFunction::evaluateFunction(Vecd& variables) {
	double ret = 0;
	double surfaceErr = 0;
	double smoothErr = 0;
	double markerErr = 0;
	int i, j;

	//uiWait();

	// update curMesh
	applyDef(variables);

	// zero out gradient
	grad.zeroElements();

	for (i = 0; i < curMesh->numPts(); i++) {
		// surface term
		if (surfaceMatchWeight > 0) {
			updateCurrent(i);
			conf[i] = curSurfaceWeight;

			// hack
			if (lockShape) {
				curSurfaceWeight *= curMesh->getPtConf(i);
			}
//			curMesh->getPtColor(i)[1] = curSurfaceWeight;

			curSurfaceWeight = baMin(1.0,CONF_MULT*curSurfaceWeight);

			surfaceErr += curSurfaceWeight * sqr(curDistance);

			addGradientVec(i, surfaceMatchWeight * curSurfaceWeight * 2.0 * curGradient);

			if (smatchShowError) {
				curMesh->getPtColor(i)[0] = 0.2 + baMin(0.8, sqr(curDistance) * 4000.0);
			}
		}
		else
			surfaceMatchWeight = 0;

		// smoothness term
		if (smoothnessWeight > 0) {
			double curSmoothnessWeight = smoothnessWeight;
				// * (1 + (1.0 - curSurfaceWeight));
			for (j=0; j < (int)curMeshNeigh[i].size(); j++) {
				int n = curMeshNeigh[i][j];

//				double nW = neighWeights[i] + neighWeights[n]; //curMesh->evalPts[i].neighWeights[j];
				double nW = curMeshNeighWeights[i][j];

				int k;
				double sum = 0;
				for (k=0; k < varsPerVert; k++) {
//					if (k % 4 == 3) {
//						nW = 1;
//					}
//					else
//						nW = 1;
					double dist = variables[i*varsPerVert + k] - variables[n*varsPerVert + k];
//					sum += curSmoothnessWeight * nW * log(1.0 + sqr(dist));
//					grad[i*varsPerVert + k] += curSmoothnessWeight * nW * (1.0/(1.0+sqr(dist))) * 2.0 * dist;
//					grad[n*varsPerVert + k] -= curSmoothnessWeight * nW * (1.0/(1.0+sqr(dist))) * 2.0 * dist;
					sum += curSmoothnessWeight * nW * sqr(dist);
					grad[i*varsPerVert + k] += curSmoothnessWeight * nW * 2.0 * dist;
					grad[n*varsPerVert + k] -= curSmoothnessWeight * nW * 2.0 * dist;
				}
				smoothErr += sum;

				if (smatchShowError) {
					curMesh->getPtColor(i)[1] = 0.2 + baMin(0.8, sum * 20000.0);
				}
			}
		}

//		if (smatchShowError) {
//			curMesh->getPtColor(i)[2] = 0.2;
//		}
/*
#ifdef SPLIT_TRANSFORMATION
		// penalize translation
		ret += sqr(variables[i*varsPerVert + 7]) + sqr(variables[i*varsPerVert + 8]) + sqr(variables[i*varsPerVert + 9]);
		grad[i*varsPerVert + 7] += 2.0 * variables[i*varsPerVert + 7];
		grad[i*varsPerVert + 8] += 2.0 * variables[i*varsPerVert + 8];
		grad[i*varsPerVert + 9] += 2.0 * variables[i*varsPerVert + 9];
#endif*/
	}

	if (surfaceMatchWeight > 0) {
		surfaceErr *= surfaceMatchWeight;
		ret += surfaceErr;
	}
	ret += smoothErr;

	if (markerMatchWeight) {
		for (i=0; i < baMin(sMarkers->numMarkers(), tMarkers->numMarkers()); i++) {
			if (sMarkers->markers[i].kind != mkrBARY)
				continue;

			Vec3d sPos = sMarkers->markers[i].curPos();
			Vec3d tPos = tMarkers->markers[i].curPos();
			if (sPos.iszero() || tPos.iszero())
				continue;

			Vec3d delta = sPos - tPos;
			markerErr += delta.length2();

			addGradientVec(sMarkers->markers[i].baryVerts[0], 
				sMarkers->markers[i].baryPos[0] * markerMatchWeight * 2.0 * delta);
			if (sMarkers->markers[i].baryVerts[1] >= 0)
				addGradientVec(sMarkers->markers[i].baryVerts[1], 
					sMarkers->markers[i].baryPos[1] * markerMatchWeight * 2.0 * delta);
			if (sMarkers->markers[i].baryVerts[2] >= 0)
				addGradientVec(sMarkers->markers[i].baryVerts[2], 
					sMarkers->markers[i].baryPos[2] * markerMatchWeight * 2.0 * delta);
		}

		markerErr *= markerMatchWeight;
		ret += markerErr;
	}

	cout << ret << " (surface: " << surfaceErr << "; smoothness: " << smoothErr << "; markers: " << markerErr << ")" << endl;

	lastErr = ret;
	return ret;
}

void LADeformationGoalFunction::evaluateGradient(Vecd& variables, Vecd& gradient) {
	gradient = grad;
}

void LADeformationGoalFunction::solverStep() {
//		if (stepCount % 4 == 0)
//			smoothnessStrength *= 0.5;

	if (capRate > 0 && (stepCount < 8 || stepCount % capRate == 0)) {
		cout << "frame " << stepCount << "; error " << lastErr << endl;
//			defMesh->calcNormals();

		if (recordOptimization) {
			char fname[80];
			sprintf(fname, "anim/%04d.tga", stepCount);
			//redrawV();
//			uiScreenshot(fname);
		}
		else {
			//redrawV();
			//uiWait();
		}
	}

	stepCount++;
}
