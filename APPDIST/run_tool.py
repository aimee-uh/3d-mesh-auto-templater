import os
import time
import sys

def step1(filename, model_size, sex, weight, height):
    try:
        # if we're not currently in the APPDIST folder, move into it
        if not os.getcwd().endswith("APPDIST"):
            os.chdir("APPDIST")
        # run step 1
        args_runsinglewebinput = "../tmp/uploaded_mesh/" + filename + ".ply " + model_size + " resultsdir " +  str(sex) + " " + str(weight) + " " + str(height)
        print("python runsinglewebinput.py " + args_runsinglewebinput)
        os.system("python runsinglewebinput.py " + args_runsinglewebinput)
        return True
    except:
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
        return True
    except:
        return False

if __name__ == "__main__":
    height = sys.argv[1]
    weight = sys.argv[2]
    sex = sys.argv[3]
    filename = sys.argv[4]
    model_size = sys.argv[5]
    result_folder = sys.argv[6]
    print("\n\nstarting step 1\n")
    step1(filename, model_size, sex, weight, height)
    print("\n\nstarting step 2\n")
    step2(result_folder, filename, model_size, sex)
