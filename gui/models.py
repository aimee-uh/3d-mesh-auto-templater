from django.db import models

# DataInput Model holds all the information for the main form
class DataInput(models.Model):
    weight = models.FloatField("Weight (kg)")
    height = models.FloatField("Height (meters)")
    age = models.PositiveIntegerField("Age (years)")
    MALE = 0
    FEMALE = 1
    sex_options = [(MALE,'Male'),(FEMALE,'Female')]
    sex = models.IntegerField(choices=sex_options, default=MALE)
    uploaded_file = models.FileField("3DO file (.PLY)", upload_to="uploaded_mesh")

class DataOutput(models.Model):
    input_data = models.TextField(default="uploaded_mesh/ply")
    model_size = models.CharField(max_length=3)
    result_folder = models.TextField(default="resultsdir")
    DXA_WEIGHT = models.FloatField(default=1.0)
    DXA_HEIGHT = models.FloatField(default=1.0)
    DXA_WBTOT_FAT = models.FloatField(default=1.0)
    DXA_WBTOT_LEAN = models.FloatField(default=1.0)
    DXA_VFAT_MASS = models.FloatField(default=1.0)
    DXA_ARM_LEAN = models.FloatField(default=1.0)
    DXA_LEG_LEAN = models.FloatField(default=1.0)
    DXA_WBTOT_PFAT = models.FloatField(default=1.0)
    DXA_TRUNK_FAT = models.FloatField(default=1.0)
    DXA_TRUNK_LEAN = models.FloatField(default=1.0)
    DXA_ARM_FAT = models.FloatField(default=1.0)
    DXA_LEG_FAT = models.FloatField(default=1.0)
    predicted_csv = models.FileField()
    result_ply = models.FileField()