@echo off
echo Limpiando cache de Arduino IDE...
echo.
echo Por favor cierra Arduino IDE antes de ejecutar este script.
pause

REM Limpiar carpeta temporal de Arduino
if exist "%LOCALAPPDATA%\Temp\arduino*" (
    echo Eliminando archivos temporales...
    for /d %%i in ("%LOCALAPPDATA%\Temp\arduino*") do (
        rd /s /q "%%i" 2>nul
    )
)

REM Limpiar carpeta build si existe
if exist "build" (
    echo Eliminando carpeta build...
    rd /s /q "build" 2>nul
)

echo.
echo Limpieza completada!
echo Ahora puedes abrir Arduino IDE y compilar de nuevo.
pause


