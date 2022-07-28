#include "bootpack.h"

#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000

unsigned int memtest(unsigned int start, unsigned end)
{
    char flg486 = 0;
    unsigned int eflag, cr0, i;

    eflag = io_load_eflags();
    eflag |= EFLAGS_AC_BIT;
    io_store_eflags(eflag);
    eflag = io_load_eflags();
    if ((eflag & EFLAGS_AC_BIT) != 0)
    {
        flg486 = 1;
    }
    eflag &= ~EFLAGS_AC_BIT;
    io_store_eflags(eflag);

    if (flg486 != 0)
    {
        cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }

    i = memtest_sub(start, end);

    if (flg486 != 0)
    {
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }

    return i;
}

#define MEMMAN_FREES 4090

struct FREEINFO
{
    unsigned int addr, size;
};

struct MEMMAN
{
    int frees, maxfrees, lostsize, losts;
    struct FREEINFO free[MEMMAN_FREES];
};

void memman_init(struct MEMMAN *man)
{
    man->frees = 0;
    man->maxfrees = 0;
    man->lostsize = 0;
    man->losts = 0;
    return;
}

unsigned int memman_total(struct MEMMAN *man)
{
    unsigned int i, t = 0;
    for (i = 0; i < man->frees; i++)
    {
        t += man->free[i].size;
    }
    return t;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
{
    unsigned int i, a;
    for (i = 0; i < man->frees; i++)
    {
        if (man->free[i].size >= size)
        {
            a = man->free[i].addr;

            man->free[i].addr += size;
            man->free[i].size -= size;
            if (man->free[i].size == 0)
            {
                man->frees--;
                for (; i < man->frees; i++)
                {
                    man->free[i] = man->free[i + 1];
                }
            }
            return a;
        }
    }
    return 0;
}

unsigned int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
    int i, j;
    // free[] order by addr
    for (i = 0; i < man->free; i++)
    {
        if (man->free[i].addr > addr)
        {
            break;
        }
    }
    if (i > 0) {
        if (man->free[i-1].addr + man->free[i-1].size == addr) {
            man->free[i-1].size += size;
            if (i < man->frees) {
                if (addr + size == man->free[i].addr) {
                    man->free[i-1].size += man->free[i].size;
                    man->frees--;
                    for (; i < man->frees; i++) {
                        man->free[i] = man->free[i+1];
                    }
                }
            }
            return 0;
        }
    }
    if (i < man->frees) {
        return 0;
    }

    return 0;
}