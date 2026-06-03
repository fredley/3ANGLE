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

// The watchface was originally designed for a 144x168 display. All sizes below
// are derived from that design and scaled to the current screen so the face
// renders centered and proportional on every platform (basalt, chalk, emery,
// flint, gabbro, ...). DESIGN_BASE is the smaller dimension of that screen.
#define DESIGN_BASE 144

// Computed geometry (filled in once the window is loaded and we know the size).
static GPoint s_center;
static GPoint s_points[12];   // the 12 clock positions around the ring
static int s_ring_r;          // radius of the ring the markers travel on
static int s_hour_r;          // hour marker outer (fill) radius
static int s_minute_r;        // minute marker outer (fill) radius
static int s_second_r;        // second marker outer (fill) radius
static int s_hour_ir;         // hour marker inner (outline) radius
static int s_minute_ir;       // minute marker inner (outline) radius
static int s_second_ir;       // second marker inner (outline) radius
static int s_point_r;         // hour-pip radius
static int s_stroke;          // stroke width

#ifdef PBL_SDK_3

// Persistent-storage slots (internal to this app, independent of message keys).
static const int CONFIG_COLOR = 0;
static const int CONFIG_BG = 1;
static const int CONFIG_SET = 2;

static GColor8 bg_color;
static GColor8 color;

static AppSync s_sync;
static uint8_t s_sync_buffer[64];

static void set_colors();

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  persist_write_bool(CONFIG_SET, true);
  if (key == MESSAGE_KEY_COLOR_KEY) {
    persist_write_int(CONFIG_COLOR, new_tuple->value->uint32);
  } else if (key == MESSAGE_KEY_BG_KEY) {
    persist_write_int(CONFIG_BG, new_tuple->value->uint32);
  }
  set_colors();
  layer_mark_dirty(s_points_layer);
  layer_mark_dirty(s_triangle_layer);
  layer_mark_dirty(s_hour_layer);
  layer_mark_dirty(s_minute_layer);
  layer_mark_dirty(s_second_layer);
}
#else
static GColor bg_color;
static GColor color;
#endif

// Scale a design-space length (in 144px-wide design units) to the current screen.
static int scaled(int design_value) {
  int base = s_center.x < s_center.y ? s_center.x * 2 : s_center.y * 2;
  int value = design_value * base / DESIGN_BASE;
  return value;
}

static void compute_geometry(GRect bounds) {
  s_center = GPoint(bounds.size.w / 2, bounds.size.h / 2);

  s_ring_r   = scaled(53);
  s_hour_r   = scaled(12);
  s_minute_r = scaled(10);
  s_second_r = scaled(8);
  s_hour_ir   = scaled(10);
  s_minute_ir = scaled(8);
  s_second_ir = scaled(5);
  s_point_r  = scaled(2);
  if (s_point_r < 1) { s_point_r = 1; }
  s_stroke   = scaled(3);
  if (s_stroke < 1) { s_stroke = 1; }

  // 12 evenly spaced points around the ring, point 0 at 12 o'clock (top).
  for (int i = 0; i < 12; i++) {
    int32_t angle = TRIG_MAX_ANGLE * i / 12;
    s_points[i].x = s_center.x + s_ring_r * sin_lookup(angle) / TRIG_MAX_RATIO;
    s_points[i].y = s_center.y - s_ring_r * cos_lookup(angle) / TRIG_MAX_RATIO;
  }
}

// Marker layers are padded a little larger than the circle they hold: a filled
// circle of radius r spans 2r+1 pixels, so a 2r layer would clip its bottom/right
// edge. The pad also leaves room for the antialiased stroke.
#define MARKER_PAD 2

static GRect marker_frame(GPoint at, int radius) {
  int half = radius + MARKER_PAD;
  return GRect(at.x - half, at.y - half, half * 2, half * 2);
}

