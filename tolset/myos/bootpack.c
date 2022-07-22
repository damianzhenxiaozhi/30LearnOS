#include "bootpack.h"

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

struct KEY_DEC {
    unsigned char data[2], phase;
};

struct MOUSE_DEC {
    unsigned char buf[3], phase;
};

void wait_KBC_sendready(void);
void init_keyboard(void);
void enable_mouse(void);

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40], mcursor[16 * 16], keybuf[32], mousebuf[128];
	int mx, my, data;
	struct KEY_DEC kdec;
	struct MOUSE_DEC mdec;
	unsigned char mouse_debuf[3], phase;

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
	enable_mouse();
	phase = 0;

	for (;;) {
		io_cli();
		if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
			io_stihlt();
		} else {
			if (fifo8_status(&keyfifo) != 0) {
				data = fifo8_get(&keyfifo);
				io_sti();
				sprintf(s, "%02X", data);
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 31, 31);
				putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
			} else if (fifo8_status(&mousefifo) != 0) {
				data = fifo8_get(&mousefifo);
				io_sti();
				if (phase == 0) {
					phase = 1;
				} else if (phase == 1) {
					mouse_debuf[0] = data;
					phase = 2;
				} else if (phase == 2) {
					mouse_debuf[1] = data;
					phase = 3;
				} else if (phase == 3) {
					mouse_debuf[2] = data;
					phase = 0;
					sprintf(s, "%02X %02X %02X", mouse_debuf[0], mouse_debuf[1], mouse_debuf[2]);
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 8 * 8 - 1, 31);
					putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
				}
			}
		}
	}
}

void wait_KBC_sendready(void) {
	for (;;) {
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
			break;
		}
	}
	return;
}

void init_keyboard(void) {
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	wait_KBC_sendready();
	return;
}

void enable_mouse(void) {
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	wait_KBC_sendready();
	return;
}
