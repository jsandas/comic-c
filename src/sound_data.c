/**
 * Sound Data Definitions
 * 
 * Contains the actual sound data arrays for the game.
 * This file ensures only one copy of each sound array exists in the executable.
 */

#include "sound_data.h"

/* Frequency definitions for special sounds */
#define SOUND_FREQ_94HZ   0x3195  /*  94.003 Hz */
#define SOUND_FREQ_95HZ   0x310f  /*  95.006 Hz */
#define SOUND_FREQ_96HZ   0x308c  /*  96.008 Hz */
#define SOUND_FREQ_97HZ   0x300c  /*  97.007 Hz */
#define SOUND_FREQ_98HZ   0x2f8f  /*  98.003 Hz */
#define SOUND_FREQ_99HZ   0x2f14  /*  99.003 Hz */
#define SOUND_FREQ_100HZ  0x2e9b  /* 100.007 Hz */
#define SOUND_FREQ_105HZ  0x2c63  /* 105.006 Hz */
#define SOUND_FREQ_110HZ  0x2a5f  /* 110.001 Hz */
#define SOUND_FREQ_125HZ  0x2549  /* 125.006 Hz */
#define SOUND_FREQ_130HZ  0x23da  /* 130.005 Hz */
#define SOUND_FREQ_146HZ  0x1fec  /* 146.009 Hz */
#define SOUND_FREQ_150HZ  0x1f12  /* 150.010 Hz */
#define SOUND_FREQ_160HZ  0x1d21  /* 160.008 Hz */
#define SOUND_FREQ_200HZ  0x174d  /* 200.031 Hz */
#define SOUND_FREQ_210HZ  0x1631  /* 210.030 Hz */
#define SOUND_FREQ_220HZ  0x152f  /* 220.022 Hz */
#define SOUND_FREQ_230HZ  0x1443  /* 230.033 Hz */
#define SOUND_FREQ_240HZ  0x136b  /* 240.029 Hz */
#define SOUND_FREQ_250HZ  0x12a4  /* 250.038 Hz */
#define SOUND_FREQ_300HZ  0x0f89  /* 300.021 Hz */
#define SOUND_FREQ_400HZ  0x0ba6  /* 400.128 Hz */
#define SOUND_FREQ_600HZ  0x07c4  /* 600.192 Hz */
#define SOUND_FREQ_900HZ  0x052d  /* 900.515 Hz */

/* Title sequence music - melody from the original game */
const uint16_t SOUND_TITLE[] = {
	NOTE_D3, 3,
	NOTE_E3, 3,
	NOTE_F3, 6,
	NOTE_A3, 3,
	NOTE_A3, 6,
	NOTE_A3, 3,
	NOTE_A3, 3,
	NOTE_A3, 3,
	NOTE_G3, 6,
	NOTE_F3, 6,
	NOTE_E3, 6,
	NOTE_D3, 3,
	NOTE_E3, 3,
	NOTE_F3, 6,
	NOTE_G3, 3,
	NOTE_G3, 5,
	NOTE_G3, 3,
	NOTE_G3, 3,
	NOTE_G3, 3,
	NOTE_F3, 6,
	NOTE_E3, 6,
	NOTE_D3, 6,
	NOTE_D3, 3,
	NOTE_E3, 3,
	NOTE_F3, 6,
	NOTE_A3, 3,
	NOTE_A3, 5,
	NOTE_A3, 3,
	NOTE_A3, 3,
	NOTE_A3, 3,
	NOTE_G3, 6,
	NOTE_F3, 7,
	NOTE_E3, 12,
	NOTE_A3, 6,
	NOTE_G3, 3,
	NOTE_F3, 6,
	NOTE_E3, 3,
	NOTE_D3, 9,
	NOTE_F3, 3,
	NOTE_E3, 6,
	NOTE_D3, 12,
	NOTE_A3, 14,
	NOTE_G3, 3,
	NOTE_F3, 3,
	NOTE_E3, 3,
	NOTE_F3, 13,
	NOTE_D3, 13,
	NOTE_G3, 15,
	NOTE_F3, 3,
	NOTE_E3, 3,
	NOTE_D3, 3,
	NOTE_E3, 13,
	NOTE_C3, 13,
	NOTE_A3, 16,
	NOTE_G3, 3,
	NOTE_F3, 3,
	NOTE_E3, 3,
	NOTE_F3, 13,
	NOTE_D3, 11,
	NOTE_A3, 6,
	NOTE_G3, 3,
	NOTE_F3, 6,
	NOTE_E3, 3,
	NOTE_D3, 10,
	NOTE_F3, 3,
	NOTE_E3, 6,
	NOTE_D3, 10,
	SOUND_TERMINATOR, 0
};

/* Door opening sound */
const uint16_t SOUND_DOOR[] = {
	0x0f00, 1,  /* 310.724 Hz */
	0x0d00, 1,  /* 358.528 Hz */
	0x0c00, 1,  /* 388.406 Hz */
	0x0b00, 1,  /* 423.715 Hz */
	0x0a00, 1,  /* 466.087 Hz */
	0x0b00, 1,  /* 423.715 Hz */
	0x0c00, 1,  /* 388.406 Hz */
	0x0d00, 1,  /* 358.528 Hz */
	0x0f00, 1,  /* 310.724 Hz */
	SOUND_TERMINATOR, 0
};

