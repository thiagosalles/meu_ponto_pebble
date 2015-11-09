#include <pebble.h>

#define GRID_POS_X 9
#define GRID_POS_Y 45
#define SEPARATOR_Y 106
#define ENTRY_TYPE_Y 5
#define TIME_Y 50
#define DATE_Y 110
#define CONFIRM_Y 5
#define REGISTERING_Y 55
#define CLOSE_TIMEOUT_MS 15000

enum AppMessageKeys {
	STATUS = 0,
	YEAR = 1,
	MONTH = 2,
	DAY = 3,
	ENTRY_TYPE = 4,
	TIME = 5
};

typedef enum EntryType {
	ENTRY_1 = 0,
	EXIT_1,
	ENTRY_2,
	EXIT_2
} EntryType;
const int EntryTypeCount = EXIT_2 - ENTRY_1 + 1;
const int EntryTypeOrder = -1;

const char* EntryTypeStrings[] = {
	"Entrada 1",
	"Saida 1",
	"Entrada 2",
	"Saida 2"
};

const int EntryTypeMinutes[] = {
	540,
	720,
	780,
	1080
};

static Window *s_main_window;
static TextLayer *s_entry_type_layer;
static EntryType entry_type;
static GBitmap *s_entry_type_grid_bitmap;
static BitmapLayer *s_entry_type_grid_layer;
static BitmapLayer *s_date_separator_layer;
static GBitmap *s_entry_type_fill_bitmap;
static BitmapLayer *s_entry_type_fill_layer;
static time_t s_date;
static time_t s_time;
static TextLayer *s_date_layer;
static TextLayer *s_time_layer;

// Confirm window
static Window *s_confirm_window;
static ActionBarLayer *s_confirm_action_layer;
static GBitmap *s_tick_icon;
static GBitmap *s_cross_icon;
static TextLayer *s_confirm_text_layer;

// Register window
static Window *s_register_window;
static TextLayer *s_register_text_layer;
static bool processing = false;


static int getTimeInMinutes() {
	if (s_time) {
		struct tm *temp_time = localtime(&s_time);
		return (temp_time->tm_hour * 60) + temp_time->tm_min;
	}
	return 0;
}

static EntryType getNearestEntryType() {
	int time_in_minutes = getTimeInMinutes();
	EntryType nearest_entry_type = ENTRY_1;
	int nearest_difference = abs(time_in_minutes - EntryTypeMinutes[nearest_entry_type]);
	for (int entry_type = EXIT_1; entry_type <= EXIT_2; entry_type++) {
		int difference = abs(time_in_minutes - EntryTypeMinutes[entry_type]);
		if (difference <= nearest_difference) {
			nearest_entry_type = entry_type;
			nearest_difference = difference;
		}
	}
	return nearest_entry_type;
}

static void close_app_handler(ClickRecognizerRef recognizer, void *context) {
	window_stack_pop_all(true);
}
static void close_app_cb(void *data) {
	window_stack_pop_all(true);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
	APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");

	// Store incoming information
	static int status;

	// Read first item
	Tuple *t = dict_read_first(iterator);

	// For all items
	while(t != NULL) {
		// Which key was received?
		switch(t->key) {
			case STATUS:
				status = t->value->int32;
				break;
			default:
				APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
				break;
		}

		// Look for next item
		t = dict_read_next(iterator);
	}

	if (s_register_window && window_stack_contains_window(s_register_window)) {
		if (status) {
			text_layer_set_text(s_register_text_layer, "OK!");
			window_stack_remove(s_main_window, false);
			app_timer_register(CLOSE_TIMEOUT_MS, close_app_cb, NULL);
		} else {
			text_layer_set_text(s_register_text_layer, "ERRO!");
		}
	}
	APP_LOG(APP_LOG_LEVEL_INFO, "Status: %d", status);
	processing = false;
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
	processing = false;
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
	window_stack_remove(s_confirm_window, false);
	processing = false;
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
	APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
	window_stack_remove(s_confirm_window, false);
	processing = true;
}

static void print_time() {
	struct tm *temp_time = localtime(&s_time);
	static char buffer[] = "00:00";
	strftime(buffer, sizeof("00:00"), "%H:%M", temp_time);
	text_layer_set_text(s_time_layer, buffer);
}

static void change_time(bool update_to_current) {
	if (!s_time || update_to_current) {
		if (s_time) vibes_short_pulse();
		s_time = time(NULL);
	} else {
		s_time = s_time - (60); // Subtracting one minute
	}
	print_time();
}

static void change_date(bool update_to_current) {

	if (!s_date || update_to_current) {
		if (s_date) vibes_short_pulse();
		s_date = time(NULL);
	} else {
		s_date = s_date - (60 * 60 * 24); // Subtracting one day
	}
	struct tm *temp_date = localtime(&s_date);

	// Create a long-lived buffer
	static char buffer[] = "__/__/____";

	// Write the current hours and minutes into the buffer
	strftime(buffer, sizeof("__/__/____"), "%d/%m/%Y", temp_date);

	// Display this time on the TextLayer
	text_layer_set_text(s_date_layer, buffer);
}



