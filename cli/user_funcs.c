#include <stdio.h>
#include <string.h>
#include "user_funcs.h"
#include "cli.h"
#include "pico/stdlib.h"
#include "tinyexpr.h"
#include "../common/picoterm_stdio.h"
#include "../common/picoterm_stddef.h"
#include "../common/picoterm_harddef.h"
#include "../common/picoterm_conio_config.h"
#include "../common/picoterm_debug.h"
#include "../pio_fatfs/ff.h"
#include "../pio_fatfs/diskio.h"


extern picoterm_conio_config_t conio_config;

usr_funcs user_functions[MAX_USER_FUNCTIONS];

void register_user_functions() {
	// Each user function you create, you must also
	// add to the init_user_functions routine.


  // This is my first user-defined function. I give
  // the command a name - in this case "blink_led".
  // You can supply a short help description. And,
  // then, I point the user_functions struct array
  // to the actual name of my user-defined function.
  strcpy(user_functions[0].command_name, "sd_info");
  strcpy(user_functions[0].command_help, "sdinfo\r\nGrab sdcard info");
  user_functions[0].user_function = cli_sd_info;

  // Here is a second user function.
  strcpy(user_functions[1].command_name, "dir");
  strcpy(user_functions[1].command_help, "dir [path] [-p]\r\nFile directory.");
  user_functions[1].user_function = cli_dir;

  //Here is a third user function.
  strcpy(user_functions[2].command_name, "calc");
  strcpy(user_functions[2].command_help, "calc math_expression");
  user_functions[2].user_function = calc;

	strcpy(user_functions[3].command_name, "type");
  strcpy(user_functions[3].command_help, "type filename [-p]\r\nShow file content.");
  user_functions[3].user_function = cli_type;

	strcpy(user_functions[4].command_name, "send_file");
  strcpy(user_functions[4].command_help, "send_file filename \r\nSend file content to UART.");
  user_functions[4].user_function = cli_send_file;

}

//--------------------------------------------------------------------+
//  calc function
//--------------------------------------------------------------------+

void calc(int token_count, char tokens[][MAX_STRING_SIZE]) {
	// This is using the TinyExprMath Expression Parser
	// found at: https://github.com/codeplea/tinyexpr
  char expr[80];
  char answer[10];

  strcpy(expr, tokens[1]);

  if (strlen(expr) <= 1) {
    print_string("Not a math_expression.\r\n");
    return;
  }

  sprintf(answer, "%f\r\n", te_interp(expr, 0)); /* Prints 25. */
  print_string(answer);

} // end calcware/uart.h"


//--------------------------------------------------------------------+
//  cli_sd_info
//--------------------------------------------------------------------+

void cli_sd_info(int token_count, char tokens[][MAX_STRING_SIZE]) {
	// Perform SD test with lot of debug messages
	FATFS fs;
	//FIL fil;
	FRESULT fr;     /* FatFs return code */
	//UINT br;
	//UINT bw;

	fr = f_mount(&fs, "", 1);
	if (fr != FR_OK) { // see FRESULT in ff.h
			sprintf( debug_msg, "SD mount error %d\r\n", fr);
			print_string( debug_msg );
			return;
	}
	print_string("SD mount ok\r\n");

	switch (fs.fs_type) {
			case FS_FAT12:
					print_string("FS type        : FAT12\r\n");
					break;
			case FS_FAT16:
					print_string("FS type        : FAT16\r\n");
					break;
			case FS_FAT32:
					print_string("FS type        : FAT32\r\n");
					break;
			case FS_EXFAT:
					print_string("FS type        : ExFAT\r\n");
					break;
			default:
					print_string("FS type        : unknown\r\n");
					break;
	}
	sprintf( debug_msg, "Card size      : %7.2f GB (GB = 1E9 bytes)\r\n", fs.csize * fs.n_fatent * 512E-9);
	print_string( debug_msg );
	// Print CID
	BYTE cid[16];
	disk_ioctl(0, MMC_GET_CID, cid);
	sprintf( debug_msg, "Manufacturer ID: %02x\r\n", (int) cid[0]);
	print_string( debug_msg );
	sprintf( debug_msg, "OEM ID         : %02x%02x\r\n", (int) cid[1], (int) cid[2]);
	print_string( debug_msg );
	sprintf( debug_msg, "Product        : %02x%02x%02x%02x%02x\r\n", cid[3], cid[4], cid[5], cid[6], cid[7]);
	print_string( debug_msg );
	sprintf( debug_msg, "Version        : %d.%d\r\n", (int) (cid[8] >> 4) & 0xf,  (int) cid[8] & 0xf);
	print_string( debug_msg );
	sprintf( debug_msg, "Serial number  : %02x%02x%02x%02x\r\n", (int) cid[9], (int) cid[10], (int) cid[11], (int) cid[12]);
	print_string( debug_msg );
	sprintf( debug_msg, "Manufact. date : %d/%d\r\n", (int) cid[14] & 0xf, ((int) cid[13] & 0xf)*16 + ((int) (cid[14] >> 2) & 0xf) + 2000);
	print_string( debug_msg );
}