/* Death sound */
const uint16_t SOUND_DEATH[] = {
	0x3000, 1,  /*  97.101 Hz */
	0x3800, 1,  /*  83.230 Hz */
	0x4000, 1,  /*  72.826 Hz */
	0x0800, 1,  /* 582.608 Hz */
	0x1000, 1,  /* 291.304 Hz */
	0x1800, 1,  /* 194.203 Hz */
	0x2000, 1,  /* 145.652 Hz */
	0x2800, 1,  /* 116.522 Hz */
	0x3000, 1,  /*  97.101 Hz */
	SOUND_TERMINATOR, 0
};

/* Teleport sound */
const uint16_t SOUND_TELEPORT[] = {
	0x2000, 2,  /* 145.652 Hz */
	0x1e00, 2,  /* 155.362 Hz */
	0x1c00, 2,  /* 166.460 Hz */
	0x1a00, 2,  /* 179.264 Hz */
	0x1c00, 2,  /* 166.460 Hz */
	0x1e00, 2,  /* 155.362 Hz */
	0x2000, 2,  /* 145.652 Hz */
	SOUND_TERMINATOR, 0
};

/* Score tally sound */
const uint16_t SOUND_SCORE_TALLY[] = {
	0x1800, 1,  /* 194.203 Hz */
	0x1600, 1,  /* 211.858 Hz */
	SOUND_TERMINATOR, 0
};

/* Materialize sound (item/enemy spawn) */
const uint16_t SOUND_MATERIALIZE[] = {
	SOUND_FREQ_200HZ, 1,
	SOUND_FREQ_220HZ, 1,
	SOUND_FREQ_210HZ, 1,
	SOUND_FREQ_230HZ, 1,
	SOUND_FREQ_220HZ, 1,
	SOUND_FREQ_240HZ, 1,
	SOUND_FREQ_900HZ, 1,
	SOUND_FREQ_600HZ, 1,
	SOUND_FREQ_400HZ, 1,
	SOUND_FREQ_300HZ, 1,
	SOUND_FREQ_250HZ, 1,
	SOUND_FREQ_200HZ, 1,
	SOUND_FREQ_150HZ, 1,
	SOUND_FREQ_125HZ, 1,
	SOUND_FREQ_110HZ, 1,
	SOUND_FREQ_105HZ, 1,
	SOUND_FREQ_100HZ, 1,
	SOUND_FREQ_99HZ, 1,
	SOUND_FREQ_98HZ, 1,
	SOUND_FREQ_97HZ, 1,
	SOUND_FREQ_96HZ, 1,
	SOUND_FREQ_95HZ, 1,
	SOUND_FREQ_94HZ, 1,
	SOUND_TERMINATOR, 0
};

/* Game over music */
const uint16_t SOUND_GAME_OVER[] = {
	NOTE_B3, 2,
	NOTE_C4, 4,
	NOTE_D4, 2,
	NOTE_E4, 6,
	NOTE_G4, 7,
	NOTE_FS4, 5,
	NOTE_E4, 2,
	NOTE_D4, 4,
	NOTE_E4, 15,
	SOUND_TERMINATOR, 0
};

/* Stage edge transition sound */
const uint16_t SOUND_STAGE_EDGE_TRANSITION[] = {
	NOTE_C4, 3,
	NOTE_D4, 3,
	NOTE_F4, 6,
	NOTE_F4, 6,
	NOTE_G4, 3,
	NOTE_A4, 6,
	NOTE_G4, 6,
	SOUND_TERMINATOR, 0
};

/* "Too Bad" sound (failing to open door/use item) */
const uint16_t SOUND_TOO_BAD[] = {
	SOUND_FREQ_130HZ, 5,
	SOUND_FREQ_146HZ, 5,
	SOUND_FREQ_130HZ, 5,
	SOUND_FREQ_160HZ, 10,
	SOUND_TERMINATOR, 0
};

/* Fireball launch sound */
const uint16_t SOUND_FIRE[] = {
	0x2000, 1,  /* 145.652 Hz */
	0x1e00, 1,  /* 155.362 Hz */
	SOUND_TERMINATOR, 0
};

/* Hit enemy with fireball sound */
const uint16_t SOUND_HIT_ENEMY[] = {
	0x0800, 1,  /* 582.608 Hz */
	0x0400, 1,  /* 1165.217 Hz */
	SOUND_TERMINATOR, 0
};

/* Take damage sound */
const uint16_t SOUND_DAMAGE[] = {
	0x3000, 2,  /*  97.101 Hz */
	0x3800, 2,  /*  83.230 Hz */
	0x4000, 2,  /*  72.826 Hz */
	SOUND_TERMINATOR, 0
};

/* Collect item sound */
const uint16_t SOUND_COLLECT_ITEM[] = {
	0x0fda, 2,  /* 294.032 Hz */
	0x0c90, 2,  /* 371.014 Hz */
	0x0a91, 2,  /* 441.102 Hz */
	0x0800, 2,  /* 582.608 Hz */
	SOUND_TERMINATOR, 0
};

/* Extra life award sound */
const uint16_t SOUND_EXTRA_LIFE[] = {
	NOTE_B4, 5,
	NOTE_D5, 6,
	NOTE_B4, 2,
	NOTE_C5, 5,
	NOTE_D5, 5,
	SOUND_TERMINATOR, 0
};
