#include "ba.h"
#include "markers.h"
#include "trimesh.h"

// Marker =================================================

Marker::Marker() {
	color = Vec3d(1, 0, 0.5);
	name[0] = 0;
	mesh = NULL;
	kind = mkrPOS;
}

Marker::Marker(Vec3d pt) {
	color = Vec3d(1, 0, 0.5);
	name[0] = 0;
	mesh = NULL;
	kind = mkrPOS;
	pos = pt;
}

Vec3d Marker::curPos() {
	if (kind == mkrPT && mesh) {
		pos = mesh->getPt(baryVerts[0]);
	}
	else if (kind == mkrBARY && mesh) {
		pos = baryPos[0] * mesh->getPt(baryVerts[0]);
		if (baryVerts[1] >= 0)
			pos += baryPos[1] * mesh->getPt(baryVerts[1]);
		if (baryVerts[2] >= 0)
			pos += baryPos[2] * mesh->getPt(baryVerts[2]);
	}

	return pos;
}

void Marker::convert(int newMode) {
	if (newMode != mkrPOS) {
		if (!mesh || !mesh->calcClosestPoint(pos, 1e6)) {
			cout << "warning: no closest point" << endl;
			return;
		}

		if (newMode == mkrBARY) {
			baryVerts[0] = mesh->closestTri[0];
			baryVerts[1] = mesh->closestTri[1];
			baryVerts[2] = mesh->closestTri[2];
			baryPos = mesh->closestBary;
		}
		else if (newMode == mkrPT) {
			if (mesh->closestBary[0] > mesh->closestBary[1] && mesh->closestBary[0] > mesh->closestBary[2])
				baryVerts[0] = mesh->closestTri[0];
			else if (mesh->closestBary[1] > mesh->closestBary[2])
				baryVerts[0] = mesh->closestTri[1];
			else
				baryVerts[0] = mesh->closestTri[2];
		}
	}

	kind = newMode;
}

//void Marker::drawGL() {
//	color.glColor();
//	if (!curPos().iszero())
//	glbSphere(curPos(), 0.0025);
//}


// MarkerSet ==============================================

MarkerSet::MarkerSet() {
}

void MarkerSet::init(int nMarkers) {
	markers.resize(nMarkers);
}

int MarkerSet::numMarkers() {
	return markers.size();
}

Vec3d MarkerSet::v(int mIndex) {
	return markers[mIndex].curPos();
}

Vec3d &MarkerSet::c(int mIndex) {
	return markers[mIndex].color;
}

//void MarkerSet::drawGL() {
//	int i;
//	for (i=0; i < (int)markers.size(); i++)
//		markers[i].drawGL();
//}

void MarkerSet::loadFromLandmarks(const char *fname) {
	ifstream in;

	init(85); //76);

	if (!openIFStream(&in, fname, "landmark file"))
		return;

	char tempStr[1024];
	int j;
	double tempDouble;

	while (!in.eof()) {
		in.getline(tempStr, 1024);
		if (strncmp(tempStr, "AUX ", 4) == 0) {
			break;
		}
	}

	int index;
	markers[0].pos = Vec3d();
	while (!in.eof()) {
		in >> tempStr;
		if (strncmp(tempStr, "END", 3) == 0)
			break;
		index = atoi(tempStr);
		for (j=0; j < 3; j++)
			in >> tempDouble;
		for (j=0; j < 3; j++) {
			in >> tempDouble;
			if (index > 0 && index <= 73)
				markers[index].pos[j] = tempDouble / 1000.0;
			else if (index >= 74)
				markers[index].pos[j] = tempDouble;
		}
		in.getline(tempStr, 1024);
	}

	// "b" files have butt block marker; gotta remove it
	if (fabs(markers[74].pos[2]) > 10) {
		markers[74].pos = Vec3d();
		markers[75].pos = Vec3d();
	}
}

void MarkerSet::loadText(const char *fname) {
	ifstream in;

	if (!openIFStream(&in, fname, "landmark file"))
		return;

	int i, j;
	Vec3d v;

	in >> j;
	init(j);

	for (i=0; i < j; i++) {
		in >> v;
		markers[i].pos = v;
	}
	in.close();
}

void MarkerSet::loadFromMkr(const char *fname) {
	ifstream in;
	int index;
	double c;
	init(85);
	for (index=0; index < 85; index++)
		markers[index].pos = Vec3d();

	if (!openIFStream(&in, fname, "mkr file"))
		return;

	for (index=1; index < 85; index++) {
		in >> markers[index].name;
		if (!(c=in.good()) || markers[index].name[0] == 0)
		{cout<<"in.good()="<<c<<endl;;break;}
		in >> markers[index].pos[0] >> markers[index].pos[1] >> markers[index].pos[2];
	}
	for (index=1; index < 85; index++) {

		cout<< markers[index].pos[0] <<" "<< markers[index].pos[1] <<" "<< markers[index].pos[2]<<endl;
	}
	cout << "loaded " << (index-1) << " markers from " << fname << endl;
	in.close();
}

void MarkerSet::save(const char *fname) {
	ofstream out;
	int index;

	if (!openOFStream(&out, fname, "mkr file"))
		return;

	out << "1" << endl;	// version
	out << markers.size() << endl;

	for (index=0; index < numMarkers(); index++) {
		if (markers[index].name[0] == 0)
			out << index;
		else
			out << markers[index].name;

		out << " " << markers[index].kind << " ";
		if (markers[index].kind == mkrPOS) {
			out << " " << markers[index].pos[0] << " " << markers[index].pos[1] << " "<< markers[index].pos[2];
		}
		else if (markers[index].kind == mkrPT) {
			out << " " << markers[index].baryVerts[0];
		}
		else {
			out << " " << markers[index].baryVerts[0] << " " << markers[index].baryVerts[1] << " "<< markers[index].baryVerts[2];
			out << " " << markers[index].baryPos[0] << " " << markers[index].baryPos[1] << " "<< markers[index].baryPos[2];
		}

		out << endl;
	}
	out.close();
}

void MarkerSet::load(const char *fname) {
	ifstream in;
	int index;

	if (!openIFStream(&in, fname, "mkr file"))
		return;

	in >> index;
	if (index != 1) {
		cout << "incompatible version (" << index << ") reading " << fname << endl;
		return;
	}

	in >> index;
	init(index);

	for (index=0; index < numMarkers(); index++) {
		in >> markers[index].name;
		in >> markers[index].kind;

		if (markers[index].kind == mkrPOS) {
			in >> markers[index].pos[0] >> markers[index].pos[1] >> markers[index].pos[2];
		}
		else if (markers[index].kind == mkrPT) {
			in >> markers[index].baryVerts[0];
		}
		else {
			in >> markers[index].baryVerts[0] >> markers[index].baryVerts[1] >> markers[index].baryVerts[2];
			in >> markers[index].baryPos[0] >> markers[index].baryPos[1] >> markers[index].baryPos[2];
		}
	}
	in.close();
}