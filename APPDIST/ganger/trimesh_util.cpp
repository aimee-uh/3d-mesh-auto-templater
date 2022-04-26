#include "trimesh.h"
#include "trimesh_util.h"

vector<int> *findTMNeighbors(TriMesh *tm) {
	vector<int> *neighbors;

	int i;

	neighbors = new vector<int>[tm->numPts()];
	for (i=0; i < tm->numTris(); i++) {
		int v0 = tm->getTri(i, 0);
		int v1 = tm->getTri(i, 1);
		int v2 = tm->getTri(i, 2);
		neighbors[v0].push_back(v1);
		neighbors[v1].push_back(v2);
		neighbors[v2].push_back(v0);
	}

	return neighbors;
}

void tmFlipNormals(TriMesh *curTM) {
	int i, v;
	for (i=0; i < curTM->numTris(); i++) {
		v = curTM->getTri(i, 2);
		curTM->getTri(i, 2) = curTM->getTri(i, 1);
		curTM->getTri(i, 1) = v;
	}
	curTM->calcNormals();
}