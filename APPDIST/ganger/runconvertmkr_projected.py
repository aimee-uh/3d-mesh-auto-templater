import sys
import os
import subprocess
import numpy as np
from subprocess import Popen
import glob
import fnmatch

#same as original but computes the projected points

manualGangerProjected = sorted(glob.glob(sys.argv[1] + "*/gangerProjectedShape.ply"))
markerlessRefinedGangerProjected = sorted(glob.glob(sys.argv[1] + "*/gangerRefinedProjectedShape.ply"))
for i in range(0, len(manualGangerProjected)):
    
    sid = manualGangerProjected[i][:manualGangerProjected[i].rindex('/')]
    sid = sid[sid.rindex('/')+1:]
    sid = sid[1:10]
    print sid
    
    queryPly = manualGangerProjected[i]    
    targetPly = glob.glob(sys.argv[2] + "/" + sid + "*.ply" )[0]
    processes=[]

    pargs = " ".join(["python", "baryMkrToXYZ.py", "template-60k-m-adjust.mkr", queryPly, targetPly])
    print pargs
    processes.append(subprocess.Popen(pargs, shell=True))
    
    processes[-1].wait()
    os.rename(queryPly[:queryPly.rindex('/')+1] + "rawtarget.ply", queryPly[:queryPly.rindex('/')+1] + "manualGangerProjectedMarkers.ply")
    os.rename(queryPly[:queryPly.rindex('/')+1] + "rawtarget.mkr", queryPly[:queryPly.rindex('/')+1] + "manualGangerProjectedMarkers.mkr")
    
    queryPlyMarkerless = markerlessRefinedGangerProjected[i]
    pargs = " ".join(["python", "baryMkrToXYZ.py", "template-60k-m-adjust.mkr", queryPlyMarkerless, targetPly])
    print pargs
    processes.append(subprocess.Popen(pargs, shell=True))
    
    processes[-1].wait()
    os.rename(queryPly[:queryPly.rindex('/')+1] + "rawtarget.ply", queryPly[:queryPly.rindex('/')+1] + "markerlessRefinedProjectedMarkers.ply")
    os.rename(queryPly[:queryPly.rindex('/')+1] + "rawtarget.mkr", queryPly[:queryPly.rindex('/')+1] + "markerlessRefinedProjectedMarkers.mkr")
    

