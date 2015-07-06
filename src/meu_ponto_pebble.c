#include <pebble.h>

#define GRID_POS_X 9
#define GRID_POS_Y 45
#define SEPARATOR_Y 106
#define ENTRY_TYPE_Y 5
#define TIME_Y 50
#define DATE_Y 110

typedef enum EntryType {
	ENTRY_1 = 0,
	EXIT_1,
	ENTRY_2,
	EXIT_2
} EntryType;
const int EntryTypeCount = EXIT_2 - ENTRY_1 + 1;
const int EntryTypeOrder = +1;

const char* EntryTypeStrings[] = {
	"Entrada 1",
	"Saida 1",
	"Entrada 2",
	"Saida 2"
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

// Register window
static Window *s_register_window;
static ActionBarLayer *s_confirm_action_layer;
static GBitmap *s_tick_icon;
static GBitmap *s_cross_icon;
static TextLayer *s_confirm_text_layer;

static void print_time() {
	struct tm *temp_time = localtime(&s_time);
	static char buffer[] = "00:00";
	strftime(buffer, sizeof("00:00"), "%H:%M", temp_time);
	text_layer_set_text(s_time_layer, buffer);
}

static void change_to_current_time() {
	if (s_date) vibes_short_pulse();
	s_time = time(NULL);
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
	entry_type = (entry_type + EntryTypeOrder) % EntryTypeCount;
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

void register_window_click_config_provider(void *context) {
	//window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) my_next_click_handler);
	//window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) my_previous_click_handler);
}

static void register_window_load(Window *window) {
	APP_LOG(APP_LOG_LEVEL_INFO, "Register window load!");
	
	s_confirm_action_layer = action_bar_layer_create();
	action_bar_layer_add_to_window(s_confirm_action_layer, window);
	action_bar_layer_set_click_config_provider(s_confirm_action_layer, register_window_click_config_provider);

	s_tick_icon = gbitmap_create_with_resource(RESOURCE_ID_TICK_ICON);
	s_cross_icon = gbitmap_create_with_resource(RESOURCE_ID_CROSS_ICON);
	
	action_bar_layer_set_icon(s_confirm_action_layer, BUTTON_ID_UP, s_tick_icon);
	action_bar_layer_set_icon(s_confirm_action_layer, BUTTON_ID_DOWN, s_cross_icon);
	
	s_confirm_text_layer = text_layer_create(GRect(5, 55, 130, 40));
	text_layer_set_background_color(s_confirm_text_layer, GColorClear);
	text_layer_set_text_color(s_confirm_text_layer, GColorBlack);
	text_layer_set_font(s_confirm_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_text(s_confirm_text_layer, "Confirma?");
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_confirm_text_layer));
}

static void register_window_unload(Window *window) {
	APP_LOG(APP_LOG_LEVEL_INFO, "Register window unload!");
	action_bar_layer_destroy(s_confirm_action_layer);
	gbitmap_destroy(s_tick_icon);
	gbitmap_destroy(s_cross_icon);
	text_layer_destroy(s_confirm_text_layer);
}

static void main_window_select_single_click_handler(ClickRecognizerRef recognizer, void *cotext) {
	APP_LOG(APP_LOG_LEVEL_INFO, "Select clicked");
	s_register_window = window_create();
	window_set_window_handlers(s_register_window, (WindowHandlers) {
		.load = register_window_load,
		.unload = register_window_unload
	});
	window_stack_push(s_register_window, true);
}

static void main_window_click_config_provider(void *context) {
	// Register the ClickHandlers
	window_single_click_subscribe(BUTTON_ID_UP, main_window_up_single_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, main_window_down_single_click_handler);
	window_long_click_subscribe(BUTTON_ID_DOWN, 0, main_window_down_long_click_handler, NULL);
	window_single_click_subscribe(BUTTON_ID_SELECT, main_window_select_single_click_handler);
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
	change_to_current_time();
	
	// Showing the date
	change_date(true);

	// Setting the starting EntryType
	entry_type = ENTRY_1;
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