//--------------------------------------------------------------------+
//  cli_dir
//--------------------------------------------------------------------+

void cli_dir(int token_count, char tokens[][MAX_STRING_SIZE]) {
  // uint8_t PIR_PIN = atoi(tokens[1]);
  // uint16_t iters = atoi(tokens[2]);
	// uint8_t LED_PIN = atoi(tokens[1]);
  // uint16_t iters = atoi(tokens[2]);
  // uint16_t millis = atoi(tokens[3]);
	DIR dir;
	char *path;
	FRESULT res;

	FATFS fs;
	FRESULT fr;     /* FatFs return code */
	bool paged = has_flag( "-p", tokens );

	// Mount SD_Card
	//
	fr = f_mount(&fs, "", 1);
	if (fr != FR_OK) { // see FRESULT in ff.h
			sprintf( debug_msg, "SD mount error %d\r\n", fr);
			print_string( debug_msg );
			return;
	}

	 // where you want to list
	 //
	if( token_count==1 ) // no parameter = root
		path = "";
	else
		path = tokens[1];

	// Open directory
	//
	res = f_opendir(&dir, path);
	if (res != FR_OK) {
			sprintf(debug_msg, "f_opendir error %d \r\n", res);
			print_string( debug_msg );
			return;
	}

	// List entries
	//
	int line_count = 0;
	char ch;
	while( true ) {
		FILINFO fno;

		res = f_readdir(&dir, &fno);
		if (res != FR_OK){
				sprintf(debug_msg, "f_readdir error %d\r\n", res);
				print_string( debug_msg );
				return;
		}

		if ((res != FR_OK) || (fno.fname[0] == 0))
			break;

		sprintf(debug_msg, "%c%c%c%c %10d %s%s%s%s\r\n",
			((fno.fattrib & AM_DIR) ? 'D' : '-'),
			((fno.fattrib & AM_RDO) ? 'R' : '-'),
			((fno.fattrib & AM_SYS) ? 'S' : '-'),
			((fno.fattrib & AM_HID) ? 'H' : '-'),
			(int)fno.fsize,
			path,
			(strlen(path)>0 ? "/" : "" ),
			fno.fname,
			((fno.fattrib & AM_DIR) ? "/" : "") );
		print_string( debug_msg );

		line_count++;
		if( paged && (line_count%(VISIBLEROWS-2))==0 ) { // -1 is enough; -2 still show the DIR command (more secure)
			print_string( "Press key...");
			ch = get_key( false ); // allow non ASCII
			if( ch==ESC ) {
				print_string("\r\nUser abort!\r\n");
				return;
			}
			else
				print_string("\r\n"); // prepare for next line to display
		} // eof paged
	} // eof While
}

