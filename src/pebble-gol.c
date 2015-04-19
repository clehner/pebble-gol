#include <pebble.h>

#define WIDTH 144

#define ACCEL_STEP_MS 50
#define ACCEL_RATIO 0x1000

static Window *window;
static Layer *window_layer;

struct {
  uint8_t x;
  uint8_t y;
} game_scroll = {0, 0};

bool game_reset = false;

static void iterate(uint8_t *from, uint8_t *to, uint8_t len) {
  uint8_t carry = 0;
  for (int x = 0; x < len; x++) {
    uint16_t value = carry ^ from[x] ^ (from[x] << 1);
    to[x] = value;
    carry = value >> 8;
  }
}

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

/*
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  layer_mark_dirty(window_layer);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  copy_row(seed, last);
  iterate(last, seed, WIDTH / 8);
  layer_mark_dirty(window_layer);
}
*/

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  /*
  window_long_click_subscribe(BUTTON_ID_SELECT, 500,
      select_long_click_handler, NULL);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 30, up_click_handler);
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

  layer_mark_dirty(window_layer);
  app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
}

static void update_proc(Layer *this_layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(this_layer);
  GBitmap *buffer = graphics_capture_frame_buffer(ctx);
  if (!buffer) return;
  uint8_t height = bounds.size.h;
  uint8_t *data = gbitmap_get_data(buffer);
  uint8_t bytes_per_row = gbitmap_get_bytes_per_row(buffer);
  uint8_t (*fb_a)[bytes_per_row] = (uint8_t (*)[bytes_per_row])data;

  if (game_reset) {
    game_reset = false;
    for (uint8_t i = 0; i < height; i++)
      for (uint8_t j = 0; j < bytes_per_row; j++)
        fb_a[i][j] = rand() % 8;

  } else for (uint8_t i = 0; i < height; i++) {
    for (uint8_t j = 0; j < bytes_per_row; j++)
      fb_a[i][j] = i%2 == 0 ? 0b01010101 : 0b10101010;
  }

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
