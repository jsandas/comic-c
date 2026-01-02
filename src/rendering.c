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
 *   points = points to add (in base-100 format, 0-99)
 */
void award_points_c(uint8_t points)
{
    uint8_t carry = points;
    uint8_t digit_index = 0;  /* Start with least significant digit */
    
    /* Process each digit with carry propagation */
    while (carry != 0 && digit_index < 3) {
        /* Add carry to current digit */
        uint8_t new_digit = score[digit_index] + carry;
        carry = 0;
        
        /* Check for overflow (>= 100) */
        if (new_digit >= 100) {
            carry = 1;           /* Carry to next digit */
            new_digit -= 100;    /* Keep only the remainder (value modulo 100) */
        }
        
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




