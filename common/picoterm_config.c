#include "picoterm_config.h"
#include "picoterm_debug.h"
#include "string.h"
#include <stdio.h>

// once written, we can access our data at flash_target_contents
const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);

void debug_print_config( struct PicotermConfig *c );

void load_config(){
  // try to load the config from the Flash memory otherwise initialize a
  // default config.
  if( is_config_in_flash() ){
    debug_print("load_config() -> read config from Flash" );
    read_config_from_flash( &config );
		debug_print_config( &config );
  }
  else {
    debug_print("load_config() -> Set default Config" );
    set_default_config( &config );
		debug_print_config( &config );
	}
}

void save_config(){
	debug_print("save_config() -> write config to Flash" );
	debug_print_config( &config );
	write_config_to_flash( &config );
}

void set_default_config( struct PicotermConfig *c ){
	//c.magic_key = MAGIC_KEY ,
	strncpy( c->magic_key, MAGIC_KEY, sizeof(MAGIC_KEY));
	c->version = CONFIG_VERSION;
	c->colour_preference = WHITE;
}

bool is_config_in_flash(){
	/* Check if the magic key is present in flash */
	struct PicotermConfig _c;
	read_config_from_flash( &_c );
	return strncmp( _c.magic_key, MAGIC_KEY, sizeof(MAGIC_KEY) )==0;
}

void read_config_from_flash( struct PicotermConfig *c ){
    memcpy( c, flash_target_contents, sizeof(struct PicotermConfig) );
}

void debug_print_config( struct PicotermConfig *c ){
	debug_print( "debug_print_config():");
  sprintf( debug_msg, "  magic_key=%s", c->magic_key );
	debug_print( debug_msg );
	sprintf( debug_msg, "  version=%u", c->version );
	debug_print( debug_msg );
	sprintf( debug_msg, "  colour_preference=%u", c->colour_preference );
  debug_print( debug_msg );
}

void write_config_to_flash( struct PicotermConfig *c ){
   /* unsafe if you have two cores concurrently executing from flash
     https://raspberrypi.github.io/pico-sdk-doxygen/group__hardware__flash.html */
    debug_print("write_config_to_flash()");
    debug_print_config( c );
    uint8_t data_to_write[FLASH_PAGE_SIZE];
    memcpy( data_to_write, c, sizeof(struct PicotermConfig) );

    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, data_to_write, FLASH_PAGE_SIZE);

}