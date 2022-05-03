# views.py
# functions for defining what is loading onto the webpages




from django.http import HttpResponse, HttpResponseRedirect
from .models import DataInput, DataOutput
from django.forms import ModelForm
from django.forms.utils import ErrorList
from django.views.decorators.clickjacking import xframe_options_exempt
from django.template import loader
from django.core.files import File
from APPDIST.run_tool import step1, step2
import time
import csv

## HELPER FUNCTIONS ##

# uses run_tool to run the algorithm
def load_data(height, weight, sex, filename, model_size):
    # set label for result folder
    sexlabel = 'm'
    if sex == 1:
        sexlabel = 'f'
    
    # set result_folder name
    result_folder = 'resultsdir/' + sexlabel + filename + '/d=' + model_size

    # if result.ply already exists, we don't need to run step 1 again
    try:
        result_ply = open(result_folder + '/result.ply')
    except:
        # run step 1
        step1(filename, model_size, sex, weight, height)
        result_ply = open(result_folder + '/result.ply')
    # check every 2.5 seconds whether result.ply has been written
    content = result_ply.read()
    while not content:
        time.sleep(2.5)
        content = result_ply.read()
    # run step 2 once we make sure result.ply is written
    step2(result_folder, filename, model_size, sex)
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
                    request.session['model_id'] = inputs.id
                    request.session['height'] = inputs.height
                    request.session['weight'] = inputs.weight
                    request.session['age'] = inputs.age
                    request.session['sex'] = inputs.sex
                    request.session['filename'] = file.name.removesuffix('.ply') # removing .ply bc the results folder doesn't include it
                    request.session['result_folder'] = ''
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
@xframe_options_exempt
def results(request):
    # get values from the session
    height = request.session['height']
    weight = request.session['weight']
    age = request.session['age']
    sex = request.session['sex']
    filename = request.session['filename']
    # get the model size
    size = '391'
    if sex == 1:
        size = '457'

    # sometimes the page is reloading and starts running the algorithm from the start
    # result_folder will be set once load_data has run so it'll stop this from happening
    if not request.session['result_folder']:
        result_folder = load_data(height, weight, sex, filename, size)
        request.session['result_folder'] = result_folder
    else:
        print("result_folder already exists")

    # get the results
    try:
        # get the result files
        result_csv_path = result_folder + '/gangerrefinedprojectpredict_' + size + '.csv'
        ply = open(result_folder + '/result_hcsmooth12.ply_deform.ply', errors='ignore')
        results_csv = open(result_csv_path)
        # save it to a DataOutput model
        final_result = DataOutput(input_data=request.session['model_id'], model_size=size, result_ply=File(ply), predicted_csv=File(results_csv))
        read_csv = csv.reader(results_csv)
        output = '<h1>Body Composition Predictions:</h1>'
        # saves each row in the CSV to the model and creates a line of HTML
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
            output += '<p><b>' + row[0].replace('_', ' ') + ':</b>'
            output += row[1] + '</p>'
        # .save(0) saves the model to the database
        final_result.save()
        # the "name" is saved as the entire path for the results, tmp_name gets just the name of the file
        ply_tmp_name = final_result.result_ply.name.split('/')[-1]
        csv_tmp_name = final_result.predicted_csv.name.split('/')[-1]
        # append the download links
        output += '<p>Result Ply: <a download=' + ply_tmp_name + ' href=' + final_result.result_ply.name + '>Download</a></p>'
        output += '<p>Predictions (CSV): <a download=' + csv_tmp_name + ' href=' + final_result.predicted_csv.name + '>Download</a></p>'
        results_csv.close()
        ply.close()
    except Exception as e:
        # if there are any issues, display a user-friendly error
        # print the actual error
        print(e)
        output = "<h3>Oh no! We hit an error trying to get your results.</h3>"
    # end the session
    end_session(request)
    # render the page
    return HttpResponse(output)