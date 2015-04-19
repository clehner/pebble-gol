#include <pebble.h>

#define WIDTH 144

#define ACCEL_STEP_MS 250
#define ACCEL_RATIO 0x1000

static Window *window;
static Layer *window_layer;

struct {
  uint8_t x;
  uint8_t y;
} game_scroll = {0, 0};

bool game_reset = false;
bool game_clear = false;

static void reset_game() {
  game_reset = true;
}

/*
static void copy_row(uint8_t *from, uint8_t *to) {
  for (int i = 0; i < WIDTH / 8; i++)
    to[i] = from[i];
}
*/

/*
static void select_long_click_handler(ClickRecognizerRef recognizer,
    void *context) {
  layer_mark_dirty(window_layer);
}
*/

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  reset_game();
  layer_mark_dirty(window_layer);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  game_clear = true;
  layer_mark_dirty(window_layer);
}

/*
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  copy_row(seed, last);
  iterate(last, seed, WIDTH / 8);
  layer_mark_dirty(window_layer);
}
*/

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  /*
  window_long_click_subscribe(BUTTON_ID_SELECT, 500,
      select_long_click_handler, NULL);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 30,
      down_click_handler);
  */
}

/*
static void game_update(uint8_t *data, uint8_t row_size, uint8_t height) {
}
*/

static void timer_callback(void *data) {
  AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };
  accel_service_peek(&accel);

  game_scroll.x += accel.x / ACCEL_RATIO;
  game_scroll.y += accel.y / ACCEL_RATIO;

  app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
  layer_mark_dirty(window_layer);
}

static void update_proc(Layer *this_layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(this_layer);
  GBitmap *buffer = graphics_capture_frame_buffer(ctx);
  if (!buffer) return;
  int8_t height = bounds.size.h;
  int8_t width = bounds.size.w;
  uint8_t *data = gbitmap_get_data(buffer);
  uint8_t bytes_per_row = gbitmap_get_bytes_per_row(buffer);
  uint8_t (*fb_a)[bytes_per_row] = (uint8_t (*)[bytes_per_row])data;

#define cell(i, j) \
  (((i) < 0 || (j) < 0) ? 1 : \
   ((i) >= (uint8_t)height || (j) >= (uint8_t)width) ? 1 :\
  ((fb_a[(i)][(uint8_t)(j)/8] & (1 << ((uint8_t)(j) % 8))) != 0))
  /*
  ((fb_a[(i) < 0 ? (i) + height : (i) >= height ? (i) - height : (i)]\
   [((j) < 0 ? (j) + width : (j) >= width ? (j) - width : (j)) / 8]\
   & (1 << (((j) < 0 ? (j) + width : (j)) % 8))) ? 1 : 0)
   */

  if (game_clear) {
    game_clear = false;
    for (uint8_t i = 0; i < (uint8_t)height; i++)
      for (uint8_t j = 0; j < bytes_per_row; j++)
        fb_a[i][j] = 0;

  } else if (game_reset) {
    game_reset = false;
    for (uint8_t i = 0; i < (uint8_t)height; i++)
      for (uint8_t j = 0; j < bytes_per_row; j++)
        fb_a[i][j] = rand() & 0xff;

  } else for (int16_t i = 0; i < (uint8_t)height; i++) {
    for (int16_t j = 0; j < (uint8_t)width; j++) {
      int8_t num_neighbors =
        cell(i-1, j-1) + cell(i, j-1) + cell(i+1, j-1) +
        cell(i-1, j  ) +                cell(i+1, j  ) +
        cell(i-1, j+1) + cell(i, j+1) + cell(i+1, j+1);
      uint8_t bitmask = 1 << ((uint8_t)j % 8);
      uint8_t live = fb_a[i][(uint8_t)j / 8] & bitmask;
      if (live ?
          !(num_neighbors == 2 || num_neighbors == 3) :
          (num_neighbors == 3))
        fb_a[i][(uint8_t)j / 8] ^= bitmask;
    }
  }
#undef cell

  graphics_release_frame_buffer(ctx, buffer);
}

static void window_load(Window *window) {
  window_layer = window_get_root_layer(window);
  layer_set_update_proc(window_layer, update_proc);
}

static void window_unload(Window *window) {
}

static void init(void) {
  reset_game();

  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
  app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
