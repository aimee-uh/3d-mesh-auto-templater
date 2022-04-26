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
#include <fstream>
#include <sstream>
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
#include "vtkPolyDataAlgorithm.h"
#include "vtkPLYWriter.h"
#include "vtkTriangleFilter.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkPLYReader.h"
#include "vtkPolyDataConnectivityFilter.h"


int numMeshes;
TriMesh **meshes;
MarkerSet **markers;
char300 *markerNames;

LBFGSSolver *solver = NULL;
LADeformationGoalFunction *deform = NULL;
bool dirtyDeform = true;

char300 sourceName, targetName;

/////////this output is for deformation with a scale of 0.01
void writelandmarksscale(const char*msg,double* data, int numberofLandmarks, double writescale)
{
	ofstream out( msg,ios::ate);
	out<<"1\n"<<numberofLandmarks/3<<"\n";
	out<<"prn 0 "<<data[0]*writescale<<" "<<data[1]*writescale<<" "<<data[2]*writescale<<"\n";
	out<<"sn 0 "<<data[3]*writescale<<" "<<data[4]*writescale<<" "<<data[5]*writescale<<"\n";
	out<<"n 0 "<<data[6]*writescale<<" "<<data[7]*writescale<<" "<<data[8]*writescale<<"\n";
	out<<"acL 0 "<<data[9]*writescale<<" "<<data[10]*writescale<<" "<<data[11]*writescale<<"\n";
	out<<"acR 0 "<<data[12]*writescale<<" "<<data[13]*writescale<<" "<<data[14]*writescale<<"\n";
	out<<"alL 0 "<<data[15]*writescale<<" "<<data[16]*writescale<<" "<<data[17]*writescale<<"\n";
	out<<"alR 0 "<<data[18]*writescale<<" "<<data[19]*writescale<<" "<<data[20]*writescale<<"\n";
	out<<"ch_L 0 "<<data[21]*writescale<<" "<<data[22]*writescale<<" "<<data[23]*writescale<<"\n";
	out<<"ch_R 0 "<<data[24]*writescale<<" "<<data[25]*writescale<<" "<<data[26]*writescale<<"\n";
	out<<"earup_L 0 "<<data[27]*writescale<<" "<<data[28]*writescale<<" "<<data[29]*writescale<<"\n";
	out<<"earback_L 0 "<<data[30]*writescale<<" "<<data[31]*writescale<<" "<<data[32]*writescale<<"\n";
	//out<<"eardown_L 0 "<<data[33]*writescale<<" "<<data[34]*writescale<<" "<<data[35]*writescale<<"\n";
	out<<"earup_R 0 "<<data[33]*writescale<<" "<<data[34]*writescale<<" "<<data[35]*writescale<<"\n";
	out<<"earback_R 0 "<<data[36]*writescale<<" "<<data[37]*writescale<<" "<<data[38]*writescale<<"\n";
	//out<<"eardown_R 0 "<<data[42]*writescale<<" "<<data[43]*writescale<<" "<<data[44]*writescale<<"\n";
	out<<"en_R 0 "<<data[39]*writescale<<" "<<data[40]*writescale<<" "<<data[41]*writescale<<"\n";
	out<<"ex_R 0 "<<data[42]*writescale<<" "<<data[43]*writescale<<" "<<data[44]*writescale<<"\n";
	out<<"en_L 0 "<<data[45]*writescale<<" "<<data[46]*writescale<<" "<<data[47]*writescale<<"\n";
	out<<"ex_L 0 "<<data[48]*writescale<<" "<<data[49]*writescale<<" "<<data[50]*writescale<<"\n";
	out.close();
	
	cout << "Finish write landmarks" <<msg<< data[0] <<data[1] <<data[2]<< endl;
}

double* readnewlandmarks(const char*msg, int NumberLandmarks)
{
    
	ifstream inFile (msg);
	string line;
    int linenum = 0;
	double* landmarks = (double *)calloc(NumberLandmarks, sizeof(double));
	
    while (getline (inFile, line))
    {
        linenum++;
		istringstream linestream(line);
        string item;
		//read all the landmarks, x, y and z
		
        int itemnum = 0;
		while (getline (linestream, item, ',') )
        {
			landmarks[itemnum] = atof(item.c_str());
            itemnum++;
            cout << "Item #" << itemnum << ": " << item << endl;
        }//end of reading landmarks
		
	}
	cout << "Finish read new landmarks" << landmarks[0] <<landmarks[1] <<landmarks[2] << endl;
	return landmarks;
	
}


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
	if (meshes[ind])
		meshes[ind]->saveFile(fname);
}

