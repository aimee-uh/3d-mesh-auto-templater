3D Auto Templater
=================

Django code by Aimee Bowen with Shepherd Research Lab

Contact: bowen26@hawaii.edu

Framework
--------

Django (Documentation: https://docs.djangoproject.com/en/4.0/)

Main Files
---------

### gui

**urls.py** - defines URLs for:
 - admin (automatically added by django, currently not used)
 - index (where form is loaded)
 - results (where results are loaded)
 - static files (to view any images or CSS added to the site)

**views.py** - functions for loading pages
- index() - loads the form, saves the inputs to a session, and switches template to loading.html
- load_data() - loaded using Ajax in the loading.html template, runs the algorithm
- results() - searches through result folder to save and display results
- endsession() - called by results() to clear the data
- UploadFileForm() - creates form based on model DataInput

**models.py** - defines DataInput model

### APPDIST

**run_tool.py** - called in views.py to run shell commands for the algorithm

Deployment
---------

### Run Locally

Make sure start.sh has execute permissions.

`./start.sh`

Runs manage.py to properly collect everything for django then deploys the app using gunicorn to localhost 8000.

Requirements
-----------
Python module requirements found in **requirements.txt**