#ifndef _PICOTERM_CONFIG_H_
#define _PICOTERM_CONFIG_H_


//#include <stdio.h>

// #include <stdlib.h>
// #include "pico.h"
// #include "pico/stdlib.h"
#include "hardware/flash.h"

#define FLASH_TARGET_OFFSET (256 * 1024)  // from start of flash
#define MAGIC_KEY "PTCFG\0"
#define CONFIG_VERSION 1

#define WHITE 0
#define LIGHTAMBER 1
#define DARKAMBER 2
#define GREEN1 3
#define GREEN2 4
#define GREEN3 5

//uint8_t colour_preference;

/* The following structure is saved as IT into flash. So only append entries
   at the end of structure and do not modifies OLD ones. Reflashing does not
   always erase the Flash content */
struct PicotermConfig {
	char magic_key[6];
	uint8_t version;
  uint8_t colour_preference;
} config;

void load_config(); // try to load config otherwise init with defaults
void save_config(); // save to configuration to flash

bool is_config_in_flash();
void set_default_config( struct PicotermConfig *c );
void read_config_from_flash( struct PicotermConfig *c );
void write_config_to_flash( struct PicotermConfig *c );

/* void write_data_to_flash(); */
/* void read_data_from_flash(); */

#endif
