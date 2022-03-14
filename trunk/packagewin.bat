@echo off
rd /q /s windows
md windows
copy README.txt windows
copy LICENSE windows
copy senshub-build-desktop\release\senshub.exe windows
copy C:\QtSDK\Desktop\Qt\4.8.0\mingw\bin\QtCore4.dll windows
copy C:\QtSDK\Desktop\Qt\4.8.0\mingw\bin\QtGui4.dll windows
copy C:\QtSDK\Desktop\Qt\4.8.0\mingw\bin\QtNetwork4.dll windows
copy C:\QtSDK\Desktop\Qt\4.8.0\mingw\bin\QtXml4.dll windows
copy C:\QtSDK\Desktop\Qt\4.8.0\mingw\bin\mingwm10.dll windows
copy C:\QtSDK\Desktop\Qt\4.8.0\mingw\bin\libgcc_s_dw2-1.dll windows


cd windows
zip -r ..\SensHub-win.zip * -x *.svn*
cd ..
zip -r SensHub-win.zip examples\* -x *.svn*
