#include "hal.h"

static volatile uint32_t jiffy = 0;
static int fps = 0;
static int nr_draw = 0;

void
incr_nr_draw(void) {
	nr_draw ++;
}

int
get_fps() {
	return fps;
}

void
timer_event(void) {
	jiffy ++;
	if(jiffy % (HZ / 2) == 0) {
		fps = nr_draw * 2 + 1;
		nr_draw = 0;
	}
}

uint32_t SDL_GetTicks() {
	/* jiffy counts the interrupts the timer of the machine has raised,
	 * and it raises HZ of them every second, so this is the number of
	 * milliseconds the game has been running for. */
	return jiffy * (1000 / HZ);
}

void SDL_Delay(uint32_t ms) {
	uint32_t end = SDL_GetTicks() + ms;

	/* The clock only moves on while the processor is idle and the
	 * interrupts are let in, so there is nothing else to do than wait
	 * for the next of them until enough time has gone by. */
	while(SDL_GetTicks() < end) {
		wait_intr();
	}
}
