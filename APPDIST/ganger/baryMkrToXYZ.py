import numpy as np
import sys
import sklearn.neighbors as neighbors

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
    #	
    vertices = np.zeros(vertNum * 3)
    for i in range(0, vertNum):
        coord = infile.readline()
        values = coord.split()
        point = [float(values[0]), float(values[1]), float(values[2])]
        vertices[i*3] = point[0]
        vertices[i*3 + 1] = point[1]
        vertices[i*3 + 2] = point[2]
    #	
    face = infile.readline()
    while len(face) > 0:
        facelist += face
        face = infile.readline()
    return vertices, headerstring, facelist

mkrLines = open(sys.argv[1], 'r').readlines()
verts, header, faces = readTemplatePlyFile(open(sys.argv[2], 'r'))

mtype = mkrLines.pop(0)
numMkr = mkrLines.pop(0)

#mkrLines = mkrLines[:-10]

mkrArray = []

for ml in mkrLines:
    tokens = ml.split(' ')
    #print tokens
    if int(tokens[1]) == 0:
        mkrArray.append([0,0,0])
        continue
    i1 = int(tokens[3])
    i2 = int(tokens[4])
    i3 = int(tokens[5])

    b1 = float(tokens[6])
    b2 = float(tokens[7])
    b3 = float(tokens[8])

    v1 = np.array([verts[i1 * 3], verts[i1 * 3 + 1], verts[i1 * 3 + 2]])
    v2 = np.array([verts[i2 * 3], verts[i2 * 3 + 1], verts[i2 * 3 + 2]])
    v3 = np.array([verts[i3 * 3], verts[i3 * 3 + 1], verts[i3 * 3 + 2]])

    xyz = v1 * b1 + v2 * b2 + v3 * b3
    mkrArray.append(xyz)
    
#read in target file, find nearest neighbor
vertsT, headerT, facesT = readTemplatePlyFile(open(sys.argv[3], 'r'))

vertsT = vertsT.reshape(len(vertsT) / 3, 3)
knn = neighbors.NearestNeighbors(n_neighbors=1, radius=0.1)
knn.fit(vertsT)

mkrArray = np.array(mkrArray)
#print mkrArray.shape

ndist, nind = knn.kneighbors(mkrArray)

#output new target mkr file and ply file 
newmkrfile = open(sys.argv[2][:sys.argv[2].rindex('/')] + "/rawtarget.mkr", 'w')
mkrplyfile = open(sys.argv[2][:sys.argv[2].rindex('/')] + "/rawtarget.ply", 'w')

#write the headers to both files
newmkrfile.write("1\n")
newmkrfile.write(str(mkrArray.shape[0]))

mkrplyfile.write("ply\nformat ascii 1.0\nelement vertex " + str(len(mkrArray)) + "\nproperty float x\nproperty float y\nproperty float z\nend_header\n")
for ni in range(len(mkrArray)):
    v = vertsT[nind[ni][0]]
    newmkrfile.write(str(ni+1) + " 1 " + str(nind[ni]) + "\n")
    mkrplyfile.write(str(v[0]) + ' ' + str(v[1]) + ' ' + str(v[2]) + '\n')
   
