#1/27/2020
#Returns a vector where each index i contains the indices of all vertices that share an edge with this vertex. Necessary for computing normals for shading
def buildNeighborTree(facelist, size):
    neighbors =  [ [] for _ in range(size) ]
    for i in range(0, len(facelist)):
        face = facelist[i];
        if face[1] not in neighbors[face[0]:
            neighbors[face[0]].append(face[1])
        if face[2] not in neighbors[face[0]:
            neighbors[face[0]].append(face[2])
            
        if face[0] not in neighbors[face[1]:
            neighbors[face[1]].append(face[0])
        if face[2] not in neighbors[face[1]:
            neighbors[face[1]].append(face[2])
            
        if face[0] not in neighbors[face[2]:
            neighbors[face[2]].append(face[0])
        if face[1] not in neighbors[face[2]:
            neighbors[face[2]].append(face[1])
            
    return neighbors

