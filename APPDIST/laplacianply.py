import numpy as np
import sys
#same as in projectPCA.py
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
    #normals = surfacenormals.recomputeSurfaceNormals(points, facelist)
    for i in range(0, int(len(points) / 3.0)):
        line = str(points[i*3]) + ' ' + str(points[i*3 + 1]) + ' ' + str(points[i*3 + 2]) + '\n'#' ' + str(normals[i][0]) + ' ' + str(normals[i][1]) + ' ' + str(normals[i][2]) + '\n'
        plyfile.write(line)
    plyfile.write(facelist)
    
def neighborlist(facelist, size):
    neighbors = [ [] for _ in range(size) ]
    facelist = facelist.split('\n')
    for i in range(0, len(facelist)):
        faceline = facelist[i].split(' ')
        if len(faceline) == 0 or faceline[0] != '3':
            continue
        #print faceline
        face = [int(faceline[1]), int(faceline[2]), int(faceline[3])]
        #print face
        if face[1] not in neighbors[face[0]]:
            neighbors[face[0]].append(face[1])
        if face[2] not in neighbors[face[0]]:
            neighbors[face[0]].append(face[2])
            
        if face[0] not in neighbors[face[1]]:
            neighbors[face[1]].append(face[0])
        if face[2] not in neighbors[face[1]]:
            neighbors[face[1]].append(face[2])
    
        if face[0] not in neighbors[face[2]]:
            neighbors[face[2]].append(face[0])
        if face[1] not in neighbors[face[2]]:
            neighbors[face[2]].append(face[1])
    #print neighbors
    return neighbors

#http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.93.4957&rep=rep1&type=pdf
#HC-Laplacian, Vollmer et al
def hclaplacianavg(vertices, neighbors, o):
    smoothed = np.zeros((int(len(vertices) / 3), 3))
    b_is = np.zeros((int(len(vertices) / 3), 3))
    alpha = 0.0 #original point influence
    beta = 0.5 #previous point influence
    #print len(vertices) / 3
    for v in range(0, int(len(vertices) / 3)): #vertices is a flattened array
        #print neighbors[v]
        #print v
        
        if len(neighbors[v]) > 0:
            #laplacian
            for n in neighbors[v]:
                smoothed[v] += np.array([vertices[n*3], vertices[n*3 + 1], vertices[n*3 + 2]])
            
            smoothed[v] /= float(len(neighbors[v]))
        oi = np.array([o[v*3], o[v*3 + 1], o[v*3 + 2]])
        qi = np.array([vertices[v*3], vertices[v*3 + 1], vertices[v*3 + 2]])
        b_is[v] = smoothed[v] - ((alpha * oi) + (1.0-alpha) * qi)
    for v in range(0, int(len(vertices) / 3)):
        if len(neighbors[v]) > 0:
            bsum = 0.0
            for n in neighbors[v]:
                bsum += b_is[n]
            smoothed[v] = smoothed[v] - (beta * b_is[v] + (1 - beta) / float(len(neighbors[v])) * bsum)
    return smoothed

#assimilates all the above functions to do hclaplacian on a file and save it as new mesh
def doHCLaplacianAndSave(fname, iters=1):
    v, h, f = readTemplatePlyFile(open(fname, 'r'))
    o = np.copy(v)
    neighbors = neighborlist(f, int(len(v) / 3))
    for i in range(iters):
        v = hclaplacianavg(v, neighbors, o)
        v = v.flatten()
        #print len(v)

    hcfile = fname[:fname.index('.ply')] + "_hcsmooth" + str(iters) + ".ply"
    writePCAtoPly(open(hcfile, 'w'), v, h, f)
    
def doHCLaplacian(v, f, iters=1):
    o = np.copy(v)
    neighbors = neighborlist(f, int(len(v) / 3))
    for i in range(iters):
        v = hclaplacianavg(v, neighbors, o)
        v = v.flatten()
        #print len(v)

    return v 

