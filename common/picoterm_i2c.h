/* ==========================================================================
        Picoterm I2C bus helper for GP26 & GP27
   ========================================================================== */

#ifndef _PICOTERM_I2C_H_
#define _PICOTERM_I2C_H_

#include "hardware/i2c.h"

void init_i2c_bus();   // Initialze the PicoTerm i2c bus.
void deinit_i2c_bus();

int reg_write(i2c_inst_t *i2c,
                const uint addr,
                const uint8_t reg,
                uint8_t *buf,
                const uint8_t nbytes);

int reg_read(   i2c_inst_t *i2c,
                const uint addr,
                const uint8_t reg,
                uint8_t *buf,
                const uint8_t nbytes);

int reg_read_timeout(  i2c_inst_t *i2c,
                const uint addr,
                const uint8_t reg,
                uint8_t *buf,
                const uint8_t nbytes,
							  const uint timeout_us );

#endif
