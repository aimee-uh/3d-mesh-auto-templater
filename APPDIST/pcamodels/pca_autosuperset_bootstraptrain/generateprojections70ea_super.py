import numpy as np
import sys
import cPickle as pickle
import glob
from sklearn.decomposition import PCA
import pcaio

pcaf = open(sys.argv[1], 'rb')
avgplyfname = sys.argv[2]
gchar = sys.argv[1].split('/')[-1][0] #first character is either m or f 
print avgplyfname
avgplyv, ah, af = pcaio.readTemplatePlyFile(open(avgplyfname, 'r'))

pca = pickle.load(pcaf)
rotation = pca.components_

csvname = gchar + "_pcavecproj.csv"
projectioncsv = open(csvname, 'w')
print csvname
projectioncsv.write("Subject ID,")
for i in range(0, rotation.shape[0]):
    projectioncsv.write("PC" + str(i) + ",")
projectioncsv.write('\n')

trainingmeshes = glob.glob(sys.argv[3] + '/*.ply') + glob.glob(sys.argv[4] + '/*.ply') + glob.glob(sys.argv[5] + '/*.ply')

for s in trainingmeshes:
    #ply file name only
    st = s.split('/')[-1][:-4]
    #remove the _fitted part of the string
    stf = st.split('_')[0]
    #print stf
	
    subj = stf
    print subj
    verts, header, faces = pcaio.readTemplatePlyFile(open(s, 'r'))
    verts = verts - avgplyv
    verts = verts.reshape(rotation.shape[1],1)
    projection = np.dot(rotation, verts)
    projectioncsv.write(subj + ",")
    for p in projection:
        projectioncsv.write(str(p[0]) + ",") 
    
    projectioncsv.write('\n')
