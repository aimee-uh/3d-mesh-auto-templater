/*************information header*******************
 Usage: Deformation <sourcefile.ply> <sourcefile.mkr> <targetfile.ply> <targetfile.mkr> 
 
 Title: 
 
 Purpose: To automatically deform source mesh to target mesh based on the markers given in mkr files
 
 
 Author: Jia Wu, Shu Liang
 
 Start date: July 25, 2011
 
 Input: <sourcefile.ply> <sourcefile.mkr> <targetfile.ply> <targetfile.mkr> 
 
 Output: <targetfile.ply_deform.ply> the deformed mesh data
 
 Algorithm description or paper citation:
 Extracted from ganger by Brett Allen
 Deform sourcefile into targetfile with a few steps.

 Comments: none.
 *//////// end of informational header ///////////


#include <iostream>
#include <cstring>
using namespace std;

#ifdef __APPLE__
#include <GLUT/glut.h> //Needed for GLUint
#else
#include <GL/glut.h> //Needed for GLUint
#endif

#include "trimesh.h"
#include "ba.h"
#include "markers.h"
#include "solver.h"
#include "surfaceDef.h"
//not use VTK version
/*#include "vtkPolyDataAlgorithm.h"
#include "vtkPLYWriter.h"
#include "vtkTriangleFilter.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkPLYReader.h"
#include "vtkPolyDataConnectivityFilter.h"*/


int numMeshes;
TriMesh **meshes;
MarkerSet **markers;
char300 *markerNames;

LBFGSSolver *solver = NULL;
LADeformationGoalFunction *deform = NULL;
bool dirtyDeform = true;

char300 sourceName, targetName;

void loadMesh(int ind, const char *fname) {
	int i;

	dirtyDeform = true;
	
	if (meshes[ind]) {
		delete meshes[ind];
	}
	meshes[ind] = new TriMesh();
	meshes[ind]->loadFile(fname);
	
	// ensure that confidence is within [0, 1], and boost low confidences
	for (i=0; i < meshes[ind]->numPts(); i++) {
		double &d = meshes[ind]->getPtConf(i);
//		d += 0.25;
		if (d < 0) d = 0;
		if (d > 1) d = 1;
	}

	meshes[ind]->calcNormals();

	if (ind == 0)
		strcpy(sourceName, fname);
	else
		strcpy(targetName, fname);
	
}

void loadMarkers(int ind, const char *fname, bool isOldStyle) {
	if (markers[ind])
		delete markers[ind];

	dirtyDeform = true;

	markers[ind] = new MarkerSet();
	strcpy(markerNames[ind], fname);
	if (fname[strlen(fname)-1]=='d')
		markers[ind]->loadFromLandmarks(fname);
	else if (isOldStyle)
		markers[ind]->loadFromMkr(fname);
	else
		markers[ind]->load(markerNames[ind]);
	int i;
	for (i=0; i < markers[ind]->numMarkers(); i++)
		markers[ind]->markers[i].mesh = meshes[ind];
	cout<<"number of markers:"<<i<<endl;

}

void convertToBary(int ind) {
	if (markers[ind]) {
		int i;
		for (i=0; i < markers[ind]->numMarkers(); i++) {
			if (!markers[ind]->markers[i].pos.iszero()) {
				markers[ind]->markers[i].mesh = meshes[ind];
				markers[ind]->markers[i].convert(mkrBARY);
			}
		}
	}
}

void prepDeform() {
	if (deform && dirtyDeform) {
		delete deform;
		deform = NULL;
	}

	if (!deform) {
		deform = new LADeformationGoalFunction(meshes[0]);
		deform->zeroDeformation();
		deform->sMarkers = markers[0];
		deform->tMarkers = markers[1];
		if (meshes[1]) {
			meshes[1]->calcHBB(16);
			deform->prepareTriMesh(meshes[1]);
		}

		dirtyDeform = false;
	}
}

void startMatch(bool global, double markerW, double surfaceW, double smoothW, 
				int maxIter) {
	if (solver) {
		cout << "solver already running" << endl;
		return;
	}

	prepDeform();
	deform->markerMatchWeight = markerW;
	deform->surfaceMatchWeight = surfaceW;
	deform->smoothnessWeight = smoothW;
	deform->globalTrans = global;
	
	cout <<"after set varables" <<endl;
	solver = new LBFGSSolver(deform);
	solver->solve(1e+3, 1e-5, deform->vars, maxIter);
	cout << "final err = " << deform->lastErr << endl;

	if (global) {
		int i, j;
		for (i=deform->varsPerVert; i < deform->vars.size(); i += deform->varsPerVert) {
			for (j=0; j < deform->varsPerVert; j++)
				deform->vars[i+j] = deform->vars[i+j-deform->varsPerVert];
		}
	}

	delete solver;
	solver = NULL;
}

