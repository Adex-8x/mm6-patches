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


__attribute((naked)) void InvalidateCache(void) {
  asm volatile("stmdb r13!,{r14}");
	// Bonjour, est-ce que vous connaissez le C-A-C-H-E ?
	asm volatile("bl InvalidateInstructionCache");
	asm volatile("bl InvalidateAndCleanDataCache");
	asm volatile("mov  r0,#0x0");
	asm volatile("mcr p15,0,r0,c7,c5,4");
	asm volatile("mcr p15,0,r0,c7,c5,6");
	asm volatile("mcr p15,0,r0,c7,c0,4");
	asm volatile("ldmia r13!,{r15}");
}

static bool SpLoadCode(struct script_routine* routine, short script_string_id) {
  // This loads code into the scratch area, using hex representation
  // e.g. "0000A0E1" string encodes single instruction "mov r0,r0"
  const char* script_string = GetSsbString(routine->states[0].ssb_info, script_string_id);
  InvalidateCache();
  int i = 0;
  int* p = (int*)0x23D7FF0;
  int n = 0;
  while (script_string[0]!=0) {
    int c = 0;
    if (script_string[0]-'a'>=0) {
      c = script_string[0]-'a'+10;
    } else if (script_string[0]-'A'>=0) {
      c = script_string[0]-'A'+10;
    } else if (script_string[0]-'0'>=0) {
      c = script_string[0]-'0';
    }
    c &= 0xF;
    n |= c << ((i^1)<<2);
    ++script_string;
    ++i;
    if (i==8) {
      *p = n;
      ++p;
      i = 0;
      n = 0;
    }
  }
  if (i!=0) {
    *p = n;
  }
  InvalidateCache();
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
    case 110:
      *return_val = SpLoadCode(routine, arg1);
      return true;
    default:
      return false;
  }
}
