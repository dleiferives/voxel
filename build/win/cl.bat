@echo off
call ms
rmdir /Q /S bin_win
mkdir bin_win
pushd bin_win

rem Name
set name=Voxel
echo %cd%

rem Include directories 
set inc=/I ..\include\

rem Source files
set src_main= ..\src\main.c ..\include\perlin\perlin.c


rem All source together
set src_all=%src_main%

rem OS Libraries
set os_libs= opengl32.lib kernel32.lib user32.lib ^
shell32.lib vcruntime.lib msvcrt.lib gdi32.lib Winmm.lib Advapi32.lib

rem Link options
set l_options=/EHsc /link /SUBSYSTEM:CONSOLE /NODEFAULTLIB:msvcrt.lib

rem Compile Release
rem cl /MP /FS /Ox /W0 /Fe%name%.exe %src_all% %inc% ^
rem /EHsc /link /SUBSYSTEM:CONSOLE /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:LIBCMT ^
rem %os_libs%

rem Compile Debug
cl /W2 /MP -Zi /DEBUG:FULL /Fe%name%.exe %src_all% %inc% ^
/EHsc /link /SUBSYSTEM:CONSOLE /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:LIBCMT ^
%os_libs%

popd
