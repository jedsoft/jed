@echo off
echo - Generating Makefile in src...
src\mkfiles\mkmake WIN32 MINGW32 DLL < src\mkfiles\makefile.all > src\Makefile

echo - Creating config file...
copy src\jedconf.h src\config.h

echo - Creating a top-level Makefile
copy mkfiles\makefile.m32 Makefile

echo Now run mingw32-make to build the editor.  But first, you should
echo look at Makefile and change the installation locations if
echo necessary.  In particular, the PREFIX variable in the top-level
echo Makefile controls where the editor will be installed.
echo -
