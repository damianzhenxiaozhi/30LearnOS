#include "bootpack.h"

struct MOUSE_DEC
{
	unsigned char buf[3], phase;
	int x, y, btn;
};

extern struct FIFO8 keyfifo, mousefifo;
void wait_KBC_sendready(void);
void init_keyboard();
void enable_mouse(struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
	char s[40], mcursor[16 * 16], keybuf[32], mousebuf[128];
	int mx, my, data, i;
	struct MOUSE_DEC mdec;

	init_gdtidt(); // gdt idt init
	init_pic();	   // pic init
	io_sti();	   // enable interrupt

	fifo8_init(&keyfifo, 32, keybuf);
	fifo8_init(&mousefifo, 128, mousebuf);
	io_out8(PIC0_IMR, 0xf9); // 11111001, bit2->slave, bit1->keyboard
	io_out8(PIC1_IMR, 0xef); // 11101111, bit12->mouse

	init_keyboard();

	init_palette();
	init_screen(binfo->vram, binfo->scrnx, binfo->scrny);
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

	enable_mouse(&mdec);

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
				if (mouse_decode(&mdec, data) != 0)
				{
					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01) != 0)
					{
						s[1] = 'L';
					}
					if ((mdec.btn & 0x02) != 0)
					{
						s[3] = 'R';
					}
					if ((mdec.btn & 0x04) != 0)
					{
						s[2] = 'C';
					}
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
					putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
				}

				boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);
				mx += mdec.x;
				my += mdec.y;
				if (mx < 0)
				{
					mx = 0;
				}
				if (my < 0)
				{
					my = 0;
				}
				if (mx > binfo->scrnx - 16)
				{
					mx = binfo->scrnx - 16;
				}
				if (my > binfo->scrny - 16)
				{
					my = binfo->scrny - 16;
				}
				sprintf(s, "(%3d, %3d)", mx, my);
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 10 * 8 - 1, 15);
				putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
				putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
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
	mdec->phase = 0;
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	wait_KBC_sendready();
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
		if ((dat & 0xc8) == 0x08)
		{
			mdec->buf[0] = dat;
			mdec->phase = 2;
			return 0;
		}
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
		mdec->btn = mdec->buf[0] & 0x7;
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];

		if ((mdec->buf[0] & 0x10) != 0)
		{
			mdec->x |= 0xffffff00;
		}
		if ((mdec->buf[0] & 0x20) != 0)
		{
			mdec->y |= 0xffffff00;
		}
		mdec->y = -mdec->y;
		return 1;
	}

	return -1;
}
