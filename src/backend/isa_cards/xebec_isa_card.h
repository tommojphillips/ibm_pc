/* xebec_isa_card.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * XEBEC HDC ISA Card
 */

#ifndef XEBEC_ISA_CARD_H
#define XEBEC_ISA_CARD_H

typedef struct ISA_BUS ISA_BUS;
typedef struct XEBEC_HDC XEBEC_HDC;

int isa_card_add_xebec(ISA_BUS* bus, XEBEC_HDC* hdc);

#endif
