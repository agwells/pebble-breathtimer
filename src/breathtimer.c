#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0xC5, 0xB2, 0x81, 0x73, 0x45, 0x19, 0x4A, 0xB6, 0xAD, 0xB1, 0x64, 0x7A, 0x6A, 0x0D, 0x8E, 0x05 }
PBL_APP_INFO(MY_UUID,
             "Breath Timer", "Aaron Wells",
             1, 11, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_STANDARD_APP);


#define ACTION_BREATHE_IN 0
#define ACTION_BREATHE_OUT 1
#define ACTION_HOLD_BEFORE_IN 2
#define ACTION_HOLD_BEFORE_OUT 3
#define ACTION_DONE 4

#define DURATION_BREATHE_IN 7
#define DURATION_BREATHE_OUT 11
#define DURATION_HOLD_BEFORE_IN 0
#define DURATION_HOLD_BEFORE_OUT 0

#define DURATION_TOTAL 300 /* The total duration of the exercise in seconds */

Window window;

int current_breath_action = ACTION_BREATHE_IN;
int current_breath_duration = DURATION_BREATHE_IN;
int current_breath_elapsed = 0;

int total_elapsed = 0;

TextLayer instr_layer;
TextLayer totaltimer_layer;
TextLayer breathtimer_layer;
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

void handle_init(AppContextRef ctx) {

    window_init(&window, "Breath timer");
    // The animated wipe uses up the first second on the tick handler, so we need to be inanimate
    // to make the first breath segment show up correctly
    window_stack_push(&window, false /* Not animated */);

    GFont numfont = fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS);
    GFont textfont = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);

    // Initialize the strings that say how much time is remaining
    itoa(current_breath_duration - current_breath_elapsed, breathstr);
    itoa(DURATION_TOTAL - total_elapsed, totalstr);

    // The text that says "IN/OUT/DONE"
    text_layer_init(&instr_layer, GRect(0, 0, 144, 50));
    text_layer_set_text_alignment(&instr_layer, GTextAlignmentCenter);
    text_layer_set_text(&instr_layer, "IN");
    text_layer_set_font(&instr_layer, textfont);
    layer_add_child(&window.layer, &instr_layer.layer);

    // Text showing number of seconds remaining in current breath phase
    text_layer_init(&breathtimer_layer, GRect(0, 50, 144, 50));
    text_layer_set_text_alignment(&breathtimer_layer, GTextAlignmentCenter);
    text_layer_set_text(&breathtimer_layer, breathstr);
    text_layer_set_font(&breathtimer_layer, numfont);
    layer_add_child(&window.layer, &breathtimer_layer.layer);

    // Text showing number of seconds remaining in total exercise
    text_layer_init(&totaltimer_layer, GRect(0, 100, 144, 50));
    text_layer_set_text_alignment(&totaltimer_layer, GTextAlignmentCenter);
    text_layer_set_text(&totaltimer_layer, totalstr);
    text_layer_set_font(&totaltimer_layer, numfont);
    layer_add_child(&window.layer, &totaltimer_layer.layer);
}

void handle_tick(AppContextRef ctxt, PebbleTickEvent *event) {
    if (current_breath_action == ACTION_DONE) {
        return;
    }
    current_breath_elapsed++;
    total_elapsed++;

    if (current_breath_elapsed >= current_breath_duration) {
        if (current_breath_action == ACTION_BREATHE_IN) {
            current_breath_action = ACTION_BREATHE_OUT;
            current_breath_duration = DURATION_BREATHE_OUT;
            text_layer_set_text(&instr_layer, "OUT");
            vibes_long_pulse();
        } else {
            if (total_elapsed >= DURATION_TOTAL) {
                current_breath_action = ACTION_DONE;
                text_layer_set_text(&instr_layer, "DONE");
//                text_layer_deinit(&total_timer_layer);
//                text_layer_deinit(&breath_timer_layer);
                vibes_double_pulse();
                return;
            } else {
                current_breath_action = ACTION_BREATHE_IN;
                current_breath_duration = DURATION_BREATHE_IN;
                text_layer_set_text(&instr_layer, "IN");
                vibes_short_pulse();
            }
        }
        layer_mark_dirty((Layer *)&instr_layer);
        current_breath_elapsed = 0;
    }

    itoa(current_breath_duration - current_breath_elapsed, breathstr);
    text_layer_set_text(&breathtimer_layer, breathstr);

    itoa(DURATION_TOTAL - total_elapsed, totalstr);
    text_layer_set_text(&totaltimer_layer, totalstr);

    layer_mark_dirty(window_get_root_layer(&window));
}

void pbl_main(void *params) {
    AppContextRef ctxt = (AppContextRef) params;

    PebbleAppHandlers handlers = {
        .init_handler = &handle_init,
        .tick_info = {
            .tick_handler = &handle_tick,
            .tick_units = SECOND_UNIT
        }
    };
    app_event_loop(ctxt, &handlers);
}
