// translation of surfacenormals.py into c++
//assumes Eigen was already included

Vector3d computeAvgVector(vector<Vector3d> v_list) {
    Vector3d total(0.0, 0.0, 0.0);
    int n = v_list.size();
    if (n == 0) return total; //empty list
    
    for (int i = 0; i < n; i++) {
        total += v_list[i];    
    }
    
    total /= float(n);
    float totalnorm = total.norm();
    if (totalnorm == 0.0) {
        return Vector3d(0,0,0);
    }
    return total / totalnorm;
}

vector<Vector3d> recomputeSurfaceNormals(vector<Vector3d> points3d, vector<Vector3d> facelist)
{
    // points3d is a list of 3d points
    // facelist is a list of triangles, denoted by a triplet [i, j, k], where i, j, and k are indices into the list points3d.
    
    //for each triangle in facelist at index n...
    	// cross product (i - j) x (k - j), normalize (eigen has a function for this)
    	// append vector to some new list at index i, j, k. You may have multiple triangles assigned to the same point, so this will be a list of lists vector<vector<Vector3d>> newnormalslist
    
    //for each point in newnormalslist at index n
    	//average all of the vectors and normalize
    	//assign result to newnormals[n]

    // Initialize vector of vectors
    vector<vector<Vector3d>> newnormalslist;
    for (int i = 0; i < points3d.size(); i++) {
        vector<Vector3d> vec_list;
        newnormalslist.push_back(vec_list);
    }

    // find each normal vector
    //assumes default CCW face winding order
    for (int i = 0; i < facelist.size(); i++) {
        Vector3d v1 = points3d[facelist[i](0)] - points3d[facelist[i](1)];
        Vector3d v2 = points3d[facelist[i](2)] - points3d[facelist[i](1)];
        Vector3d v3 = v2.cross(v1);
        //v3.normalize(); //normalize after average to use a weighted average weighted by face area
        
        newnormalslist[facelist[i].x()].push_back(v3);
        newnormalslist[facelist[i].y()].push_back(v3);
        newnormalslist[facelist[i].z()].push_back(v3);
    }

    // take the average normal vector
    vector<Vector3d> newnormals;
    for (int i = 0; i < newnormalslist.size(); i++) {
        newnormals.push_back(computeAvgVector(newnormalslist[i]).normalized());
    }
    
    return newnormals;
}

//4/15/2018
//restructure this slightly to return the points and faces in vector<Vector3d> also. We need to operate on these to recompute normals every iteration.
vector<vector<Vector3d>> readMeshPlyFileFLANN(ifstream& infile, ofstream& outfile)
{
    bool headerEnd = false;
    int vertNum = 0;
    int faceNum = 0;
    while (!headerEnd)
    {
        string line;
        getline(infile, line);
        
        if (line.compare("end_header") == 0)
        {
            headerEnd = true;
        }
        if (line.find("element vertex") != string::npos)
        {
            string token;
            char linetoconstchar[line.size()];
            
            memcpy(linetoconstchar, line.c_str(), line.size());
            
            char* words = strtok (linetoconstchar, " ");
            while (words != NULL)
            {
                token = string(words);
                words = strtok(NULL, " ");
            }
            
            vertNum = stoi(token);
            
        }
        if (line.find("float nx") == string::npos && line.find("float ny") == string::npos && line.find("float nz") == string::npos)
        {
            outfile << (line + "\n").c_str();
            
        }
        if (line.find("element face") != string::npos)	//if faces found
        {
        	string token;
            char linetoconstchar[line.size()];
            
            memcpy(linetoconstchar, line.c_str(), line.size());
            
            char* words = strtok (linetoconstchar, " ");
            while (words != NULL)
            {
                token = string(words);
                words = strtok(NULL, " ");
            }
            
            faceNum = stoi(token);
        }
    }
    
    vector<Vector3d> normals(vertNum);
    vector<Vector3d> faces(faceNum);
    vector<Vector3d> points(vertNum);
    
    for (int i = 0; i < vertNum; i++)
    {
        string coord;
        getline(infile, coord);
        
        vector<string> splitcoords(6);
        
        char* values = strtok (const_cast<char*> (coord.c_str()), " ");
        int j = 0;
        while (values != NULL)
        {
            string token = string(values);
            splitcoords[j] = token;
            values = strtok(NULL, " ");
            j++;
        }
        
        Vector3d normal(stod(splitcoords[3], NULL), stod(splitcoords[4], NULL), stod(splitcoords[5], NULL));
        normal.normalize();  
        
        normals[i] = normal;
        
        Vector3d point3d(stod(splitcoords[0], NULL), stod(splitcoords[1], NULL), stod(splitcoords[2], NULL));
    	points[i] = point3d;
    }
    
    //now read faces
    for (int i = 0; i < faceNum; i++)
    {
    	string coord;
        getline(infile, coord);
        
        vector<string> splitcoords(4);
        
        char* values = strtok (const_cast<char*> (coord.c_str()), " ");
        int j = 0;
        while (values != NULL)
        {
            string token = string(values);
            splitcoords[j] = token;
            values = strtok(NULL, " ");
            j++;
        }
        
        Vector3d face(stod(splitcoords[1], NULL), stod(splitcoords[2], NULL), stod(splitcoords[3], NULL));
        faces[i] = face;
    }
    
    vector<vector<Vector3d>> returnvecs(3);
    returnvecs[0] = points;
    returnvecs[1] = normals;
    returnvecs[2] = faces;
    return returnvecs;
}

