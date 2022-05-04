from django.contrib import admin
from django.urls import path
from django.conf import settings
from django.conf.urls.static import static
from django.contrib.auth.models import User
from . import views

urlpatterns = [
    path('admin/', admin.site.urls),
    path('', views.index, name='index'),
    path('result/<str:upload_file>', views.result, name='result'),
    path('query/', views.query, name='query'),
] + static(settings.STATIC_URL, document_root=settings.STATIC_ROOT)

urlpatterns += static('result/', document_root=settings.MEDIA_ROOT)