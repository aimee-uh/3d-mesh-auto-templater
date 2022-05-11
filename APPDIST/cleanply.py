import numpy as np
import surfacenormals
import glob
import os
import sys

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

def cleanPly(p):

	#remove the confidence and color values
	print(p)
	verts, header, faces = readTemplatePlyFile(open(p, 'r'))

	hsplit = header.index("property float z")
	faceindex = header.index("element face")
	header = header[:hsplit+17] + "property float nx\nproperty float ny\nproperty float nz\n" + header[faceindex:]
	    
	outfile = open(p, 'w') #just overwrite
	writePCAtoPly(outfile, verts, header, faces)

