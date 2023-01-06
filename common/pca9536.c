/* ==========================================================================
        PCA9536 - 4bits GPIO Expander for I2C bus
   ========================================================================== */

#include <stdbool.h>
#include "pca9536.h"
#include "hardware/i2c.h"
#include "../common/picoterm_i2c.h" // reg_write & reg_read

#include <stdio.h>
#include "../common/picoterm_debug.h"

#define PCA9536_ADDR 0x41
#define REG_INPUT    0  // default
#define REG_OUTPUT   1
#define REG_POLARITY 2
#define REG_CONFIG   3

bool has_pca9536( i2c_inst_t *i2c ){
	// try to read configuration of the PC9536 on the I2C bus. Check for immuable
	// configuration bits
	uint8_t data[6];
	int nb_bytes = reg_read_timeout( i2c, PCA9536_ADDR, REG_INPUT, data, 4, 20000 ); // 20mS
	if(nb_bytes <= 0){
		debug_print( "Nothing read from the bus!" );
		return false;
	}
	for( int i=0; i<=3; i++) {
		sprintf( debug_msg, "Reg %i = %i", i, data[i] );
		debug_print( debug_msg );
	}
	return true;
}

bool pca9536_setup_io( i2c_inst_t *i2c, uint8_t io, uint8_t io_mode ){
	if( io>IO_3 )
		return false;

	uint8_t data[2];
	reg_read( i2c, PCA9536_ADDR, REG_CONFIG, data, 1 );

	uint8_t iodir = data[0];
	if( io_mode == IO_MODE_IN )
		iodir |= (1 << io); // Set the bit
	else
		if( io_mode == IO_MODE_OUT )
			iodir &= ~(1 << io); // Reset the bit
		else
			return false;

	// Write the config.
	data[0] = iodir;
	reg_write( i2c, PCA9536_ADDR, REG_CONFIG, data, 1 );
	return true;
}

bool pca9536_output_io( i2c_inst_t *i2c, uint8_t io, bool value ){
	if( io>IO_3 )
		return false;

	uint8_t data[2];
	reg_read( i2c, PCA9536_ADDR, REG_OUTPUT, data, 1 );

	uint8_t gpio_state = data[0];
	if( value )
		gpio_state |= (1 << io); // Set the bit
	else
		gpio_state &= ~(1 << io); // Clear the bit

	data[0] = gpio_state;
	reg_write( i2c, PCA9536_ADDR, REG_OUTPUT, data, 1 );
	return true;
}

bool pca9536_output_reset( i2c_inst_t *i2c, uint8_t mask ){
	/* 4 lower bits of mask indicates which of the output pins should be resetted */
	uint8_t data[2];
	reg_read( i2c, PCA9536_ADDR, REG_OUTPUT, data, 1 );

	uint8_t gpio_state = data[0];
	for( int idx=0; idx<=3; idx++)
		if (mask & (1<<idx)) { // should we reset the idx'th bit
			gpio_state &= ~(1 << idx); // Clear the bit
		}

	data[0] = gpio_state;
	reg_write( i2c, PCA9536_ADDR, REG_OUTPUT, data, 1 );
	return true;
}

bool pca9536_input_io( i2c_inst_t *i2c, uint8_t io ) {
	// read state an input gpio
	if( io>IO_3 )
		return false;

	uint8_t data[2];
	reg_read( i2c, PCA9536_ADDR, REG_INPUT, data, 1 );

	return (data[0] & (1<<io)) ? true : false ;
}
