@echo off

set "ini=default.ini"
if not "%~1" == "" (
    set "ini=%~1"
)

echo ; IBM PC CONFIG >> %ini%

echo. >> %ini%

echo ; ----------------- PC Config ------------------ >> %ini%
echo model =            '5150_64_256' ; 5150_16_64, 5150_64_256 >> %ini%
echo video_adapter =    'CGA'         ; MDA, CGA, CGA40 >> %ini%
echo conventional_ram = '0x40000'     ; 0x4000 - 0xB8000 (16K - 736K) >> %ini%
echo num_floppies =     '2'           ; 0, 1, 2, 4 >> %ini%
echo ; ---------------------------------------------- >> %ini%

echo. >> %ini%

echo ; --------------- DIP SWITCHES ----------------- >> %ini%
echo sw1_override = '0' >> %ini%
echo sw2_override = '0' >> %ini%
echo sw1 =          '0b00000000' >> %ini%
echo sw2 =          '0b00000000' >> %ini%
echo ; ---------------------------------------------- >> %ini%

echo. >> %ini%

echo ; --------------- Floppy Drives ---------------- >> %ini%
echo ;disk = [ drive = '^<drive_letter^>', path = '^<path^>' ] >> %ini%
echo ;disk = [ drive = '^<drive_letter^>', path = '^<path^>' ] >> %ini%
echo ; ---------------------------------------------- >> %ini%

echo. >> %ini%

echo ; -------------------- ROMS -------------------- >> %ini%
echo ;rom = [ address = ^<address^>, path = '^<path^>' ] >> %ini%
echo ;rom = [ address = ^<address^>, path = '^<path^>' ] >> %ini%
echo ; ---------------------------------------------- >> %ini%

echo. >> %ini%

echo ; --------------- Display Config --------------- >> %ini%
echo texture_scale_mode   = 'Nearest' ; Nearest, Linear >> %ini%
echo display_scale_mode   = 'Fit'     ; Fit, Stretched >> %ini%
echo display_view_mode    = 'Cropped' ; Cropped, Full >> %ini%
echo correct_aspect_ratio = '1' >> %ini%
echo emulate_max_scanline = '1' >> %ini%
echo ; ---------------------------------------------- >> %ini%

echo. >> %ini%

echo ; --------------- Display Fonts ---------------- >> %ini%
echo mda_font = 'fonts/Bm437_IBM_MDA.FON' >> %ini%
echo cga_font = 'fonts/Bm437_IBM_CGA.FON' >> %ini%
echo ; ---------------------------------------------- >> %ini%

echo. >> %ini%