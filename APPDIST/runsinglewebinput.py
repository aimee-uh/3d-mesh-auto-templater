import sys
import os
import subprocess
import numpy as np
from subprocess import Popen
import glob
import fnmatch
from os import path

input_ply = sys.argv[1]
pcomps = int(sys.argv[2])
baseresdir = sys.argv[3]

print(baseresdir)

gender = sys.argv[4] #0 or 1
genderprefix = 'males'
if int(gender) == 1:
    genderprefix = 'females' 

subj_wt = float(sys.argv[5])
subj_height = float(sys.argv[6])

inputid = input_ply.split('/')[-1][:-4]

subjdir = baseresdir + "/" + genderprefix[0] + str(inputid)
print(subjdir)
resultdir = subjdir + "/d=" + str(pcomps)
stdoutfile=None
stderrfile=None
try:
    os.mkdir(subjdir)
    os.mkdir(resultdir)
except OSError as e:
    print(e)

try:
    stdoutfile = open(resultdir + "/stdout.txt", 'w')
    stderrfile = open(resultdir + "/stderr.txt", 'w')
except IOError as e:
    print(e)
       
processes = []

#run an instance of the selfie pipeline 
os.chdir("./C")
#lda = 0.001 #original
lda = 0.01
#lda = 0.1   
#lda = 0.0 #for training set members only
#tol = 0.01
tol = 0.1
#tol = 0.0 #training only
pargs = " ".join(["./pcamatch", "../" + input_ply, str(lda) ,str(gender), str(subj_height), str(subj_wt) , "../" + resultdir, str(tol)])
print(pargs)
processes.append(subprocess.Popen(pargs, stdout=stdoutfile, stderr=stderrfile, shell=True))
os.chdir("../")
processes[0].wait()