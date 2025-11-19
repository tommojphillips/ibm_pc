/* fdc_isa_card.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * FDC ISA Card
 */

#ifndef FDC_ISA_CARD_H
#define FDC_ISA_CARD_H

typedef struct ISA_BUS ISA_BUS;
typedef struct FDC FDC;

int isa_card_add_fdc(ISA_BUS* bus, FDC* fdc);

#endif
