#include "bootpack.h"

void init_pic(void)
{
    io_out8(PIC0_IMR, 0xff);
    io_out8(PIC1_IMR, 0xff);

    io_out8(PIC0_ICW1, 0x11); // edge trigger mode
    io_out8(PIC0_ICW2, 0x20); // IRQ0-7 INT20-INT27
    io_out8(PIC0_ICW3, 1 << 2);
    io_out8(PIC0_ICW4, 0x01);

    io_out8(PIC1_ICW1, 0x11); // edge trigger mode
    io_out8(PIC1_ICW2, 0x28); // IRQ8-15 INT28-INT2f
    io_out8(PIC1_ICW3, 2);
    io_out8(PIC1_ICW4, 0x01);

    io_out8(PIC0_IMR, 0xfb);
    io_out8(PIC1_IMR, 0xff);
    
    return;
}

struct FIFO8 keyfifo;

void inthandler21(int *esp)
{
    int data;
    io_out8(PIC0_OCW2, 0x61); // 0x60 + 1
    data = io_in8(PORT_KEYDAT);
    fifo8_put(&keyfifo, data);
    return;
}

void inthandler27(int *esp)
{
    io_out8(PIC0_OCW2, 0x67);
    return;
}

struct FIFO8 mousefifo;

void inthandler2c(int *esp)
{
    int data;
    io_out8(PIC1_OCW2, 0x64); // 0x60 + 4
    io_out8(PIC0_OCW2, 0x62); // 0x60 + 2
    data = io_in8(PORT_KEYDAT);
    fifo8_put(&mousefifo, data);
    return;
}
