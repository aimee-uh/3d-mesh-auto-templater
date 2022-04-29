python manage.py makemigrations gui
python manage.py migrate gui
python manage.py collectstatic --noinput
echo starting the app with gunicorn
gunicorn --timeout 3600 gui.wsgi \
    --bind 0.0.0.0:8000