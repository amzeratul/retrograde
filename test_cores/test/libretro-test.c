/* vim: set et sw=3 ts=3 sts=3: */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795f
#endif

#include "libretro.h"

static uint32_t *frame_buf;
static struct retro_log_callback logging;
static retro_log_printf_t log_cb;
static bool use_audio_cb;
static float last_aspect;
static float last_sample_rate;
static bool analog_mouse = true;
static bool analog_mouse_relative = false;
static bool enable_audio = true;

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
   (void)level;
   va_list va;
   va_start(va, fmt);
   vfprintf(stderr, fmt, va);
   va_end(va);
}

void retro_init(void)
{
   frame_buf = calloc(320 * 240, sizeof(uint32_t));
}

void retro_deinit(void)
{
   free(frame_buf);
   frame_buf = NULL;
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   #define max_descriptions  10
   static struct retro_input_descriptor empty_input_descriptor[] = { { 0 } };
   struct retro_input_descriptor descriptions[max_descriptions+1] = {0}; /* set final record to nulls */
   struct retro_input_descriptor *needle = &descriptions[0];

   log_cb(RETRO_LOG_INFO, "Blanking existing controller descriptions.\n", device, port);
   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, empty_input_descriptor); /* is this necessary? it was in the sample code */

   log_cb(RETRO_LOG_INFO, "Plugging device %u into port %u.\n", device, port);

   switch(device)
   {
      case RETRO_DEVICE_NONE:
        break;
      case RETRO_DEVICE_LIGHTGUN:
        needle->port = port;  needle->device = RETRO_DEVICE_JOYPAD;  needle->index = 0;
        needle->id = RETRO_DEVICE_ID_LIGHTGUN_TRIGGER; needle->description = "Gun Trigger";
        needle++;
        needle->port = port;  needle->device = RETRO_DEVICE_JOYPAD;  needle->index = 0;
        needle->id = RETRO_DEVICE_ID_LIGHTGUN_RELOAD; needle->description = "Gun Reload";
        needle++;
        needle->port = port;  needle->device = RETRO_DEVICE_JOYPAD;  needle->index = 0;
        needle->id = RETRO_DEVICE_ID_LIGHTGUN_START; needle->description = "Gun Start";
        needle++;
        needle->port = port;  needle->device = RETRO_DEVICE_JOYPAD;  needle->index = 0;
        needle->id = RETRO_DEVICE_ID_LIGHTGUN_SELECT; needle->description = "Gun Select";
        needle++;
        break;
      case RETRO_DEVICE_JOYPAD: /* digital only controls */
        needle->port = port;  needle->device = RETRO_DEVICE_JOYPAD;  needle->index = 0;
        needle->id = RETRO_DEVICE_ID_JOYPAD_UP; needle->description = "DPad Up";
        needle++;
        needle->port = port;  needle->device = RETRO_DEVICE_JOYPAD;  needle->index = 0;
        needle->id = RETRO_DEVICE_ID_JOYPAD_DOWN; needle->description = "DPad Down";
        needle++;
        needle->port = port;  needle->device = RETRO_DEVICE_JOYPAD;  needle->index = 0;
        needle->id = RETRO_DEVICE_ID_JOYPAD_LEFT; needle->description = "DPad Left";
        needle++;
        needle->port = port;  needle->device = RETRO_DEVICE_JOYPAD;  needle->index = 0;
        needle->id = RETRO_DEVICE_ID_JOYPAD_RIGHT; needle->description = "DPad Right";
        needle++;
      default: /*one of the digital+analog retropads */
        needle->port = port;  needle->device = RETRO_DEVICE_JOYPAD;  needle->index = 0;
        needle->id = RETRO_DEVICE_ID_JOYPAD_UP; needle->description = "DPad Up";
        needle++;
        needle->port = port;  needle->device = RETRO_DEVICE_JOYPAD;  needle->index = 0;
        needle->id = RETRO_DEVICE_ID_JOYPAD_DOWN; needle->description = "DPad Down";
        needle++;
        needle->port = port;  needle->device = RETRO_DEVICE_JOYPAD;  needle->index = 0;
        needle->id = RETRO_DEVICE_ID_JOYPAD_LEFT; needle->description = "DPad Left";
        needle++;
        needle->port = port;  needle->device = RETRO_DEVICE_JOYPAD;  needle->index = 0;
        needle->id = RETRO_DEVICE_ID_JOYPAD_RIGHT; needle->description = "DPad Right";
        needle++;
        needle->port = port;  needle->device = RETRO_DEVICE_JOYPAD;  needle->index = 0;
        needle->id = RETRO_DEVICE_ID_JOYPAD_START; needle->description = "Digital Start";
        needle++;
        needle->port = port;  needle->device = RETRO_DEVICE_JOYPAD;  needle->index = 0;
        needle->id = RETRO_DEVICE_ID_JOYPAD_SELECT; needle->description = "Digital Select";
        needle++;
        break;
   }

   /* construct final zeroed record */
   needle->port = 0;  needle->device = 0;  needle->index = 0;
   needle->id = 0;    needle->description = NULL;

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, descriptions);
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "TestCore";
   info->library_version  = "v1";
   info->need_fullpath    = false;
   info->valid_extensions = NULL; // Anything is fine, we don't care.
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   float aspect = 4.0f / 3.0f;
   struct retro_variable var = { .key = "test_aspect" };
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "4:3"))
         aspect = 4.0f / 3.0f;
      else if (!strcmp(var.value, "16:9"))
         aspect = 16.0f / 9.0f;
   }

   float sampling_rate = 30000.0f;
   var.key = "test_samplerate";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      sampling_rate = strtof(var.value, NULL);

   info->timing = (struct retro_system_timing) {
      .fps = 60.0,
      .sample_rate = sampling_rate,
   };

   info->geometry = (struct retro_game_geometry) {
      .base_width   = 320,
      .base_height  = 240,
      .max_width    = 320,
      .max_height   = 240,
      .aspect_ratio = aspect,
   };

   last_aspect = aspect;
   last_sample_rate = sampling_rate;
}

