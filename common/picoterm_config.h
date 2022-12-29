#ifndef _PICOTERM_CONFIG_H_
#define _PICOTERM_CONFIG_H_


//#include <stdio.h>

// #include <stdlib.h>
// #include "pico.h"
// #include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/uart.h"

#define FLASH_TARGET_OFFSET (256 * 1024)  // from start of flash
#define MAGIC_KEY "PTCFG\0"
#define CONFIG_VERSION 4

#define WHITE 0
#define LIGHTAMBER 1
#define DARKAMBER 2
#define GREEN1 3
#define GREEN2 4
#define GREEN3 5
#define PURPLE 6

/* The following structure is saved as IT into flash. So only append entries
   at the end of structure and do not modifies OLD ones. Reflashing does not
   always erase the Flash content */
typedef struct PicotermConfig {
  char magic_key[6];
  uint8_t version;
  uint8_t colour_preference;
  // version 2
  uint32_t baudrate;
  uint8_t databits;
  uint8_t parity;
  uint8_t stopbits; // 1, 2
	// version 3
	//    STARTUP FONT_ID. Was initially named `nupetscii` (0=ANSI, 1=NUPETSCII) before version 4.
	//    Since version 4, it has been renamed font_id and contains the font_id to
	//      use at startup (0 ASCII 7bit, 1 ANSI Nupetscii, 2 ANSI CP437....)
	//    During execution, font_id contains the font currently used for rendering
	//      the char on the screen.
	uint8_t font_id;
	// version 4
	uint8_t graph_id;  // ANSI Font_ID to use when switching to graphical ANSI font.
} picoterm_config_t; // Issue #13, conversion to typedef required, awesome contribution of Spock64

void load_config(); // try to load config otherwise init with defaults
void save_config(); // save to configuration to flash

bool is_config_in_flash();
void upgrade_config( struct PicotermConfig *c );
void set_default_config( struct PicotermConfig *c );
void read_config_from_flash( struct PicotermConfig *c );
void write_config_to_flash( struct PicotermConfig *c );

#endif
