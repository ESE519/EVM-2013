#include "states.h"

extern TOP_LEVEL_SIGNAL my_signal;
extern MID_LEVEL_SIGNAL mid_sig;

void send_middle_level_signal (MIDDLE_LEVEL_SIGNAL sig) {
	mid_sig = sig;
};
	