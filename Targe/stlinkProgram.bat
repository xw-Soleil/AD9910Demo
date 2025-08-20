@echo off
setlocal

:: 配置路径
set OPENOCD="c:\ST\STM32CubeCLT_1.18.0\OpenOCD-20250613-0.12.0\bin\openocd.exe"
set SCRIPTS="C:\ST\STM32CubeCLT_1.18.0\OpenOCD-20250613-0.12.0\share\openocd\scripts"

:: 查找.elf文件
for /R "%~dp0..\build" %%F in (*.elf) do set ELF_FILE=%%F

if not defined ELF_FILE (
    echo 找不到.elf文件！
    pause
    exit
)

echo 烧录: %ELF_FILE%
set ELF_UNIX=%ELF_FILE:\=/%

:: 使用标准配置和多个-c命令 - 完全按照参考格式
%OPENOCD% -s %SCRIPTS% -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -c "reset halt; wait_halt; flash write_image erase %ELF_UNIX%" -c reset -c shutdown

if %errorlevel% equ 0 (
    echo 烧录成功！
) else (
    echo 烧录失败！
)

pause