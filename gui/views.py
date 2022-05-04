# views.py
# functions for defining what is loading onto the webpages

from django.http import HttpResponse, HttpResponseRedirect
from django.urls import reverse
from django.forms import ModelForm
from django.forms.utils import ErrorList
from django.views.decorators.clickjacking import xframe_options_exempt
from django.template import loader
from django.core.files import File
from django.core.cache import cache

from . import settings
from .models import DataInput, DataOutput

import csv
import os
from subprocess import Popen
import shutil


## HELPER FUNCTIONS ##

# uses run_tool to run the algorithm
def load_data(height, weight, sex, filename, model_size, result_folder):
    if not os.getcwd().endswith("APPDIST"):
        os.chdir("APPDIST")

    print("start process")
    pargs = " ".join(["python run_tool.py", str(height), str(weight), str(sex), filename, model_size, result_folder])
    print(pargs)
    pr = Popen(pargs, shell=True)

    return result_folder



# end session is added for security and clears all information that had been submitted
def end_session(upload_file, size, result_folder):
    # delete result folder
    error = ""
    try:
        data_result_dir = "d=" + size
        main_result_dir = result_folder.removesuffix(data_result_dir)
        error = main_result_dir
        shutil.rmtree(main_result_dir)
        error = settings.MEDIA_ROOT+"/uploaded_mesh"
        shutil.rmtree(settings.MEDIA_ROOT+"/uploaded_mesh")
    except:
        error += "\n deleting files"
    
    # clear database
    which_model = ""
    try:
        which_model = "input"
        userid = int(upload_file.split("_")[-1])
        uploaded_data = DataInput.objects.get(id=userid)
        uploaded_data.delete()
        which_model = "output"
        result_data = DataOutput.objects.get(input_data=upload_file)
        result_data.delete()
    except:
        error += "\n clearing " + which_model + " model from database"

    print("Errors while ending session: " + error)



# checks if the uploaded file is in ascii format
def ply_is_ascii(file_path):
    try:
        with open(file_path, 'r') as this_file:
            format = this_file.readlines()[1]
            return format == 'format ascii 1.0\n'
    except:
        return False

# get stdout
def get_loading_output(result_folder):
    output = ''
    stdout_path = result_folder + '/stdout.txt'
    gangerstdout_path = result_folder + '/gangerstdout.txt'
    try:
        with open(gangerstdout_path) as f:
            gangerstdout_info = f.readlines()
            for line in gangerstdout_info:
                output += line + "\n"
    except:
        try:
            with open(stdout_path) as f:
                stdout_info = f.readlines()
                for line in stdout_info:
                    output += line + "\n"
        except Exception as e: 
            print(e)
            output += "Checking files..."
    return output

# takes DataInput model to generate the form
class UploadFileForm(ModelForm):
    class Meta:
        model = DataInput
        fields = '__all__'






## VIEW FUNCTIONS ##



# home page
@xframe_options_exempt
def index(request):
    form = UploadFileForm()
    # when the form is submitted
    if request.method == 'POST':
        form = UploadFileForm(request.POST, request.FILES)
        if form.is_valid():
            # get file and check if it is a .ply file before continuing
            file = request.FILES['uploaded_file']
            if file.name.endswith('.ply'):
                # save items from form to the database
                inputs = form.save()
                file_path = inputs.uploaded_file.path
                # check if the ply is in ascii format
                if ply_is_ascii(file_path):
                    # save any variables we need later to the session
                    height = inputs.height
                    weight = inputs.weight
                    sex = inputs.sex
                    # renaming file
                    filename = inputs.uploaded_file.name.removeprefix('uploaded_mesh/').removesuffix('.ply') + "_" + str(inputs.id)
                    renamed_file_path = file_path.removesuffix('.ply') + "_" + str(inputs.id) + ".ply"
                    os.rename(file_path, renamed_file_path)
                    with open(renamed_file_path) as new_file: inputs.uploaded_file = File(new_file)
                    # set label for result folder and set model size
                    sexlabel = 'm'
                    size = '391'
                    if sex == 1:
                        sexlabel = 'f'
                        size = '457'
                    # set result_folder name
                    result_folder = 'resultsdir/' + sexlabel + filename + '/d=' + size
                    print(result_folder)
                    load_data(height, weight, sex, filename, size, result_folder)
                    # save result database
                    outputs = DataOutput(input_data=filename, model_size=size, result_folder=result_folder)
                    outputs.save()
                    # redirect to the results page
                    url = reverse('result', kwargs={'upload_file': filename})
                    return HttpResponseRedirect(url)
                else:
                    errors = form._errors.setdefault('uploaded_file', ErrorList())
                    errors.append(u'Mesh format must be ASCII 1.0')
            else:
                errors = form._errors.setdefault('uploaded_file', ErrorList())
                errors.append(u'Only .PLY files allowed')
        else:
            form = UploadFileForm()
    # render the page with the form
    formTemplate = loader.get_template('input_form.html')
    context = {'form':form}
    return HttpResponse(formTemplate.render(context, request))

