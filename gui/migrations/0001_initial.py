# Generated by Django 4.0.3 on 2022-04-21 00:57

from django.db import migrations, models


class Migration(migrations.Migration):

    initial = True

    dependencies = [
    ]

    operations = [
        migrations.CreateModel(
            name='DataInput',
            fields=[
                ('id', models.BigAutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('weight', models.FloatField(verbose_name='Weight (kg)')),
                ('height', models.FloatField(verbose_name='Height (meters)')),
                ('age', models.PositiveIntegerField(verbose_name='Age (years)')),
                ('sex', models.IntegerField(choices=[(0, 'Male'), (1, 'Female')], default=0)),
                ('uploaded_file', models.FileField(upload_to='uploaded_mesh', verbose_name='3DO file (.PLY)')),
            ],
        ),
    ]
