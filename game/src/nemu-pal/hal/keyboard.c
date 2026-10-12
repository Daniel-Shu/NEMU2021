#include "hal.h"

#define NR_KEYS 18

enum {KEY_STATE_EMPTY, KEY_STATE_WAIT_RELEASE, KEY_STATE_RELEASE, KEY_STATE_PRESS};

/* Only the following keys are used in NEMU-PAL. */
static const int keycode_array[] = {
	K_UP, K_DOWN, K_LEFT, K_RIGHT, K_ESCAPE,
	K_RETURN, K_SPACE, K_PAGEUP, K_PAGEDOWN, K_r,
	K_a, K_d, K_e, K_w, K_q,
	K_s, K_f, K_p
};

static int key_state[NR_KEYS];

/* The states a key goes through are: it is pressed and the game has not
 * been told yet (PRESS), the game knows about it and the key is still
 * held down (WAIT_RELEASE), it is let go of and the game has not been
 * told yet (RELEASE), and finally there is nothing to say about it
 * (EMPTY).
 */

void
keyboard_event(void) {
	int i;
	int scancode = in_byte(0x60);

	/* The keyboard only tells that a key was let go of by sending its
	 * make code with the highest bit set. */
	int released = (scancode & 0x80) != 0;

	for (i = 0; i < NR_KEYS; i ++) {
		if(get_keycode(i) != (scancode & 0x7f)) {
			continue;
		}

		if(released) {
			key_state[i] = KEY_STATE_RELEASE;
		}
		else if(query_key(i) == KEY_STATE_EMPTY) {
			/* The keyboard sends the make code over and over for as long
			 * as the key is held down, and only the first one of them is
			 * a press: the game is written for a keyboard that does not
			 * repeat itself. */
			key_state[i] = KEY_STATE_PRESS;
		}
	}
}

static inline int
get_keycode(int index) {
	assert(index >= 0 && index < NR_KEYS);
	return keycode_array[index];
}

static inline int
query_key(int index) {
	assert(index >= 0 && index < NR_KEYS);
	return key_state[index];
}

static inline void
release_key(int index) {
	assert(index >= 0 && index < NR_KEYS);
	key_state[index] = KEY_STATE_WAIT_RELEASE;
}

static inline void
clear_key(int index) {
	assert(index >= 0 && index < NR_KEYS);
	key_state[index] = KEY_STATE_EMPTY;
}

bool
process_keys(void (*key_press_callback)(int), void (*key_release_callback)(int)) {
	int i;

	/* No key may change its state while the states are looked at, so
	 * the keyboard is not allowed to interrupt until the one event
	 * that is found has been taken care of. */
	cli();

	for (i = 0; i < NR_KEYS; i ++) {
		if(query_key(i) == KEY_STATE_PRESS) {
			/* The game is told about the press, and the release is what
			 * is waited for from now on. */
			release_key(i);
			key_press_callback(get_keycode(i));
			sti();
			return true;
		}
		else if(query_key(i) == KEY_STATE_RELEASE) {
			clear_key(i);
			key_release_callback(get_keycode(i));
			sti();
			return true;
		}
	}

	sti();
	return false;
}
