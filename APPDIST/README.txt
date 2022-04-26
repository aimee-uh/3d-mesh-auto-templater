Batch execution: DO NOT USE FOR SINGLE WEB INPUTS, RELIES ON CSV FILE WITH ALL HEIGHT WEIGHT INFO
	python runpcafit <INPUTDIR> <MODEL_SIZE> <OUTPUTDIR>
	i.e. python runpcafit.py inputmeshes/testexamples/males/ 391 resultsdir/results_uniquetestexamples/
	INPUTDIR is a folder with ply files (in ascii format, with normals encoded. Will crash otherwise. Needs to be scaled to metric with proper alignment but this won't cause a crash). This script processes all files in the folder.
	MODEL_SIZE is 391 for males and 457 for females. You can specify a number smaller than this and it should attempt to fit with less principal components, but just stick with max model size as I haven't tested this.
	OUTPUTDIR is where result folders will be written. A separate folder for males and females will be automatically created and the results split.

1. python runsinglewebinput.py <TARGET_PLY> <MODEL_SIZE> <OUTPUTDIR> <SEX> <WEIGHTKG> <HEIGHTM>
	i.e. python runsinglewebinput.py inputmeshes/testexamples/males/02ADL0015_styku.ply 391 resultsdir/ 0 63.68 1.634
	Specify relative path to a specific individual ply file. 
	OUTPUTDIR is the folder where the result folder will be put in directly. No male / female split here.
	Input sex as 0 for male and 1 for female. Weight and height are in kg and m.

2. python singletarget_gangerprojectpredict.py <RESULT_FOLDER> <TARGET_PLY> <PCADIR> <MODEL_SIZE> <SEX>
	Does the next step in surface to surface alignment, calls the compiled C++ program Deformation in the ganger/build directory. MAKE SURE THIS HAS EXECUTE PRIVELEGES, chmod if you need to. Same for ply2asc.
	Predicts body comp values from this final fit using regression matrices from PCA coords.
	i.e.  python singletarget_gangerprojectpredict.py resultsdir/webinputtest/m02ADL0015_styku/d\=391/ inputmeshes/testexamples/males/02ADL0015_styku.ply
	<RESULT_FOLDER> is the folder from the previous step that contains the result.ply file. You need to specify the d=# subfolder
	<TARGET_PLY> is the original input file passed by the user
	<PCADIR> is the folder containing pca info. This should just be "pcamodels/pca_autosuperset_bootstraptrain/superset/" for all adult experiments.
	<MODEL_SIZE> is same as before, 391 for male and 457 for female
	<SEX> is 0 or 1 for male or female

	Outputs a csv called gangerrefinedprojectpredict_MODELSIZE.csv. Return this and result_hcsmooth12.ply_deform.ply to the user (the binary file is a smaller transfer)