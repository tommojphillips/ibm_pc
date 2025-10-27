/* mda_isa_card.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * MDA ISA Card
 */

#ifndef MDA_ISA_CARD_H
#define MDA_ISA_CARD_H

typedef struct ISA_BUS ISA_BUS;
typedef struct MDA MDA;

int isa_card_add_mda(ISA_BUS* bus, MDA* mda);

#endif