//reads the  pca vectors, which are supplied in binary and not plaintext

MatrixXd readPCA(ifstream& pca)
{
	// number of PCA vectors, should equal 125
	char ds[4];
	pca.read(ds, 4);
	char ns[4];
	pca.read(ns, 4);
	int n = *(int*)ds;
	int d = *(int*)ns;
	MatrixXd pcam(n , d);
    //cout << to_string(d) << endl;
    //cout << to_string(n) << endl;
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < d; j++)
		{
			char entry[8];
			pca.read(entry, 8);
            //Changed this to row major 6/4/2020
			pcam(i , j) = *(double*)entry;
		}
	}

	return pcam;
}

//reads an n length sigma vector and places entries of sqrt(alpha) / sigma_i on the diagonal of an n x n matrix
//This is the Tikhonov Matrix for regularizing
MatrixXd readSigmas(ifstream& sigmaFile, int dims, double alpha)
{
	MatrixXd tik(dims,dims);
    tik.setZero();
	for (int i = 0; i < dims; i++)
	{
		string sig;
		getline(sigmaFile, sig);
		double sigval = atof(sig.c_str());
		tik(i, i) = sqrt(alpha) / sigval;
	}
    return tik;
}

VectorXd ridgeRegression(MatrixXd A, VectorXd y, VectorXd average, MatrixXd sigmas, VectorXd initialShape)
{
	//A is the PCA matrix of dim < 180003 x 125
	VectorXd b = y - average;
    MatrixXd variances = (sigmas.transpose() * sigmas);
    //6/25/2020 regularize to initial shape not mean
    //(X^TX + sigmaI)^(-1)(X^Ty + sigma * w_i)
	//w is a length 125 vector as a result of tikhonov regularization
    VectorXd w = ( (A.transpose() * A) + variances).inverse() * (A.transpose() * b + variances * initialShape);
	return w;
}

VectorXd coordsToVector(vector<Vector3d>& c)
{
    int dim = c.size();
    VectorXd w0(dim * 3);
    for (int i = 0; i < dim; i++)
    {
        w0(i * 3) = c[i][0];
        w0(i * 3 + 1) = c[i][1];
        w0(i * 3 + 2) = c[i][2];
    }
    return w0;
}

vector<Vector3d> VectorXdToArray(VectorXd v)
{
    vector<Vector3d> convert(v.size() / 3);
    for (int i = 0; i < v.size() / 3; i++)
    {
        Vector3d vec(v[i*3], v[i*3+1], v[i*3+2]);
        convert[i] = vec;
    }
    return convert;
}

