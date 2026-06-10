#include <pmdsky.h>
#include <cot.h>
#include "extern.h"

// Special process 100: Change border color
// Based on https://github.com/SkyTemple/eos-move-effects/blob/master/example/process/set_frame_color.asm
static int SpChangeBorderColor(short arg1) {
  SetBothScreensWindowsColor(arg1);
  return 0;
}

static bool SpSetTextboxTransparency(short arg1) {
  if(arg1 == 0)
    TextboxSolid();
  else
    TextboxTransparent();
  return true;
}

static bool SpChangeTextboxColor(short color, short val) {
  if(color >= 0 && color <= 3) {
    uint8_t* color_buffer = (uint8_t*)&(TEXTBOX_COLOR_ATTRIBUTES);
    color_buffer[color] = val;
  }
  return true;
}

bool SpResetTextboxColor(void) {
  TEXTBOX_COLOR_ATTRIBUTES.r = 32;
  TEXTBOX_COLOR_ATTRIBUTES.g = 32;
  TEXTBOX_COLOR_ATTRIBUTES.b = 32;
  TEXTBOX_COLOR_ATTRIBUTES.a = 144;
  return true;
}

static int SpCreatePerformers(void) {
  struct performer_spawn performer_main = {
    .kind = 0x0,
    .direction = {0},
    .collision_box_size_x = 0x1,
    .collision_box_size_y = 0x1,
    .x = 0x10,
    .y = 0x25,
    .flags_x = 0x2,
    .flags_y = 0x2
  };
  struct performer_spawn performer_sub = {
    .kind = 0x1,
    .direction = {0},
    .collision_box_size_x = 0x1,
    .collision_box_size_y = 0x1,
    .x = 0x31,
    .y = 0xC,
    .flags_x = 0x2,
    .flags_y = 0x0
  };
  CreateLivePerformer(-1, &performer_main, 6, 0, false);
  CreateLivePerformer(-1, &performer_sub, 6, 0, false);
  return 581;
}

static bool SpSwapFont(struct script_routine* routine, short script_string_id, bool swap_unkno) {
  const char* script_string = GetSsbString(routine->states[0].ssb_info, script_string_id);
  SwapFont(script_string, swap_unkno);
  return true;
}

static bool SpChangeExtraFont(bool staffont) {
  if(staffont)
    LoadStaffont();
  else
    LoadMarkfont();
  return true;
}

// Called for special process IDs 100 and greater.
//
// Set return_val to the return value that should be passed back to the game's script engine. Return true,
// if the special process was handled.
bool CustomScriptSpecialProcessCall(struct script_routine* routine, uint32_t special_process_id, short arg1, short arg2, int* return_val) {
  switch (special_process_id) {
    case 100:
      *return_val = SpChangeBorderColor(arg1);
      return true;
    case 101:
      *return_val = SpSetTextboxTransparency(arg1);
      return true;
    case 102:
      *return_val = SpChangeTextboxColor(arg1, arg2);
      return true;
    case 103:
      *return_val = SpResetTextboxColor();
      return true;
    case 104:
      *return_val = SpCreatePerformers();
      return true;
    case 105:
      *return_val = SpSwapFont(routine, arg1, arg2);
      return true;
    case 106:
      *return_val = last_selected_scene;
      return true;
    case 107:
      last_selected_scene++;
      *return_val = 0;
      return true;
    case 108:
      *return_val = SpChangeExtraFont(arg1);
      return true;
    default:
      return false;
  }
}
