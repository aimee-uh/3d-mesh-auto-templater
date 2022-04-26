import numpy as np
import sys
import surfacenormals

def readTemplatePlyFile(infile):
	headerEnd = False
	vertNum = 0
	headerstring = ""
	facelist = ""
	while not headerEnd:
		line = infile.readline()
		#print line
		if line == "end_header\n":
			headerEnd = True
		if "element vertex" in line:
			vertNum = int(line.split()[2])
			#print vertNum
		#if "float nx" not in line and "float ny" not in line and "float nz" not in line:
		headerstring += line
		
	vertices = np.zeros(vertNum * 3)
	for i in range(0, vertNum):
		coord = infile.readline()
		values = coord.split()
		point = [float(values[0]), float(values[1]), float(values[2])]
		vertices[i*3] = point[0]
		vertices[i*3 + 1] = point[1]
		vertices[i*3 + 2] = point[2]
		
	face = infile.readline()
	while len(face) > 0:
		facelist += face
		face = infile.readline()
	return vertices, headerstring, facelist

def writePCAtoPly(plyfile, points, header, facelist):
    plyfile.write(header)
    normals = surfacenormals.recomputeSurfaceNormals(points, facelist)
    for i in range(0, int(len(points) / 3)):
        line = str(points[i*3]) + ' ' + str(points[i*3 + 1]) + ' ' + str(points[i*3 + 2]) + ' ' + str(normals[i][0]) + ' ' + str(normals[i][1]) + ' ' + str(normals[i][2]) + '\n'
        plyfile.write(line)
    plyfile.write(facelist)

def projectPCAshape(pcanpy, targetmesh, averagemesh, gender):
    print(targetmesh)
    #read PCA matrix
    pca = np.load(pcanpy)
    #read ply file
    verts, header, faces = readTemplatePlyFile(open(targetmesh, 'r'))
    uverts, uheader, ufaces = readTemplatePlyFile(open(averagemesh, 'r')) #need to read the average in
    gender = float(gender)

    #Aw + u = s
    #A^-1(s - u) = w
    poff = verts - uverts

    #Multiply ply vector by matrix transpose to transform 3D coordinates to PCA coordinates
    projection = np.linalg.pinv(pca).dot(poff)

    #print results
    #for p in projection:
    #    print p
    return projection

if __name__ == '__main__':

    projection = projectPCAshape(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
    
    #predict the stats now
    ptofnpy = np.load("../regression/malep_to_f_180.npy")
    if gender == 1:
        ptofnpy = np.load("../regression/femalep_to_f_180.npy")

    print("predicted features:")
    projection_1 = np.append(projection, 1.0)
    pred = np.dot(ptofnpy, projection_1)
    for p in pred:
        print(p)