static void print_entry_type() {
	APP_LOG(APP_LOG_LEVEL_INFO, "Printing entry type %d.", (int)entry_type);
	text_layer_set_text(s_entry_type_layer, EntryTypeStrings[(int)entry_type]);
	layer_set_frame(bitmap_layer_get_layer(s_entry_type_fill_layer), GRect(GRID_POS_X+1+((int)entry_type*30)+(int)entry_type, GRID_POS_Y+1, 30, 2));
	layer_set_hidden(bitmap_layer_get_layer(s_entry_type_fill_layer), false);
}

static void change_entry_type() {
	entry_type = (entry_type + EntryTypeOrder + EntryTypeCount) % EntryTypeCount;
	print_entry_type();
}

static void main_window_up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	change_entry_type();
}

static void main_window_down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	change_date(false);
}

static void main_window_down_long_click_handler(ClickRecognizerRef recognizer, void *context) {
	change_date(true);
}

static void confirm_window_down_click_handler(ClickRecognizerRef recognizer, void *context) {
	window_stack_pop(true);
}

static void register_window_load(Window *window) {
	APP_LOG(APP_LOG_LEVEL_INFO, "Register window load!");

	s_register_text_layer = text_layer_create(GRect(0, REGISTERING_Y, 144, 40));
	text_layer_set_background_color(s_register_text_layer, GColorClear);
	text_layer_set_text_color(s_register_text_layer, GColorBlack);
	text_layer_set_font(s_register_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_text_alignment(s_register_text_layer, GTextAlignmentCenter);
	text_layer_set_text(s_register_text_layer, "Registrando...");
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_register_text_layer));
}

static void register_window_unload(Window *window) {
	APP_LOG(APP_LOG_LEVEL_INFO, "Confirm window unload!");
	text_layer_destroy(s_register_text_layer);
}

static void send_register() {
	// Begin dictionary
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);

	struct tm *temp_date = localtime(&s_date);
	static char buffer_year[] = "0000";
	static char buffer_month[] = "00";
	static char buffer_day[] = "00";
	strftime(buffer_year, sizeof("0000"), "%Y", temp_date);
	strftime(buffer_month, sizeof("00"), "%m", temp_date);
	strftime(buffer_day, sizeof("00"), "%d", temp_date);
	struct tm *temp_time = localtime(&s_time);
	static char buffer_time[] = "00:00";
	strftime(buffer_time, sizeof("00:00"), "%H:%M", temp_time);

	// Add a key-value pair
	dict_write_cstring(iter, YEAR, buffer_year);
	dict_write_cstring(iter, MONTH, buffer_month);
	dict_write_cstring(iter, DAY, buffer_day);
	dict_write_uint8(iter, ENTRY_TYPE, entry_type);
	dict_write_cstring(iter, TIME, buffer_time);

	// Send the message!
	app_message_outbox_send();
}

void register_window_click_config_provider(void *context) {
	window_single_click_subscribe(BUTTON_ID_SELECT, close_app_handler);
}

static void open_register_window() {
	s_register_window = window_create();
	window_set_window_handlers(s_register_window, (WindowHandlers) {
		.load = register_window_load,
		.unload = register_window_unload
	});
	window_set_click_config_provider(s_register_window, register_window_click_config_provider);
	window_stack_push(s_register_window, true);
	send_register();
}

static void confirm_window_up_click_handler(ClickRecognizerRef recognizer, void *context) {
	open_register_window();
}

void confirm_window_click_config_provider(void *context) {
	window_single_click_subscribe(BUTTON_ID_DOWN, confirm_window_down_click_handler);
	window_single_click_subscribe(BUTTON_ID_UP, confirm_window_up_click_handler);
}

static void confirm_window_load(Window *window) {
	APP_LOG(APP_LOG_LEVEL_INFO, "Confirm window load!");

	s_confirm_action_layer = action_bar_layer_create();
	action_bar_layer_add_to_window(s_confirm_action_layer, window);
	action_bar_layer_set_click_config_provider(s_confirm_action_layer, confirm_window_click_config_provider);

	s_tick_icon = gbitmap_create_with_resource(RESOURCE_ID_TICK_ICON);
	s_cross_icon = gbitmap_create_with_resource(RESOURCE_ID_CROSS_ICON);

	action_bar_layer_set_icon(s_confirm_action_layer, BUTTON_ID_UP, s_tick_icon);
	action_bar_layer_set_icon(s_confirm_action_layer, BUTTON_ID_DOWN, s_cross_icon);

	s_confirm_text_layer = text_layer_create(GRect(5, CONFIRM_Y, 130, 40));
	text_layer_set_background_color(s_confirm_text_layer, GColorClear);
	text_layer_set_text_color(s_confirm_text_layer, GColorBlack);
	text_layer_set_font(s_confirm_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_text(s_confirm_text_layer, "Confirma?");
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_confirm_text_layer));
}

static void confirm_window_unload(Window *window) {
	APP_LOG(APP_LOG_LEVEL_INFO, "Confirm window unload!");
	action_bar_layer_destroy(s_confirm_action_layer);
	gbitmap_destroy(s_tick_icon);
	gbitmap_destroy(s_cross_icon);
	text_layer_destroy(s_confirm_text_layer);
}

