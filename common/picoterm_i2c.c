/* ==========================================================================
        Picoterm I2C bus helper for GP26 & GP27
   ==========================================================================
check the Shawn Hymel tutorial published at DigiKey.
https://www.digikey.be/en/maker/projects/raspberry-pi-pico-rp2040-i2c-example-with-micropython-and-cc/47d0c922b79342779cdbd4b37b7eb7e2
*/

#include "picoterm_harddef.h" // Hardware definition
#include "picoterm_i2c.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"


// Global flag informing about the usage of gp26 & gp27 on the Picoterm expansion
// When true, the GPIO are used as I2C, otherwise they are used as simple GPIO.
bool i2c_bus_available;

i2c_inst_t *i2c_bus = i2c1;

void init_i2c_bus(){
	//Initialize I2C port at 400 kHz
	i2c_init(i2c_bus, 400 * 1000);

	// Initialize I2C pins
	gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
	gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);

}

void deinit_i2c_bus(){
	// de-Initialize I2C pins
	gpio_set_function(SDA_PIN, GPIO_FUNC_NULL);
	gpio_set_function(SCL_PIN, GPIO_FUNC_NULL);

	// de-initialize I2C hardware block
	i2c_deinit( i2c_bus );
}

// Write 1 byte to the specified register
int reg_write(  i2c_inst_t *i2c,
                const uint addr,
                const uint8_t reg,
                uint8_t *buf,
                const uint8_t nbytes) {

    int num_bytes_write = 0;
    uint8_t msg[nbytes + 1];

    // Check to make sure caller is sending 1 or more bytes
    if (nbytes < 1) {
        return 0;
    }

    // Append register address to front of data packet
    msg[0] = reg;
    for (int i = 0; i < nbytes; i++) {
        msg[i + 1] = buf[i];
    }

    // Write data to register(s) over I2C
    num_bytes_write = i2c_write_blocking(i2c, addr, msg, (nbytes + 1), false);

    return num_bytes_write;
}

// Read byte(s) from specified register. If nbytes > 1, read from consecutive
// registers. Returns the number for bytes readed.
int reg_read(  i2c_inst_t *i2c,
                const uint addr,
                const uint8_t reg,
                uint8_t *buf,
                const uint8_t nbytes) {

    int num_bytes_read = 0;

    // Check to make sure caller is asking for 1 or more bytes
    if (nbytes < 1) {
        return 0;
    }

    // Read data from register(s) over I2C
    i2c_write_blocking(i2c, addr, &reg, 1, true);
    num_bytes_read = i2c_read_blocking(i2c, addr, buf, nbytes, false);

    return num_bytes_read;
}

// Read byte(s) from specified register. If nbytes > 1, read from consecutive
// registers. Returns the number for bytes readed.
int reg_read_timeout(  i2c_inst_t *i2c,
                const uint addr,
                const uint8_t reg,
                uint8_t *buf,
                const uint8_t nbytes,
							  const uint timeout_us ) {

    int num_bytes_read = 0;

    // Check to make sure caller is asking for 1 or more bytes
    if (nbytes < 1) {
        return 0;
    }

    // Read data from register(s) over I2C
    i2c_write_timeout_us(i2c, addr, &reg, 1, true, timeout_us ); // 20ms = 20 000 us
    num_bytes_read = i2c_read_timeout_us(i2c, addr, buf, nbytes, false, timeout_us);

    return num_bytes_read;
}
