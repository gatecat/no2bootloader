/*
 * spi.c
 *
 * Copyright (C) 2019-2020 Sylvain Munaut
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "spi.h"


struct spi {
	uint32_t config;
	uint32_t divider;
	uint32_t tx_data;
	uint32_t rx_data;
	uint32_t status;

} __attribute__((packed,aligned(4)));


static volatile struct spi * const spi_regs = (void*)(SPI_BASE);


void
spi_init(void)
{
	spi_regs->config = 2;
	spi_regs->divider = 2;

}

void
spi_xfer(unsigned cs, struct spi_xfer_chunk *xfer, unsigned n)
{
	uint8_t rxd;

	/* Setup CS */
	spi_regs->config = 2 | 4;

	/* Run the chunks */
	while (n--) {
		for (int i=0; i<xfer->len; i++)
		{
			spi_regs->tx_data = xfer->write ? xfer->data[i] : 0x00;
			while (spi_regs->status & 1);
			if (xfer->read)
				xfer->data[i] = spi_regs->rx_data;
		}
		xfer++;
	}

	/* Clear CS */
	spi_regs->config = 2;
}


#define FLASH_CMD_DEEP_POWER_DOWN	0xb9
#define FLASH_CMD_WAKE_UP		0xab
#define FLASH_CMD_WRITE_ENABLE		0x06
#define FLASH_CMD_WRITE_ENABLE_VOLATILE	0x50
#define FLASH_CMD_WRITE_DISABLE		0x04

#define FLASH_CMD_READ_MANUF_ID		0x9f
#define FLASH_CMD_READ_UNIQUE_ID	0x4b

#define FLASH_CMD_READ_SR1		0x05
#define FLASH_CMD_READ_SR2		0x35
#define FLASH_CMD_READ_SR3		0x15
#define FLASH_CMD_WRITE_SR1		0x01
#define FLASH_CMD_WRITE_SR2		0x31
#define FLASH_CMD_WRITE_SR3		0x11

#define FLASH_CMD_READ_DATA		0x03
#define FLASH_CMD_PAGE_PROGRAM		0x02
#define FLASH_CMD_CHIP_ERASE		0x60
#define FLASH_CMD_SECTOR_ERASE		0x20
#define FLASH_CMD_BLOCK_ERASE_32k	0x52
#define FLASH_CMD_BLOCK_ERASE_64k	0xd8

void
flash_cmd(uint8_t cmd)
{
	struct spi_xfer_chunk xfer[1] = {
		{ .data = (void*)&cmd, .len = 1, .read = false, .write = true,  },
	};
	spi_xfer(SPI_CS_FLASH, xfer, 1);
}

void
flash_deep_power_down(void)
{
	flash_cmd(FLASH_CMD_DEEP_POWER_DOWN);
}

void
flash_wake_up(void)
{
	flash_cmd(FLASH_CMD_WAKE_UP);
}

void
flash_write_enable(void)
{
	flash_cmd(FLASH_CMD_WRITE_ENABLE);
}

void
flash_write_enable_volatile(void)
{
	flash_cmd(FLASH_CMD_WRITE_ENABLE_VOLATILE);
}

void
flash_write_disable(void)
{
	flash_cmd(FLASH_CMD_WRITE_DISABLE);
}

void
flash_manuf_id(void *manuf)
{
	uint8_t cmd = FLASH_CMD_READ_MANUF_ID;
	struct spi_xfer_chunk xfer[2] = {
		{ .data = (void*)&cmd,  .len = 1, .read = false, .write = true,  },
		{ .data = (void*)manuf, .len = 3, .read = true,  .write = false, },
	};
	spi_xfer(SPI_CS_FLASH, xfer, 2);
}

void
flash_unique_id(void *id)
{
	uint8_t cmd = FLASH_CMD_READ_UNIQUE_ID;
	struct spi_xfer_chunk xfer[3] = {
		{ .data = (void*)&cmd, .len = 1, .read = false, .write = true,  },
		{ .data = (void*)0,    .len = 4, .read = false, .write = false, },
		{ .data = (void*)id,   .len = 8, .read = true,  .write = false, },
	};
	spi_xfer(SPI_CS_FLASH, xfer, 3);
}

uint8_t
flash_read_sr(int srno)
{
	uint8_t cmd;
	uint8_t rv;
	struct spi_xfer_chunk xfer[2] = {
		{ .data = (void*)&cmd, .len = 1, .read = false, .write = true,  },
		{ .data = (void*)&rv,  .len = 1, .read = true,  .write = false, },
	};
	switch (srno) {
	case 1:  cmd = FLASH_CMD_READ_SR1; break;
	case 2:  cmd = FLASH_CMD_READ_SR2; break;
	case 3:  cmd = FLASH_CMD_READ_SR3; break;
	default: return 0;
	}
	spi_xfer(SPI_CS_FLASH, xfer, 2);
	return rv;
}

void
flash_write_sr(int srno, uint8_t srval)
{
	uint8_t cmd[2];
	struct spi_xfer_chunk xfer[1] = {
		{ .data = (void*)cmd, .len = 2, .read = false, .write = true,  },
	};
	switch (srno) {
	case 1:  cmd[0] = FLASH_CMD_WRITE_SR1; break;
	case 2:  cmd[0] = FLASH_CMD_WRITE_SR2; break;
	case 3:  cmd[0] = FLASH_CMD_WRITE_SR3; break;
	default: return;
	}
	cmd[1] = srval;
	spi_xfer(SPI_CS_FLASH, xfer, 1);
}

void
flash_read(void *dst, uint32_t addr, unsigned len)
{
	uint8_t cmd[4] = { FLASH_CMD_READ_DATA, ((addr >> 16) & 0xff), ((addr >> 8) & 0xff), (addr & 0xff)  };
	struct spi_xfer_chunk xfer[2] = {
		{ .data = (void*)cmd, .len = 4,   .read = false, .write = true,  },
		{ .data = (void*)dst, .len = len, .read = true,  .write = false, },
	};
	spi_xfer(SPI_CS_FLASH, xfer, 2);
}

void
flash_page_program(const void *src, uint32_t addr, unsigned len)
{
	uint8_t cmd[4] = { FLASH_CMD_PAGE_PROGRAM, ((addr >> 16) & 0xff), ((addr >> 8) & 0xff), (addr & 0xff)  };
	struct spi_xfer_chunk xfer[2] = {
		{ .data = (void*)cmd, .len = 4,   .read = false, .write = true, },
		{ .data = (void*)src, .len = len, .read = false, .write = true, },
	};
	spi_xfer(SPI_CS_FLASH, xfer, 2);
}

static void
_flash_erase(uint8_t cmd_byte, uint32_t addr)
{
	uint8_t cmd[4] = { cmd_byte, ((addr >> 16) & 0xff), ((addr >> 8) & 0xff), (addr & 0xff)  };
	struct spi_xfer_chunk xfer[1] = {
		{ .data = (void*)cmd, .len = 4,   .read = false, .write = true,  },
	};
	spi_xfer(SPI_CS_FLASH, xfer, 1);
}

void
flash_sector_erase(uint32_t addr)
{
	_flash_erase(FLASH_CMD_SECTOR_ERASE, addr);
}

void
flash_block_erase_32k(uint32_t addr)
{
	_flash_erase(FLASH_CMD_BLOCK_ERASE_32k, addr);
}

void
flash_block_erase_64k(uint32_t addr)
{
	_flash_erase(FLASH_CMD_BLOCK_ERASE_64k, addr);
}
