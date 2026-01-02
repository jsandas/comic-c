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
#pragma aux comic_hp "*"
#pragma aux increment_comic_hp_c "*"
#pragma aux decrement_comic_hp_c "*"
#pragma aux blit_8x16 "*"

/* External declarations for globals needed by award_points_c, increment_comic_hp_c, decrement_comic_hp_c */
extern uint8_t score[3];
extern uint8_t score_10000_counter;
extern uint8_t comic_hp;
extern void award_extra_life(void);
extern void blit_8x16(void);  /* Assembly function to blit 8x16 graphics */

/* Award points to the player. Points are added with carry propagation
 * through a base-100 digit system. Each carry into the ten-thousands digit
 * (digit index 2) represents 10,000 points; every 5 such carries (50,000 total
 * points) awards an extra life via the score_10000_counter mechanism.
 *
 * Input:
 *   points = points to add (0–99; values >= 100 are not recommended as they may
 *            produce unexpected results unless proper carry division is implemented)
 * 
 * The function uses carry propagation where each digit holds 0–99.
 * If a digit exceeds 99 after addition, the excess carries to the next digit.
 * score_10000_counter tracks individual 10,000-point carries; when it reaches 5,
 * an extra life is awarded and the counter resets.
 */
void award_points_c(uint8_t points)
{
    uint8_t carry = points;
    uint8_t digit_index = 0;  /* Start with least significant digit */
    
    /* Process each digit with carry propagation */
    while (carry != 0 && digit_index < 3) {
        /* Add carry to current digit */
        uint8_t digit_sum = score[digit_index] + carry;
        
        /* Extract the carry and remainder using division/modulo.
         * This handles cases where carry > 100 (e.g., if a caller passes points >= 100).
         * digit_sum % 100 gives the digit value (0–99)
         * digit_sum / 100 gives the carry for the next digit
         */
        carry = digit_sum / 100;
        
        /* Store the updated digit */
        score[digit_index] = digit_sum % 100;
        
        /* TODO: Update video display for this digit */
        /* For now, just update the score array */
        
        /* Check for extra life bonus. Each carry into the ten-thousands digit
         * (digit index 2) represents 10,000 points. We increment score_10000_counter
         * and award a bonus life when the counter reaches 5 (representing 50,000 points).
         * This occurs when there is a carry out of digit index 1 (the thousands digit)
         * into digit index 2 (the ten-thousands digit).
         */
        if (digit_index == 1 && carry > 0) {
            score_10000_counter++;

            /* Award extra life every 5 such events (50000 points) */
            if (score_10000_counter >= 5) {
                score_10000_counter = 0;
                award_extra_life();
            }
        }
        
        /* Move to next digit */
        digit_index++;
    }
}

/* Add 1 unit to comic_hp, unless comic_hp is already at MAX_HP, in which case
 * award 1800 points.
 * 
 * Output:
 *   comic_hp = incremented by 1 unless already at MAX_HP
 *   score = increased by 1800 points if comic_hp was already at MAX_HP
 */
void increment_comic_hp_c(void)
{
    /* Check if HP is already at maximum */
    if (comic_hp >= MAX_HP) {
        /* HP is already full. Award overcharge bonus points.
         * 1800 points = 18 * 100, so call award_points_c with 18 to award 1800 points.
         */
        award_points_c(18);
        return;
    }
    
    /* Increment HP */
    comic_hp++;
    
    /* Note: Video display update (blit_8x16) is handled by the assembly wrapper.
     * The assembly increment_comic_hp function calls blit_8x16 with GRAPHIC_METER_FULL
     * to update the HP meter at position (240, 82).
     */
}

/* Play SOUND_DAMAGE (if invoked by an external assembly wrapper) and subtract
 * 1 unit from comic_hp, unless already at 0.
 *
 * Note: This C function does not itself trigger the damage sound; any sound
 * effect must be handled by an external assembly wrapper or caller before
 * or around the call to this function.
 *
 * Output:
 *   comic_hp = decremented by 1 unless already at 0
 */
void decrement_comic_hp_c(void)
{
    /* Damage sound is not played in this C function; if desired, an external
     * assembly wrapper should invoke the appropriate sound routine before
     * calling decrement_comic_hp_c.
     */
    
    /* Check if HP is already at 0 */
    if (comic_hp == 0) {
        return;  /* No HP remaining to take away */
    }
    
    /* Decrement HP */
    comic_hp--;
    
    /* Note: Video display update (blit_8x16) is handled by the assembly wrapper.
     * The assembly decrement_comic_hp function calls blit_8x16 with GRAPHIC_METER_EMPTY
     * to update the HP meter at position (240, 82).
     */
}

/* For now, just a placeholder - the real work is done in assembly */
void __near render_map_c(void)
{
    /* This is never called - render_map in assembly calls render_map directly */
}




