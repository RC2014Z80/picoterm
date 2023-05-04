/* ==========================================================================
    Manage the basic operations around the SD card over PIO_SPI
   ========================================================================== */

#include <stdbool.h>
#include "pio_spi.h"
#include "pio_sd.h"
#include "pico/stdlib.h"
#include "stdio.h"
#include "picoterm_harddef.h" // Hardware definition
#include "../pio_fatfs/ff.h"

#include "picoterm_debug.h"

pio_spi_inst_t spi_sd = {
				.pio = pio1, // pio0,
				.sm = 1,     // 0,
				.cs_pin = SPI_SD_CSN_PIN
};

bool _mounted = false;


void spi_sd_init(){
	 	// initialize the PIO SPI bus attached to the SD card (see include pio_spi.h)
	 	// We must also activates the pull-up on the RX, TX pin
	 	// see example from https://github.com/raspberrypi/pico-examples/blob/master/pio/spi/spi_flash.c
	 	gpio_init( SPI_SD_CSN_PIN );
	 	gpio_put( SPI_SD_CSN_PIN, 1);
	 	gpio_set_dir( SPI_SD_CSN_PIN, GPIO_OUT);
	 	gpio_init( SPI_SD_RX_PIN );
	 	gpio_pull_up( SPI_SD_RX_PIN );
	 	gpio_init( SPI_SD_TX_PIN );
	 	gpio_pull_up( SPI_SD_TX_PIN );
	 	uint offset = pio_add_program(spi_sd.pio, &spi_cpha0_program);
	 	// printf("spi_sd program loaded at %d\n", offset);
	 	pio_spi_init(spi_sd.pio, spi_sd.sm, offset,
	 							 8,       // 8 bits per SPI frame
	 							 31.25f, // 31.25f,  // 31.25 for 1 MHz @ 125 clk_sys
	 							 false,   // CPHA = 0
	 							 false,   // CPOL = 0
	 							 SPI_SD_SCK_PIN,
	 							 SPI_SD_TX_PIN,
	 							 SPI_SD_RX_PIN
	 	);
	 	sleep_ms( 200 );
		_mounted = false;
}


bool sd_mount( void ){
		// Perform SD test with lot of debug messages
		FATFS fs;
		FRESULT fr;     /* FatFs return code */

		debug_print("pio_sd: mount()");
		_mounted = false;

		fr = f_mount(&fs, "", 1);
	  if (fr != FR_OK) { // see FRESULT in ff.h
	        sprintf( debug_msg, "pio_sd: mount error %d", fr);
					debug_print( debug_msg );
	        return false;
	  }
		_mounted = true;
	  debug_print("pio_sd: mount ok");

		switch (fs.fs_type) {
				case FS_FAT12:
						debug_print("Type is FAT12");
						break;
				case FS_FAT16:
						debug_print("Type is FAT16");
						break;
				case FS_FAT32:
						debug_print("Type is FAT32");
						break;
				case FS_EXFAT:
						debug_print("Type is EXFAT");
						break;
				default:
						debug_print("Type is unknown");
						_mounted = false;
						break;
		}
		//sprintf( debug_msg, "Card size: %7.2f GB (GB = 1E9 bytes)\n\n", fs.csize * fs.n_fatent * 512E-9);
		//debug_print( debug_msg );
		return _mounted;
}

void sd_unmount(){
	// Just reset the mount flag!
	_mounted = false;
}

bool is_sd_mount(){
	// did the last SPI_sd_mount succeed ?
	// If not... just try to mount it now
	if( !_mounted ){
		_mounted = sd_mount();
	}
	return _mounted;
}

bool send_file_to_uart( char *filename ){
	// send content of the named file to uart (to host). Kindly useful for
	// shortcut key management. If the host does echo then you will see the content.
	//
	// Check debugging messages in case of trouble.
	FIL file;
	FATFS fs;
	FRESULT fr;     /* FatFs return code */
	UINT bytesRead;

	// send content of file to the host uart (used for shortcut key)
	sprintf( debug_msg, "send_file_to_uart: %s", filename );
	debug_print( debug_msg );
	//if( !is_sd_mount() ) // will attempt to remount
	//	return false;

	// We must mount the file system to make it working.
	fr = f_mount(&fs, "", 1);
	if (fr != FR_OK) { // see FRESULT in ff.h
			sprintf( debug_msg, "SD mount error %d", fr);
			debug_print( debug_msg );
			return false;
	}

	// try to open the file in read mode
	fr = f_open(&file, filename, FA_READ);
	if (fr != FR_OK) { // see FRESULT in ff.h
			sprintf( debug_msg, "File open error %d", fr);
			debug_print( debug_msg );
			return false;
	}

	// using debug_msg buffer to read FIRST CHUNCK of file content
	fr = f_read(&file, debug_msg, sizeof(debug_msg), &bytesRead);
	if (fr != FR_OK) { // see FRESULT in ff.h
			sprintf( debug_msg, "File read error %d", fr);
			debug_print( debug_msg );
			f_close(&file);
			return false;
	}

	while( bytesRead > 0 ){
			// send file CHUNCK over serial
			for( int i=0; i<bytesRead; i++) {
					uart_putc( UART_ID, debug_msg[i] );
					sleep_ms( 1 );
	    }
			// Chunck sent!
			// read next CHUNCK
			fr = f_read(&file, debug_msg, sizeof(debug_msg), &bytesRead);
			if (fr != FR_OK) { // see FRESULT in ff.h
					sprintf( debug_msg, "\r\nFile read error %d", fr);
					debug_print( debug_msg );
					f_close(&file);
					return false;
			}
	}

	f_close(&file);
	return true;
}
