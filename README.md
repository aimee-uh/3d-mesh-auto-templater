3D Auto Templater
=================

Code by Aimee Bowen with Shepherd Research Lab

Contact: bowen26@hawaii.edu

Framework
--------

Django (Documentation: https://docs.djangoproject.com/en/4.0/)

Heroku CLI (Reference: https://devcenter.heroku.com/articles/getting-started-with-python)

Main Files
---------

### gui

**urls.py** - defines URLs for:
 - admin (automatically added by django, currently not used)
 - index (where form is loaded)
 - results (where results are loaded)
 - static files (to view any images or CSS added to the site)

**views.py** - functions for loading pages
- index() - loads the form, saves the inputs to a session, and redirects to results
- results() - takes in session variables to run the algorithm, renders the results
- endsession() - called by results() to clear the data
- UploadFileForm() - creates form based on model DataInput

**models.py** - defines DataInput model

### APPDIST

**run_tool.py** - called in views.py to run shell commands for the algorithm

Editing and Running
---------

### Clone

Download and install the [Heroku CLI](https://devcenter.heroku.com/articles/heroku-cli).

If you haven't already, log in to your Heroku account and follow the prompts to create a new SSH public key.

`heroku login`

**Clone the repository**

Use Git to clone bodycomp-from-3do's source code to your local machine.

`heroku git:clone -a bodycomp-from-3do`

`cd bodycomp-from-3do`

### Run Locally

**Collect Static** - only necessary if you have static files such as CSS or images

`python manage.py collectstatic`

**Run**  - runs the site at http://localhost:5000

Windows

`heroku local -f Procfile.windows`

Linux and macOS

`heroku local`

### Deploy

Push changes live using git

`git add .`

`git commit -m "commit message"`

`git push heroku master`

Requirements
-----------
Python module requirements found in **requirements.txt**