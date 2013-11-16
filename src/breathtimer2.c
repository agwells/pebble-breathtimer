#include "pebble.h"
#include "pebble_fonts.h"

#define ACTION_BREATHE_IN 0
#define ACTION_BREATHE_OUT 1
#define ACTION_HOLD_AFTER_IN 2
#define ACTION_HOLD_AFTER_OUT 3
#define ACTION_DONE 4

#define DURATION_BREATHE_IN 2
#define DURATION_BREATHE_OUT 2
#define DURATION_HOLD_AFTER_IN 1
#define DURATION_HOLD_AFTER_OUT 1

#define DURATION_TOTAL 600 /* The total duration of the exercise in seconds */

Window *window;

int current_breath_action = ACTION_BREATHE_IN;
int current_breath_duration = DURATION_BREATHE_IN;
int current_breath_elapsed = 0;

int total_elapsed = 0;

TextLayer *instr_layer;
TextLayer *totaltimer_layer;
TextLayer *breathtimer_layer;
char totalstr[5];
char breathstr[5];

/**
* Ansi C "itoa" based on Kernighan & Ritchie's "Ansi C":
*/
void strreverse(char* begin, char* end) {
    char aux;
    while(end>begin) {
        aux=*end, *end--=*begin, *begin++=aux;
    }
}

void itoa(int value, char* str) {
    static char num[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char* wstr=str;
    int sign;
    int base=10;

    // Take care of sign
    if ((sign=value) < 0) {
        value = -value;
    }

    // Conversion. Number is reversed.
    do {
        *wstr++ = num[value%base];
    } while(value/=base);

    if (sign<0) {
        *wstr++='-';
    }

    *wstr='\0';

    // Reverse string
    strreverse(str,wstr-1);
}


void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
    // We no longer care about ticks after the 5 minutes are up, but since I don't know how to unregister
    // a tick handler, I'll just quit this if we're done
    if (current_breath_action == ACTION_DONE) {
        return;
    }

    // Increment timers
    current_breath_elapsed++;
    total_elapsed++;

    // Check if we need to switch breath phases
    if (current_breath_elapsed >= current_breath_duration) {

        switch(current_breath_action) {
            // End of breath-in phase. Change to breath-out
            case ACTION_BREATHE_IN:
                current_breath_action = ACTION_BREATHE_OUT;
                current_breath_duration = DURATION_BREATHE_OUT;
                text_layer_set_text(instr_layer, "OUT");
                vibes_short_pulse();
                break;

            // End of breath-out phase. Change to hold after out
            case ACTION_BREATHE_OUT:
                current_breath_action = ACTION_HOLD_AFTER_OUT;
                current_breath_duration = DURATION_HOLD_AFTER_OUT;
                text_layer_set_text(instr_layer, "HOLD");
                vibes_short_pulse();
                break;

            case ACTION_HOLD_AFTER_OUT:

                // We only want to end the full exercise on the end of an in-out cycle.
                // So since we just finished an in-out breathing cycle, we check for
                // end of the total period now.
                if (total_elapsed >= DURATION_TOTAL) {
                    // End of the full exercise. Set all the displays to whatever we want
                    // and quit so that it doesn't get overwritten
                    current_breath_action = ACTION_DONE;
                    text_layer_set_text(instr_layer, "DONE");
                    text_layer_set_text(breathtimer_layer, "0");
                    vibes_double_pulse();
                    return;

                // Not the end of the full exercise, so just move on to the next breath in
                } else {
                    current_breath_action = ACTION_BREATHE_IN;
                    current_breath_duration = DURATION_BREATHE_IN;
                    text_layer_set_text(instr_layer, "IN");
                    vibes_long_pulse();
                }
                break;
        }
        layer_mark_dirty((Layer *) instr_layer);
        current_breath_elapsed = 0;
    }

    // Display current breath remaining
    itoa(current_breath_duration - current_breath_elapsed, breathstr);
    text_layer_set_text(breathtimer_layer, breathstr);

    // Display total time remaining
    itoa(DURATION_TOTAL - total_elapsed, totalstr);
    text_layer_set_text(totaltimer_layer, totalstr);

    layer_mark_dirty(window_get_root_layer(window));
}


void handle_init() {

	tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);

	window = window_create();

	// The animated wipe uses up the first second on the tick handler, so we need to be inanimate
    // to make the first breath segment show up correctly
    window_stack_push(window, false /* Not animated */);

    GFont numfont = fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS);
    GFont textfont = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);

    // Initialize the strings that say how much time is remaining
    itoa(current_breath_duration - current_breath_elapsed, breathstr);
    itoa(DURATION_TOTAL - total_elapsed, totalstr);

    // The text that says "IN/OUT/DONE"
    instr_layer = text_layer_create(GRect(0, 0, 144, 50));
    text_layer_set_text_alignment(instr_layer, GTextAlignmentCenter);
    text_layer_set_text(instr_layer, "IN");
    text_layer_set_font(instr_layer, textfont);
    layer_add_child(window_get_root_layer(window), (Layer*)instr_layer);

    // Text showing number of seconds remaining in current breath phase
    breathtimer_layer = text_layer_create(GRect(0, 50, 144, 50));
    text_layer_set_text_alignment(breathtimer_layer, GTextAlignmentCenter);
    text_layer_set_text(breathtimer_layer, breathstr);
    text_layer_set_font(breathtimer_layer, numfont);
    layer_add_child(window_get_root_layer(window), (Layer*)breathtimer_layer);

    // Text showing number of seconds remaining in total exercise
    totaltimer_layer = text_layer_create(GRect(0, 100, 144, 50));
    text_layer_set_text_alignment(totaltimer_layer, GTextAlignmentCenter);
    text_layer_set_text(totaltimer_layer, totalstr);
    text_layer_set_font(totaltimer_layer, numfont);
    layer_add_child(window_get_root_layer(window), (Layer*)totaltimer_layer);
}

void handle_deinit() {
	text_layer_destroy(totaltimer_layer);
	text_layer_destroy(breathtimer_layer);
	text_layer_destroy(instr_layer);
	window_destroy(window);
}


int main(void) {
	handle_init();
	app_event_loop();
	handle_deinit();
}
