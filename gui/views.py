from django.http import HttpResponse, HttpResponseRedirect
from .models import DataInput
from django.forms import ModelForm
from django.forms.utils import ErrorList
from django.views.decorators.clickjacking import xframe_options_exempt
from django.template import loader
from APPDIST.run_tool import create_results
import csv

# home page
@xframe_options_exempt
def index(request):
    form = UploadFileForm()
    # when the form is submitted
    if request.method == 'POST':
        form = UploadFileForm(request.POST, request.FILES)
        print("check if form is valid")
        if form.is_valid():
            # get file and check if it is a .ply file before continuing
            print("form is valid")
            print("check if file ends with .ply")
            file = request.FILES['uploaded_file']
            if file.name.endswith('.ply'):
                print("file ends with .ply")
                # save items from form to the current session
                inputs = form.save()
                file_path = inputs.uploaded_file.path
                print("check if ply is ascii")
                if ply_is_ascii(file_path):
                    print("ply is ascii")
                    request.session['model_id'] = inputs.id
                    request.session['height'] = inputs.height
                    request.session['weight'] = inputs.weight
                    request.session['age'] = inputs.age
                    request.session['sex'] = inputs.sex
                    request.session['filename'] = file.name.removesuffix('.ply')
                    print("try to redirect")
                    return HttpResponseRedirect('/results/')
                else:
                    errors = form._errors.setdefault("uploaded_file", ErrorList())
                    errors.append(u"Mesh format must be ASCII 1.0")
            else:
                errors = form._errors.setdefault("uploaded_file", ErrorList())
                errors.append(u"Only .PLY files allowed")
        else:
            form = UploadFileForm()
    # rendering page using HttpResponse for iFrame functionality
    formTemplate = loader.get_template("input_form.html")
    context = {'form':form}
    return HttpResponse(formTemplate.render(context, request))
    # return render(request,"input_form.html", context={'form': form})

# results after form submit
@xframe_options_exempt
def results(request):
    print("successfully redirected")
    # get values from the session
    height = request.session['height']
    weight = request.session['weight']
    age = request.session['age']
    sex = request.session['sex']
    filename = request.session['filename']
    # this is where we'll run the algorithm
    model_size = '391'
    if sex == 1:
        model_size = '457'
    result_folder = create_results(filename, model_size, sex, weight, height)
    # get the results
    results_csv = open("APPDIST/" + result_folder + "/gangerrefinedprojectpredict_" + model_size + ".csv")
    read_csv = csv.reader(results_csv)
    output = "<h1>Body Composition Predictions:</h1>"
    for row in read_csv:
        try:
            row[1]
            output += "<p><b>" + row[0].replace("_", " ") + ":</b>"
            output += row[1] + "</p>"
        except:
            continue
    results_csv.close()
    # end the session
    end_session(request)
    # placeholder output
    return HttpResponse(output)

# end session is added for security and clears all information that had been submitted
# if we want to be storing the information, this needs to be modified or removed
def end_session(request):
    # commented out code will delete the uploaded file
    # session_file = request.session['filename']
    # file_path = "tmp\\uploaded_mesh\\"+session_file
    # if os.path.exists(file_path): os.remove(file_path)
    # else: print("Failed to delete file " + session_file)

    # flush() removes all session data from the database, info stays in model db
    request.session.flush()

def ply_is_ascii(file_path):
    with open(file_path, 'r') as this_file:
        try:
            format = this_file.readlines()[1]
        except:
            return False
    return format == "format ascii 1.0\n"

# takes DataInput model to generate the form
class UploadFileForm(ModelForm):
    class Meta:
        model = DataInput
        fields = "__all__"