# IBM PC Emulator 

An IBM PC Emulator written in C using SDL3

## Emulated Hardware
 - i8086 CPU
 - i8237 DMA
 - i8253 PIT
 - i8255 PPI
 - i8259 PIC
 - i8272 FDC (NEC uPD765)
 - MDA ISA Card (IBM Monochrome Display Adapter)
 - CGA ISA Card (IBM Color Graphics Adapter)

## Building

The project is built in Visual Studio 2022

| Dependencies   |                                               |
| -------------- | --------------------------------------------- |
| I8086          | https://github.com/tommojphillips/i8086       |
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
 Some ROMS are required for full functionality.
 ROMS can be found on 86Box's github: https://github.com/86Box/roms/

### IBM PC 5150 ROM

 - The earlier 1981 BIOSes only read the first 4 bits of SW2 resulting in a max of 544KB of RAM 
 - The later 1982 BIOSes read the first 5 bits of SW2 resulting in a offical max of 640KB of RAM

| Description  | Date        | Size | Address     | Location                                              |
|--------------|-------------|------|-------------|-------------------------------------------------------|
| IBM 5150 U33 | 24 APR 1981 | 8K   | **0xFE000** | `machines\ibmpc\BIOS_IBM5150_24APR81_5700051_U33.BIN` |
| IBM 5150 U33 | 19 Oct 1981 | 8K   | **0xFE000** | `machines\ibmpc\BIOS_IBM5150_19OCT81_5700671_U33.BIN` |
| IBM 5150 U33 | 16 AUG 1982 | 8K   | **0xFE000** | `machines\ibmpc82\BIOS_5150_16AUG82_5000024_U33.BIN`  |
| IBM 5150 U33 | 27 Oct 1982 | 8K   | **0xFE000** | `machines\ibmpc82\BIOS_5150_27OCT82_1501476_U33.BIN`  |

### BASIC ROMS

#### Basic version C1.0 ROM 

 | Description    | Size  | Address     | Location                                                                     |
 |----------------|------ |-------------|----------------------------------------------------------------------------- |
 | BASIC C1.0 U29 | 8K    | **0xF6000** | `machines\ibmpc\IBM 5150 - Cassette BASIC version C1.00 - U29 - 5700019.bin` |
 | BASIC C1.0 U30 | 8K    | **0xF8000** | `machines\ibmpc\IBM 5150 - Cassette BASIC version C1.00 - U30 - 5700027.bin` |
 | BASIC C1.0 U31 | 8K    | **0xFA000** | `machines\ibmpc\IBM 5150 - Cassette BASIC version C1.00 - U31 - 5700035.bin` |
 | BASIC C1.0 U32 | 8K    | **0xFC000** | `machines\ibmpc\IBM 5150 - Cassette BASIC version C1.00 - U32 - 5700043.bin` |
 | BASIC C1.0     | 32K   | **0xF6000** | `machines\ibmpc\ibm-basic-1.00.rom`                                          |

#### Basic version C1.1 ROM

 | Description    | Size  | Address     | Location                                                                       |
 |----------------|-------|-------------|------------------------------------------------------------------------------- |
 | BASIC C1.1 U29 | 8K    | **0xF6000** | `machines\ibmpc82\IBM 5150 - Cassette BASIC version C1.10 - U29 - 5000019.bin` |
 | BASIC C1.1 U30 | 8K    | **0xF8000** | `machines\ibmpc82\IBM 5150 - Cassette BASIC version C1.10 - U30 - 5000021.bin` |
 | BASIC C1.1 U31 | 8K    | **0xFA000** | `machines\ibmpc82\IBM 5150 - Cassette BASIC version C1.10 - U31 - 5000022.bin` |
 | BASIC C1.1 U32 | 8K    | **0xFC000** | `machines\ibmpc82\IBM 5150 - Cassette BASIC version C1.10 - U32 - 5000023.bin` |
 | BASIC C1.1     | 32K   | **0xF6000** | `machines\ibmpc82\ibm-basic-1.10.rom`                                          |

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
| `-video <video_adapter>`     | `-v`                   | Selects which display adapter to emulate                | `MDA`, `CGA`                   |
| `-ram <ram>`                 | `-r <ram>`             | Amount of conventional RAM.                             | 16KB - 736KB                   |
| `-sw1 <sw1>`                 | N/A                    | Value of motherboard DIP switch SW1                     | `0` - `255`                    |
| `-sw2 <sw2>`                 | N/A                    | Value of motherboard DIP switch SW2                     | `0` - `255`                    |
| `-model <model>`             | N/A                    | Motherboard model                                       | `5150_16_64`, `5150_64_256`    |
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
| Key                    | Type   | Description                                        | Values                         |
|------------------------|--------|----------------------------------------------------|--------------------------------|
| `model`                | ENUM   | Motherboard model                                  | `5150_16_64`, `5150_64_256`    |
| `video_adapter`        | ENUM   | Selects which display adapter to emulate           | `MDA`, `CGA`, `CGA40`, `CGA80` |
| `conventional_ram`     | INT    | Amount of conventional RAM installed               | `16K` - `736K`                 |
| `num_floppies`         | INT    | Number of floppy drives present                    | `0` - `4`                      |
| `sw1`                  | INT    | Value of motherboard DIP switch SW1                | `0` - `255`                    |
| `sw2`                  | INT    | Value of motherboard DIP switch SW2                | `0` - `255`                    |
| `sw1_override`         | BOOL   | Forces manual SW1 value instead of auto-calculated | `true`, `false`                |
| `sw2_override`         | BOOL   | Forces manual SW2 value instead of auto-calculated | `true`, `false`                |
| `disk`                 | STRUCT | Defines one or more floppy disk drives             | See below                      |
| `rom`                  | STRUCT | Defines one or more ROM images                     | See below                      |
| `texture_scale_mode`   | ENUM   | Texture sampling mode                              | `Nearest`, `Linear`            |
| `display_scale_mode`   | ENUM   | Display scaling behavior                           | `Fit`, `Stretched`             |
| `display_view_mode`    | ENUM   | Display framing mode                               | `Cropped`, `Full`              |
| `correct_aspect_ratio` | BOOL   | Enables 4:3 aspect correction                      | `true`, `false`                |
| `emulate_max_scanline` | BOOL   |                                                    | `true`, `false`                |
| `mda_font`             | STRING | Path to the MDA font file                          | file path                      |
| `cga_font`             | STRING | Path to the CGA font file                          | file path                      |
| `dbg_ui`               | BOOL   | Enables the debug UI                               | `true`, `false`                |

