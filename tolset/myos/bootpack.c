#include "bootpack.h"

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

struct KEY_DEC
{
	unsigned char data[2], phase;
};

struct MOUSE_DEC
{
	unsigned char buf[3], phase;
};

void wait_KBC_sendready(void);
void init_keyboard();
void enable_mouse(struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
	char s[40], mcursor[16 * 16], keybuf[32], mousebuf[128];
	int mx, my, data, i;
	struct KEY_DEC kdec;
	struct MOUSE_DEC mdec;
	unsigned char mouse_debuf[3], mouse_phase;

	init_gdtidt();
	init_pic();
	io_sti(); // enable interrupt

	init_palette();
	init_screen(binfo->vram, binfo->scrnx, binfo->scrny);
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

	fifo8_init(&keyfifo, 32, keybuf);
	fifo8_init(&mousefifo, 128, mousebuf);
	io_out8(PIC0_IMR, 0xf9); // 11111001, bit2->slave, bit1->keyboard
	io_out8(PIC1_IMR, 0xef); // 11101111, bit12->mouse

	init_keyboard();
	enable_mouse(&mdec);
	mouse_phase = 0;

	for (;;)
	{
		io_cli();
		if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0)
		{
			io_stihlt();
		}
		else
		{
			if (fifo8_status(&keyfifo) != 0)
			{
				data = fifo8_get(&keyfifo);
				io_sti();
				sprintf(s, "%02X", data);
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 31, 31);
				putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
			}
			else if (fifo8_status(&mousefifo) != 0)
			{
				data = fifo8_get(&mousefifo);
				io_sti();
				i = mouse_decode(&mdec, data);
				if (i == 1)
				{
					sprintf(s, "%02X %02X %02X", mdec.buf[0], mdec.buf[1], mdec.buf[2]);
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 8 * 8 - 1, 31);
					putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
				}
			}
		}
	}
}

void wait_KBC_sendready(void)
{
	for (;;)
	{
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0)
		{
			break;
		}
	}
	return;
}

void init_keyboard(void)
{
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	wait_KBC_sendready();
	return;
}

void enable_mouse(struct MOUSE_DEC *mdec)
{
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	mdec->phase = 0;
	return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
	if (mdec->phase == 0)
	{
		if (dat == 0xfa)
		{
			mdec->phase = 1;
		}
		return 0;
	}

	if (mdec->phase == 1)
	{
		mdec->buf[0] = dat;
		mdec->phase = 2;
		return 0;
	}

	if (mdec->phase == 2)
	{
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}

	if (mdec->phase == 3)
	{
		mdec->buf[2] = dat;
		mdec->phase = 1;
		return 1;
	}

	return -1;
}
