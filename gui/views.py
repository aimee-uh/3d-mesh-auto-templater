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
        shutil.rmtree(main_result_dir)
        shutil.rmtree(os.path.join(settings.MEDIA_ROOT,"uploaded_mesh"))
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
                    # this line is supposed to update the database, doesn't seem to work
                    # with open(renamed_file_path) as new_file: inputs.uploaded_file = File(new_file)
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


    result_csv_path = os.path.join(result_folder,'gangerrefinedprojectpredict_' + size + '.csv')

    output = ''
    context = {}

    exists = os.path.exists(result_csv_path)
    if not exists:
        print("result csv does not exist")
        output = get_loading_output(result_folder)
        context = {'output':output}
    else:
        results_csv = open(result_csv_path)
        if len(results_csv.readlines()) < 2:
            print("result csv is incomplete")
            output = get_loading_output(result_folder)
            context = {'output':output}
        else:
            # get the results
            try:
                # get the result files
                print("get the results")
                ply_path = os.path.join(result_folder, 'result_hcsmooth12.ply_deform_ascii.ply')
                ply = open(ply_path, errors='ignore')
                results_csv = open(result_csv_path)
                # save it to a DataOutput model
                final_result.result_ply = File(ply)
                final_result.predicted_csv = File(results_csv)
                read_csv = csv.reader(results_csv)

                lines = []

                # saves each row in the CSV to the model and creates a line of HTML
                predict_headers = ''
                predict_values = ''
                for row in read_csv:
                    lines.append(row)

                predheaders = lines[0]
                predvals = lines[1]

                final_result.DXA_WBTOT_FAT = predvals[0]
                final_result.DXA_WBTOT_LEAN = predvals[1]
                final_result.DXA_VFAT_MASS = predvals[2]
                final_result.DXA_ARM_LEAN = predvals[3]
                final_result.DXA_LEG_LEAN = predvals[4]
                final_result.DXA_WBTOT_PFAT = predvals[5]
                final_result.DXA_TRUNK_FAT = predvals[6]
                final_result.DXA_TRUNK_LEAN = predvals[7]
                final_result.DXA_ARM_FAT = predvals[8]
                final_result.DXA_LEG_FAT = predvals[9]

                predict_headers += '\n'.join(predheaders)
                predict_values += '\n'.join(predvals)
                
                # .save(0) saves the model to the database, moves upload files
                final_result.save()

                # create new filename
                new_ply_name = "result_" + upload_file + ".ply"
                new_csv_name = "predictions_" + upload_file + ".csv"
                # relative path (for the download href)
                new_ply = os.path.join(result_folder, new_ply_name)
                new_csv = os.path.join(result_folder, new_csv_name)
                # absolute path (seemed easier for the renaming)
                new_ply_abs = os.path.join(final_result.result_ply.path.removesuffix(final_result.result_ply.name),new_ply)
                new_csv_abs = os.path.join(final_result.predicted_csv.path.removesuffix(final_result.predicted_csv.name),new_csv)
                print(final_result.result_ply.path)
                print(new_ply_abs)
                os.rename(final_result.result_ply.path, new_ply_abs)
                os.rename(final_result.predicted_csv.path, new_csv_abs)

                # append the download links
                template = loader.get_template('final_results.html')
                context = {'predict_headers':predict_headers,'predict_values':predict_values, 'ply_name':new_ply_name, 'ply_link':new_ply, 'csv_name':new_csv_name, 'csv_link':new_csv}
                results_csv.close()
                ply.close()
            except Exception as e:
                # if there are any issues, display a user-friendly error
                # print the actual error
                print(e)
                output = "Oh no! We hit an error trying to get your results. Please try again."
                context = {'output':output}
            # end the session
            end_session(upload_file, size, result_folder)
    # render the page
    return HttpResponse(template.render(context, request))