/* ==========================================================================
    Manage the basic operations around the SD card over PIO_SPI
   ========================================================================== */

#ifndef _PIO_SD_H
#define _PIO_SD_H


void spi_sd_init();   // Initialise the SPI interface
bool sd_mount();         // mount the SD card
void sd_unmount(); 	// reset the mount flag!
bool is_sd_mount(); 	// did the last SPI_sd_mount succeed ?

bool send_file_to_uart( char *filename );

#endif
