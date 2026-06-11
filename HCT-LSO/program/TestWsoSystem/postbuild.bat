echo Copying WsoToolkit output files from %1 to deploy folder %2

if not exist %2 (
	mkdir %2
)

rd /s /q %2\logs 2>nul
rd /s /q %2\results 2>nul 
rd /s /q %2\patients 2>nul 
del %2\system.log 2>nul

xcopy %1\*.dll %2\ /y /d
xcopy %1\*.exe %2\ /y /d
xcopy %1\*.json %2\ /y /d

xcopy %1\system_libs\*.dll %2\system_libs\ /y /d
xcopy %1\system_libs\*.cti %2\system_libs\ /y /d /c