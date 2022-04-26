import glob
import numpy as np
import pcaio
from sklearn.decomposition import PCA
import pickle as pickle
import sys

#pca_kidsadultsq4merged]$ python buildpca.py ../../../pcatemplate_kids/pca_super/superset/trainingmeshes/males/ ../../../separatepcperdevice_102920/autofittedplys/SUPERSETTRAINAUGMESHES_TRAINING/males/ 0

trainingmeshes = glob.glob(sys.argv[1] + '/*.ply') + glob.glob(sys.argv[2] + '/*.ply') #merge kids and adults
gender = int(sys.argv[3])

gchar = 'm' if gender == 0 else 'f' 
lf = len(trainingmeshes)
verts_nxd = np.zeros((180003, lf))  
j=0

for p in trainingmeshes:
    print(p)
    verts, header, faces = pcaio.readTemplatePlyFile(open(p, 'r'))
    verts_nxd[:, j] = verts.T #assign jth column to this mesh    
    j+=1
        
avg = np.sum(verts_nxd, axis=1) / verts_nxd.shape[1]
plyfile = open(gchar + "_average_superset.ply", 'w')
pcaio.writePCAtoPly(plyfile, avg, header, faces) #write out average shape
mnp = np.subtract(verts_nxd,avg.reshape(180003, 1)) #subtract out average
print((mnp.shape))
#now do PCA, write out rotation matrix and SDs
pca = PCA() #we picked 80 for variance cutoff before
pca.fit(mnp.transpose())
print((pca.components_.shape))
#pickle pca
pickle.dump(pca, open(gchar + "_pca_superset", "wb"))
