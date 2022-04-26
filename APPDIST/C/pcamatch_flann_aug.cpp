#include <Eigen/Dense>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
//#include <ANN/ANN.h>
#include <flann/flann.hpp>
#include <stdio.h>
#include <cmath>
#include "pcamatch.h"
#include "Q2_PCDA_bennett.hpp"
using namespace std;
using namespace Eigen;
#include "pcaio_autofit.hpp"
int main(int argc, char* argv[])
{
    bool matchNormals = false;
    bool tposewarp = false;    
    bool lengthenarms = false;
    ifstream templatefile;
    ifstream mvefile;
    ifstream sigmafile;
    ifstream pcafile; //read in binary mode
    ofstream outputfile;
    ofstream scaleandtranslate;
    ofstream matchdiagram;
    
    mvefile.open(argv[1], ifstream::in);
    double alpha = atof(argv[2]); //0.001
    int mf = atoi(argv[3]);
    cout << "moop" << endl;
    float inputHeight = atof(argv[4]);
    float inputWeight = atof(argv[5]);
    string output_prefix = argv[6];
    float convergencetol = atof(argv[7]); //0.1
    string device = "superset"; //hardcode for superset

    string basepcadir = "../pcamodels/pca_autosuperset_bootstraptrain/"; //change which pca to use here

    cout << "USING PCA SPACE: " << basepcadir << endl;
    cout << "DEVICE: " << device << endl;
    string genderstr = "male";
    if (mf == 1) genderstr = "female";
    string avgfilename = basepcadir + device + "/pcafiles/" + genderstr[0] + "_average_superset.ply";
    templatefile.open(avgfilename, ifstream::in);
    outputfile.open(output_prefix + "/result.ply", ofstream::out);
    pcafile.open(basepcadir + device + "/pcafiles/" + genderstr[0] + "_pca.bin", ifstream::in);
    matchdiagram.open(output_prefix + "/pins.ply", ofstream::out);
    scaleandtranslate.open(output_prefix + "/intermediate.ply", ofstream::out);    
    cout << avgfilename << endl;
    
    if (!templatefile or !mvefile)
    {
        cerr << "INPUT MESH FILE READ ERROR!!!" << endl;
        exit (EXIT_FAILURE);
    }
    if (!pcafile or !sigmafile)
    {
        cerr << "INPUT PCA OR SIGMA FILE READ ERROR!!!" << endl;
        exit (EXIT_FAILURE);
    }
    
    vector<vector<Vector3d>> templatePNF = readMeshPlyFileFLANN(templatefile, outputfile); //points normals faces
    cout << "Read template file..." << endl;

    vector<vector<Vector3d>> targetPNF = readMeshPlyFileFLANN(mvefile, scaleandtranslate);
    cout << "Read MVE file..." << endl;
    
    
    //create search data structure using ANN, refer to ANN manual section 2.1.4
    int             nPts = targetPNF[0].size();
    int             dim = 3;
    int             nn = 60;    //number of matches to return. If satisfactory result (n1 * n2 > 0) not found within k matches, return no hit
    double          sqRad = 0.25;   //fixed radius search. k is set to number of points within sqrt of this value. half a meter to start
    
    double* querymem = new double[3 * templatePNF[0].size()];
    flann::Matrix<double> query(querymem, templatePNF[0].size(), 3);
    flann::Matrix<int> indices(new int[query.rows*nn], query.rows, nn);
    flann::Matrix<double> dists(new double[query.rows*nn], query.rows, nn);
    
    double* dataPts = new double[nPts * 3];
    //initialze values for ANNpointArray
    for (int data_i = 0; data_i < nPts; data_i++)
    {
        dataPts[data_i * 3] = targetPNF[0][data_i][0];
        dataPts[data_i * 3 + 1] = targetPNF[0][data_i][1];
        dataPts[data_i * 3 + 2] = targetPNF[0][data_i][2];
    }
    flann::Matrix<double> dataset(dataPts, nPts, 3);
    flann::Index<flann::L2<double> > index(dataset, flann::KDTreeIndexParams(1)); 
    index.buildIndex();
    
    
    //converts array of coordinate structs to a 18003 length column vector for use in regularization
    VectorXd w0 = coordsToVector(templatePNF[0]);
    //set matrix values
    MatrixXd    pcafull = readPCA(pcafile);

    //MatrixXd pca = pcafull.block(0,0, pcafull.rows(), 80);
    MatrixXd pca = pcafull;
    cout << "moop" << endl;
    sigmafile.open(basepcadir + device + "/pcafiles/" + genderstr[0] + "sigma_" + to_string(pca.cols()) + ".txt", ifstream::in);
    MatrixXd    sigmas = readSigmas(sigmafile, pca.cols(), alpha);
    //initial value
    VectorXd    Xw = w0;
    //error margin, default to 0
    double eps = 0.0;
    
    
    
    //these are lists of match pairs in order to be used to write a pins n needles ply file
    //declared here because of scope
    vector<string> matchpairs;
    vector<string> edges;
    int mpairs = 0;
    
    /*
    bool manualInput = false;
    //manual correlations specified. preprocess step
    if (manualInput)
    {
        //read in set of predetermined points
        VectorXd y_manual(1);
        
        //build w0 and pca out of existing w0 and pca using read in indices
        MatrixXd pca_manual(1,1);
        VectorXd w0_manual(1);
        
        VectorXd w = ridgeRegression(pca_manual, y_manual, w0_manual, sigmas);
        
        //corresponding indices in pca and w0
        vector<int> pcaindices;
        
        Xw = w0 + pca * w;
    
    }
    */
    
    cout << "moop3" << endl;
    cout << "pca cols: " << to_string(pca.cols()) << endl;
    //6/4/2020
    //initialize with height and weight
    ifstream hwToPCAbin;
    ifstream PCAToPredictionsbin;
    int ptofcols = 70;
    //if (mf == 1) ptofcols = ;
    hwToPCAbin.open(basepcadir + device + "/pcafiles/regressions/" + genderstr + "featurestopca_" + to_string(pca.cols()) + ".bin" , ifstream::in);
    PCAToPredictionsbin.open(basepcadir + device + "/pcafiles/regressions/" + genderstr + "pcatofeatures_" + to_string(pca.cols()) + ".bin" , ifstream::in);
    
    if (!hwToPCAbin or !PCAToPredictionsbin)
    {
        cerr << "REGRESSION MATRIX FILE READ ERROR!!!" << endl;
        exit (EXIT_FAILURE);
    }
    
    MatrixXd hwToPCA_M = readPCA(hwToPCAbin);
    MatrixXd PCAToPredictions_M = readPCA(PCAToPredictionsbin);

    Vector3d hw1(inputWeight, inputHeight, 1);
    VectorXd w_init = hwToPCA_M * hw1;
    Xw = w0 + pca * w_init; //initialize to expected height, weight
    cout << "Xw size: " << to_string(Xw.rows()) << endl;
    
    //set some variables to measure convergence
    VectorXd Xw_prev = VectorXd(w0);
    bool converged = false;
    int iters = 0;
    int maxiters = 150;
    //maxiters = 0; //uncomment this to run only one iter for debug
    double maxD = 0.5;
    VectorXd w = w_init; //need to declare w for scoping reasons
    VectorXd y(pca.rows());
    
    //try and force the arms to some angle in degrees with -Y axis
    //hack to try and make T pose work

        int lshoulderidx = 7334;
        int lelbowidx1 = 4462;
        int lelbowidx2 = 4699;
        int lelbowidx3 = 5911;
        int lwristidx1 = 2985;
        int lwristidx2 = 1493;
    
        int rshoulderidx = 35921;
        int relbowidx1 = 32989;
        int relbowidx2 = 32316;
        int relbowidx3 = 31687;
        int rwristidx1 = 28562;
        int rwristidx2 = 29193;
    
    //find the position of these joints in 3D space, take average of the markers
    //preserving distance to shoulder, specify new position which is some distance d from the shoulder joint along vector v that is angle theta from the -Y axis
    //for t pose this is 90 degrees
    
        Vector3d ls3d(Xw(3.0 * lshoulderidx), Xw(3.0 * lshoulderidx + 1), Xw(3.0 * lshoulderidx + 2));
        Vector3d le1_3d(Xw(3.0 * lelbowidx1), Xw(3.0 * lelbowidx1 + 1), Xw(3.0 * lelbowidx1 + 2));
        Vector3d le2_3d(Xw(3.0 * lelbowidx2), Xw(3.0 * lelbowidx2 + 1), Xw(3.0 * lelbowidx2 + 2));
        Vector3d le3_3d(Xw(3.0 * lelbowidx3), Xw(3.0 * lelbowidx3 + 1), Xw(3.0 * lelbowidx3 + 2));
        Vector3d lw1_3d(Xw(3.0 * lwristidx1), Xw(3.0 * lwristidx1 + 1), Xw(3.0 * lwristidx1 + 2));
        Vector3d lw2_3d(Xw(3.0 * lwristidx2), Xw(3.0 * lwristidx2 + 1), Xw(3.0 * lwristidx2 + 2));
    
        Vector3d rs3d(Xw(3.0 * rshoulderidx), Xw(3.0 * rshoulderidx + 1), Xw(3.0 * rshoulderidx + 2));
        Vector3d re1_3d(Xw(3.0 * relbowidx1), Xw(3.0 * relbowidx1 + 1), Xw(3.0 * relbowidx1 + 2));
        Vector3d re2_3d(Xw(3.0 * relbowidx2), Xw(3.0 * relbowidx2 + 1), Xw(3.0 * relbowidx2 + 2));
        Vector3d re3_3d(Xw(3.0 * relbowidx3), Xw(3.0 * relbowidx3 + 1), Xw(3.0 * relbowidx3 + 2));
        Vector3d rw1_3d(Xw(3.0 * rwristidx1), Xw(3.0 * rwristidx1 + 1), Xw(3.0 * rwristidx1 + 2));
        Vector3d rw2_3d(Xw(3.0 * rwristidx2), Xw(3.0 * rwristidx2 + 1), Xw(3.0 * rwristidx2 + 2));
    
        Vector3d le_avg = (le1_3d + le2_3d + le3_3d) / 3.0;
        Vector3d lw_avg = (lw1_3d + lw2_3d) / 2.0;
        Vector3d re_avg = (re1_3d + re2_3d + re3_3d) / 3.0;
        Vector3d rw_avg = (rw1_3d + rw2_3d) / 2.0;
    
        Vector3d lwv = lw_avg - ls3d; //arm directions
        Vector3d rwv = rw_avg - rs3d;
        Vector3d lev = le_avg - ls3d; //arm directions
        Vector3d rev = re_avg - rs3d;
        
        if (lengthenarms) {
        //lengthen arms by 33%
        double armscale = 1.33;
        le_avg = lev * armscale + ls3d;
        lw_avg = lwv * armscale + ls3d;
        re_avg = rev * armscale + rs3d;
        rw_avg = rwv * armscale + rs3d;
        }
        
        float le_d = (le_avg - ls3d).norm();
        float lw_d = (lw_avg - ls3d).norm();
        float re_d = (re_avg - rs3d).norm();
        float rw_d = (rw_avg - rs3d).norm();
    
        //target the above indices to these points
        //rotate -Y by some number of degrees for target, just gonna cheat and say 90 here
        Vector3d le_tpose(ls3d[0] + le_d, ls3d[1], ls3d[2]);
        Vector3d lw_tpose(ls3d[0] + lw_d, ls3d[1], ls3d[2]);
        Vector3d re_tpose(rs3d[0] - re_d, rs3d[1], rs3d[2]);
        Vector3d rw_tpose(rs3d[0] - rw_d, rs3d[1], rs3d[2]);
        
        if (!tposewarp) {
        le_tpose = le_avg;
        lw_tpose = lw_avg;
        re_tpose = re_avg;
        rw_tpose = rw_avg;
        }
        
        if (tposewarp || lengthenarms) {
        //make a reduced regression problem using only the indices of the shoulders wrists and elbows
        VectorXd y_tarms(12 * 3);
        y_tarms << ls3d, le_tpose, le_tpose, le_tpose, lw_tpose, lw_tpose, rs3d, re_tpose, re_tpose, re_tpose, rw_tpose, rw_tpose ;
        MatrixXd pca_tarms(12*3, pca.cols());
        VectorXd w0_tarms(12*3);
    
    //initialize w0 and pca with rows corresponding to indices of joints
        pca_tarms << pca.block(3 * lshoulderidx,0, 3, pca.cols()), pca.block(3 *lelbowidx1,0, 3, pca.cols()), pca.block(3 *lelbowidx2,0, 3, pca.cols()), pca.block(3 *lelbowidx3,0, 3, pca.cols()), pca.block(3 *lwristidx1,0, 3, pca.cols()), pca.block(3 *lwristidx2,0, 3, pca.cols()), pca.block(3 * rshoulderidx,0, 3, pca.cols()), pca.block(3 * relbowidx1,0, 3, pca.cols()), pca.block(3 * relbowidx2,0, 3, pca.cols()), pca.block(3 * relbowidx3,0, 3, pca.cols()), pca.block(3 * rwristidx1,0, 3, pca.cols()), pca.block(3 * rwristidx2,0, 3, pca.cols());
    cout << "initialize pca tarms " << endl;
        w0_tarms << w0.segment(3 * lshoulderidx, 3), w0.segment(3 * lelbowidx1, 3), w0.segment(3 * lelbowidx2, 3), w0.segment(3 * lelbowidx3, 3), w0.segment(3 * lwristidx1, 3), w0.segment(3 * lwristidx2, 3), w0.segment(3 * rshoulderidx, 3), w0.segment(3 * relbowidx1, 3), w0.segment(3 * relbowidx2, 3), w0.segment(3 * relbowidx3, 3), w0.segment(3 * rwristidx1, 3), w0.segment(3 * rwristidx2, 3); 
        cout << "initialize w0 tarms " << endl;
    
        w = ridgeRegression(pca_tarms, y_tarms, w0_tarms, 0.3 * sigmas, w_init); //downweight regularization here or arms dont move
        Xw = w0 + pca * w;
        
        }

    for (int l = 0; l < Xw.rows() / 3.0; l++)
    {
        //cout << l << endl;
        string outputline = to_string(Xw[3.0 * l]) + " " + to_string(Xw[3.0 * l + 1]) + " " + to_string(Xw[3.0 * l + 2]);
        scaleandtranslate << outputline.c_str() << endl; //why does \n not work? apparently you need to flush with endl.
        //scaleandtranslate << l << endl;
    }
    
    //while not converged...
    while (!converged && iters < maxiters)
    {
        iters += 1;
        cout << iters << endl;
        
        //12/11/2020
        //recompute surface normal for matching
        vector<Vector3d> currentPoints = VectorXdToArray(Xw);
        //cout << "meep" << endl;
        vector<Vector3d> currentNormals;
        if (matchNormals) currentNormals = recomputeSurfaceNormals(currentPoints, templatePNF[2]);
        //cout << "moop" << endl;

        vector<int> misses;
        mpairs = 0;
        matchpairs.clear();
        edges.clear();
        
        //search KD tree for closest match to query in template (PCA) space.
        //in closest k matches, pick closest point such that n1 * n2 > 0
        //pragma omp parallelize: need to avoid race conditions with queryPt, nnidx, dists
        for (int i = 0; i < templatePNF[0].size(); i++)
        {
            //cout << "Matching point " << i << endl;
            
            query[i][0] = Xw(3.0 * i);
            query[i][1] = Xw(3.0 * i + 1);
            query[i][2] = Xw(3.0 * i + 2);
        }
        // construct an randomized kd-tree index using 1 kd-trees
        index.knnSearch(query, indices, dists, nn, flann::SearchParams(-1));
        for (int i = 0; i < templatePNF[0].size(); i++)
        {   
            Vector3d hit; 
            
            if (matchNormals) hit = closestNormalMatch(indices[i], dists[i], targetPNF[0], templatePNF[0][i], targetPNF[1], currentNormals[i],nn, maxD);  
            else hit = targetPNF[0][indices[i][0]];
            
            if (hit.norm() != 0.0) //no nullptr, so check if the return is the origin. This is a signal for no match
            {
                y(3.0 * i) = hit[0];
                y(3.0 * i + 1) = hit[1];
                y(3.0 * i + 2) = hit[2];
                
                // also make a new list of pairs to draw pins diagram to illustrate matches
                string tempquery = to_string(query[i][0]) + " " + to_string(query[i][1]) + " " + to_string(query[i][2]) + " 0 255 0" + "\n";
                string mvehit = to_string(hit[0]) + " " + to_string(hit[1]) + " " + to_string(hit[2]) + " 0 0 255" + "\n";
                string edgeline = to_string(mpairs) + " " + to_string(mpairs + 1) + " 255 0 0" + "\n";
                
                matchpairs.push_back(tempquery);
                matchpairs.push_back(mvehit);
                edges.push_back(edgeline);
                mpairs += 2;
            }
            else
            {
                misses.push_back(i);
            }
        }
        
        MatrixXd pca_reduced = reduceRows(pca, misses);
        VectorXd w0_reduced = reduceRowsVector(w0, misses);
        VectorXd y_reduced = reduceRowsVector(y, misses);
        
    /*
        cout << pca.rows() << endl;
        cout << pca.cols() << endl;
        
        cout << y.rows() << endl;
        cout << y.cols() << endl;
        
        cout << misses.size() << endl;
        
        cout << pca_reduced.rows() << endl;
        cout << pca_reduced.cols() << endl;
        
        cout << y_reduced.rows() << endl;
        cout << y_reduced.cols() << endl;
    */
        w = ridgeRegression(pca_reduced, y_reduced, w0_reduced, sigmas, w_init);
    
        Xw = w0 + pca * w;
        
        MatrixXd Xw_diff = Xw - Xw_prev;
        double xwdn = Xw_diff.norm();
        //arbitrary convergence check
        cout << xwdn << endl;
        converged = xwdn <= convergencetol;
        Xw_prev = VectorXd(Xw);

        if (matchNormals && xwdn <= convergencetol * 10.0) matchNormals = false; //switch off normal matching for faster convergence when shape gets close 
    }

    cout << "Computed Xw." << endl;
    
    //endloop
    
    // write resulting match to file
    
    for (int l = 0; l < Xw.rows() / 3.0; l++)
    {
        string outputline = to_string(Xw[3.0 * l]) + " " + to_string(Xw[3.0 * l + 1]) + " " + to_string(Xw[3.0 * l + 2]) + "\n";
        outputfile << outputline.c_str();
    }
    
    //write faces to file
    for (int fi = 0; fi < templatePNF[2].size(); fi++)
    {
        string faceline = "3 " + to_string(int(templatePNF[2][fi][0])) +  " " + to_string(int(templatePNF[2][fi][1])) + " " + to_string(int(templatePNF[2][fi][2]));
        outputfile << (faceline + '\n').c_str();
        scaleandtranslate << (faceline + '\n').c_str();
    }
    
    writeCorrelations(matchpairs, edges, matchdiagram);
    
    //6/4/2020
    //Predict body composition from resulting fit
    /*
    VectorXd wfinal_1 (ptofcols + 1);
    wfinal_1 << w.head(ptofcols), 1.0; //#6/16/2020 cut the prediction matrix to prevent overfitting //now 70
    cout << PCAToPredictions_M.rows() << endl;
    cout << PCAToPredictions_M.cols() << endl;
    MatrixXd PCAToPredictions_M_block = PCAToPredictions_M.block(0, 0, PCAToPredictions_M.rows(), ptofcols); //70 components to stay consistent with previous
    */
    
    //use all components
    VectorXd wfinal_1 (PCAToPredictions_M.cols());
    wfinal_1 << w, 1.0;
    VectorXd statresults = PCAToPredictions_M * wfinal_1;
    
    double cbexp = 1.0;
    
    double fmassresult = pow(statresults[2], cbexp);
    double lmassresult = pow(statresults[3], cbexp);
    double pfatresult = statresults[7]; //direct regression
    //double pfatmatrix = pow(statresults[4], 3);readSil
    double visceralfat = pow(statresults[4], cbexp);
    double armleanmassresult = pow(statresults[5], cbexp);
    double legleanmassresult = pow(statresults[6], cbexp);
    double appLMIresult = (legleanmassresult + armleanmassresult) / (inputHeight * inputHeight);
    double FMI = fmassresult / pow(inputHeight, 2);
    double LMI = lmassresult / pow(inputHeight, 2);
    double trunkfatmass = pow(statresults[8] , cbexp);
    double trunkleanmass = pow(statresults[9] , cbexp);
    double armfatmass = pow(statresults[10] , cbexp);
    double legfatmass = pow(statresults[11] , cbexp);
    
    ofstream fatresults;
    fatresults.open(output_prefix + "/selfiereturnlinear.txt", ofstream::out);
    
    fatresults << (to_string(pfatresult) + '\n').c_str();
    fatresults << (to_string(lmassresult) + '\n').c_str();
    fatresults << (to_string(fmassresult) + '\n').c_str();
    fatresults << (to_string(visceralfat) + '\n').c_str();
    fatresults << (to_string(legleanmassresult) + '\n').c_str();
    fatresults << (to_string(armleanmassresult) + '\n').c_str();    
    fatresults << (to_string(appLMIresult) + '\n').c_str();
    fatresults << (to_string(FMI) + '\n').c_str();
    fatresults << (to_string(LMI) + '\n').c_str();
    fatresults << (to_string(trunkfatmass) + '\n').c_str();
    fatresults << (to_string(trunkleanmass) + '\n').c_str();
    fatresults << (to_string(armfatmass) + '\n').c_str();
    fatresults << (to_string(legfatmass) + "\n\n").c_str();
    
    VectorXd bennettpredictions = PCDA_predictions(w, inputHeight, mf);
    ofstream bennettfatresults;
    bennettfatresults.open(output_prefix + "/selfiereturnbennett.txt", ofstream::out);
    bennettfatresults << to_string(bennettpredictions[0]) << endl;
    bennettfatresults << to_string(bennettpredictions[1]) << endl;
    bennettfatresults << to_string(bennettpredictions[2]) << endl;
    bennettfatresults << to_string(bennettpredictions[3]) << endl;
    bennettfatresults << to_string(bennettpredictions[4]) << endl;
    bennettfatresults << to_string(bennettpredictions[5]) << endl;
    bennettfatresults << to_string(bennettpredictions[6]) << endl;
    bennettfatresults << to_string(bennettpredictions[7]) << endl;
    bennettfatresults << to_string(bennettpredictions[8]) << endl;
    bennettfatresults << to_string(bennettpredictions[9]) << endl;
    bennettfatresults << to_string(bennettpredictions[10]) << endl;
    bennettfatresults << to_string(bennettpredictions[11]) << endl;
    bennettfatresults << to_string(bennettpredictions[12]) << endl;

    fatresults << "PC Weights:" << endl;
    bennettfatresults << "PC Weights:" << endl;
    
    for (int p = 0; p < w.size(); p++) {
        fatresults << to_string(w[p]) << endl;
        bennettfatresults << to_string(w[p]) << endl;
    }
}


