from django.contrib import admin
from django.urls import path
from django.conf import settings
from django.conf.urls.static import static
from django.contrib.auth.models import User
from . import views

urlpatterns = [
    path('admin/', admin.site.urls),
    path('', views.index, name='index'),
    path('results/', views.results, name='results'),
] + static(settings.STATIC_URL, document_root=settings.STATIC_ROOT)