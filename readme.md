# IBM PC Emulator 

An IBM PC Emulator written in C using SDL3

 - [Building](#building)
 - [Fonts](#font-files)
 - [ROMS](#rom-files)
   - [BIOS](#ibm-pc-5150-rom)
   - [Basic](#basic-roms)
   - [XEBEC](#xebec-rom)
 - [Usage](#launching)
   - [Command line](#command-line)
     - [Definitions](#command-line)
   - [Configuration file](#configuration-file)
     - [Definitions](#settings)
     - [Format](#format)
 - [XEBEC Hard disk drive](#xebec)

## Emulated Hardware
 - i8086 CPU
 - i8237 DMA
 - i8253 PIT
 - i8255 PPI
 - i8259 PIC
 - i8272 FDC (NEC uPD765)
 - IBM Fixed Disk Drive (XEBEC)

 ## Emulated ISA Cards
 - MDA ISA Card (IBM Monochrome Display Adapter)
 - CGA ISA Card (IBM Color Graphics Adapter)
 - FDC ISA Card
 - XEBEC ISA Card

## Building

The project is built in Visual Studio 2022 or 2026

| Dependencies   | Github                                        |
| -------------- | --------------------------------------------- |
| I8086          | https://github.com/tommojphillips/i8086       |
| TOMI           | https://github.com/tommojphillips/TOMI        |
| SDL3 3.2.24    | https://github.com/libsdl-org/SDL             |
| SDL3_ttf-3.2.2 | https://github.com/libsdl-org/SDL_ttf         |
| ImGui 1.92.3   | https://github.com/ocornut/imgui/tree/v1.92.3 |
 
 ---

1. Clone the repo and submodules  
  ```
  git clone --recurse-submodules https://github.com/tommojphillips/ibm_pc.git
  ```

2. cd dir to `ibm_pc` 
  ```
  cd ibm_pc
  ```

3. Fetch SDL3/SDL3_TTF VC Release
  ```
  scripts\fetch_sdl3.sh
  ```
 
4. Open `lib\UI\vc\UI.sln`, build

5. Open `vc\ibm_pc.sln`, build, run.

 Make sure these files are next to `ibm_pc.exe` or on your PATH:
 - SDL3.dll
 - SDL3_ttf.dll
 - UI.dll

 If building from source, and launching through visual studio, all .dll files are already on your PATH. 
 You only need to copy the .FON files to the same directory as ibm_pc.exe

 ---

## FONT Files
 Font files are required for full functionality. 
 FONTS can be found here: https://int10h.org/oldschool-pc-fonts/download/
 
| Description   | File              |
|---------------|-------------------|
| MDA Font 9x14 | Bm437_IBM_MDA.FON |
| CGA Font 8x8  | Bm437_IBM_CGA.FON |

 They have been included in the `fonts\` directory of the repo.

 ---

## ROM Files
 ROMS are required for full functionality.
 ROMS can be found on 86Box's github: https://github.com/86Box/roms/

### IBM PC 5150 (PC) ROMS

 - The earlier 1981 BIOSes only read the first 4 bits of SW2 resulting in a max of 544KB of RAM 
 - The later 1982 BIOSes read the first 5 bits of SW2 resulting in a offical max of 640KB of RAM

| Description  | Date        | Size | Address     | Location                                              |
|--------------|-------------|------|-------------|-------------------------------------------------------|
| IBM 5150 U33 | 24 APR 1981 | 8K   | **0xFE000** | `machines\ibmpc\BIOS_IBM5150_24APR81_5700051_U33.BIN` |
| IBM 5150 U33 | 19 Oct 1981 | 8K   | **0xFE000** | `machines\ibmpc\BIOS_IBM5150_19OCT81_5700671_U33.BIN` |
| IBM 5150 U33 | 16 AUG 1982 | 8K   | **0xFE000** | `machines\ibmpc82\BIOS_5150_16AUG82_5000024_U33.BIN`  |
| IBM 5150 U33 | 27 Oct 1982 | 8K   | **0xFE000** | `machines\ibmpc82\BIOS_5150_27OCT82_1501476_U33.BIN`  |

### IBM PC 5160 (XT) ROMS

| Description  | Date        | Size | Address     | Location                                              |
|--------------|-------------|------|-------------|-------------------------------------------------------|
| IBM 5160 U18 | 16 AUG 1982 | 32K  | **0xF8000** | `machines\ibmxt\BIOS_5160_16AUG82_U18_5000026.BIN`    |
| IBM 5160 U19 | 16 AUG 1982 | 8K   | **0xF6000** | `machines\ibmxt\BIOS_5160_16AUG82_U19_5000027.BIN`    |

| Description  | Date        | Size | Address     | Location                                              |
|--------------|-------------|------|-------------|-------------------------------------------------------|
| IBM 5160 U18 | 08 NOV 1982 | 32K  | **0xF8000** | `machines\ibmxt\BIOS_5160_08NOV82_U18_1501512.BIN`    |
| IBM 5160 U19 | 08 NOV 1982 | 8K   | **0xF6000** | `machines\ibmxt\BIOS_5160_08NOV82_U19_5000027.BIN`    |

| Description  | Date        | Size | Address     | Location                                                                |
|--------------|-------------|------|-------------|-------------------------------------------------------------------------|
| IBM 5160 U18 | 10 JAN 1986 | 32K  | **0xF8000** | `machines\ibmxt86\BIOS_5160_10JAN86_U18_62X0851_27256_F800.BIN`         |
| IBM 5160 U19 | 10 JAN 1986 | 32K  | **0xF0000** | `machines\ibmxt86\BIOS_5160_10JAN86_U19_62X0854_27256_F000.BIN`         |

| Description  | Date        | Size | Address     | Location                                                                |
|--------------|-------------|------|-------------|-------------------------------------------------------------------------|
| IBM 5160 U18 | 09 MAY 1986 | 32K  | **0xF8000** | `machines\ibmxt86\BIOS_5160_09MAY86_U18_59X7268_62X0890_27256_F800.BIN` |
| IBM 5160 U19 | 09 MAY 1986 | 32K  | **0xF0000** | `machines\ibmxt86\BIOS_5160_09MAY86_U19_62X0819_68X4370_27256_F000.BIN` |

### BASIC ROMS

#### Basic version C1.0 ROM 

 Basic is optional and only for the 5150 (PC). Basic comes included in the 5160 (XT) BIOSes.

 | Description    | Size  | Address     | Location                                                                     |
 |----------------|------ |-------------|----------------------------------------------------------------------------- |
 | BASIC C1.0 U29 | 8K    | **0xF6000** | `machines\ibmpc\IBM 5150 - Cassette BASIC version C1.00 - U29 - 5700019.bin` |
 | BASIC C1.0 U30 | 8K    | **0xF8000** | `machines\ibmpc\IBM 5150 - Cassette BASIC version C1.00 - U30 - 5700027.bin` |
 | BASIC C1.0 U31 | 8K    | **0xFA000** | `machines\ibmpc\IBM 5150 - Cassette BASIC version C1.00 - U31 - 5700035.bin` |
 | BASIC C1.0 U32 | 8K    | **0xFC000** | `machines\ibmpc\IBM 5150 - Cassette BASIC version C1.00 - U32 - 5700043.bin` |
 | BASIC C1.0 U29,U30,U31,U32 | 32K   | **0xF6000** | `machines\ibmpc\ibm-basic-1.00.rom`                              |

#### Basic version C1.1 ROM

 | Description    | Size  | Address     | Location                                                                       |
 |----------------|-------|-------------|------------------------------------------------------------------------------- |
 | BASIC C1.1 U29 | 8K    | **0xF6000** | `machines\ibmpc82\IBM 5150 - Cassette BASIC version C1.10 - U29 - 5000019.bin` |
 | BASIC C1.1 U30 | 8K    | **0xF8000** | `machines\ibmpc82\IBM 5150 - Cassette BASIC version C1.10 - U30 - 5000021.bin` |
 | BASIC C1.1 U31 | 8K    | **0xFA000** | `machines\ibmpc82\IBM 5150 - Cassette BASIC version C1.10 - U31 - 5000022.bin` |
 | BASIC C1.1 U32 | 8K    | **0xFC000** | `machines\ibmpc82\IBM 5150 - Cassette BASIC version C1.10 - U32 - 5000023.bin` |
 | BASIC C1.1 U29,U30,U31,U32 | 32K   | **0xF6000** | `machines\ibmpc82\ibm-basic-1.10.rom`                              |

### XEBEC ROM

  The Xebec ROM is required to use the Hard disks. 

 | Description    | Size  | Address     | Location                                                                       |
 |----------------|-------|-------------|------------------------------------------------------------------------------- |
 | Xebec          | 4K    | **0xC8000** | `hdd\st506\ibm_xebec_62x0822_1985.bin`                                         |

 ---

## Launching 

 Make sure these files are next to `ibm_pc.exe`:
 - Bm437_IBM_CGA.FON
 - Bm437_IBM_MDA.FON

### Command-Line:

| Switch                       | Switch (ALT)           | Description                                             | Values                         |
|------------------------------|------------------------|---------------------------------------------------------|--------------------------------|
| `-config <config_file>`      | `-c <config_file>`     | Set the config file                                     | file path                      |
| `-o <offset>`                | N/A                    | Set the memory offset for the **next ROM file**.        | `0x00000` - `0xFFFFF`          |
| `<rom_file>`                 | N/A                    | Path to a ROM file. It is loaded at the current offset. | file path                      |
| `<A-D>: <disk_path>`         | N/A                    | Load a disk image into a drive.                         | `A`, `B`, `C`, `D` ; file path |
| `-disk-write-protect`        | `-dwp`                 | Write protect the next loaded disk.                     | N/A                            |
| `-disks <0-4>`               | `-ds <0-4>`            | Number of floppy drives present. 0-4.                   | `0` - `4`                      |
| `-video <video_adapter>`     | `-v <video_adapter>`   | Selects which display adapter to emulate                | `MDA`, `CGA`                   |
| `-ram <ram>`                 | `-r <ram>`             | Amount of conventional RAM.                             | 16KB - 736KB                   |
| `-sw1 <sw1>`                 | N/A                    | Value of motherboard DIP switch SW1                     | `0` - `255`                    |
| `-sw2 <sw2>`                 | N/A                    | Value of motherboard DIP switch SW2                     | `0` - `255`                    |
| `-model <model>`             | N/A                    | Motherboard model                                       | `5150_16_64`, `5150_64_256`, `5160` |
| `-dbg`                       | N/A                    | Enables the debug UI                                    | N/A                            |

### Notes
 - Numbers can be written in decimal, hexadecimal (0x), or binary (0b) form.
 - RAM can be provided in KB or BYTES.
 - When a ROM is loaded, `-o` is automatically incremented by the files size; You can chain ROM files with a single `-o <offset>` 
 - If a DIP switch (`sw1`, `sw2`) isnt provided; it is set automatically based on the provided config

 ### Example:
```
ibm_pc.exe -o 0xF6000 cb10_1.bin cb10_2.bin cb10_3.bin cb10_4.bin bios.bin -video CGA -ram 640 -model 5150_64_256 A: msdos4_startup.img

Means:
 Load cb10_1.bin (2K file) at 0xF6000
 Load cb10_2.bin (2K file) at 0xF8000
 Load cb10_3.bin (2K file) at 0xFA000
 Load cb10_4.bin (2K file) at 0xFC000
 Load bios.bin   (8K file) at 0xFE000
 Video adapter is a CGA Card
 640KB conventional RAM installed
 Model 64KB-256KB Motherboard
 msdos4_startup.img disk is in drive A:
```

 ---

### Configuration File:

The emulator supports configuration files written in a custom structured INI-like format called **TOMI** (Tom’s Object Markup INI).  
It extends traditional INI syntax with support for **inline structures** and **nested data**, making it easy to define complex machine setups.

### Format:

#### Basic Key-Value Pairs

Each setting is defined as a **key-value pair**.  
The assignment character `=` associates a key with a value. Whitespace is optional.

```ini
  key1=value
  key2 = value
  key3 = 'value'
  key4 = "value"
  ...
```

Multiple pairs can also appear on the same line, separated by commas:

```ini
  key1=value, key2 = value, key3 = 'value', key4 = "value", ...
```

#### Structs

A struct is a collection of **key-value pairs** grouped together using square brackets `[` `]`.
This allows related settings to be logically grouped.

```ini
  struct = [
    key1=value,
    key2 = value,
    key3 = 'value',
    key4 = "value",
    ...
  ]
```

Structs can also be written in a single line:

```ini
  struct = [ key1=value, key2 = value, key3 = 'value', key4 = "value", ... ]
```
### Settings:
| Key                     | Type   | Description                                        | Values                         |
|-------------------------|--------|----------------------------------------------------|--------------------------------|
| `model`                 | ENUM   | Motherboard model                                  | `5150_16_64`, `5150_64_256`, `5160` |
| `video_adapter`         | ENUM   | Selects which display adapter to emulate           | `MDA`, `CGA`, `CGA40`, `CGA80` |
| `conventional_ram`      | INT    | Amount of conventional RAM installed               | `16K` - `736K`                 |
| `num_floppies`          | INT    | Number of floppy drives present                    | `0` - `4`                      |
| `sw1`                   | INT    | Value of motherboard DIP switch SW1                | `0` - `255`                    |
| `sw2`                   | INT    | Value of motherboard DIP switch SW2                | `0` - `255`                    |
| `sw1_override`          | BOOL   | Forces manual SW1 value instead of auto-calculated | `true`, `false`                |
| `sw2_override`          | BOOL   | Forces manual SW2 value instead of auto-calculated | `true`, `false`                |
| `disk`                  | STRUCT | Defines one or more floppy disk drives             | See below                      |
| `hdd`                   | STRUCT | Defines one or more Hard disk images               | See below                      |
| `rom`                   | STRUCT | Defines one or more ROM images                     | See below                      |
| `texture_scale_mode`    | ENUM   | Texture sampling mode                              | `Nearest`, `Linear`            |
| `display_scale_mode`    | ENUM   | Display scaling behavior                           | `Fit`, `Stretched`             |
| `display_view_mode`     | ENUM   | Display framing mode                               | `Cropped`, `Full`              |
| `correct_aspect_ratio`  | BOOL   | Enables 4:3 aspect correction                      | `true`, `false`                |
| `emulate_max_scanline`  | BOOL   |                                                    | `true`, `false`                |
| `allow_display_disable` | BOOL   | Allow software to disable the display              | `true`, `false`                |
| `delay_display_disable` | BOOL   | Delay the display disable  (prevents flickering due to software disabling and enabling the display rapidly)   | `true`, `false`                |
| `delay_display_disable_time` | INT | The display disable delay time                   | milliseconds                          |
| `mda_font`              | STRING | Path to the MDA font file                          | file path                      |
| `cga_font`              | STRING | Path to the CGA font file                          | file path                      |
| `dbg_ui`                | BOOL   | Enables the debug UI                               | `true`, `false`                |

### ROM settings:
| Key      | Type   | Description                                | Values                |
|----------|--------|--------------------------------------------|-----------------------|
| `path`   | STRING | Path to the ROM file                       | file path             |
| `offset` | INT    | The offset to load the ROM file            | `0x00000` - `0xFFFFF` |

### DISK settings:
| Key             | Type   | Description                         | Values             |
|-----------------|--------|-------------------------------------|--------------------|
| `path`          | STRING | Path to the Disk file               | file path          |
| `drive`         | INT    | The drive to load the disk file in. | `A`, `B`           |
| `write_protect` | BOOL   | write protects the drive            | `true`, `false`    |

### HDD settings:
| Key             | Type   | Description                         | Values             |
|-----------------|--------|-------------------------------------|--------------------|
| `path`          | STRING | Path to the Disk file               | file path          |
| `drive`         | INT    | The drive to load the disk file in. | `C`, `D`           |
| `geometry`      | STRUCT | Geometry override for RAW images    | See below          |
| `type`          | ENUM   | Type override for RAW images        | `Type1`, `Type2`, `Type13`, `Type16` |
 
 - If loading a raw hard disk image; some geometries have ambiguous file sizes and cant determine the geometry from the file size alone.
 - Use `geometry` OR `type` parameter of the hard disk to tell the emulator what the disk geometry is.
 - If both `geometry` and `type` are supplied then the `geometry` is used to determine it.

### GEOMETRY settings:
| Key             | Type   | Description                         | Values             |
|-----------------|--------|-------------------------------------|--------------------|
| `c`             | INT    | Max cylinders of the hard disk      |                    |
| `h`             | INT    | Max heads of the hard disk          |                    |
| `s`             | INT    | Max sectors of the hard disk        |                    |

### Type settings
  See [Hard disk drive geometries](#hard-disk-geometries)

### Notes
 - Each `disk`, `hdd` or `rom` struct can appear multiple times to define multiple drives or ROMS.
 - Numbers can be written in decimal, hexadecimal (0x), or binary (0b) form.
 - Comments start with `;` and extend to the end of the line.
 - Missing values fall back to defaults defined in the emulator.
 - If a `sw1_override` or `sw2_override` is `false`; the associated SW is set automatically based on the provided config
 - RAM must be provided in BYTES.

### Example INI:

```ini
; ----------------- PC Config ------------------
model = '5150_64_256'         ; 5150_16_64, 5150_64_256, 5160
video_adapter = 'CGA'         ; MDA, CGA, CGA40, CGA80
conventional_ram = 0x40000    ; 0x4000 - 0xB8000 (16K - 736K)
num_floppies = 2              ; 0, 1, 2
; ----------------------------------------------

; --------------- DIP SWITCHES -----------------
sw1_override = false
sw2_override = false
sw1 = 0b00000000
sw2 = 0b00000000
; ----------------------------------------------

; --------------- Floppy Disk Drives ----------------
disk = [ drive = 'A', path = 'floppies/msdos4_startup.img'  ]
disk = [ drive = 'B', path = 'floppies/msdos4_working6.img' ]
; ----------------------------------------------

; --------------- Hard Disk Drives ----------------
hdd = [ drive = 'C', path = 'hdds/hdd.vhd'  ]
hdd = [ drive = 'D', path = 'hdds/raw_hdd.raw', type = 'Type13'  ]
; ----------------------------------------------

; -------------------- ROMS --------------------
rom = [ address = 0xF6000, path = 'roms/5150cb10_1.bin' ]
rom = [ address = 0xF8000, path = 'roms/5150cb10_2.bin' ]
rom = [ address = 0xFA000, path = 'roms/5150cb10_3.bin' ]
rom = [ address = 0xFC000, path = 'roms/5150cb10_4.bin' ]
rom = [ address = 0xFE000, path = 'roms/BIOS_IBM5150_27OCT82.BIN' ]
rom = [ address = 0xCC000, path = 'roms/expansion_rom.bin' ]
; ----------------------------------------------

; --------------- Display Config ---------------
texture_scale_mode = 'Nearest' ; Nearest, Linear
display_scale_mode = 'Fit'     ; Fit, Stretched
display_view_mode = 'Cropped'  ; Cropped, Full
correct_aspect_ratio = true
emulate_max_scanline = true
allow_display_disable = true
delay_display_disable = true
delay_display_disable_time = 200
; ----------------------------------------------

; --------------- Display Fonts ----------------
mda_font = 'fonts/Bm437_IBM_MDA.FON'
cga_font = 'fonts/Bm437_IBM_CGA.FON'
; ----------------------------------------------

; ------------------- DEBUG --------------------
dbg_ui = false
; ----------------------------------------------

```

### XEBEC

2 Image formats are currently support for hard disks. VHD or RAW/IMG images. 

#### VHD
If using a VHD the geometry of the disk will be automatically detected since the VHD Stores the disk geometry. 
Using the VHD format allows you to browse a FAT12/FAT16 formated Hard disk in your Host OS like any other VHD.

- A VHD cannot be saved while it is mounted in the Host OS. 
- A VHD can be loaded while it is mounted in the Host OS.

- DOS will not see changes to the VHD made by the Host OS until a reboot of the emulator has been done. DOS caches the FAT and accessed files in RAM. Any new files created or modified might not show up until a reboot of the emulator. Using a program i have created i have been able to get around this issue. See [FDFLUSH.COM](#fdflushcom)

#### RAW IMAGE
If using a RAW image, Some hard disk geometries cant be determined from the file size alone due to multiple hard disk geometries having the same total size.

### Hard Disk Geometries
| Type            | Geometry                            | Total size (MB)     |
|-----------------|-------------------------------------|---------------------|
| `Type1`         | C = 306, H = 4, S = 17              | 10.16MB             |
| `Type2`         | C = 615, H = 4, S = 17              | 20.41MB             |
| `Type13`        | C = 306, H = 8, S = 17              | 20.32MB (ambiguous) |
| `Type16`        | C = 612, H = 4, S = 17              | 20.32MB (ambiguous) |

### Creating your Hard disk
 [TODO]

### Formating your Hard disk

#### Using FDISK/FORMAT
 - Load the unformatted hard disk in the emulator. For this demo; the hard disk is in Drive `C:`
 - Boot a DOS floppy in drive A: that has the `FDISK` and `FORMAT` utilities
 - Run `FDISK` to create a partition table on the hard disk.
 - Run `FORMAT C: /s` to format the hard disk and copy the system files to it.
 - Eject the floppy from drive A:
 - Reboot the emulator.

#### FDFLUSH.COM
This program invalidates the FAT and forces all floppies and hdds to be re-cached next time they are accessed. This allows changes made by the host OS to show up without a reboot of the emulator. This program can be found in the `programs\` directory of the repo.
