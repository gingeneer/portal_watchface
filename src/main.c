#include <pebble.h>

Window *s_main_window;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static BitmapLayer *s_seconds_layer;
static GBitmap *s_seconds_bitmap;

static TextLayer *s_date_layer;
static GFont *s_date_font;

#define TOTAL_IMAGE_SLOTS 4
#define NUMBER_OF_IMAGES 10
#define EMPTY_SLOT -1

// bigger images for hour
const int IMAGE_RESOURCE_IDS_HOUR[NUMBER_OF_IMAGES] = {
  RESOURCE_ID_IMAGE_NUM_0, RESOURCE_ID_IMAGE_NUM_1, RESOURCE_ID_IMAGE_NUM_2,
  RESOURCE_ID_IMAGE_NUM_3, RESOURCE_ID_IMAGE_NUM_4, RESOURCE_ID_IMAGE_NUM_5,
  RESOURCE_ID_IMAGE_NUM_6, RESOURCE_ID_IMAGE_NUM_7, RESOURCE_ID_IMAGE_NUM_8,
  RESOURCE_ID_IMAGE_NUM_9
};

// smaller images for minutes
const int IMAGE_RESOURCE_IDS_MIN[NUMBER_OF_IMAGES] = {
  RESOURCE_ID_IMAGE_MIN_0, RESOURCE_ID_IMAGE_MIN_1, RESOURCE_ID_IMAGE_MIN_2, 
  RESOURCE_ID_IMAGE_MIN_3, RESOURCE_ID_IMAGE_MIN_4, RESOURCE_ID_IMAGE_MIN_5, 
  RESOURCE_ID_IMAGE_MIN_6, RESOURCE_ID_IMAGE_MIN_7, RESOURCE_ID_IMAGE_MIN_8,
  RESOURCE_ID_IMAGE_MIN_9
};

static int s_image_slot_state[TOTAL_IMAGE_SLOTS] = {EMPTY_SLOT, EMPTY_SLOT, EMPTY_SLOT, EMPTY_SLOT};

static GBitmap *s_images[TOTAL_IMAGE_SLOTS];
static BitmapLayer *s_image_layers[TOTAL_IMAGE_SLOTS];

static void bluetooth_connection_callback(bool connected) {
  if(!connected)
    vibes_double_pulse();
  else
    vibes_long_pulse();
}

static void load_image_into_slot(int slot_number, int digit_value) {

  if ((slot_number < 0) || (slot_number >= TOTAL_IMAGE_SLOTS)) {
    return;
  }

  if ((digit_value < 0) || (digit_value > 9)) {
    return;
  }

  if (s_image_slot_state[slot_number] != EMPTY_SLOT) {
    return;
  }
  
  s_image_slot_state[slot_number] = digit_value;
  
  // hour slot
  if(slot_number < 2){
    s_images[slot_number] = gbitmap_create_with_resource(IMAGE_RESOURCE_IDS_HOUR[digit_value]);
  }
  else{
    s_images[slot_number] = gbitmap_create_with_resource(IMAGE_RESOURCE_IDS_MIN[digit_value]);
  }
  
  #ifdef PBL_PLATFORM_BASALT
    GRect bounds = gbitmap_get_bounds(s_images[slot_number]);
  #else
    GRect bounds = s_images[slot_number]->bounds;
  #endif
  
  BitmapLayer *bitmap_layer;    
  if(slot_number < 2){
    bitmap_layer = bitmap_layer_create(GRect(((slot_number % 2) * 24) + 22, 9, bounds.size.w, bounds.size.h));
  }
  else{
    bitmap_layer = bitmap_layer_create(GRect(((slot_number % 2) * 12) + 78, 44, bounds.size.w, bounds.size.h));
  }
  s_image_layers[slot_number] = bitmap_layer;
  bitmap_layer_set_bitmap(bitmap_layer, s_images[slot_number]);
  Layer *window_layer = window_get_root_layer(s_main_window);
  layer_add_child(window_layer, bitmap_layer_get_layer(bitmap_layer));
}

static void unload_image_from_slot(int slot_number){
  if(s_image_slot_state[slot_number] != EMPTY_SLOT){
    layer_remove_from_parent(bitmap_layer_get_layer(s_image_layers[slot_number]));
    bitmap_layer_destroy(s_image_layers[slot_number]);
    gbitmap_destroy(s_images[slot_number]);
    s_image_slot_state[slot_number] = EMPTY_SLOT;
  }
}

static void display_value(unsigned short value, bool hour){
  //if hour == true then write into hour position, else minute
  value = value % 100;
  
  for(int position = 1; position >= 0; position--){
    int slot_number = (!hour * 2) + position;
    unload_image_from_slot(slot_number);
    load_image_into_slot(slot_number, value % 10);
    value = value / 10;
  }
}

static unsigned short get_display_hour(unsigned short hour) {
  if (clock_is_24h_style()) {
    return hour;
  }
  unsigned short display_hour = hour % 12;
  // Converts "0" to "12"
  return display_hour ? display_hour : 12;
}

static void update_seconds(int sec){
  //if(sec % 2 == 0){
     layer_set_frame(bitmap_layer_get_layer(s_seconds_layer), GRect((sec * 2) + 22, 97, 119, 16));
  //}
}

static void update_date(struct tm *tick_time){
  static char date_text[] = "01/01";
  strftime(date_text, sizeof(date_text), "%d/%m", tick_time);
  text_layer_set_text(s_date_layer, date_text);
}

static void update_time(struct tm *tick_time){
  display_value(get_display_hour(tick_time->tm_hour), true);
  display_value(tick_time->tm_min, false);
  if(!tick_time->tm_hour && !tick_time->tm_min){
    update_date(tick_time);
  }
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed){
  update_seconds(tick_time->tm_sec);
  if(tick_time->tm_sec == 0){
    update_time(tick_time);
  }
}

static void main_window_load(Window *window) {
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
  s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
  
  s_seconds_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HIDE_SECONDS);
  s_seconds_layer = bitmap_layer_create(GRect(22, 97, 119, 16));
  bitmap_layer_set_bitmap(s_seconds_layer, s_seconds_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_seconds_layer));
  
  s_date_layer = text_layer_create(GRect(28, 84, 30, 10));
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text(s_date_layer, "01/01");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_THOMA_9));
  text_layer_set_font(s_date_layer, s_date_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentLeft);
  
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  update_date(tick_time);
  update_time(tick_time);
  update_seconds(tick_time->tm_sec);

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
  
}

static void main_window_unload(Window *window){
  gbitmap_destroy(s_background_bitmap);
  bitmap_layer_destroy(s_background_layer);
  for(int i = 0; i < TOTAL_IMAGE_SLOTS; i++){
    unload_image_from_slot(i);
  }
  gbitmap_destroy(s_seconds_bitmap);
  bitmap_layer_destroy(s_seconds_layer);
  text_layer_destroy(s_date_layer);
}

void handle_init(void) {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
  bluetooth_connection_service_subscribe(bluetooth_connection_callback);
  
}

void handle_deinit(void) {
  bluetooth_connection_service_unsubscribe();
  window_destroy(s_main_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
