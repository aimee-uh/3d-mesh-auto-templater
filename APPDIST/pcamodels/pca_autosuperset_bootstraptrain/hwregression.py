import numpy as np
import sys
import math

#python hwregression.py superset/pcafiles/m_pcavecproj.csv 0

f = open("../dxa/SUA_DXA_2021-05-06_V23.csv", 'r')
trainingpccsv = open(sys.argv[1], 'r')  #this was generated from the training set meshes. PC projections onto PCA basis. DXA file does not have unique rows
gender = sys.argv[2]

lines = f.readlines()
pcvecs = trainingpccsv.readlines()

labels = lines.pop(0).replace('"','').split(',')
labelspcs = pcvecs.pop(0).replace('"','').split(',')

subjiddxa = [l.replace('"','').split(',')[0] for l in lines]
subjidpc = [l.replace('"','').split(',')[0] for l in pcvecs]

wtindx = labels.index("DXA_WBTOT_MASS")
htindx = labels.index("DXA_HEIGHT")

rows = len(pcvecs)

inputFeatures = np.zeros((3, rows), dtype=np.float64)
predictedFeatures = np.zeros((12, rows), dtype=np.float64)

i = 0

#cbrtexp = (1. / 3.) #change this to 1 to get rid of cbrt
cbrtexp = 1.0

#Section 4.3 of allen 2003
for line in pcvecs:
    pcs = line.replace('"','').split(',')
    sid = pcs[0]
    print(sid)

    if sid not in subjiddxa:
        print(sid + " is missing from DXA")
        continue
        
    dxaline = lines[subjiddxa.index(sid)].replace('"','').split(',') #find line in DXA sheet corresponding to this subject

    wtcbrt = (float(dxaline[wtindx]) / 1000.0) ** cbrtexp
    inputFeatures[0, i] = wtcbrt    #weight cube rooted
    inputFeatures[1, i] = float(dxaline[htindx]) / 100.0 #height in m
    inputFeatures[2, i] = 1
    
    predictedFeatures[0, i] =  wtcbrt    #weight cube rooted
    predictedFeatures[1, i] = float(dxaline[htindx]) / 100.0 #height in m
    predictedFeatures[2, i] = (float(dxaline[labels.index("DXA_WBTOT_FAT")]) / 1000.0) ** cbrtexp #fat mass kg
    predictedFeatures[3, i] = (float(dxaline[labels.index("DXA_WBTOT_LEAN")]) / 1000.0) ** cbrtexp #lean mass kg
    predictedFeatures[4, i] = (float(dxaline[labels.index("DXA_VFAT_MASS")]) / 1000.0) ** cbrtexp # visceral fat kg
    predictedFeatures[5, i] = (float(dxaline[labels.index("DXA_ARM_LEAN")]) / 1000.0) ** cbrtexp # arm lean kg
    predictedFeatures[6, i] = (float(dxaline[labels.index("DXA_LEG_LEAN")]) / 1000.0) ** cbrtexp # leg lean kg
    predictedFeatures[7, i] = (float(dxaline[labels.index("DXA_WBTOT_PFAT")])) # pfat
    predictedFeatures[8, i] = (float(dxaline[labels.index("DXA_TRUNK_FAT")]) / 1000.0) ** cbrtexp # trunk fat
    predictedFeatures[9, i] = (float(dxaline[labels.index("DXA_TRUNK_LEAN")]) / 1000.0) ** cbrtexp # trunk ffm
    predictedFeatures[10, i] = (float(dxaline[labels.index("DXA_ARM_FAT")]) / 1000.0) ** cbrtexp # arm fat
    predictedFeatures[11, i] = (float(dxaline[labels.index("DXA_LEG_FAT")]) / 1000.0) ** cbrtexp # leg fat
    i+=1

M = np.dot(predictedFeatures, np.linalg.pinv(inputFeatures))

mbinname = "male_hwtoallstats_"
if gender == "1":
    mbinname = "female_hwtoallstats_"

mbin = open(mbinname + ".bin", 'wb')
mbin.write(np.int32(2))    #write dim of matrix
mbin.write(np.int32(3))
M.tofile(mbin)

ftopname = "male_hwtoallstats"
if gender == "1":
    ftopname = "female_hwtoallstats"

np.save(ftopname, M)