void saveMesh(int ind, const char *fname) {
	cout << fname <<":fname"<<endl;
	if (meshes[ind])
		meshes[ind]->saveFile(fname);
}

/*void writeFilePLY(char* file_name,vtkPolyDataAlgorithm* indata)
{
	//write ply file
	vtkPLYWriter *data = vtkPLYWriter::New();
	data->SetInput(indata->GetOutput());
	data->SetFileName(file_name);
	data->SetFileType(2);
	data->Write();
	data->Delete();
}

vtkPolyDataAlgorithm* readFilePLY(char* file_name)
{
	//load stl file
	vtkPLYReader *data = vtkPLYReader::New();
	data->SetFileName(file_name);
	data->Update();
	return data;
}

vtkPolyDataAlgorithm* sizechange(vtkPolyDataAlgorithm *input, double Decifrac)
{
	vtkPolyData *poly = vtkPolyData::New();
	// poly is the vtkPolyData object
	poly->SetPoints(input->GetOutput()->GetPoints());
	poly->SetPolys(input->GetOutput()->GetPolys());
	
	vtkIdType numPoints = poly->GetPoints()->GetNumberOfPoints();
	cout << "numPoints " << numPoints << endl;
	
	vtkPoints *points = vtkPoints::New();
	double *temp,xyz[3]; 
	
	for(int i = 0; i < numPoints; i++)
	{	
		temp = poly->GetPoints()->GetPoint(i);
		
		xyz[0] = Decifrac * (temp[0]);
		xyz[1] = Decifrac * (temp[1]);
		xyz[2] = Decifrac * (temp[2]);
		
		points->InsertNextPoint(xyz);
	} 
	
	poly->SetPoints(points);
	
	//triagnlate file
	vtkTriangleFilter *triangles = vtkTriangleFilter::New();
	triangles->SetInput(poly);
	triangles->Update();
	
	return triangles;
}*/

