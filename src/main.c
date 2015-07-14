#include <pebble.h>

static Window *s_main_window;
static Layer *s_points_layer;
static Layer *s_hour_layer;
static Layer *s_minute_layer;
static Layer *s_second_layer;
static Layer *s_triangle_layer;

static PropertyAnimation *s_hour_animation;
static PropertyAnimation *s_minute_animation;
static PropertyAnimation *s_second_animation;

static const int hour_points_x[] = {72,96,116,124,116,96,72,48,28,20,28,48};
static const int hour_points_y[] = {31,39,58,84,110,129,137,129,110,84,58,39};

#ifdef PBL_SDK_3
  
static const int CONFIG_COLOR = 0;
static const int CONFIG_BG = 1;
static const int CONFIG_SET = 2;

static GColor8 bg_color;
static GColor8 color;
  
static AppSync s_sync;
static uint8_t s_sync_buffer[64];

enum CONFIG {
  COLOR_KEY = 0x0,
  BG_KEY = 0X1
};

static void set_colors();

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  persist_write_bool(CONFIG_SET,true);
  persist_write_int(key,new_tuple->value->uint32);
  if((int)key == CONFIG_BG){
     set_colors();
     layer_mark_dirty(s_points_layer);
     layer_mark_dirty(s_triangle_layer);
     layer_mark_dirty(s_hour_layer);
  }
}
#else
static GColor bg_color;
static GColor color;
#endif
  
static void trigger_hour_animation(bool force) {
  time_t temp = time(NULL); 
  struct tm *t = localtime(&temp);
  int hour = t->tm_hour % 12;
  int minute = t->tm_min / 5;
  int second = t->tm_sec / 5;
  
  GRect second_from_frame = layer_get_frame(s_second_layer);
  GRect second_to_frame = GRect(hour_points_x[second] - 8, hour_points_y[second] - 8, 16, 16);
  s_second_animation = property_animation_create_layer_frame(s_second_layer, &second_from_frame, &second_to_frame);
  animation_schedule((Animation*) s_second_animation);
  
  if(force || t->tm_sec == 0){
    GRect minute_from_frame = layer_get_frame(s_minute_layer);
    GRect minute_to_frame = GRect(hour_points_x[minute] - 10, hour_points_y[minute] - 10, 20, 20);
    s_minute_animation = property_animation_create_layer_frame(s_minute_layer, &minute_from_frame, &minute_to_frame);
    animation_schedule((Animation*) s_minute_animation);
    if(force || t->tm_min == 0){
      GRect hour_from_frame = layer_get_frame(s_hour_layer);
      GRect hour_to_frame = GRect(hour_points_x[hour] - 12, hour_points_y[hour] - 12, 24, 24);
      s_hour_animation = property_animation_create_layer_frame(s_hour_layer, &hour_from_frame, &hour_to_frame);
      animation_schedule((Animation*) s_hour_animation);
    }
  }
}

static void set_colors(){
#ifdef PBL_COLOR
  if(persist_read_bool(CONFIG_SET)){
    bg_color = GColorFromHEX(persist_read_int(CONFIG_BG));
    color = GColorFromHEX(persist_read_int(CONFIG_COLOR));
  }else{
    bg_color = GColorBlack;
    color = GColorCyan;
  }
#else
    bg_color = GColorBlack;
    color = GColorWhite;
#endif
}

static void hour_update_proc(Layer *this_layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(this_layer);
  GPoint center = GPoint(bounds.size.w / 2, (bounds.size.h / 2));
#ifdef PBL_COLOR
  graphics_context_set_antialiased(ctx, true);
  if(persist_read_bool(CONFIG_SET)){
    if(gcolor_equal(bg_color, GColorWhite)) {
      graphics_context_set_stroke_color(ctx, GColorBlack);
    }else{
      graphics_context_set_stroke_color(ctx, GColorWhite);
    }
  }else{
    graphics_context_set_stroke_color(ctx, GColorWhite);
  }
  graphics_context_set_stroke_width(ctx, 3);
#endif
  graphics_context_set_fill_color(ctx, bg_color);
  graphics_fill_circle(ctx, center, 12);
  graphics_draw_circle(ctx, center, 10);
};
static void minute_update_proc(Layer *this_layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(this_layer);
  GPoint center = GPoint(bounds.size.w / 2, (bounds.size.h / 2));
  graphics_context_set_fill_color(ctx, bg_color);
#ifdef PBL_COLOR
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, GColorLightGray);
  graphics_context_set_stroke_width(ctx, 3);
