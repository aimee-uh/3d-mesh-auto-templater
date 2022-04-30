import os
import time

def step1(filename, model_size, sex, weight, height):
    try:
        # if we're not currently in the APPDIST folder, move into it
        if not os.getcwd().endswith("APPDIST"):
            os.chdir("APPDIST")
        # run step 1
        args_runsinglewebinput = "../tmp/uploaded_mesh/" + filename + ".ply " + model_size + " resultsdir " +  str(sex) + " " + str(weight) + " " + str(height)
        print("python runsinglewebinput.py " + args_runsinglewebinput)
        os.system("python runsinglewebinput.py " + args_runsinglewebinput)
        os.chdir("..")
        return True
    except:
        os.chdir("..")
        return False

def step2(result_folder, filename, model_size, sex):
    try:
        # if we're not currently in the APPDIST folder, move into it
        if not os.getcwd().endswith("APPDIST"):
            os.chdir("APPDIST")

        #run step 2
        args_singletarget_gangerprojectpredict = result_folder + " " + filename + ".ply pcamodels/pca_autosuperset_bootstraptrain/superset/ " + model_size + " " + str(sex)
        print("python singletarget_gangerprojectpredict.py " + args_singletarget_gangerprojectpredict)
        os.system("python singletarget_gangerprojectpredict.py " + args_singletarget_gangerprojectpredict)
        # return the location of the results
        os.chdir("..")
        return True
    except:
        os.chdir("..")
        return False