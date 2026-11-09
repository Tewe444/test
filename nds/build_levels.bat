@echo off
rem Level (levels\*.txt) neu nach source\levels.h uebersetzen (Windows, braucht g++ / MinGW)
cd /d "%~dp0"
g++ -O2 -std=c++17 tools\leveltool.cpp -o tools\leveltool.exe || goto :fehler
tools\leveltool.exe levels --nds source\levels.h || goto :fehler
echo Fertig! source\levels.h wurde neu erzeugt.
pause
exit /b 0
:fehler
echo Es gab einen Fehler - siehe oben.
pause