#elif PBL_BW
  graphics_context_set_stroke_color(ctx, GColorWhite);
#endif
  graphics_fill_circle(ctx, center, 10);
  graphics_draw_circle(ctx, center, 8);
};
static void second_update_proc(Layer *this_layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(this_layer);
  GPoint center = GPoint(bounds.size.w / 2, (bounds.size.h / 2));
  graphics_context_set_fill_color(ctx, bg_color);
#ifdef PBL_COLOR
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_context_set_stroke_width(ctx, 3);
#elif PBL_BW
  graphics_context_set_stroke_color(ctx, GColorWhite);
#endif
  graphics_fill_circle(ctx, center, 8);
  graphics_draw_circle(ctx, center, 5);
};
static void triangle_update_proc(Layer *this_layer, GContext *ctx) {
  // Draw the triangle
#ifdef PBL_COLOR
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_width(ctx, 3);
#endif
  graphics_context_set_fill_color(ctx, color);
  graphics_context_set_stroke_color(ctx, color);
  
  GRect bounds = layer_get_frame(s_second_layer);
  GPoint sec = GPoint(bounds.origin.x + 8, bounds.origin.y + 8);
  bounds = layer_get_frame(s_minute_layer);
  GPoint minute = GPoint(bounds.origin.x + 10, bounds.origin.y + 10);
  bounds = layer_get_frame(s_hour_layer);
  GPoint hour = GPoint(bounds.origin.x + 12, bounds.origin.y + 12);
  
  GPathInfo triangle_path = {
    .num_points = 3,
    .points = (GPoint []) {sec, minute, hour}
  };
  GPath *path = gpath_create(&triangle_path);
  gpath_draw_filled(ctx, path);
  gpath_draw_outline(ctx, path);
  gpath_destroy(path);
};

static void points_update_proc(Layer *this_layer, GContext *ctx) {
  
  GRect bounds = layer_get_bounds(this_layer);
  //draw bg
#ifdef PBL_COLOR
  graphics_context_set_antialiased(ctx, true);
#endif
  graphics_context_set_fill_color(ctx, bg_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  
#ifdef PBL_COLOR
  graphics_context_set_stroke_width(ctx, 3);
#endif
  graphics_context_set_fill_color(ctx, color);
  graphics_context_set_stroke_color(ctx, color);

  // Get the center of the screen (non full-screen)
  GPoint center;
  for(int i=0;i<12;i++){
    center = GPoint(hour_points_x[i], hour_points_y[i]);
    graphics_fill_circle(ctx, center, 2);
  }
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  // Create Layer
  s_points_layer = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
  layer_add_child(window_layer, s_points_layer);
  
  s_triangle_layer = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
  layer_add_child(window_layer, s_triangle_layer);
  
  s_hour_layer = layer_create(GRect(window_bounds.size.w/2 - 12, window_bounds.size.h/2 - 12, 24, 24));
  layer_add_child(window_layer, s_hour_layer);
  s_minute_layer = layer_create(GRect(window_bounds.size.w/2 - 10, window_bounds.size.h/2 - 10, 20, 20));
  layer_add_child(window_layer, s_minute_layer);
  s_second_layer = layer_create(GRect(window_bounds.size.w/2 - 8, window_bounds.size.h/2 - 8, 16, 16));
  layer_add_child(window_layer, s_second_layer);

  // Set the update_proc
  layer_set_update_proc(s_points_layer, points_update_proc);
  layer_set_update_proc(s_hour_layer, hour_update_proc);
  layer_set_update_proc(s_minute_layer, minute_update_proc);
  layer_set_update_proc(s_second_layer, second_update_proc);
  layer_set_update_proc(s_triangle_layer, triangle_update_proc);
  
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
  trigger_hour_animation(true);
}

static void main_window_unload(Window *window) {
  // Destroy Layer
  layer_destroy(s_points_layer);
  layer_destroy(s_hour_layer);
  layer_destroy(s_minute_layer);
  layer_destroy(s_second_layer);
  layer_destroy(s_triangle_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if(tick_time->tm_sec % 5 == 0){
    trigger_hour_animation(false);
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