# results after form submit
@xframe_options_exempt
def result(request, upload_file):
    print("open results page and reset cache")
    this_url = '/result/'+ upload_file
    cache.delete(this_url)

    template = loader.get_template('loading.html')

    if not os.getcwd().endswith("APPDIST"):
        os.chdir("APPDIST")

    # pull result from database
    final_result = DataOutput.objects.get(input_data=upload_file)
    size = final_result.model_size
    result_folder = final_result.result_folder


    result_csv_path = result_folder + '/gangerrefinedprojectpredict_' + size + '.csv'

    output = ''
    context = {}

    exists = os.path.exists(result_csv_path)
    if not exists:
        print("result csv does not exist")
        output = get_loading_output(result_folder)
        context = {'output':output}
    else:
        results_csv = open(result_csv_path)
        if len(results_csv.readlines()) < 13:
            print("result csv is incomplete")
            output = get_loading_output(result_folder)
            context = {'output':output}
        else:
            # get the results
            try:
                # get the result files
                print("get the results")
                ply = open(result_folder + '/result_hcsmooth12.ply_deform_ascii.ply', errors='ignore')
                results_csv = open(result_csv_path)
                # save it to a DataOutput model
                final_result.result_ply = File(ply)
                final_result.predicted_csv = File(results_csv)
                read_csv = csv.reader(results_csv)
                # saves each row in the CSV to the model and creates a line of HTML
                predict_headers = ''
                predict_values = ''
                for row in read_csv:
                    if row[0] == "DXA_WEIGHT": final_result.DXA_WEIGHT = row[1]
                    elif row[0] == "DXA_HEIGHT": final_result.DXA_HEIGHT = row[1]
                    elif row[0] == "DXA_WBTOT_FAT": final_result.DXA_WBTOT_FAT = row[1]
                    elif row[0] == "DXA_WBTOT_LEAN": final_result.DXA_WBTOT_LEAN = row[1]
                    elif row[0] == "DXA_VFAT_MASS": final_result.DXA_VFAT_MASS = row[1]
                    elif row[0] == "DXA_ARM_LEAN": final_result.DXA_ARM_LEAN = row[1]
                    elif row[0] == "DXA_LEG_LEAN": final_result.DXA_LEG_LEAN = row[1]
                    elif row[0] == "DXA_WBTOT_PFAT": final_result.DXA_WBTOT_PFAT = row[1]
                    elif row[0] == "DXA_TRUNK_FAT": final_result.DXA_TRUNK_FAT = row[1]
                    elif row[0] == "DXA_TRUNK_LEAN": final_result.DXA_TRUNK_LEAN = row[1]
                    elif row[0] == "DXA_ARM_FAT": final_result.DXA_ARM_FAT = row[1]
                    elif row[0] == "DXA_LEG_FAT": final_result.DXA_LEG_FAT = row[1]
                    else: continue
                    predict_headers += row[0].replace('_', ' ') + '\n'
                    predict_values += row[1] + '\n'
                
                # .save(0) saves the model to the database, moves upload files
                final_result.save()

                # the "name" is saved as the entire path for the results, tmp_name gets just the name of the file
                ply_tmp_name = final_result.result_ply.name.split('/')[-1]
                csv_tmp_name = final_result.predicted_csv.name.split('/')[-1]
                # append the download links
                template = loader.get_template('final_results.html')
                context = {'predict_headers':predict_headers,'predict_values':predict_values, 'ply_name':ply_tmp_name, 'ply_link':final_result.result_ply.name, 'csv_name':csv_tmp_name, 'csv_link':final_result.predicted_csv.name}
                results_csv.close()
                ply.close()
            except Exception as e:
                # if there are any issues, display a user-friendly error
                # print the actual error
                print(e)
                output = "Oh no! We hit an error trying to get your results."
                context = {'output':output}
            # end the session
            end_session(upload_file, size, result_folder)
    # render the page
    return HttpResponse(template.render(context, request))


def query(request):
    with_id = DataOutput.objects.get(id=2)
    test = DataOutput.objects.get(input_data="01ADL0007_fit3d_2")
    return HttpResponse("<p>"+test.result_folder+"</p><p>"+with_id.result_folder+"</p>")