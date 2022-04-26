import os
import time

def create_results(filename, model_size, sex, weight, height):
    print("create_results")
    # based on sex, set label for the output folder
    sexlabel = 'm'
    if sex == 1:
        sexlabel = 'f'
    # if we're not currently in the APPDIST folder, move into it
    if not os.getcwd().endswith("APPDIST"):
        os.chdir("APPDIST")
    # run step 1
    result_folder = "resultsdir/" + sexlabel + filename + "/d=" + model_size 
    args_runsinglewebinput = "../tmp/uploaded_mesh/" + filename + ".ply " + model_size + " resultsdir " +  str(sex) + " " + str(weight) + " " + str(height)
    print("python runsinglewebinput.py " + args_runsinglewebinput)
    os.system("python runsinglewebinput.py " + args_runsinglewebinput)
    print("running pcamatch")
    result_ply = open(result_folder + "/result.ply")
    content = result_ply.read()
    i = 0
    while not content:
        time.sleep(2.5)
        i += 2.5
        if i % 10 == 0:
            print("Waited ... " + str(i) + " seconds")
        content = result_ply.read()
    print("result.ply is ready\n")
    result_ply.close()
    #run step 2
    args_singletarget_gangerprojectpredict = result_folder + " " + filename + ".ply pcamodels/pca_autosuperset_bootstraptrain/superset/ " + model_size + " " + str(sex)
    print("python singletarget_gangerprojectpredict.py " + args_singletarget_gangerprojectpredict)
    os.system("python singletarget_gangerprojectpredict.py " + args_singletarget_gangerprojectpredict)
    # return the location of the results
    return result_folder

if __name__ == "__main__":
    create_results("MALE_example3Dmesh_initial_ascii_n_scaled", '391', 0, 63.68, 1.634)
    # python runsinglewebinput.py ../tmp/uploaded_mesh/MALE_example3Dmesh_initial_ascii_n_scaled_2.ply 391 resultsdir 0 63.68 1.634