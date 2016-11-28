#include <pebble.h>

static Window *window;
static Window *menu_window;
static MenuLayer *s_menu_layer;
static TextLayer *text_layer;
static TextLayer *max_hp_layer;
static TextLayer *hp_mod_layer;
static TextLayer *time_layer;
static Layer *s_canvas_layer;
uint32_t key;
static int user;
const uint16_t num_users = 5;
static int max_hp;
static int current_hp;
static int hp_mod;
static char buf[] = "00000000000";
static char c_hp_buf[] = "00000000000";
static char max_hp_buf[] = "00000000000";
static char s_buffer[8];

static void tick_handler(struct tm *tick_time, TimeUnits changed) {
  //static char s_buffer[8];

  // Read time into a string buffer
  strftime(s_buffer, sizeof(s_buffer), "%I:%M", tick_time);
  text_layer_set_text(time_layer, s_buffer);
  APP_LOG(APP_LOG_LEVEL_INFO, "Time is now %s", s_buffer);
}

//*********************************************
//Menu
static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return num_users;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  static char s_buff[16];
  key = (int)cell_index->row * 10;
  if (persist_exists(key)){
    persist_read_string(key, s_buff, sizeof(s_buff));
  } else {
    snprintf(s_buff, sizeof(s_buff), "Character %d", (int)cell_index->row);
  }
  
  // Draw this row's index
  menu_cell_basic_draw(ctx, cell_layer, s_buff, NULL, NULL);
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  const int16_t cell_height = 44;
  return cell_height;
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  // Do something in response to the button press
  user = (int)cell_index->row;
  
  window_stack_push(window, true);
}
//*********************************************
static void update_hp_mod() {
  snprintf(buf, sizeof(buf), "%d", hp_mod);
  if (hp_mod > 0){
    text_layer_set_text(hp_mod_layer, buf);
  } else if(hp_mod < 0){
    text_layer_set_text(hp_mod_layer, buf);
  } else {
    text_layer_set_text(hp_mod_layer, "");
  }
}

static void load_user() {
  /*
  0-name
  1-max
  2-current
  */
  key = user * 10;
  if (persist_exists(key)){
    current_hp = persist_read_int(key + 2);
    max_hp = persist_read_int(key + 1);
  }
  else {
    max_hp = 10;
    current_hp = max_hp;
  }
}

static void save_user() {
  /*
  0-name
  1-max
  2-current
  */
  key = user * 10;
  persist_write_int(key + 2, current_hp);
  persist_write_int(key + 1, max_hp);

}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  current_hp = current_hp + hp_mod;
  snprintf(c_hp_buf, sizeof(c_hp_buf), "%d", current_hp);
  hp_mod = 0;
  update_hp_mod();
  text_layer_set_text(text_layer, c_hp_buf);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (current_hp + hp_mod < max_hp) 
  {
    hp_mod = hp_mod + 1;
    update_hp_mod();
  }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  hp_mod = hp_mod - 1;
  update_hp_mod();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 200, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 200, down_click_handler);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  // Custom drawing happens here!
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  // Set the line color
  graphics_context_set_stroke_color(ctx, GColorFromRGB(0,0,100));
  // Set the fill color
  //graphics_context_set_fill_color(ctx, GColorFromRGB(0,0,100));
  graphics_context_set_stroke_width(ctx, 5);
  GPoint center = GPoint(bounds.size.w / 2, -50);
  uint16_t radius = 100;
  // Draw the outline of a circle
  graphics_draw_circle(ctx, center, radius);
  // Fill a circle
  //graphics_fill_circle(ctx, center, radius);
  
}

static void window_load(Window *window) {
  load_user();
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Get updates when the current minute changes
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Create canvas layer
  s_canvas_layer = layer_create(bounds);
  // Assign the custom drawing procedure
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  // Add to Window
  layer_add_child(window_layer, s_canvas_layer);

  text_layer = text_layer_create(GRect(0, 72, bounds.size.w, 40));
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
  snprintf(c_hp_buf, sizeof(c_hp_buf), "%d", current_hp);
  text_layer_set_text(text_layer, c_hp_buf);
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
  
  max_hp_layer = text_layer_create(GRect(0, 20, bounds.size.w, 20));
  snprintf(max_hp_buf, sizeof(max_hp_buf), "%d", max_hp);
  text_layer_set_text(max_hp_layer, max_hp_buf);
  text_layer_set_text_alignment(max_hp_layer, GTextAlignmentCenter);
  text_layer_set_background_color(max_hp_layer, GColorFromRGBA(0,0,0,0));
  layer_add_child(window_layer, text_layer_get_layer(max_hp_layer));
  
  hp_mod_layer = text_layer_create(GRect(0,60, bounds.size.w, 20));
  text_layer_set_text(hp_mod_layer, "");
  text_layer_set_text_alignment(hp_mod_layer, GTextAlignmentCenter);
  text_layer_set_background_color(hp_mod_layer, GColorFromRGBA(0,0,0,0));
  layer_add_child(window_layer, text_layer_get_layer(hp_mod_layer));
  
  time_layer = text_layer_create(GRect(0,bounds.size.h - 30, bounds.size.w, 20));
  text_layer_set_text(time_layer, "");
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  text_layer_set_background_color(time_layer, GColorFromRGBA(0,0,0,0));
  layer_add_child(window_layer, text_layer_get_layer(time_layer));
}

static void window_unload(Window *window) {
  save_user();
  layer_destroy(s_canvas_layer);
  text_layer_destroy(text_layer);
  text_layer_destroy(max_hp_layer);
  text_layer_destroy(hp_mod_layer);
}

static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
  // Read and store config data
  window_stack_pop_all(true);
  for(int x=0; x <= num_users; x++){
    key = x * 10;
    Tuple *s_name = dict_find(iter, MESSAGE_KEY_CharName + x);
    if(s_name) {
      persist_write_string(key, (s_name)->value->cstring);
    } 
    Tuple *s_max_hp = dict_find(iter, MESSAGE_KEY_MaxHP + x);
    if(s_max_hp) {
      persist_write_int(key + 1, atoi((s_max_hp)->value->cstring));
    } 
  }
  window_stack_push(menu_window, true);

}

void prv_init(void) {
  // ...

  // Open AppMessage connection
  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(128, 128);

  // ...
}

static void menu_load(Window *menu_window) {
  Layer *menu_window_layer = window_get_root_layer(menu_window);
  GRect bounds = layer_get_bounds(menu_window_layer);
  s_menu_layer = menu_layer_create(bounds);
  
  // Let it receive click events
menu_layer_set_click_config_onto_window(s_menu_layer, menu_window);

// Set the callbacks for behavior and rendering
menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_rows = get_num_rows_callback,
    .draw_row = draw_row_callback,
    .get_cell_height = get_cell_height_callback,
    .select_click = select_callback,
});

// Add to the Window
layer_add_child(menu_window_layer, menu_layer_get_layer(s_menu_layer));
}

static void menu_unload(Window *menu_window) {
  menu_layer_destroy(s_menu_layer);
}

static void init(void) {
  prv_init();
  
  menu_window = window_create();
  window_set_click_config_provider(menu_window, click_config_provider);
  window_set_window_handlers(menu_window, (WindowHandlers) {
    .load = menu_load,
    .unload = menu_unload,
  });
  
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  //window_stack_push(window, animated);
  window_stack_push(menu_window, animated);
  hp_mod=0;
}

static void deinit(void) {
  window_destroy(window);
  window_destroy(menu_window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}