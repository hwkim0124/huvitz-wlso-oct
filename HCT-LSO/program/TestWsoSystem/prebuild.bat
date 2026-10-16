echo Copying HlsoToolkit external files from %1 to output folder %2

xcopy %1\CamCmosOctUsb3.dll %2\system_libs\ /y /d
xcopy %1\GenTlCamCmosOctUsb3.cti %2\system_libs\ /y /d
xcopy %1\FTD3XX.dll %2\system_libs\ /y /d
xcopy %1\Spinnaker_v140.dll %2\system_libs\ /y /d
xcopy %1\GCBase_MD_VC140_v3_0.dll %2\system_libs\ /y /d
xcopy %1\GenApi_MD_VC140_v3_0.dll %2\system_libs\ /y /d
xcopy %1\libiomp5md.dll %2\system_libs\ /y /d

rem Pupil-centre model of the EyeAlign detector (externs\models -> <out>\models)
xcopy %1\..\models\pupil_track_engine.onnx %2\models\ /y /d
xcopy %1\..\models\pupil_track_engine.json %2\models\ /y /d