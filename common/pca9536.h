/* ==========================================================================
        PCA9536 - 4bits GPIO Expander for I2C bus
   ========================================================================== */

#ifndef _PCA9536_H_
#define _PCA9536_H_

#include <stdbool.h>
#include "../common/picoterm_i2c.h"

#define IO_MODE_IN 1
#define IO_MODE_OUT 0

#define IO_0 0
#define IO_1 1
#define IO_2 2
#define IO_3 3

bool has_pca9536( i2c_inst_t *i2c );

bool pca9536_setup_io( i2c_inst_t *i2c, uint8_t io, uint8_t io_mode ); // set gpio mode (input or output)
bool pca9536_output_io( i2c_inst_t *i2c, uint8_t io, bool value ); // set gpio outputstate
bool pca9536_output_reset( i2c_inst_t *i2c, uint8_t mask ); // reset state of several gpio @ once
bool pca9536_input_io( i2c_inst_t *i2c, uint8_t io ); // read state an input gpio

#endif
