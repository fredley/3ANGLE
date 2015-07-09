#include <pebble.h>

static Window *s_main_window;
static Layer *s_canvas_layer;

static const int hour_points_x[] = {72,96,116,124,116,96,72,48,28,20,28,48};
static const int hour_points_y[] = {31,39,58,84,110,129,137,129,110,84,58,39};

#ifdef PBL_SDK_3
  
static const int CONFIG_COLOR = 0;
static const int CONFIG_BG = 1;
static const int CONFIG_SET = 2;
  
static AppSync s_sync;
static uint8_t s_sync_buffer[64];

enum CONFIG {
  COLOR_KEY = 0x0,
  BG_KEY = 0X1
};

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  persist_write_bool(CONFIG_SET,true);
  persist_write_int(key,new_tuple->value->uint32);
  if((int)key == CONFIG_BG){
    layer_mark_dirty(s_canvas_layer);
  }
}
#endif

static void canvas_update_proc(Layer *this_layer, GContext *ctx) {
  time_t temp = time(NULL); 
  struct tm *t = localtime(&temp);
  int hour = t->tm_hour % 12;
  int minute = t->tm_min / 5;
  int second = t->tm_sec / 5;
  GRect bounds = layer_get_bounds(this_layer);
  
  //draw bg
#ifdef PBL_COLOR
  if(persist_read_bool(CONFIG_SET)){
    graphics_context_set_fill_color(ctx, GColorFromHEX(persist_read_int(CONFIG_BG)));
  }else{
    graphics_context_set_fill_color(ctx, GColorBlack);
  }
#else
  graphics_context_set_fill_color(ctx, GColorBlack);
#endif
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Get the center of the screen (non full-screen)
  GPoint center = GPoint(bounds.size.w / 2, (bounds.size.h / 2));
  
  // Draw the triangle
#ifdef PBL_COLOR
  graphics_context_set_antialiased(ctx, true);
  if(persist_read_bool(CONFIG_SET)){
    // values have been set
    graphics_context_set_fill_color(ctx, GColorFromHEX(persist_read_int(CONFIG_COLOR)));
    graphics_context_set_stroke_color(ctx, GColorFromHEX(persist_read_int(CONFIG_COLOR)));
  }else{
    graphics_context_set_fill_color(ctx, GColorCyan);
    graphics_context_set_stroke_color(ctx, GColorCyan);
  }
  graphics_context_set_stroke_width(ctx, 3);
#elif PBL_BW
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorWhite);
#endif
  
  GPathInfo triangle_path = {
    .num_points = 3,
    .points = (GPoint []) {{hour_points_x[second], hour_points_y[second]}, {hour_points_x[minute], hour_points_y[minute]}, {hour_points_x[hour], hour_points_y[hour]}}
  };
  GPath *path = gpath_create(&triangle_path);
  gpath_draw_filled(ctx, path);
  gpath_draw_outline(ctx, path);
  gpath_destroy(path);
  
  for(int i=0;i<12;i++){
    center = GPoint(hour_points_x[i], hour_points_y[i]);
    graphics_fill_circle(ctx, center, 2);
  }
  
#ifdef PBL_COLOR
  if(persist_read_bool(CONFIG_SET)){
    graphics_context_set_fill_color(ctx, GColorFromHEX(persist_read_int(CONFIG_BG)));
    if(gcolor_equal(GColorFromHEX(persist_read_int(CONFIG_BG)), GColorWhite)) {
      graphics_context_set_stroke_color(ctx, GColorBlack);
    }else{
      graphics_context_set_stroke_color(ctx, GColorWhite);
    }
  }else{
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_context_set_stroke_color(ctx, GColorWhite);
  }
#else
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_color(ctx, GColorWhite);
#endif
  center = GPoint(hour_points_x[hour], hour_points_y[hour]);
  graphics_fill_circle(ctx, center, 12);
  graphics_draw_circle(ctx, center, 10);
#ifdef PBL_COLOR
  graphics_context_set_stroke_color(ctx, GColorLightGray);
#elif PBL_BW
  graphics_context_set_stroke_color(ctx, GColorWhite);
#endif
  center = GPoint(hour_points_x[minute], hour_points_y[minute]);
  graphics_fill_circle(ctx, center, 10);
  graphics_draw_circle(ctx, center, 8);
  
#ifdef PBL_COLOR
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
#elif PBL_BW
  graphics_context_set_stroke_color(ctx, GColorWhite);
#endif
  center = GPoint(hour_points_x[second], hour_points_y[second]);
  graphics_fill_circle(ctx, center, 8);
  graphics_draw_circle(ctx, center, 5);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  // Create Layer
  s_canvas_layer = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
  layer_add_child(window_layer, s_canvas_layer);

  // Set the update_proc
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
#ifdef PBL_SDK_3
  
  int col;
  int bg;
  
  if(persist_read_bool(CONFIG_SET)){
    col = persist_read_int(CONFIG_COLOR);
    bg = persist_read_int(CONFIG_BG);
  }else{
    col = 0x00FFFF;
    bg = 0x000000;
  }
  
  Tuplet initial_values[] = {
      TupletInteger(COLOR_KEY, col),
      TupletInteger(BG_KEY, bg)
    };

  app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer), 
      initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL
  );
#endif
}

static void main_window_unload(Window *window) {
  // Destroy Layer
  layer_destroy(s_canvas_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if(tick_time->tm_sec % 5 == 0){
    layer_mark_dirty(s_canvas_layer);
  }
}

static void init(void) {
  // Create main Window
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  window_stack_push(s_main_window, true);
#ifdef PBL_SDK_3
  app_message_open(64, 64);
#endif
}

static void deinit(void) {
  // Destroy main Window
  window_destroy(s_main_window);
#ifdef PBL_SDK_3
  app_sync_deinit(&s_sync);
#endif
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}