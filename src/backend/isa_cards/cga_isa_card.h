/* cga_isa_card.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * CGA ISA Card
 */

#ifndef CGA_ISA_CARD_H
#define CGA_ISA_CARD_H

typedef struct ISA_BUS ISA_BUS;
typedef struct CGA CGA;

int isa_card_add_cga(ISA_BUS* bus, CGA* cga);

#endif
