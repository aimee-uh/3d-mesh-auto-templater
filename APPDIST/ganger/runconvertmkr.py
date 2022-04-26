import sys
import os
import subprocess
import numpy as np
from subprocess import Popen
import glob
import fnmatch

markerlessFits = glob.glob(sys.argv[1] + "*/d=-1/result.ply")

#compute marker locations for markerless fits using auto templating
for queryPly in markerlessFits:
    
    sid = queryPly[:-16]
    sid = sid[sid.rindex('/')+1:]
    sid = sid[1:sid.index('_')]
    print sid
    
    targetPly = glob.glob(sys.argv[2] + "/" + sid + "*.ply" )[0]
    processes=[]

    pargs = " ".join(["python", "baryMkrToXYZ.py", "template-60k-m-adjust.mkr", queryPly, targetPly])
    print pargs
    processes.append(subprocess.Popen(pargs, shell=True))
    processes[-1].wait()
    os.rename(queryPly[:queryPly.rindex('/')+1] + "rawtarget.ply", queryPly[:queryPly.rindex('/')+1] + "markersPCAfit.ply")
    os.rename(queryPly[:queryPly.rindex('/')+1] + "rawtarget.mkr", queryPly[:queryPly.rindex('/')+1] + "markersPCAfit.mkr")

    #compute original marker locations on markered ganger fits    
    markeredGanger = glob.glob(sys.argv[3] + sid + "*.ply")[0]
    pargs = " ".join(["python", "baryMkrToXYZ.py", "template-60k-m-adjust.mkr", markeredGanger, targetPly])
    print pargs
    processes.append(subprocess.Popen(pargs, shell=True))
    processes[-1].wait()

    #move mkr and ply file from markered fit to directory under new names

    os.rename(sys.argv[3] + "rawtarget.ply", queryPly[:-10] + "markersManualGanger.ply")
    os.rename(sys.argv[3] + "rawtarget.mkr", queryPly[:-10] + "markersManualGanger.mkr")