static void open_confirm_window() {
	s_confirm_window = window_create();
	window_set_window_handlers(s_confirm_window, (WindowHandlers) {
		.load = confirm_window_load,
		.unload = confirm_window_unload
	});
	window_stack_push(s_confirm_window, true);
}

static void main_window_select_single_click_handler(ClickRecognizerRef recognizer, void *cotext) {
	change_time(false);
}

static void main_window_select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
	change_time(true);
}

static void main_window_up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
	open_confirm_window();
}

static void main_window_click_config_provider(void *context) {
	window_single_click_subscribe(BUTTON_ID_UP, main_window_up_single_click_handler);
	window_long_click_subscribe(BUTTON_ID_UP, 0, main_window_up_long_click_handler, NULL);

	window_single_click_subscribe(BUTTON_ID_DOWN, main_window_down_single_click_handler);
	window_long_click_subscribe(BUTTON_ID_DOWN, 0, main_window_down_long_click_handler, NULL);

	window_single_click_subscribe(BUTTON_ID_SELECT, main_window_select_single_click_handler);
	window_long_click_subscribe(BUTTON_ID_SELECT, 0, main_window_select_long_click_handler, NULL);
}

static void main_window_load(Window *window) {
	// Creating and printing entry type grid
	s_entry_type_grid_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ENTRY_TYPE_GRID);
	s_entry_type_grid_layer = bitmap_layer_create(GRect(GRID_POS_X, GRID_POS_Y, 125, 4));
	bitmap_layer_set_bitmap(s_entry_type_grid_layer, s_entry_type_grid_bitmap);
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_entry_type_grid_layer));

	// Creating and printing entry type fill
	s_entry_type_fill_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ENTRY_TYPE_FILL);
	s_entry_type_fill_layer = bitmap_layer_create(GRect(GRID_POS_X+1, GRID_POS_Y+1, 30, 2));
	bitmap_layer_set_bitmap(s_entry_type_fill_layer, s_entry_type_fill_bitmap);
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_entry_type_fill_layer));
	layer_set_hidden(bitmap_layer_get_layer(s_entry_type_fill_layer), true);

	// Creating and printing date separator using the same Bitmap used for the grid
	s_date_separator_layer = bitmap_layer_create(GRect(GRID_POS_X, SEPARATOR_Y, 125, 1));
	bitmap_layer_set_bitmap(s_date_separator_layer, s_entry_type_grid_bitmap);
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_date_separator_layer));

	// Create entry type TextLayer
	s_entry_type_layer = text_layer_create(GRect(0, ENTRY_TYPE_Y, 144, 40)); // GRect(x, y, width, high). Full Resolution is 144x168
	text_layer_set_background_color(s_entry_type_layer, GColorClear);
	text_layer_set_text_color(s_entry_type_layer, GColorBlack);
	text_layer_set_font(s_entry_type_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_text_alignment(s_entry_type_layer, GTextAlignmentCenter);

	s_date_layer = text_layer_create(GRect(0, DATE_Y, 144, 40));
	text_layer_set_background_color(s_date_layer, GColorClear);
	text_layer_set_text_color(s_date_layer, GColorBlack);
	text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

	s_time_layer = text_layer_create(GRect(0, TIME_Y, 144, 50));
	text_layer_set_background_color(s_time_layer, GColorClear);
	text_layer_set_text_color(s_time_layer, GColorBlack);
	text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

	// Showing the time
	change_time(true);

	// Showing the date
	change_date(true);

	// Setting the starting EntryType
	entry_type = getNearestEntryType();
	print_entry_type();

	// Putting TextLayer on the window
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_entry_type_layer));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
}

static void main_window_unload(Window *window) {
	// Destroy TextLayers
	text_layer_destroy(s_entry_type_layer);
	text_layer_destroy(s_date_layer);
	text_layer_destroy(s_time_layer);
	// Destroy GBitmaps
	gbitmap_destroy(s_entry_type_grid_bitmap);
	gbitmap_destroy(s_entry_type_fill_bitmap);
	// Destroy BitmapLayers
	bitmap_layer_destroy(s_entry_type_grid_layer);
	bitmap_layer_destroy(s_entry_type_fill_layer);
	bitmap_layer_destroy(s_date_separator_layer);
}

static void init() {
	// Create main Window element and assign to pointer
	s_main_window = window_create();

	// Set handlers to manage the elements inside the Window
	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});

	window_set_click_config_provider(s_main_window, main_window_click_config_provider);

	// Register callbacks
	app_message_register_inbox_received(inbox_received_callback);
	app_message_register_inbox_dropped(inbox_dropped_callback);
	app_message_register_outbox_failed(outbox_failed_callback);
	app_message_register_outbox_sent(outbox_sent_callback);

	// Open AppMessage
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

	// Show the Window on the watch, with animated=true
	window_stack_push(s_main_window, true);
}

static void deinit() {
	// Destroy Window
	window_destroy(s_main_window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