static struct retro_rumble_interface rumble;

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   static const struct retro_variable vars[] = {
      { "test_aspect", "Aspect Ratio; 4:3|16:9" },
      { "test_samplerate", "Sample Rate; 30000|20000" },
      { "test_analog_mouse", "Left Analog as mouse; true|false" },
      { "test_analog_mouse_relative", "Analog mouse is relative; false|true" },
      { "test_audio_enable", "Enable Audio; true|false" },
      { NULL, NULL },
   };

   cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void*)vars);

   bool no_content = true;
   cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

   if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
      log_cb = logging.log;
   else
      log_cb = fallback_log;

   static const struct retro_subsystem_memory_info mem1[] = {{ "ram1", 0x400 }, { "ram2", 0x401 }};
   static const struct retro_subsystem_memory_info mem2[] = {{ "ram3", 0x402 }, { "ram4", 0x403 }};

   static const struct retro_subsystem_rom_info content[] = {
      { "Test Rom #1", "bin", false, false, true, mem1, 2, },
      { "Test Rom #2", "bin", false, false, true, mem2, 2, },
   };

   static const struct retro_subsystem_info types[] = {
      { "Foo", "foo", content, 2, 0x200, },
      { NULL },
   };

   cb(RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO, (void*)types);

   #define DUMMY1     RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG, 0)
   #define DUMMY2     RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG, 1)
   #define RETRO_GUN  RETRO_DEVICE_LIGHTGUN
   #define AUGMENTED  RETRO_DEVICE_JOYPAD

   #define NUMBER_OF_CONTROLS  2

   static const struct retro_controller_description controllers[] = {
      { "Dummy Controller #1", DUMMY1    },
      { "Dummy Controller #2", DUMMY2    },
      { "Lightgun",            RETRO_GUN },
      { "Augmented Joypad",    AUGMENTED }, // Test overriding generic description in UI.
   };

   static const struct retro_controller_info ports[] = {
      { controllers, 4 },
      { controllers, 4 },
      { NULL, 0 },
   };

   cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

static unsigned x_coord;
static unsigned y_coord;
static unsigned phase;
static int mouse_rel_x;
static int mouse_rel_y;

void retro_reset(void)
{
   x_coord = 0;
   y_coord = 0;
}

