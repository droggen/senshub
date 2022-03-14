@echo off
del /q source\debug\*
del /q source\release\*
zip -r SensHub-src.zip source\* -x *.svn*
zip -r SensHub-src.zip examples\* -x *.svn*
zip -r SensHub-src.zip LICENSE
zip -r SensHub-src.zip README.txt