static void trigger_hour_animation(bool force) {
  time_t temp = time(NULL);
  struct tm *t = localtime(&temp);
  int hour = t->tm_hour % 12;
  int minute = t->tm_min / 5;
  int second = t->tm_sec / 5;

  GRect second_from_frame = layer_get_frame(s_second_layer);
  GRect second_to_frame = marker_frame(s_points[second], s_second_r);
  s_second_animation = property_animation_create_layer_frame(s_second_layer, &second_from_frame, &second_to_frame);
  animation_schedule((Animation*) s_second_animation);

  if(force || t->tm_sec == 0){
    GRect minute_from_frame = layer_get_frame(s_minute_layer);
    GRect minute_to_frame = marker_frame(s_points[minute], s_minute_r);
    s_minute_animation = property_animation_create_layer_frame(s_minute_layer, &minute_from_frame, &minute_to_frame);
    animation_schedule((Animation*) s_minute_animation);
    if(force || t->tm_min == 0){
      GRect hour_from_frame = layer_get_frame(s_hour_layer);
      GRect hour_to_frame = marker_frame(s_points[hour], s_hour_r);
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
  graphics_context_set_stroke_width(ctx, s_stroke);
#endif
  graphics_context_set_fill_color(ctx, bg_color);
  graphics_fill_circle(ctx, center, s_hour_r);
#ifdef PBL_COLOR
  // Rim hugs the fill edge so the marker has a crisp outer border.
  graphics_draw_circle(ctx, center, s_hour_r - s_stroke / 2);
#else
  graphics_draw_circle(ctx, center, s_hour_ir);
#endif
};

static void minute_update_proc(Layer *this_layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(this_layer);
  GPoint center = GPoint(bounds.size.w / 2, (bounds.size.h / 2));
  graphics_context_set_fill_color(ctx, bg_color);
#ifdef PBL_COLOR
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, GColorLightGray);
  graphics_context_set_stroke_width(ctx, s_stroke);
#elif PBL_BW
  graphics_context_set_stroke_color(ctx, GColorWhite);
#endif
  graphics_fill_circle(ctx, center, s_minute_r);
#ifdef PBL_COLOR
  graphics_draw_circle(ctx, center, s_minute_r - s_stroke / 2);
#else
  graphics_draw_circle(ctx, center, s_minute_ir);
#endif
};
static void second_update_proc(Layer *this_layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(this_layer);
  GPoint center = GPoint(bounds.size.w / 2, (bounds.size.h / 2));
  graphics_context_set_fill_color(ctx, bg_color);
#ifdef PBL_COLOR
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_context_set_stroke_width(ctx, s_stroke);
#elif PBL_BW
  graphics_context_set_stroke_color(ctx, GColorWhite);
#endif
  graphics_fill_circle(ctx, center, s_second_r);
#ifdef PBL_COLOR
  graphics_draw_circle(ctx, center, s_second_r - s_stroke / 2);
#else
  graphics_draw_circle(ctx, center, s_second_ir);
#endif
};

static void triangle_update_proc(Layer *this_layer, GContext *ctx) {
  // Draw the triangle
#ifdef PBL_COLOR
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_width(ctx, s_stroke);
#endif
  graphics_context_set_fill_color(ctx, color);
  graphics_context_set_stroke_color(ctx, color);

  GRect bounds = layer_get_frame(s_second_layer);
  GPoint sec = grect_center_point(&bounds);
  bounds = layer_get_frame(s_minute_layer);
  GPoint minute = grect_center_point(&bounds);
  bounds = layer_get_frame(s_hour_layer);
  GPoint hour = grect_center_point(&bounds);

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
  graphics_context_set_stroke_width(ctx, s_stroke);
#endif
  graphics_context_set_fill_color(ctx, color);
  graphics_context_set_stroke_color(ctx, color);

  // The 12 hour pips around the ring.
  for(int i=0;i<12;i++){
    graphics_fill_circle(ctx, s_points[i], s_point_r);
  }
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  compute_geometry(window_bounds);

  // Create Layer
  s_points_layer = layer_create(window_bounds);
  layer_add_child(window_layer, s_points_layer);

  s_triangle_layer = layer_create(window_bounds);
  layer_add_child(window_layer, s_triangle_layer);

  s_hour_layer = layer_create(marker_frame(s_center, s_hour_r));
  layer_add_child(window_layer, s_hour_layer);
  s_minute_layer = layer_create(marker_frame(s_center, s_minute_r));
  layer_add_child(window_layer, s_minute_layer);
  s_second_layer = layer_create(marker_frame(s_center, s_second_r));
  layer_add_child(window_layer, s_second_layer);

  // Set the update_proc
  layer_set_update_proc(s_points_layer, points_update_proc);
  layer_set_update_proc(s_hour_layer, hour_update_proc);
  layer_set_update_proc(s_minute_layer, minute_update_proc);
  layer_set_update_proc(s_second_layer, second_update_proc);
  layer_set_update_proc(s_triangle_layer, triangle_update_proc);

#ifdef PBL_SDK_3
  set_colors();

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
      TupletInteger(MESSAGE_KEY_COLOR_KEY, col),
      TupletInteger(MESSAGE_KEY_BG_KEY, bg)
    };

  app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer),
      initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL
  );
#endif
  trigger_hour_animation(true);
}

static void prv_window_unload(Window *window) {
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

static void prv_init(void) {
#ifndef PBL_COLOR
  set_colors();
#endif
  // Create main Window
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  window_stack_push(s_main_window, true);
#ifdef PBL_SDK_3
  app_message_open(64, 64);
#endif
}

static void prv_deinit(void) {
  // Destroy main Window
  window_destroy(s_main_window);
#ifdef PBL_SDK_3
  app_sync_deinit(&s_sync);
#endif
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
