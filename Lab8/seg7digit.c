#include <stdint.h>
#include <stdbool.h>
#include "seg7.h"

/*
 * 7-segment coding table. See https://en.wikipedia.org/wiki/Seven-segment_display. The segments
 * are named as A, B, C, D, E, F, G. In this coding table, segments A-G are mapped to bits 0-7.
 * Bit 7 is not used in the coding. This display uses active high signal, in which '1' turns ON a
 * segment, and '0' turns OFF a segment.
 *
 * Note: The table is incomplete. Fill the codes for digits 4-9.
 */


static uint8_t seg7_coding_table[11] = {
    0b00111111,   // 0
    0b00000110,   // 1
    0b01011011,   // 2
    0b01001111,   // 3
    0b01100110,   // 4
    0b01101101,   // 5
    0b01111101,   // 6
    0b00000111,   // 7
    0b01111111,   // 8
    0b01101111,   // 9
    0b00000000    // 10 (BLANK_DIGIT)
};



void Seg7Update(Seg7Display *seg7)
{
    uint8_t seg7_code[4];
    uint8_t colon_code;
    uint8_t digit_val;

    // Get the raw encoding for the colon
    colon_code = seg7->colon_on ? 0b10000000 : 0b00000000;


    // Digit 0 (right-most)
    digit_val = seg7->digit[0];
    seg7_code[0] = (digit_val > 9) ? seg7_coding_table[BLANK_DIGIT] : seg7_coding_table[digit_val];
    seg7_code[0] += colon_code;

    // Digit 1
    digit_val = seg7->digit[1];
    seg7_code[1] = (digit_val > 9) ? seg7_coding_table[BLANK_DIGIT] : seg7_coding_table[digit_val];
    seg7_code[1] += colon_code;

    // Digit 2
    digit_val = seg7->digit[2];
    seg7_code[2] = (digit_val > 9) ? seg7_coding_table[BLANK_DIGIT] : seg7_coding_table[digit_val];
    seg7_code[2] += colon_code;

    // Digit 3 (left-most)
    digit_val = seg7->digit[3];
    seg7_code[3] = (digit_val > 9) ? seg7_coding_table[BLANK_DIGIT] : seg7_coding_table[digit_val];
    seg7_code[3] += colon_code;

    // Update the 7-segment
    Seg7RawUpdate(seg7_code);
}
