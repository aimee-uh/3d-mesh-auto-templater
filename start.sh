echo checking python requirements
pip install -r requirements.txt
echo making django migrations
python manage.py makemigrations
python manage.py migrate
python manage.py makemigrations gui
python manage.py migrate gui
echo collecting static files
python manage.py collectstatic --noinput
echo applying execute permissions to necessary files
chmod +x APPDIST/ganger/build/Deformation
chmod +x APPDIST/ply2asc
echo starting the app with gunicorn
gunicorn --timeout 3600 gui.wsgi -w 4 \
    --bind 0.0.0.0:8000