//takes n x d matrix m and returns a matrix of dimensions k x d where k < n and each row index in vector misses is removed
//does not modify m
MatrixXd reduceRows(MatrixXd& m, vector<int>& misses)
{
    if (misses.size() == 0) return m;
    
    MatrixXd newMatrix(m.rows() - misses.size(), m.cols());
    
    //the last index ommitted for regularization, initialized to -1.
    //block of size (thisIndex - lastindex) * d is copied into the block of equivalent size in the returned matrix.
    int lastIndex = -1;
    
    //which row to start copying block to
    int currentRow = 0;
    
    //use block operators to copy block up to row of miss index mi
    for (int mi = 0; mi < misses.size(); mi ++)
    {
        int cutIndex = misses[mi];
        int blockSize = cutIndex - (lastIndex + 1);
        //block size is between last index and cut index not including either
        newMatrix.block( currentRow, 0, blockSize , m.cols() ) = m.block(lastIndex + 1, 0, blockSize, m.cols());
        lastIndex = cutIndex;
        currentRow = currentRow + blockSize;
    }
    
    return newMatrix;

}

//takes n x d matrix m and returns a matrix of dimensions k x d where k < n and each row index in vector misses is removed
//does not modify m
//same as above but returns a vector
VectorXd reduceRowsVector(VectorXd& m, vector<int>& misses)
{
    if (misses.size() == 0) return m;
    
    VectorXd newVector(m.rows() - misses.size());
    
    //the last index ommitted for regularization, initialized to -1.
    //block of size (thisIndex - lastindex) * d is copied into the block of equivalent size in the returned matrix.
    int lastIndex = -1;
    
    //which row to start copying block to
    int currentRow = 0;
    
    //use block operators to copy block up to row of miss index mi
    for (int mi = 0; mi < misses.size(); mi ++)
    {
        int cutIndex = misses[mi];
        int blockSize = cutIndex - (lastIndex + 1);
        //block size is between last index and cut index not including either
        newVector.segment(currentRow, blockSize) = m.segment(lastIndex + 1, blockSize);
        lastIndex = cutIndex;
        currentRow = currentRow + blockSize;
    }
    
    return newVector;
    
}

//given indices of matches, list of mve points, query point, and number of hits k, return closest hit such that n_q * n_i > 0
//match must be within square root of max dist (in meters)
Vector3d closestNormalMatch(int* idx, double* dists, vector<Vector3d>& targetpts, Vector3d& q, vector<Vector3d>& targetn, Vector3d& qn, int k, double maxDist)
{
    for (int ki = 0; ki < k; ki++)
    {
        Vector3d potentialMatch = targetpts[idx[ki]];
        Vector3d tni = targetn[idx[ki]];
        
        double normalproduct = qn.normalized().dot( tni.normalized() );
        
        if (normalproduct > 0) return potentialMatch;
        //if (normalproduct > 0 && dists[ki] < maxDist) return potentialMatch; //not exactly metric
        //else return nullptr;

    }
    
    return Vector3d(0, 0, 0);
}


//writes a pins n needles visualization indicating where matches are
void writeCorrelations(vector<string> pairs, vector<string> edges, ofstream& pins)
{
    //write header first
    
    int vertex = pairs.size();
    int edgecount = edges.size();
    
    pins << "ply\n";
    pins << "format ascii 1.0\n";
    pins << (string("element vertex ") + to_string(vertex) + "\n").c_str();
    pins << "property float x\nproperty float y\nproperty float z\nproperty uchar red\nproperty uchar green\nproperty uchar blue\n";
    pins << (string("element edge ") + to_string(edgecount) + "\n").c_str();
    pins << "property int vertex1\nproperty int vertex2\nproperty uchar red\nproperty uchar green\nproperty uchar blue\n";
    pins << "end_header\n";
    
    for (int p1 = 0; p1 < vertex; p1+=2)
    {
        pins << pairs[p1].c_str();
        pins << pairs[p1 + 1].c_str();
    }
    for (int p2 = 0; p2 < edgecount; p2++)
    {
        pins << edges[p2].c_str();
    }
    
}
