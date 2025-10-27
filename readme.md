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

| Dependencies   |                                         |
| -------------- | --------------------------------------- |
| I8086          | https://github.com/tommojphillips/i8086 |

 ---

 1. Clone the repo and submodules
  
  ```
  git clone --recurse-submodules https://github.com/tommojphillips/ibm_pc.git
  ```

 2. Open `ibm_pc\vc\ibm_pc.sln`, build, run.
