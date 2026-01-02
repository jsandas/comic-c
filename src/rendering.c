/*
 * rendering.c - Graphics rendering wrapper
 * 
 * Currently just wraps the assembly implementation while providing
 * a C interface for future porting. The actual rendering is still done
 * by the assembly render_map function.
 * 
 * TODO: Port render_map to pure C once 16-bit calling conventions
 * and far pointer handling are fully working.
 */

#include "rendering.h"

/* For now, just a placeholder - the real work is done in assembly */
void __near render_map_c(void)
{
    /* This is never called - render_map in assembly calls render_map directly */
}




