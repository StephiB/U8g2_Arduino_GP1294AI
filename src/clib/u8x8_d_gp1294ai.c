/*

u8x8_d_gp1294ai.c

Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

Copyright (c) 2016, olikraus@gmail.com
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or other
materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include "u8x8.h"

#define SWAP8(a) ((((a)&0x80) >> 7) | (((a)&0x40) >> 5) | (((a)&0x20) >> 3) | (((a)&0x10) >> 1) | (((a)&0x08) << 1) | (((a)&0x04) << 3) | (((a)&0x02) << 5) | (((a)&0x01) << 7))

/* ========== GP1287AI ========== */
static const u8x8_display_info_t u8x8_gp1294ai_display_info = {
	/* chip_enable_level = */ 0,
	/* chip_disable_level = */ 1,

	/* post_chip_enable_wait_ns = */ 240, /* tCS-CLK */
	/* pre_chip_disable_wait_ns = */ 120, /* tCLK-CS */
	/* reset_pulse_width_ms = */ 1,       /* Trw     */
	/* post_reset_wait_ms = */ 1,         /* Trth    */
	/* sda_setup_time_ns = */ 60,         /* tsu     */
	/* sck_pulse_width_ns = */ 120,       /* tcyc/2  */
	/* sck_clock_hz = */ 4000000UL,       /* MAX 4.16 MHz */
	/* spi_mode = */ 3,                   /* active low, falling edge, LSBFIRST */
	/* i2c_bus_clock_100kHz = */ 4,       /*         */
	/* data_setup_time_ns = */ 60,        /* tsu     */
	/* write_pulse_width_ns = */ 120,     /* tcyc/2  */
	/* tile_width = */ 32,                /* 32*8=256 GRAM size */
	/* tile_hight = */ 6,                 /* 6*8=48   GRAM size */
	/* default_x_offset = */ 0,           /*         */
	/* flipmode_x_offset = */ 0,          /*         */
	/* pixel_width = */ 256,              /* Pattern pixel */
	/* pixel_height = */ 48               /* Pattern pixel */
};
static const uint8_t u8x8_d_gp1294ai_init_seq[] = {
	U8X8_START_TRANSFER(),
	U8X8_C(SWAP8(0x0AA)), /* Software reset */
	U8X8_END_TRANSFER(),
	U8X8_DLY(1), /* Wait for reset */

	U8X8_START_TRANSFER(),
	U8X8_CA(SWAP8(0x078), SWAP8(0x008)), /* Oscillation Setting */
	U8X8_END_TRANSFER(),
	
	U8X8_START_TRANSFER(),
	U8X8_CAAA(SWAP8(0x0CC), SWAP8(0x001), SWAP8(0x01F), SWAP8(0x000)), /* VFD Controller Init */
	U8X8_A4(SWAP8(0x0FF), SWAP8(0x02F), SWAP8(0x000), SWAP8(0x020)),   /* VFD Controller Init */
	U8X8_END_TRANSFER(),

	U8X8_START_TRANSFER(),
	U8X8_CAA(SWAP8(0x0A0), SWAP8(0x000), SWAP8(0x002)), /* Dimming level Setting  */
	U8X8_END_TRANSFER(),

	U8X8_START_TRANSFER(),
	U8X8_CAA(SWAP8(0x0C0), SWAP8(0x000), SWAP8(0x000)), /* Pattern area position setting */
	U8X8_END_TRANSFER(),

	U8X8_START_TRANSFER(),
	U8X8_CA(SWAP8(0x008), SWAP8(0x000)), /* T1 INT Setting */
	U8X8_END_TRANSFER(),

	U8X8_START_TRANSFER(),
	U8X8_CA(SWAP8(0x080), SWAP8(0x000)), /* Display Mode Setting */
	U8X8_END_TRANSFER(),

	U8X8_END() /* end of sequence */
};

static void u8x8_d_gp1294ai_clear_ram(u8x8_t *u8x8)
{
	u8x8_cad_StartTransfer(u8x8);  // Transmit start cb
	u8x8_cad_SendCmd(u8x8, SWAP8(0x0F0)); // GRAMWR Command
	u8x8_cad_SendArg(u8x8, 0);     // Xpos
	u8x8_cad_SendArg(u8x8, 0);     // Ypos
	u8x8_cad_SendArg(u8x8, SWAP8(0x07f)); // Return length
	for(uint16_t i = 0; i < ((256*128) / 8); i ++)
	{
		u8x8_cad_SendArg(u8x8,0x00); // Write 0x00 to 256x128 dot GRAM
	}
	u8x8_cad_EndTransfer(u8x8);    // Transmit end cb
}