//--------------------------------------------------------------------+
//  cli_type
//--------------------------------------------------------------------+
void cli_type( int token_count, char tokens[][MAX_STRING_SIZE]){
		char *filename;
		char ch;
		//FRESULT res;
		FIL file;
		FATFS fs;
		FRESULT fr;     /* FatFs return code */
		uint16_t size;

		bool paged = has_flag( "-p", tokens );
		UINT bytesRead;

		static bool _cr_lf_issued = false;

		void type_char( char c ){
			// Type a char on screen by taking care about \r or \n or \r\n
			// return true when NEW_LINE is issued
			if((c==13) || (c==10))
				_cr_lf_issued = true;
			else {
				if( _cr_lf_issued ) {
					print_string( "\r\n");
					print_char( c );
					_cr_lf_issued = false;
				}
				else
					print_char( c );
			}
		}


		if( token_count<1 ){
			print_string("Missing filename!\r\n" );
			return;
		}
		// file to open
		filename = tokens[1];


		// Mount SD_Card
		//
		fr = f_mount(&fs, "", 1);
		if (fr != FR_OK) { // see FRESULT in ff.h
				sprintf( debug_msg, "SD mount error %d\r\n", fr);
				print_string( debug_msg );
				return;
		}

		// Read the file
		//
		fr = f_open(&file, filename, FA_READ);
		if (fr != FR_OK) { // see FRESULT in ff.h
				sprintf( debug_msg, "File open error %d\r\n", fr);
				print_string( debug_msg );
				return;
		}

		// file size
		size = f_size(&file);

		// using debug_msg buffer to read FIRST CHUNCK of file content
		fr = f_read(&file, debug_msg, sizeof(debug_msg), &bytesRead);
		if (fr != FR_OK) { // see FRESULT in ff.h
				sprintf( debug_msg, "File read error %d\r\n", fr);
				print_string( debug_msg );
				f_close(&file);
				return;
		}

		int line_count = 0;
		while( bytesRead > 0 ){
			// print_out the file CHUNCK
			for( int i=0; i<bytesRead; i++) {
				// type char takes care of /r or /n or /r/n
				type_char( debug_msg[i] );
				// if cursor @ position after a print_char --> we had a line return
				if( conio_config.cursor.pos.x==1 )
					line_count++;

				//char ln[50];
				//sprintf( ln, "[%d]=%c , x=%d", i, debug_msg[i], conio_config.cursor.pos.x );
				//debug_print( ln );

				if( paged && (line_count>VISIBLEROWS-3) && (line_count%VISIBLEROWS-2)==0 ) {
						// hide the initial char and WAIT for key press
						print_string( "\rPress key...");
						ch = get_key( false ); // allow non ASCII input
						if( ch==ESC ) {
							print_string("\r\nUser abort!\r\n");
							return;
						}
						else {
							print_string("\r\n" );
							print_char( debug_msg[i] ); // reprint the initial char.
							line_count++;
						}
				} // eof paged
			} // for each char

			// read next CHUNCK
			fr = f_read(&file, debug_msg, sizeof(debug_msg), &bytesRead);
			if (fr != FR_OK) { // see FRESULT in ff.h
					sprintf( debug_msg, "File read error %d\r\n", fr);
					print_string( debug_msg );
					f_close(&file);
					return;
			}
		}
		f_close(&file);
}

//--------------------------------------------------------------------+
//  cli_send_file
//--------------------------------------------------------------------+

void cli_send_file( int token_count, char tokens[][MAX_STRING_SIZE]){
	char *filename;
	char ch;
	FIL file;
	FATFS fs;
	FRESULT fr;     /* FatFs return code */
	uint16_t size, readed;

	UINT bytesRead;

	if( token_count<1 ){
		print_string("Missing filename!\r\n" );
		return;
	}
	// file to open
	filename = tokens[1];

	// Mount SD_Card
	//
	fr = f_mount(&fs, "", 1);
	if (fr != FR_OK) { // see FRESULT in ff.h
			sprintf( debug_msg, "SD mount error %d\r\n", fr);
			print_string( debug_msg );
			return;
	}

	// Read the file
	//
	fr = f_open(&file, filename, FA_READ);
	if (fr != FR_OK) { // see FRESULT in ff.h
			sprintf( debug_msg, "File open error %d\r\n", fr);
			print_string( debug_msg );
			return;
	}

	// file size
	size = f_size(&file);

	// using debug_msg buffer to read FIRST CHUNCK of file content
	fr = f_read(&file, debug_msg, sizeof(debug_msg), &bytesRead);
	if (fr != FR_OK) { // see FRESULT in ff.h
			sprintf( debug_msg, "File read error %d\r\n", fr);
			print_string( debug_msg );
			f_close(&file);
			return;
	}

	readed = 0;
	char line[40];
	print_string("\r\nSending...");
	while( bytesRead > 0 ){
		// send file CHUNCK over serial
		for( int i=0; i<bytesRead; i++) {
				uart_putc( UART_ID, debug_msg[i] );
				sleep_ms( 1 );
    }
		// Chunck sent!
		readed = readed + bytesRead;
		sprintf( line, "\r%d / %d sent!", readed, size );
		print_string( line );
		// read next CHUNCK
		fr = f_read(&file, debug_msg, sizeof(debug_msg), &bytesRead);
		if (fr != FR_OK) { // see FRESULT in ff.h
				sprintf( debug_msg, "\r\nFile read error %d\r\n", fr);
				print_string( debug_msg );
				f_close(&file);
				return;
		}
	}
	sprintf( line, "\r%d / %d sent!\r\n", size, size );

	f_close(&file);
}
