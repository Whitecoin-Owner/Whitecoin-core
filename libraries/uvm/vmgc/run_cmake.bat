setlocal
call "setenv.bat"
cd %GRA_ROOT%/vmgc
cmake-gui -G "Visual Studio 12 2013"