static const uint8_t u8x8_d_gp1294ai_standby_seq[] = {
	U8X8_START_TRANSFER(),
	U8X8_C(SWAP8(0x061)), /* Standby */
	U8X8_END_TRANSFER(),

	U8X8_END() /* end of sequence */
};
static const uint8_t u8x8_d_gp1294ai_wakeup_seq[] = {
	U8X8_START_TRANSFER(),
	U8X8_C(SWAP8(0x06D)), /* Wake up */
	U8X8_END_TRANSFER(),
	U8X8_DLY(1), /* Wait OSC */

	U8X8_START_TRANSFER(),
	U8X8_CA(SWAP8(0x080), SWAP8(0x000)), /* Set "SC" Bit 0 */
	U8X8_END_TRANSFER(),

	U8X8_END() /* end of sequence */
};

uint8_t u8x8_d_gp1294ai_common(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
	uint8_t *ptr;
	uint8_t x, y;
	uint16_t tx_cnt;
	uint8_t swapped_byte;
	switch (msg)
	{
		case U8X8_MSG_DISPLAY_SET_POWER_SAVE:
		if (arg_int == 0)
		u8x8_cad_SendSequence(u8x8, u8x8_d_gp1294ai_wakeup_seq);
		else
		u8x8_cad_SendSequence(u8x8, u8x8_d_gp1294ai_standby_seq);
		break;
		#ifdef U8X8_WITH_SET_CONTRAST
		case U8X8_MSG_DISPLAY_SET_CONTRAST:
		u8x8_cad_StartTransfer(u8x8);
		u8x8_cad_SendCmd(u8x8, SWAP8(0x0A0));
		u8x8_cad_SendArg(u8x8, SWAP8((arg_int * 4) & 0xFF)); /* DIMMING LEVEL REG LSB */
		u8x8_cad_SendArg(u8x8, SWAP8((arg_int * 4) >> 8));   /* DIMMING LEVEL REG MSB */
		u8x8_cad_EndTransfer(u8x8);
		break;
		#endif
		case U8X8_MSG_DISPLAY_DRAW_TILE:
		ptr = ((u8x8_tile_t *)arg_ptr)->tile_ptr;    /* Data buffer array pointer */
		x = ((u8x8_tile_t *)arg_ptr)->x_pos * 8;     /* Xpos */
		y = ((u8x8_tile_t *)arg_ptr)->y_pos * 8;     /* Ypos */
		tx_cnt = ((u8x8_tile_t *)arg_ptr)->cnt * 8;  /* Data length */

		u8x8_cad_StartTransfer(u8x8);

		u8x8_cad_SendCmd(u8x8, SWAP8(0x0F0));  /* GRAMWR */
		u8x8_cad_SendArg(u8x8, SWAP8(x));      /* Xpos */
		u8x8_cad_SendArg(u8x8, SWAP8(y));      /* Ypos */
		u8x8_cad_SendArg(u8x8, SWAP8(0x007));  /* 8Pixel Y return lenght */

		do
		{
			ptr = ((u8x8_tile_t *)arg_ptr)->tile_ptr;
			tx_cnt = ((u8x8_tile_t *)arg_ptr)->cnt * 8;
			while (tx_cnt > 0)
			{
				swapped_byte = SWAP8(*ptr);
				u8x8_cad_SendData(u8x8, 1, &swapped_byte);
				ptr += 1;
				tx_cnt -= 1;
			}
			arg_int--;
		} while (arg_int > 0);

		u8x8_cad_EndTransfer(u8x8);
		break;
		default:
		return 0;
	}
	return 1;
}
uint8_t u8x8_d_gp1294ai_256x48(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
	switch (msg)
	{
		case U8X8_MSG_DISPLAY_SETUP_MEMORY:
		u8x8_d_helper_display_setup_memory(u8x8, &u8x8_gp1294ai_display_info);
		break;
		case U8X8_MSG_DISPLAY_INIT:
		u8x8_d_helper_display_init(u8x8);
		u8x8_d_gp1294ai_clear_ram(u8x8);
		u8x8_cad_SendSequence(u8x8, u8x8_d_gp1294ai_init_seq);
		break;
		default:
		return u8x8_d_gp1294ai_common(u8x8, msg, arg_int, arg_ptr);
	}
	return 1;
}