int main(int argc, char* argv[])
{
	int i;
	cout<<"enter program."<<endl;
	if (argc<5)
	{
		cerr<<"The usage is <sourcefile.ply> <sourcefile.mkr> <targetfile.ply> <targetfile.mkr> "<<endl;
		return -1;
	}
	char outputFile[strlen(argv[1])+40]; // CHANGED THIS TO SAVE TO SOURCE MESH LOCATION
	sprintf(outputFile,"%s_deform.ply",argv[1]);

	//read in source file and target file
/*	vtkPolyDataAlgorithm *source_data = vtkPolyDataAlgorithm::New();
	vtkPolyDataAlgorithm *target_data = vtkPolyDataAlgorithm::New();
	
	source_data = readFilePLY(argv[1]);
	target_data = readFilePLY(argv[3]);
	
	double* bounds = (double *)calloc(6, sizeof(double));
	bounds = source_data->GetOutput()->GetBounds();
	//cout << bounds[1] - bounds[0] << ", " << bounds[3] - bounds[2] << " , " << bounds[5] - bounds[4] << endl;
	
	double* bounds2 = (double *)calloc(6, sizeof(double));
	bounds2 = target_data->GetOutput()->GetBounds();
	//cout << bounds2[1] - bounds2[0] << ", " << bounds2[3] - bounds2[2] << " , " << bounds2[5] - bounds2[4] << endl;
	
	
	//change size of sourcefile and targetfile, so it is between -1 to 1
	double sizeenlarge = 0.5*(bounds2[1] - bounds2[0]);
	double sizeshrink[2];
	sizeshrink[0] = 2/(bounds[1] - bounds[0]);
	sizeshrink[1] = 1/sizeenlarge;
*/
	//write down as temporary file
/*	char sourcetempName[strlen(argv[1])+30];
	char targettempName[strlen(argv[3])+30];
	
	sprintf(sourcetempName, "%s_source.ply", argv[1]);
	sprintf(targettempName, "%s_target.ply", argv[3]);
	
	vtkPolyDataAlgorithm *source_datanew = sizechange(source_data, sizeshrink[0]);
	
	//extract largest region for target...
	vtkPolyDataConnectivityFilter *connectFilter = vtkPolyDataConnectivityFilter::New();
	connectFilter->SetInputConnection(target_data->GetOutputPort());
	
	connectFilter->SetExtractionModeToLargestRegion();
	connectFilter->Update();

	
	vtkPolyDataAlgorithm *target_datanew = sizechange(connectFilter, sizeshrink[1]);
	
	writeFilePLY(sourcetempName,source_datanew);
	writeFilePLY(targettempName,target_datanew);
*/	
	
	numMeshes = 2;
	meshes = new TriMesh*[numMeshes];
	//colors = new Vec3d*[numMeshes];
	//colorModes = new int[numMeshes];
	markers = new MarkerSet*[numMeshes];	
	markerNames = new char300[numMeshes];
	//markerAlphas = new double[numMeshes];	
	//selMarker = new int[numMeshes];
	for (i=0; i < numMeshes; i++) {
		meshes[i] = NULL;
		//colors[i] = NULL;
		//colorModes[i] = cmSOLID;

		markers[i] = NULL;
		markerNames[i][0] = 0;
		//markerAlphas[i] = 1;
		//selMarker[i] = -1;
	}
	cout<<"Initialize done."<<endl;

	//	loadMesh(0, sourcetempName);
	loadMesh(0, argv[1]);
	cout<<"loadMesh0 done."<<endl;

	loadMarkers(0,argv[2], false);
	convertToBary(0);

	//	loadMesh(1, targettempName);
	loadMesh(1, argv[3]);
	
	cout<<"loadMesh1 done."<<endl;
	
	loadMarkers(1,argv[4], false);
	cout<<"loadMarkers1 done."<<endl;
	
	//change the size for markers
	for (i=0; i < numMeshes; i++) {
		for (int k=0; k<markers[i]->numMarkers(); k++) {
			for (int j=0; j<3; j++) {
//				markers[i]->markers[k].pos[j] = markers[i]->markers[k].pos[j]*sizeshrink[i];
			}		
//		cout <<markers[i]->markers[k].name <<":"<< markers[i]->markers[k].pos[0] << ","<< markers[i]->markers[k].pos[1] << "," << markers[i]->markers[k].pos[2]<<endl;	
		}
	}
	
	//matching here
/*
	startMatch(0, 2, 0, 0.2, 200);
	startMatch(0,10,0,0.2,40);
	startMatch(0,10 ,0.2,0.2,100);
	startMatch(0,5   ,1 ,  0.3 ,100);
	startMatch(0 ,0.5 ,10 , 0.5 ,100);
	startMatch(0,0.1 ,20,  0.3, 20);
*/

// 7k template

	startMatch(0, 2,   0,   0.2, 200);
	startMatch(0, 10,  0,   0.2, 40);
	startMatch(0, 10,  0.2, 0.2, 100);
	startMatch(0, 5,   1,   0.3, 100);
	startMatch(0, 0.5, 10,  0.5, 100);
	startMatch(0, 0.1, 20,  0.3, 20);


// 60k template
/*
	startMatch(0, 0.25,   0,   0.2, 1000);
	startMatch(0, 0.5,   0,   0.2, 1000);
	startMatch(0, 0.75,   0,   0.2, 1000);
	startMatch(0, 1.0,   0,   0.2, 1000);
	startMatch(0, 2,   0,   0.2, 1000);
	startMatch(0, 5,  0,   0.2, 1000);
	startMatch(0, 10,  0,   0.2, 1000);outputFile
	startMatch(0, 10,  0.2, 0.2, 1000);
	startMatch(0, 5,   1,   0.3, 1000);
	startMatch(0, 0.5, 10,  0.5, 1000);
	startMatch(0, 0.1, 20,  0.3, 1000);
*/
	
	cout<<"startMatch done."<<endl;
	saveMesh(0, outputFile);
	
	//change everything back to original size
/*	vtkPolyDataAlgorithm *transferdata = vtkPolyDataAlgorithm::New();
	transferdata = readFilePLY(outputFile);
	
	vtkPolyDataAlgorithm *transferdatanew = sizechange(transferdata, sizeenlarge);
	writeFilePLY(outputFile,transferdatanew);
	
	transferdatanew->Delete();
	transferdata->Delete();
	source_datanew->Delete();
	source_data->Delete();
	target_data->Delete();
	target_datanew->Delete();

	cout << "shrink number"<< sizeshrink[0] <<" , " << sizeshrink[1] << endl;
	cout << bounds[1] - bounds[0] << ", " << bounds[3] - bounds[2] << " , " << bounds[5] - bounds[4] << endl;
	cout << bounds2[1] - bounds2[0] << ", " << bounds2[3] - bounds2[2] << " , " << bounds2[5] - bounds2[4] << endl;

	
	remove(sourcetempName);
	remove(targettempName);
*/
	return 0;
}

