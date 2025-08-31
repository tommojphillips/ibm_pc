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
  git clone --recurse-submodules https://github.com/tommojphillips/ibm_pc_.git
  ```

 2. Open `ibm_pc\vc\ibm_pc.sln`, build, run.

## Sources
 
Notes:
 - IBM released the BIOS source code; listing it in `The IBM Technical Reference`, including comments and addresses.
 - The 8086 Users Manual Oct79 has CPU cycle listings per opcode, including transfers.

 ---
 
 | IBM PC 5150                   |                                                                                               |
 | ----------------------------- | --------------------------------------------------------------------------------------------- |
 | `IBM Techinical Manual AUG81` | https://www.minuszerodegrees.net/manuals/IBM/IBM_5150_Technical_Reference_6025005_AUG81.pdf   |
 | `IBM Techinical Manual APR84` | https://www.minuszerodegrees.net/manuals/IBM/IBM_5150_Technical_Reference_6322507_APR84.pdf   |
 | `IBM Options & Adapters`      | https://www.minuszerodegrees.net/oa/oa.htm                                                    |
 | `Port Definitions (OS2 Site)` | https://www.os2site.com/sw/info/memory/ports.txt                                              |
 | `Minus Zero Degrees`          | https://www.minuszerodegrees.net                                                              |

 ---

 | IBM Disk Drive 5 1/4"         |                                                                                               |
 | ----------------------------- | --------------------------------------------------------------------------------------------- |
 | `IBM 5 1/4" Disk Drive`       | https://www.minuszerodegrees.net/oa/OA%20-%20IBM%205.25%20Diskette%20Drive.pdf                |

 ---
 
 | CPU i8086                       |                                                                                                    |
 | ------------------------------- | -------------------------------------------------------------------------------------------------- |
 | `Intel 8086 Users Manual Oct79` | https://bitsavers.org/components/intel/8086/9800722-03_The_8086_Family_Users_Manual_Oct79.pdf      |
 | `The 8086 Book`                 | https://vtda.org/books/Computing/Programming/The8086Book_RussellRectorGeorgeAlexy.pdf              |
 | `Programming The 8086/8088`     | https://ia803207.us.archive.org/15/items/Programming_the_8086_8088/Programming_the_8086_8088.pdf   |
 | `Ken's Blogs`                   | https://www.righto.com/2023/02/                                                                    |

 ---

 | CPU i8086 undocumented infomation               |                                                                                     |
 | ----------------------------------------------- | ----------------------------------------------------------------------------------- |
 | `Undocumented Instructions (Ken's Blog)`        | https://www.righto.com/2023/07/undocumented-8086-instructions.html                  |
 | `Undocumented Opcodes part I (os2 museum)`      | https://www.os2museum.com/wp/undocumented-8086-opcodes-part-i/                      |
 | `Undocumented Opcodes (os2 museum)`             | https://www.os2museum.com/wp/undocumented-8086-opcodes/                             |
 | `Undocumented Infomation (pcjs)`                | https://www.pcjs.org/documents/manuals/intel/8086/                                  |

 ---
 
 | PIT i8253                 |                                                                                                           |
 | ------------------------- | --------------------------------------------------------------------------------------------------------- |
 | `osdev wiki`              | https://wiki.osdev.org/Programmable_Interval_Timer                                                        |
 | `datasheet`               | https://www.cpcwiki.eu/imgs/e/e3/8253.pdf                                                                 |

 ---
 
 | PIC i8259                 |                                                                                                           |
 | ------------------------- | --------------------------------------------------------------------------------------------------------- |
 | `osdev wiki`              | https://wiki.osdev.org/8259_PIC                                                                           |
 | `datasheet`               | https://pdos.csail.mit.edu/6.828/2014/readings/hardware/8259A.pdf                                         |

 ---
 
 | PPI i8255                 |                                                                                                           |
 | ------------------------- | --------------------------------------------------------------------------------------------------------- |
 | `datasheet`               | http://aturing.umcs.maine.edu/~meadow/courses/cos335/Intel8255A.pdf                                       |

 ---
 
 | DMA i8237                 |                                                                                                           |
 | ------------------------- | --------------------------------------------------------------------------------------------------------- |
 | `datasheet`               | https://pdos.csail.mit.edu/6.828/2012/readings/hardware/8237A.pdf                                         |

 ---
 
 | FDC i8272 (NEC uPD765)    |                                                                                                           |
 | ------------------------- | --------------------------------------------------------------------------------------------------------- |
 | `datasheet`               | http://www.threedee.com/jcm/terak/docs/Intel%208272A%20Floppy%20Controller.pdf                            |
 | `datasheet`               | https://bitsavers.org/components/nec/_appNotes/AN08_uPD765_preliminary_197902.pdf                         |
 | `datasheet`               | https://www.bitsavers.org/components/nec/_dataSheets/uPD765_Data_Sheet_Dec78.pdf                          |
 | `datasheet`               | http://dunfield.classiccmp.org/r/765.pdf                                                                  |
 | `cpc wiki`                | https://www.cpcwiki.eu/index.php/765_FDC                                                                  |
 | `osdev wiki`              | https://wiki.osdev.org/Floppy_Disk_Controller                                                             |

 ---
 
 | CRTC MC6845               |                                                                                                           |
 | ------------------------- | --------------------------------------------------------------------------------------------------------- |
 | `datasheet`               | https://bitsavers.trailing-edge.com/components/motorola/_dataSheets/6845.pdf                              |

 ---

 | MDA (Monochrome Display Adapter)                     |                                                                                                             |
 | ---------------------------------------------------- | ----------------------------------------------------------------------------------------------------------- |
 | `IBM Monochrome Display and Printer Adapter 84 vol1` | https://www.minuszerodegrees.net/oa/OA%20-%20IBM%20Monochrome%20Display%20(5151).pdf                        |
 | `IBM Monochrome Display and Printer Adapter 84 vol2` | https://www.minuszerodegrees.net/oa/OA%20-%20IBM%20Monochrome%20Display%20and%20Printer%20Adapter.pdf       |
 | `MDA notes (vintage pc)`                             | https://www.seasip.info/VintagePC/mda.html                                                                  |

 ---

 | CGA (Color Graphics Adapter)          |                                                                                                     |
 | ------------------------------------- | --------------------------------------------------------------------------------------------------- |
 | `IBM Color/Graphics Adapter 84 vol1`  | https://www.minuszerodegrees.net/oa/OA%20-%20IBM%20Color%20Display%20(5153).pdf                     |
 | `IBM Color/Graphics Adapter 84 vol2`  | https://www.minuszerodegrees.net/oa/OA%20-%20IBM%20Color%20Graphics%20Monitor%20Adapter%20(CGA).pdf |
 | `CGA notes (vintage pc)`              | https://www.seasip.info/VintagePC/cga.html                                                          |

 ---

 | EGA (Enhanced Graphics Adapter)         |                                                                                              |
 | --------------------------------------- | -------------------------------------------------------------------------------------------- |
 | `IBM Enhanced Color Display 84 vol1`    | https://www.minuszerodegrees.net/oa/OA%20-%20IBM%20Enhanced%20Color%20Display%20(5154).pdf   |
 | `IBM Enhanced Graphics Adapter 84 vol2` | https://www.minuszerodegrees.net/oa/OA%20-%20IBM%20Enhanced%20Graphics%20Adapter.pdf         |