static void update_input(void)
{
   int port = 0;

   int dir_x = 0;
   int dir_y = 0;

   input_poll_cb();

   if (input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RETURN))
      log_cb(RETRO_LOG_INFO, "Return key is pressed!\n");

   if (input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_x))
      log_cb(RETRO_LOG_INFO, "x key is pressed!\n");

   for(port = 0; port < NUMBER_OF_CONTROLS; port++)
   {
      if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))
         dir_y--;
      if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))
         dir_y++;
      if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))
         dir_x--;
      if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT))
         dir_x++;

      int16_t mouse_x;
      int16_t mouse_y;

      bool mouse_l      = input_state_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
      bool mouse_r      = input_state_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);
      bool mouse_down   = input_state_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELDOWN);
      bool mouse_up     = input_state_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELUP);
      bool mouse_middle = input_state_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE);

      if (analog_mouse)
      {
         mouse_x = (320.0f / 32767.0f) * input_state_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
         mouse_y = (240.0f / 32767.0f) * input_state_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);

         if (analog_mouse_relative)
         {
            mouse_x /= 32;
            mouse_y /= 32;
         }
         else
         {
            mouse_x /= 2;
            mouse_y /= 2;

            mouse_x += 320 / 2;
            mouse_y += 240 / 2;
         }
      }
      else
      {
         mouse_x = input_state_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
         mouse_y = input_state_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
      }

      if (mouse_l)
         log_cb(RETRO_LOG_INFO, "Mouse #: %d     L pressed.   X: %d   Y: %d\n", port, mouse_x, mouse_y);
      if (mouse_r)
         log_cb(RETRO_LOG_INFO, "Mouse #: %d     R pressed.   X: %d   Y: %d\n", port, mouse_x, mouse_y);
      if (mouse_down)
         log_cb(RETRO_LOG_INFO, "Mouse #: %d     wheeldown pressed.   X: %d   Y: %d\n", port, mouse_x, mouse_y);
      if (mouse_up)
         log_cb(RETRO_LOG_INFO, "Mouse #: %d     wheelup pressed.     X: %d   Y: %d\n", port, mouse_x, mouse_y);
      if (mouse_middle)
         log_cb(RETRO_LOG_INFO, "Mouse #: %d     middle pressed.      X: %d   Y: %d\n", port, mouse_x, mouse_y);

      if ((analog_mouse && analog_mouse_relative) || !analog_mouse)
      {
         mouse_rel_x += mouse_x;
         mouse_rel_y += mouse_y;
      }
      else
      {
         mouse_rel_x = mouse_x;
         mouse_rel_y = mouse_y;
      }

      if (mouse_rel_x >= 310)
         mouse_rel_x = 309;
      else if (mouse_rel_x < 10)
         mouse_rel_x = 10;
      if (mouse_rel_y >= 230)
         mouse_rel_y = 229;
      else if (mouse_rel_y < 10)
         mouse_rel_y = 10;

      bool pointer_pressed = input_state_cb(port, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
      int16_t pointer_x = input_state_cb(port, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
      int16_t pointer_y = input_state_cb(port, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
      if (pointer_pressed)
         log_cb(RETRO_LOG_INFO, "Pointer Pressed #: %d    : (%6d, %6d).\n", port, pointer_x, pointer_y);

      dir_x += input_state_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X) / 5000;
      dir_y += input_state_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y) / 5000;
      dir_x += input_state_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X) / 5000;
      dir_y += input_state_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y) / 5000;

      x_coord = (x_coord + dir_x) & 31;
      y_coord = (y_coord + dir_y) & 31;

      if (rumble.set_rumble_state)
      {
         static bool old_start;
         static bool old_select;
         uint16_t strength_strong = input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2) ? 0x4000 : 0xffff;
         uint16_t strength_weak = input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2) ? 0x4000 : 0xffff;
         bool start = input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START);
         bool select = input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT);
         if (old_start != start)
            log_cb(RETRO_LOG_INFO, "Port #: %d   Strong rumble: %s.\n", port, start ? "ON": "OFF");
         rumble.set_rumble_state(port, RETRO_RUMBLE_STRONG, start * strength_strong);

         if (old_select != select)
            log_cb(RETRO_LOG_INFO, "Port #: %d   Weak rumble: %s.\n", port, select ? "ON": "OFF");
         rumble.set_rumble_state(port, RETRO_RUMBLE_WEAK, select * strength_weak);

         old_start = start;
         old_select = select;
      }

      int16_t lightgun_x = 0;
      int16_t lightgun_y = 0;
      bool trigger_pressed = false;

      lightgun_x      = input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X);
      lightgun_y      = input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y);
      trigger_pressed = input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_TRIGGER );

      if (trigger_pressed)
         log_cb(RETRO_LOG_INFO, "Lightgun Trigger Pressed #: %d   Lightgun X: %d   Lightgun Y: %d\n", port, lightgun_x, lightgun_y);   }
}

static void render_checkered(void)
{
   /* Try rendering straight into VRAM if we can. */
   uint32_t *buf = NULL;
   unsigned stride = 0;
   struct retro_framebuffer fb = {0};
   fb.width = 320;
   fb.height = 240;
   fb.access_flags = RETRO_MEMORY_ACCESS_WRITE;
   if (environ_cb(RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER, &fb) && fb.format == RETRO_PIXEL_FORMAT_XRGB8888)
   {
      buf = fb.data;
      stride = fb.pitch >> 2;
   }
   else
   {
      buf = frame_buf;
      stride = 320;
   }

   uint32_t color_r = 0xff << 16;
   uint32_t color_g = 0xff <<  8;

   uint32_t *line = buf;
   for (unsigned y = 0; y < 240; y++, line += stride)
   {
      unsigned index_y = ((y - y_coord) >> 4) & 1;
      for (unsigned x = 0; x < 320; x++)
      {
         unsigned index_x = ((x - x_coord) >> 4) & 1;
         line[x] = (index_y ^ index_x) ? color_r : color_g;
      }
   }

   for (unsigned y = mouse_rel_y - 5; y <= mouse_rel_y + 5; y++)
      for (unsigned x = mouse_rel_x - 5; x <= mouse_rel_x + 5; x++)
         buf[y * stride + x] = 0xff;

   video_cb(buf, 320, 240, stride << 2);
}

