/*
 * rendering.c - Graphics rendering and game logic in C
 * 
 * This file contains game logic functions ported from assembly to C.
 * Hardware-specific graphics operations remain in assembly.
 */

#include "rendering.h"

/* Prevent name mangling for assembly symbols */
#pragma aux score "*"
#pragma aux score_10000_counter "*"
#pragma aux award_extra_life "*"
#pragma aux award_points_c "*"

/* External declarations for globals needed by award_points */
extern uint8_t score[3];
extern uint8_t score_10000_counter;
extern void award_extra_life(void);

/* Award points to the player. Points are added with carry propagation
 * through a base-100 digit system. Every 50000 points (5 carries into
 * the ten-thousands digit) awards an extra life.
 *
 * Input:
 *   points = points to add (0–99; values >= 100 are not recommended as they may
 *            produce unexpected results unless proper carry division is implemented)
 * 
 * The function uses carry propagation where each digit holds 0–99.
 * If a digit exceeds 99 after addition, the excess carries to the next digit.
 */
void award_points_c(uint8_t points)
{
    uint8_t carry = points;
    uint8_t digit_index = 0;  /* Start with least significant digit */
    
    /* Process each digit with carry propagation */
    while (carry != 0 && digit_index < 3) {
        /* Add carry to current digit */
        uint8_t new_digit = score[digit_index] + carry;
        
        /* Extract the carry and remainder using division/modulo.
         * This handles cases where carry > 100 (e.g., if a caller passes points >= 100).
         * new_digit % 100 gives the digit value (0–99)
         * new_digit / 100 gives the carry for the next digit
         */
        carry = new_digit / 100;
        new_digit = new_digit % 100;
        
        /* Store the updated digit */
        score[digit_index] = new_digit;
        
        /* TODO: Update video display for this digit */
        /* For now, just update the score array */
        
        /* Move to next digit */
        digit_index++;
    }
    
    /* Check for extra life bonus (every 50000 points).
     * We treat any addition that reaches the third digit (ten-thousands place)
     * or beyond (digit_index >= 2) as a "carry" event for the bonus counter.
     */
    if (digit_index >= 2) {
        /* The addition reached/processed the ten-thousands digit */
        score_10000_counter++;
        
        /* Award extra life every 5 such events (50000 points) */
        if (score_10000_counter >= 5) {
            score_10000_counter = 0;
            award_extra_life();
        }
    }
}

/* For now, just a placeholder - the real work is done in assembly */
void __near render_map_c(void)
{
    /* This is never called - render_map in assembly calls render_map directly */
}




