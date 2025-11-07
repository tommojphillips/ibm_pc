@echo off

if "%~1" == "/?" (
    goto :help
)
if "%~1" == "-?" (
    goto :help
)
if "%~1" == "\?" (
    goto :help
)
if "%~1" == "?" (
    goto :help
)

set "platform=x64"
set "config=release"
set "output_dir=release\"
set "ini=%output_dir%ibm_pc.ini"

if not "%~1" == "" (
    set "platform=%~1"
)

if not "%~2" == "" (
    set "config=%~2"
)

if exist "%output_dir%" ( 
    del /q "%output_dir%*"
    for /d %%d in ("%output_dir%*") do rmdir /s /q "%%d"
) else (
    mkdir "%output_dir%"
)

xcopy /q "bin\%platform%\%config%\ibm_pc.exe" "%output_dir%" >nul
xcopy /q "lib\UI\bin\%platform%\UI.dll" "%output_dir%" >nul
xcopy /q "lib\SDL3\lib\%platform%\SDl3.dll" "%output_dir%" >nul
xcopy /q "lib\SDL3_ttf\lib\%platform%\SDl3_ttf.dll" "%output_dir%" >nul
xcopy /q "fonts\" "%output_dir%fonts\" >nul

mkdir "%output_dir%roms\"

call scripts\make_ini.bat %ini%

exit /b 0

:help
    echo.
    echo make_release.bat [platform] [config]
    echo.
    echo platform: x64 (default), x86
    echo config:   release (default), debug
    echo.
    echo call this from the root directory of the repo
    