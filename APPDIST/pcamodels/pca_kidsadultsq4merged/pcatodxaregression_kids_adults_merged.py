import numpy as np
import sys
from sklearn import linear_model
from sklearn.metrics import r2_score
import math

f = open("../../dxa/merged_SUAQ4ADL_SUKQ2.csv", 'r')
trainingpccsv = open(sys.argv[1], 'r')
gender = sys.argv[2]
#pcregdir = sys.argv[1][:sys.argv[1].rindex('/')+1]
pcregdir = "./"

lines = f.readlines()
pcvecs = trainingpccsv.readlines()

labels = lines.pop(0).replace('"','').split(',')
labelspcs = pcvecs.pop(0).replace('"','').split(',')

subjiddxa = [l.replace('"','').split(',')[0] for l in lines]
subjidpc = [l.replace('"','').split(',')[0] for l in pcvecs]

wtindx = labels.index("DXA_WBTOT_MASS")
htindx = labels.index("DXA_HEIGHT")
ageindx = labels.index("AGE")

#print htindx
#print wtindx
#rows = len(lines) #THIS WAS A MAJOR PROBLEM, DXA FILE IS NOT UNIQUE
rows = len(pcvecs)
#cols = len(labelspcs) - 2 
cols = int(sys.argv[3]) # how many pca to build on

if cols < 0:
    cols = len(labelspcs) - 2

print(rows)
print(cols)

features = np.zeros((4, rows), dtype=np.float64)
pcomps = np.zeros((cols, rows), dtype=np.float64)

i = 0

cbrtexp = (1. / 3.) #change this to 1 to get rid of cbrt
cbrtexp = 1.0


#Section 4.3 of allen 2003
for line in pcvecs:
    pcs = line.replace('"','').split(',')
    #print len(pcs)
    sid = pcs[0]
    print(sid)
    #print subjiddxa.index(sid
    dxaline = lines[subjiddxa.index(sid)].replace('"','').split(',')

    #if len(dxaline) != len(labels):
    #	print dxaline[0]
    if not dxaline[wtindx] or dxaline[wtindx].isspace():
        print(sid + " HAS NO DXA")
        continue

    wtcbrt = (float(dxaline[wtindx]) / 1000.0) ** cbrtexp
    features[0, i] = wtcbrt    #weight cube rooted
    features[1, i] = float(dxaline[htindx]) / 100.0 #height in m
    features[2, i] = math.floor(float(dxaline[ageindx])) #age in years, rounded down since no one says im 16.89 years old
    features[3, i] = 1

    for j in range(0, cols):
        pcomps[j, i] = float(pcs[j+1])
        #print pcomps[j,i]
    i+=1

M = np.dot(pcomps, np.linalg.pinv(features))

mbinname = "malefeaturestopca_"
if gender == "1":
	mbinname = "femalefeaturestopca_"

mbin = open(pcregdir + '/' + mbinname + str(cols) + ".bin", 'wb')
mbin.write(np.int32(cols))    #write dim of matrix
mbin.write(np.int32(4)) #THIS IS 4 WHEN THERE IS AGE
M.tofile(mbin)

ftopname = "malef_to_p_"
if gender == "1":
	ftopname = "femalef_to_p_"

np.save(pcregdir + '/' + ftopname + str(cols), M)

#now find the mapping from PCA to features.
i = 0
features = np.delete(features, [2,3], 0)
features = np.append(features, np.zeros((5, rows)),0)
features = np.append(features, np.zeros((5, rows)),0)


for line in pcvecs:
    pcs = line.replace('"','').split(',')
    sid = pcs[0]
    #print sid
    
    dxaline = lines[subjiddxa.index(sid)].replace('"','').split(',')
    if not dxaline[wtindx] or dxaline[wtindx].isspace():
        print(sid + " HAS NO DXA")
        continue

    features[2, i] = (float(dxaline[labels.index("DXA_WBTOT_FAT")]) / 1000.0) ** cbrtexp #fat mass kg
    features[3, i] = (float(dxaline[labels.index("DXA_WBTOT_LEAN")]) / 1000.0) ** cbrtexp #lean mass kg
    features[4, i] = (float(dxaline[labels.index("DXA_VFAT_MASS")]) / 1000.0) ** cbrtexp # visceral fat kg
    features[5, i] = (float(dxaline[labels.index("DXA_ARM_LEAN")]) / 1000.0) ** cbrtexp # arm lean kg
    features[6, i] = (float(dxaline[labels.index("DXA_LEG_LEAN")]) / 1000.0) ** cbrtexp # leg lean kg
    features[7, i] = (float(dxaline[labels.index("DXA_WBTOT_PFAT")])) # pfat
    features[8, i] = (float(dxaline[labels.index("DXA_TRUNK_FAT")]) / 1000.0) ** cbrtexp # trunk fat
    features[9, i] = (float(dxaline[labels.index("DXA_TRUNK_LEAN")]) / 1000.0) ** cbrtexp # trunk ffm
    features[10, i] = (float(dxaline[labels.index("DXA_ARM_FAT")]) / 1000.0) ** cbrtexp # arm fat
    features[11, i] = (float(dxaline[labels.index("DXA_LEG_FAT")]) / 1000.0) ** cbrtexp # leg fat
    
    i+=1

#same idea as section 4.3, make a row of 1s. This is the value of the feature for the average, i.e. when all the pca offsets are 0

x = []
y_r2 = []

cutcols = cols

pcompsfull = np.copy(pcomps)
featuresfull = np.copy(features)
#for regrdim in range(1, cols+1):
for regrdim in range(cols, cols+1):
    print(regrdim)
    #pcomps = pcompsfull[0:regrdim, 0:cutcols] #wtf?? why is this only using the first c subjects?
    #features = featuresfull[: , 0:cutcols]
    #pcomps = np.append(pcomps, np.ones((1, cutcols)), 0)
    
    pcomps = pcompsfull[0:regrdim, :]
    features = featuresfull[: , :]
    pcomps = np.append(pcomps, np.ones((1, pcomps.shape[1])), 0)
    print(pcomps.shape)
    print(cutcols)
    
    Mptof = np.dot(features, np.linalg.pinv(pcomps)) #this can produce overfitting
    minvbinname = "malepcatofeatures_"
    if gender == "1":
        minvbinname = "femalepcatofeatures_"
        
    minvbin = open(pcregdir + '/' + minvbinname + str(regrdim) + ".bin", 'wb')
    
    minvbin.write(np.int32(features.shape[0]))
    minvbin.write(np.int32(regrdim + 1))
    Mptof.tofile(minvbin)
    
    ptofname = "malep_to_f_"
    if gender == "1":
	    ptofname = "femalep_to_f_"
    
    np.save(pcregdir + '/' + ptofname + str(regrdim), Mptof)
    print(Mptof[:,-1])    #average stats
