#include "bootpack.h"

struct SHTCTL *sheet_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize)
{
    struct SHTCTL ctl;
    int i;
    ctl = (struct SHTCTL *) memman_alloc_4k(memman, sizeof(struct SHTCTL));
    if (ctl == 0) {
        goto err;
    }
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1;
    for (i = 0; i < MAX_SHEETS; i++) {
        ctl->sheets0[i].flags = 0;
    }
err:
    return ctl;
}

#define SHEET_USE 1

struct SHEET *sheet_alloc(struct SHTCTL *ctl)
{
    struct SHEET *sheet;
    int i;
    for (i = 0; i < MAX_SHEETS; i++) {
        if (ctl->sheets0[i].flags == 0) {
            sheet = &ctl->sheets0[i];
            sheet->flags = SHEET_USE;
            sheet->height = -1;
            return sheet;
        }
    }
    return 0;
}

void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int bxsize, int bysize, int col_inv)
{
    sht->buf = buf;
    sht->bxsize = bxsize;
    sht->bysize = bysize;
    sht->col_inv = col_inv;
    return;
}

void sheet_updown(struct SHTCTL *ctl, struct SHEET *sht, int height)
{
    int h, old = sht->height;
    if (height < -1) {
        height = -1;
    }
    if (height >= ctl->top + 1) {
        height = ctl->top + 1;
    }
    sht->height = height;

    if (old > height) {
        if (height >= 0) {
            for (h = old; h > height; h--) {
                ctl->sheets[h] = ctl->sheets[h-1];
                ctl->sheets[h].heigth = h;
            }
            ctl->sheets[height] = sht;
        } else {
            if (old < ctl->top) {
                for (h = old; h < ctl->top; h++) {
                    ctl->sheets[h] = ctl->sheets[h+1];
                    ctl->sheets[h].height = h;
                }
            }
            ctl->top--;
        }
    } else if (old < height) {
        if (old >= 0) {
            for (h = old; h < height; h++) {
                ctl->sheets[h] = ctl->sheets[h+1];
                ctl->sheets[h].height = h;
            }
            ctl->sheets[height] = sht;
        } else {
            for (h = ctl->top; h >= height; h--) {
                ctl->sheets[h+1] = ctl->sheets[h];
                ctl->sheets[h+1].height = h+1;
            }
            ctl->sheets[height] = sht;
            ctl->top++;
        }
    }
    sheet_refresh(ctl);
    return;
}

void sheet_refresh(struct SHTCTL *shtctl)
{

}