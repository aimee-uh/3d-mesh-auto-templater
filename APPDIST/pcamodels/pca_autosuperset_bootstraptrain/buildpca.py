import glob
import numpy as np
import pcaio
from sklearn.decomposition import PCA
import cPickle as pickle
import sys

meshdirs = [sys.argv[1], sys.argv[2], sys.argv[3]] #modified just for this use, point this at each device folder
trainingmeshes = []

for d in meshdirs:
    meshes = glob.glob(d + '/*.ply')
    trainingmeshes.extend(meshes) # as opposed to append
#print trainingmeshes
gender = int(sys.argv[4])

gchar = 'm' if gender == 0 else 'f' 
lf = len(trainingmeshes)
verts_nxd = np.zeros((180003, lf))  
j=0

for p in trainingmeshes:
    print p
    verts, header, faces = pcaio.readTemplatePlyFile(open(p, 'r'))
    verts_nxd[:, j] = verts.T #assign jth column to this mesh    
    j+=1
        
avg = np.sum(verts_nxd, axis=1) / verts_nxd.shape[1]
plyfile = open(gchar + "_average_superset.ply", 'w')
pcaio.writePCAtoPly(plyfile, avg, header, faces) #write out average shape
mnp = np.subtract(verts_nxd,avg.reshape(180003, 1)) #subtract out average
print mnp.shape
#now do PCA, write out rotation matrix and SDs
pca = PCA() #we picked 80 for variance cutoff before
pca.fit(mnp.transpose())
print pca.components_.shape
#pickle pca
pickle.dump(pca, open(gchar + "_pca_superset", "wb"))
