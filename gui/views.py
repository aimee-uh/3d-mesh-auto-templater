# views.py
# functions for defining what is loading onto the webpages




from django.http import HttpResponse, HttpResponseRedirect
from .models import DataInput
from django.forms import ModelForm
from django.forms.utils import ErrorList
from django.views.decorators.clickjacking import xframe_options_exempt
from django.template import loader
from APPDIST.run_tool import step1, step2
import time
import csv
import os.path
import asyncio

## HELPER FUNCTIONS ##
def load_data(height, weight, sex, filename, model_size):
    print('\n\n-------\nstarting analysis')
    sexlabel = 'm'
    if sex == 1:
        sexlabel = 'f'
    # set result_folder name
    result_folder = 'resultsdir/' + sexlabel + filename + '/d=' + model_size

    print("\n\n------\nrunning step 1")
    step1(filename, model_size, sex, weight, height)
    print('waiting for pcamatch to create result.ply')
    # check every 2.5 seconds whether result.ply has been written
    result_ply = open(result_folder + '/result.ply')
    content = result_ply.read()
    i = 0
    while not content:
        time.sleep(2.5)
        i += 2.5
        if i % 60 == 0:
            print('Running for ... ' + str(i/60) + ' minutes')
        content = result_ply.read()
    print('result.ply is ready\n')
    # if this function is reloaded, check if the final result file exists before trying to rerun step 2
    print("check if result_hcsmooth12.ply_deform.ply exists")
    print("--result_hcsmooth12.ply_deform.ply does NOT exist")
    print("\n\n------\nrunning step 2")
    step2(result_folder, filename, model_size, sex)
    print("redirect to results page")
    return result_folder



# end session is added for security and clears all information that had been submitted
def end_session(request):
    # here we can add anything else for ending the session, such as deleting files
    # flush() removes all session data from the database, info stays in model db
    request.session.flush()


# checks if the uploaded file is in ascii format
def ply_is_ascii(file_path):
    with open(file_path, 'r') as this_file:
        try:
            format = this_file.readlines()[1]
        except:
            return False
    return format == 'format ascii 1.0\n'



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
        print('check if form is valid')
        if form.is_valid():
            # get file and check if it is a .ply file before continuing
            print('--form is valid')
            print('check if file ends with .ply')
            file = request.FILES['uploaded_file']
            if file.name.endswith('.ply'):
                print('--file ends with .ply')
                # save items from form to the current session
                inputs = form.save()
                file_path = inputs.uploaded_file.path
                print('check if ply is ascii')
                if ply_is_ascii(file_path):
                    print('--ply is ascii')
                    request.session['model_id'] = inputs.id
                    request.session['height'] = inputs.height
                    request.session['weight'] = inputs.weight
                    request.session['age'] = inputs.age
                    request.session['sex'] = inputs.sex
                    request.session['filename'] = file.name.removesuffix('.ply')
                    # redirect to the results page
                    return HttpResponseRedirect('results')
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
def results(request):
    # get values from the session
    height = request.session['height']
    weight = request.session['weight']
    age = request.session['age']
    sex = request.session['sex']
    filename = request.session['filename']
    model_size = '391'
    if sex == 1:
        model_size = '457'
    
    # helper for running algorithm
    result_folder = load_data(height, weight, sex, filename, model_size)

    print("\n\n------\nget results")
    # get the results
     # check every 2.5 seconds whether result_hcsmooth12.ply_deform.ply has been created
    print("waiting to make sure predict csv exists")
    result_csv_path = '../APPDIST/' + result_folder + '/gangerrefinedprojectpredict_' + model_size + '.csv'
    i = 0
    while not os.path.isfile(result_csv_path):
        time.sleep(1)
        i += 1
    print('Waited for ... ' + str(i) + ' seconds')
    print('predict csv is ready\n')
    print("open prediction results csv")
    results_csv = open(result_csv_path)
    print("read data")
    read_csv = csv.reader(results_csv)
    output = '<h1>Body Composition Predictions:</h1>'
    print("read each line in the csv")
    for row in read_csv:
        try:
            row[1]
            output += '<p><b>' + row[0].replace('_', ' ') + ':</b>'
            output += row[1] + '</p>'
            print(output)
        except:
            continue
    results_csv.close()
    # end the session
    print("End session")
    end_session(request)
    # placeholder output
    print("render page")
    return HttpResponse(output)