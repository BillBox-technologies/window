@echo off

echo  Compilation started ... 


cl /EHsc /MD /std:c++17 src\main.cpp src\convert.cpp src\BillBoxDialog.cpp src\S3Uploader.cpp ^
/I D:\window\include ^
/link /LIBPATH:D:\window ^
libcurl.lib Shlwapi.lib user32.lib gdi32.lib kernel32.lib comdlg32.lib advapi32.lib ^
/SUBSYSTEM:WINDOWS /out:gui.exe && del *.obj


if errorlevel 1 (
  echo Build failed ...
  pause
  exit /b 1

)else (

  echo Compilation Sucessfull ...

)