static void check_variables(void)
{
   struct retro_variable var = {0};

   var.key = "test_analog_mouse";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      analog_mouse = !strcmp(var.value, "true") ? true : false;
      log_cb(RETRO_LOG_INFO, "Key -> Val: %s -> %s.\n", var.key, var.value);
   }

   var.key = "test_analog_mouse_relative";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      analog_mouse_relative = !strcmp(var.value, "true") ? true : false;
      log_cb(RETRO_LOG_INFO, "Key -> Val: %s -> %s.\n", var.key, var.value);
   }

   var.key = "test_audio_enable";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      enable_audio = !strcmp(var.value, "true") ? true : false;
      log_cb(RETRO_LOG_INFO, "Key -> Val: %s -> %s.\n", var.key, var.value);
   }

   float last = last_aspect;
   float last_rate = last_sample_rate;
   struct retro_system_av_info info;
   retro_get_system_av_info(&info);

   if ((last != last_aspect && last != 0.0f) || (last_rate != last_sample_rate && last_rate != 0.0f))
   {
      // SET_SYSTEM_AV_INFO can only be called within retro_run().
      // check_variables() is called once in retro_load_game(), but the checks
      // on last and last_rate ensures this path is never hit that early.
      // last_aspect and last_sample_rate are not updated until retro_get_system_av_info(),
      // which must come after retro_load_game().
      bool ret;
      if (last_rate != last_sample_rate && last_rate != 0.0f) // If audio rate changes, go through SET_SYSTEM_AV_INFO.
         ret = environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &info);
      else // If only aspect changed, take the simpler path.
         ret = environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &info.geometry);
      log_cb(RETRO_LOG_INFO, "SET_SYSTEM_AV_INFO/SET_GEOMETRY = %u.\n", ret);
   }
}

static void audio_callback(void)
{
   if (!enable_audio)
      return;

   for (unsigned i = 0; i < 30000 / 60; i++, phase++)
   {
      int16_t val = 0x800 * sinf(2.0f * M_PI * phase * 300.0f / 30000.0f);
      audio_cb(val, val);
   }

   phase %= 100;
}

static void audio_set_state(bool enable)
{
   (void)enable;
}

void retro_run(void)
{
   update_input();
   render_checkered();
   if (!use_audio_cb)
      audio_callback();

   bool updated = false;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables();
}

static void keyboard_cb(bool down, unsigned keycode,
      uint32_t character, uint16_t mod)
{
   log_cb(RETRO_LOG_INFO, "Down: %s, Code: %d, Char: %u, Mod: %u.\n",
         down ? "yes" : "no", keycode, character, mod);
}


bool retro_load_game(const struct retro_game_info *info)
{
   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
      { 0 },
   };

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
      return false;
   }

   struct retro_keyboard_callback cb = { keyboard_cb };
   environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cb);
   if (environ_cb(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &rumble))
      log_cb(RETRO_LOG_INFO, "Rumble environment supported.\n");
   else
      log_cb(RETRO_LOG_INFO, "Rumble environment not supported.\n");

   struct retro_audio_callback audio_cb = { audio_callback, audio_set_state };
   use_audio_cb = environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK, &audio_cb);

   check_variables();

   (void)info;
   return true;
}

void retro_unload_game(void)
{
   last_aspect = 0.0f;
   last_sample_rate = 0.0f;
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   if (type != 0x200)
      return false;
   if (num != 2)
      return false;
   return retro_load_game(NULL);
}

size_t retro_serialize_size(void)
{
   return 2;
}

bool retro_serialize(void *data_, size_t size)
{
   if (size < 2)
      return false;

   uint8_t *data = data_;
   data[0] = x_coord;
   data[1] = y_coord;
   return true;
}

bool retro_unserialize(const void *data_, size_t size)
{
   if (size < 2)
      return false;

   const uint8_t *data = data_;
   x_coord = data[0] & 31;
   y_coord = data[1] & 31;
   return true;
}

void *retro_get_memory_data(unsigned id)
{
   (void)id;
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   (void)id;
   return 0;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