void writeFilePLY(char* file_name,vtkPolyDataAlgorithm* indata)
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
}

int main(int argc, char* argv[])
{
	int i;
	cout<<"enter program."<<endl;
	if (argc<5)
	{
		cerr<<"The usage is <sourcefile.ply> <sourcefile.mkr> <targetfile.ply> <targetfile.mkr> "<<endl;
		return -1;
	}
	
	//read in source file and target file
	vtkPolyDataAlgorithm *source_data = vtkPolyDataAlgorithm::New();
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
	

	//write down as temporary file
	char sourcetempName[strlen(argv[1])+30];
	char targettempName[strlen(argv[3])+30];
	char outputFile[strlen(argv[3])+40];
	
	sprintf(sourcetempName, "%s_source.ply", argv[1]);
	sprintf(targettempName, "%s_target.ply", argv[3]);
	sprintf(outputFile,"%s_deform.ply",argv[3]);
	
	vtkPolyDataAlgorithm *source_datanew = sizechange(source_data, sizeshrink[0]);
	
	//extract largest region for target...
	vtkPolyDataConnectivityFilter *connectFilter = vtkPolyDataConnectivityFilter::New();
	connectFilter->SetInputConnection(target_data->GetOutputPort());
	
	connectFilter->SetExtractionModeToLargestRegion();
	connectFilter->Update();

	
	vtkPolyDataAlgorithm *target_datanew = sizechange(connectFilter, sizeshrink[1]);
	
	writeFilePLY(sourcetempName,source_datanew);
	writeFilePLY(targettempName,target_datanew);
	
	//change the size for markers
	for (i=0; i < numMeshes; i++) {
		for (int k=0; k<markers[i]->numMarkers(); k++) {
			for (int j=0; j<3; j++) {
				//				markers[i]->markers[k].pos[j] = markers[i]->markers[k].pos[j]*sizeshrink[i];
			}		
			//		cout <<markers[i]->markers[k].name <<":"<< markers[i]->markers[k].pos[0] << ","<< markers[i]->markers[k].pos[1] << "," << markers[i]->markers[k].pos[2]<<endl;	
		}
	}
	//read in the other landmark file and change the size of landmarks
	int NumberofLandmarks = 51;
	double* landmarks1 = (double *)calloc(NumberofLandmarks, sizeof(double));
	landmarks1 = readnewlandmarks(argv[2],NumberofLandmarks);
	
	double* landmarks2 = (double *)calloc(NumberofLandmarks, sizeof(double));
	landmarks2 = readnewlandmarks(argv[4],NumberofLandmarks);
	
	char sourcetempLMKName[strlen(argv[2])+30];
	char targettempLMKName[strlen(argv[4])+30];
	
	sprintf(sourcetempLMKName, "%s_source.txt", argv[2]);
	sprintf(targettempLMKName, "%s_target.txt", argv[4]);
	
	writelandmarksscale(sourcetempLMKName,landmarks1, NumberofLandmarks, sizeshrink[0]);
	writelandmarksscale(targettempLMKName,landmarks2, NumberofLandmarks, sizeshrink[1]);
	

	
	//write down the template mkr files
	
	numMeshes = 2;
	meshes = new TriMesh*[numMeshes];
	markers = new MarkerSet*[numMeshes];	
	markerNames = new char300[numMeshes];
	for (i=0; i < numMeshes; i++) {
		meshes[i] = NULL;
		markers[i] = NULL;
		markerNames[i][0] = 0;
	}
	cout<<"Initialize done."<<endl;
	loadMesh(0, sourcetempName);
	
	cout<<"loadMesh0 done."<<endl;
	loadMarkers(0,sourcetempLMKName, false);
	convertToBary(0);
	
	cout<<"loadMarkers0 done."<<endl;
	loadMesh(1, targettempName);
	
	cout<<"loadMesh1 done."<<endl;
	loadMarkers(1,targettempLMKName, false);
	cout<<"loadMarkers1 done."<<endl;
	

	
	//matching here
	cout << "start match" <<endl;
	startMatch(0, 2, 0, 0.2, 200);
	startMatch(0,10,0,0.2,40);
	startMatch(0,10 ,0.2,0.2,100);
	startMatch(0,5   ,1 ,  0.3 ,100);
	startMatch(0 ,0.5 ,10 , 0.5 ,100);
	startMatch(0,0.1 ,20,  0.3, 20);


	cout<<"startMatch done."<<endl;
	saveMesh(0, outputFile);
	
	//change everything back to original size
	vtkPolyDataAlgorithm *transferdata = vtkPolyDataAlgorithm::New();
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
	remove(targettempLMKName);
	remove(sourcetempLMKName);

	return 0;
}

