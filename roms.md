ROMS can be found on 86Box's github: https://github.com/86Box/roms/

- [5150 BIOS](#ibm-pc-5150-pc-roms)
- [5160 BIOS](#ibm-pc-5160-xt-roms)
- [Basic ROM](#basic-roms)
- [XEBEC ROM](#xebec-rom)

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
