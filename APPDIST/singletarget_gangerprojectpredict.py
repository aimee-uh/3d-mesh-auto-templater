import laplacianply
import numpy as np
import glob
import sys
import subprocess
from os import path
import projectPCA

overwrite = False

#wrapper for laplacianply
#takes PCA fit result, averages it with HC-laplacian
#saves that output and passes it to ganger

#later: can use projectPCA to solve for projection prediction


targetfolder = sys.argv[1]
originalscan = sys.argv[2] #the original input raw scan

ply = targetfolder + "/result.ply"

#ganger command
# Input: <sourcefile.ply> <sourcefile.mkr> <targetfile.ply> <targetfile.mkr> 
# Output: <targetfile.ply_deform.ply> the deformed mesh data

iters = 12
smoothedply = ply[:ply.index('.ply')] + "_hcsmooth" + str(iters) + ".ply"
print(smoothedply)

laplacianply.doHCLaplacianAndSave(ply, iters)

processes = []
pargs = " ".join(["./ganger/build/Deformation", smoothedply, "./ganger/build/blank.mkr", originalscan, "./ganger/build/blank.mkr"])
print(pargs)
    
prefix = ply[:ply.rindex('/')]
print(prefix)  
#run these in parallel?
stdoutfile=None
stderrfile=None
try:
    stdoutfile = open(prefix + "/gangerstdout.txt", 'w')
    stderrfile = open(prefix + "/gangerstderr.txt", 'w')
except IOError as e:
    print(e)
processes.append(subprocess.Popen(pargs, stdout=stdoutfile, stderr=stderrfile, shell=True))
processes[0].wait()

    
gangerply = targetfolder + "/result_hcsmooth12.ply_deform.ply"


pcabasedir = sys.argv[3]
pcomps = sys.argv[4]
gender = sys.argv[5] #0 or 1
gstr= 'males'
if gender == '1':
    gstr = 'females'
    
pcanpy = glob.glob(pcabasedir + 'pcafiles/' + gstr[0] + 'pca8020_*_np.npy')[0]
pcam = np.load(pcanpy)
if int(pcomps) < 0:
    pcomps = pcam.shape[1] #use max number of columns

print(("# pca comps: " + str(pcomps)))
print(pcanpy)
averagemesh = pcabasedir + 'pcafiles/'+ gstr[0] + '_average_superset.ply'
linregrmtx = np.load(pcabasedir + 'pcafiles/regressions/' + gstr[:-1] + 'p_to_f_' + str(pcomps) + '.npy')

avgverts, header, faces = projectPCA.readTemplatePlyFile(open(averagemesh, 'r'))

numpcomps = linregrmtx.shape[1] - 1
    
predictfiletxt = targetfolder + '/gangerrefinedprojectpredict_' + str(numpcomps) + '.csv'
projectshapeply = targetfolder + '/gangerRefinedProjectedShape_' + str(numpcomps) + '.ply'
            
#by default ganger outputs binary. Convert here
pargs = " ".join(['./ply2asc <', './' + gangerply,  '> ./' + gangerply[:-4] + '_ascii.ply'])
print("\n\n")
print(pargs)
print("\n\n")
pr = subprocess.Popen(pargs, shell=True)
pr.wait()
plyascii = gangerply[:-4] + "_ascii.ply"
    
#writes projected shape as ply and the predicted stats into text
gangerprojectedpredicts = open(predictfiletxt, 'w')
projected = projectPCA.projectPCAshape(pcanpy, plyascii ,averagemesh, gender)
projshape = np.dot(pcam[:, :numpcomps], projected[:numpcomps]) + avgverts
projectPCA.writePCAtoPly(open(projectshapeply, 'w'), projshape, header, faces)
    
linpredictions = np.dot(linregrmtx, np.append(projected[:numpcomps],  1.0))
    
#THESE ARE NOT NECESSARILY IN THE SAME ORDER

predictedlabels = ["DXA_WEIGHT", "DXA_HEIGHT", "DXA_WBTOT_FAT", "DXA_WBTOT_LEAN", "DXA_VFAT_MASS", "DXA_ARM_LEAN", "DXA_LEG_LEAN", "DXA_WBTOT_PFAT", "DXA_TRUNK_FAT", "DXA_TRUNK_LEAN", "DXA_ARM_FAT", "DXA_LEG_FAT"]

gangerprojectedpredicts.write('linear predictions ' + str(numpcomps) + ':\n')
for i in range(len(predictedlabels)):
    p = linpredictions[i]
    l = predictedlabels[i]
    gangerprojectedpredicts.write(l + ", " + str(p) + '\n')