### ROM settings:
| Key      | Type   | Description                                | Values                |
|----------|--------|--------------------------------------------|-----------------------|
| `path`   | STRING | Path to the ROM file                       | file path             |
| `offset` | INT    | The offset to load the ROM file            | `0x00000` - `0xFFFFF` |

### DISK settings:
| Key             | Type   | Description                         | Values             |
|-----------------|--------|-------------------------------------|--------------------|
| `path`          | STRING | Path to the Disk file               | file path          |
| `drive`         | INT    | The drive to load the disk file in. | `A`, `B`, `C`, `D` |
| `write_protect` | BOOL   | write protects the drive            | `true`, `false`    |

### Notes
 - Each disk or rom struct can appear multiple times to define multiple drives or ROMS.
 - Numbers can be written in decimal, hexadecimal (0x), or binary (0b) form.
 - Comments start with `;` and extend to the end of the line.
 - Missing values fall back to defaults defined in the emulator.
 - If a `sw1_override` or `sw2_override` is `false`; the associated SW is set automatically based on the provided config
 - RAM must be provided in BYTES.

### Example INI:

```ini
; ----------------- PC Config ------------------
model = '5150_64_256'         ; 5150_16_64, 5150_64_256
video_adapter = 'CGA'         ; MDA, CGA, CGA40, CGA80
conventional_ram = 0x40000    ; 0x4000 - 0xB8000 (16K - 736K)
num_floppies = 2              ; 0, 1, 2, 4
; ----------------------------------------------

; --------------- DIP SWITCHES -----------------
sw1_override = false
sw2_override = false
sw1 = 0b00000000
sw2 = 0b00000000
; ----------------------------------------------

; --------------- Floppy Drives ----------------
disk = [ drive = 'A', path = 'floppies/msdos4_startup.img'  ]
disk = [ drive = 'B', path = 'floppies/msdos4_working6.img' ]
; ----------------------------------------------

; -------------------- ROMS --------------------
rom = [ address = 0xF6000, path = 'roms/5150cb10_1.bin' ]
rom = [ address = 0xF8000, path = 'roms/5150cb10_2.bin' ]
rom = [ address = 0xFA000, path = 'roms/5150cb10_3.bin' ]
rom = [ address = 0xFC000, path = 'roms/5150cb10_4.bin' ]
rom = [ address = 0xFE000, path = 'roms/BIOS_IBM5150_27OCT82.BIN' ]
rom = [ address = 0xC8000, path = 'roms/expansion_rom.bin' ]
; ----------------------------------------------

; --------------- Display Config ---------------
texture_scale_mode = 'Nearest' ; Nearest, Linear
display_scale_mode = 'Fit'     ; Fit, Stretched
display_view_mode = 'Cropped'  ; Cropped, Full
correct_aspect_ratio = true
emulate_max_scanline = true
; ----------------------------------------------

; --------------- Display Fonts ----------------
mda_font = 'fonts/Bm437_IBM_MDA.FON'
cga_font = 'fonts/Bm437_IBM_CGA.FON'
; ----------------------------------------------

; ------------------- DEBUG --------------------
dbg_ui = false
; ----------------------------------------------

```
