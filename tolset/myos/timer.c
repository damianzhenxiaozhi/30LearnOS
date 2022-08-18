#include "bootpack.h"

#define PIT_CTRL    0x0043
#define PIT_CNT0    0x0040

#define TIMER_FLAGS_ALLOC 1 // timer was alloc
#define TIMER_FLAGS_USING 2 // timer is using

struct TIMERCTL timerctl;

void init_pit(void)
{
    int i;
    struct TIMER *t;
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e);

    for (i = 0; i < MAX_TIMERS; i++) {
        timerctl.timers0[i].flags = 0;
    }
    t = timer_alloc();
    t->timeout = 0xffffffff;
    t->flags = TIMER_FLAGS_USING;
    t->next_timer = 0;
    timerctl.t0 = t;
    timerctl.count = 0;
    timerctl.next_timeout = 0xffffffff;
    return;
}

struct TIMER *timer_alloc(void)
{
    int i;
    for (i = 0; i < MAX_TIMERS; i++) {
        if (timerctl.timers0[i].flags == 0) {
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timers0[i];
        }
    }
    return 0;
}

void timer_free(struct TIMER *timer)
{
    timer->flags = 0;
    return;
}

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data)
{
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout)
{
    int eflags;
    struct TIMER *t, *s;
    eflags = io_load_eflags();
    io_cli();
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;

    t = timerctl.t0;
    if (timer->timeout <= t->timeout) {
        timerctl.t0 = timer;
        timer->next_timer = t;
        timerctl.next_timeout = timer->timeout;
        io_store_eflags(eflags);
        return;
    }

    for (;;) {
        s = t;
        t = t->next_timer;
        if (timer->timeout <= t->timeout) {
            s->next_timer = timer;
            timer->next_timer = t;
            io_store_eflags(eflags);
            return;
        }
    }
}

void inthandler20(int *esp)
{
    struct TIMER *timer;
    io_out8(PIC0_OCW2, 0x60); // 0x60 + 0
    timerctl.count++;
    if (timerctl.next_timeout > timerctl.count) {
        return;
    }
    timer = timerctl.t0;
    for (;;) {
        if (timer->timeout > timerctl.count) {
            break;
        }
        timer->flags = TIMER_FLAGS_ALLOC;
        fifo32_put(timer->fifo, timer->data);
        timer = timer->next_timer;
    }
    timerctl.t0 = timer;
    timerctl.next_timeout = timerctl.t0->timeout;
    return;
}
