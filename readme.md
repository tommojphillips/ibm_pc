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
 - PC Speaker

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
 2. Download SDL3 VC package from their github repo [`SDL3-devel-X.X.X-VC.zip`](https://github.com/libsdl-org/SDL/releases/download/release-3.2.24/SDL3-devel-3.2.24-VC.zip) (direct download link)
     - Extract and copy the `include` and `lib` folders into `ibm_pc/lib/SDL3` you will have to create the `SDL3` directory.
     - Extract and copy the `include` and `lib` folders into `ibm_pc/lib/UI/lib/SDL3` you will have to create the `SDL3` directory.
     
 3. Download SDL3-TTF VC package from their github repo [`SDL3-devel-X.X.X-VC.zip`](https://github.com/libsdl-org/SDL_ttf/releases/download/release-3.2.2/SDL3_ttf-devel-3.2.2-VC.zip) (direct download link)
     - Extract and copy the `include` and `lib` folders into `ibm_pc/lib/SDL3_ttf` you will have to create the `SDL3_ttf` directory.

 2. Open `ibm_pc\lib\UI\vc\UI.sln`, build

 3. Open `ibm_pc\vc\ibm_pc.sln`, build, run.
