# IBM 5150 PC Emulator 

An IBM 5150 Emulator written in C using SDL3

## Emulated Hardware
 - i8086 CPU
 - i8237 DMA
 - i8253 PIT
 - i8255 PPI
 - i8259 PIC
 - i8272 FDC (NEC uPD765)
 - MDA ISA Card (IBM Monochrome Display Adapter)
 - CGA ISA Card (IBM Color Graphics Adapter)
 - FDC ISA Card (Floppy Disk Controller)

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

## FON Files
 FON files can be downloaded from here: https://int10h.org/oldschool-pc-fonts/download/
 
| Description   | File              |
|---------------|-------------------|
| MDA Font 9x14 | Bm437_IBM_MDA.FON |
| CGA Font 8x8  | Bm437_IBM_CGA.FON |

 They have been included in the repo in the `fonts\` directory

## ROM Files

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

 - The BASIC ROM is optional on the IBM PC. The BASIC ROM is required for early DOS versions of the BASIC interpreter, which took the BASIC in ROM and extended it.
 - BASIC ROMS come in two variants, 4 ROM images of 8k in size, and a combined 32K ROM image. You either need all four 8k images, or just the single 32K image.

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

## Launching 

 Make sure these files are next to `ibm_pc.exe`:
 - Bm437_IBM_CGA.FON
 - Bm437_IBM_MDA.FON
 
| Switch                       | Description                                                                                       |
|------------------------------|---------------------------------------------------------------------------------------------------|
| `-o <offset>`                | Sets the memory offset for the **next ROM file**. The offset is in bytes                          |
| `<rom_file>`                 | Path to a ROM file. It is loaded at the current offset. Multiple ROMs can be loaded sequentially. |
| `-disk [A-B:]<disk_path>`    | Load a disk image into drive A or B. Example: `-disk A:msdos4_startup.img`.                       |
| `-disk-write-protect [A-B:]` | Write protect the disk image in drive A or B.                                                     |
| `-video <video_adapter>`     | Select video hardware: MDA, CGA, NONE.                                                            |
| `-ram <ram>`                 | Amount of conventional RAM. For 16–64 KB, multiples of 16 KB; for 64–736 KB, multiples of 32 KB.  |
| `-sw1 <sw1>` / `-sw2 <sw2>`  | Overrides DIP switch settings for the motherboard. If a DIP switch isnt provided; it is set automaticlly based on the config |
| `-model <model>`             | Motherboard model: `5150_16_64` (16–64 KB), `5150_64_256` (64–256 KB), etc.                       |
| `-dbg`                       | Opens a debug window for development.                                                             |
| Numbers                      | Can be in decimal, hex (`0x`), or binary (`0b`).                                                  |

 ### Example:
```
ibm_pc.exe -o 0xF6000 cb10_1.bin cb10_2.bin cb10_3.bin cb10_4.bin bios.bin -video CGA -ram 640 -model 5150_64_256 -disk A:msdos4_startup.img

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
