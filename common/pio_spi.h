/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
 /* copied from   pico-examples/pio/spi/spi_flash.c    */

#ifndef _PIO_SPI_H
#define _PIO_SPI_H

#include "hardware/pio.h"
#include "spi.pio.h"

typedef struct pio_spi_inst {
    PIO pio;
    uint sm;
    uint cs_pin;
} pio_spi_inst_t;

void pio_spi_write8_blocking(const pio_spi_inst_t *spi, const uint8_t *src, size_t len);

void pio_spi_read8_blocking(const pio_spi_inst_t *spi, uint8_t *dst, size_t len);

void pio_spi_write8_read8_blocking(const pio_spi_inst_t *spi, uint8_t *src, uint8_t *dst, size_t len);

/* Added by Domeu for SD Card over SPI */
void pio_spi_read8_blocking_with(const pio_spi_inst_t *spi, char tx_val, uint8_t *dst, size_t len);

#endif
