#include <pmdsky.h>
#include <cot.h>
#include "extern.h"

const char MOTTO[] = "なんでもできる";
const char SOUND_BGM_PATH[] = "SOUND/BGM";
const char DECOI_BRANCH_PREFIXES[3] = {'E', 'C', 'O'};

int participant_popup_timer = -1;
int participant_popup_window_id = -1;

uint16_t text_numerator = 1;
uint16_t text_denominator = 1;
uint16_t text_count = 0;

enum decoi_branch selected_branch = BRANCH_EXPLORATION;

int GetParticipantTextStringId(int scene) {
    int text_string_id = (TEXT_STRING_PARTICIPANT_NAME_START+scene) + (selected_branch * TOTAL_SCENES_PER_BRANCH);
    return text_string_id;
}

void ResetTextSpeedValues(void) {
	text_numerator = 1;
	text_denominator = 1;
	text_count = 0;
}

/*
	Creates the participant popup window that shows when a scene begins only when selecting Play All Scenes.
*/
void CreateParticipantPopup(void) {
	int scene_number = last_selected_scene;
	char* participant_string;
	char popup_message[0x200];
	if(scene_number == 0)
		strncpy(popup_message, "[UNK:0][FT:1][CN]Ｉｎｉｔｉａｌ　Ｓｃｅｎｅ\n[UNK:1][FT:0][CN]", sizeof(popup_message));
	else
		sprintf(popup_message, "[UNK:0][FT:1][CN]Ｓｃｅｎｅ　%d\n[UNK:1][FT:0][CN]", scene_number);
	struct window_params window_params = { .x_offset = 0x1, .y_offset = 0x1, .width = 0x1E, .height = 0x4, .screen = {SCREEN_MAIN}, .box_type = {0xFC} };
	struct preprocessor_flags preprocessor_flags = {.flags_1 = 0b000000010, .timer_2 = true};
	participant_string = StringFromId(GetParticipantTextStringId(scene_number));
	// This never really runs for MMR, but whatever, leftover code go brr
	for(int i = 0; i < strlen(participant_string); i++) {
		if(participant_string[i] == '\n')
			participant_string[i] = ' ';
	}
	strcat(popup_message, participant_string);
	if(participant_popup_timer < 0)
		participant_popup_window_id = CreateDialogueBox(&window_params);
	ShowStringInDialogueBox(participant_popup_window_id, preprocessor_flags, popup_message, NULL);
	participant_popup_timer = 480;
}

/*
	Hijacks the scene name of "DECOI" to instead load a scene name based off the result of Message Menu 80.
	Also resets some shared variables to their default values.
*/
__attribute((used)) void CustomGetActingSceneName(char* truncated_scene_name, char* full_scene_name) {
	GetSceneName(truncated_scene_name, full_scene_name);
	if(strncmp(truncated_scene_name, "DECOI", 8) != 0)
		return;
	if(last_selected_scene == 0)
		strncpy(truncated_scene_name, "initial", 8);
	else
		snprintf(truncated_scene_name, 8, "%02d", last_selected_scene);
		
	#if EVENT_FINISHED
	strncat(truncated_scene_name, DECOI_BRANCH_PREFIXES+selected_branch, 1);
	#endif
	
	SetPerformanceFlagWithChecks(62, 0);
	ResetTextSpeedValues();
	// Reset the SCENARIO_SUB variables...
	// SkyTemple's debugger actually raises errors for out-of-bounds indexing, so just play nicely.
	uint8_t* buffer = (uint8_t*)&(DIALOGUE_BOX_DEFAULT_WINDOW_PARAMS.x_offset);
	for(int i = 0; i < 3; i++) {
		for(int j = 0; j < 2; j++) {
			SaveScriptVariableValueAtIndex(NULL, VAR_SCENARIO_SUB1+i, j, *buffer);
			buffer++;
		}
	}
	if(playing_all_scenes && last_selected_scene > 0)
		CreateParticipantPopup();
}

/*
	Hijacks a call to CreateDialogueBox to use an entirely new set of window_params, if PERFORMANCE_PROGRESS_LIST[62] is set.
*/
__attribute((used)) int CustomCreateDialogueBox(struct window_params* window_params) {
	struct window_params new_params;
	if (GetPerformanceFlagWithChecks(62)) {
		uint8_t* buffer = (uint8_t*)&(new_params.x_offset);
		for(int i = 0; i < 6; i++)
			buffer[i] = LoadScriptVariableValueAtIndex(NULL, VAR_SCENARIO_SUB1, i); // Intentional out-of-bounds indexing, yay!
		window_params = &new_params;
	}
	return CreateDialogueBox(window_params);
}

/*
	Hijacks a call to CreatePortraitBox to place a portrait loaded by a script command on the Top Screen, if PERFORMANCE_PROGRESS_LIST[62] is set.
	Please note that framing doesn't seem to work for portraits on the Top Screen.
*/
__attribute((used)) int CustomCreatePortraitBox(enum screen screen, uint32_t palette_idx, bool framed) {
	enum screen new_screen = screen;
	if(GetPerformanceFlagWithChecks(62))
		new_screen = LoadScriptVariableValueAtIndex(NULL, VAR_SCENARIO_SUB3, 0);
	return CreatePortraitBox(new_screen, palette_idx, framed);
}

/*
	Changes a currently-loaded font.
		- "kanji_rd.dat" is the main font in NA EoS.
		- "unkno_rd.dat" is the Unown font in NA EoS.
		
	Any arbitrary font binary can be loaded, so long as it's formatted like the above two files.
*/
void SwapFont(const char* filepath, bool swap_unkno) {
	struct iovec iov;
	void** base = NULL;
	void** data = NULL;
	if(swap_unkno) {
		base = &(FONT_DATA.unkno_rd_base);
		data = &(FONT_DATA.unkno_rd_data);
	}
	else {
		base = &(FONT_DATA.kanji_rd_base);
		data = &(FONT_DATA.kanji_rd_data);
	}
	MemFree(*base);
	LoadFileFromRom(&iov, filepath, 1);
	*base = iov.iov_base;
	*data = (void*)((uint32_t)(iov.iov_base) + 0x4);
}

/*
	Parses custom uppercase text tags.
		- "VS:X:Y" ("VITESSE") modifies text speed by X/Y. For example, "[VS:1:2]" halves speed, but "[VS:4]" quadruples it. The second parameter is optional, and if missing, will default to 1.
		- "VR" reverts text speed to normal (equivalent to "[VS:1:1]" and "[VS:1]").
		- "U:X" ("UNLOCK") unlocks the the Xth scripting lock.
		- "UNK" swaps the unkno_rd.dat font for kanji_jp.dat.
		
	To ignore a text tag in a textbox that doesn't scroll, check for dialogue_display_state::flags.timer_2.
*/
__attribute((used)) bool ParseCustomUppercaseTextTags(struct dialogue_display_state* state, const char* tag, const char** tag_params, int tag_param_count) {
	int tag_vals[4] = {0};
	if(tag_param_count > ARRAY_LENGTH(tag_vals))
		tag_param_count = ARRAY_LENGTH(tag_vals);
		
	for(int i = 0; i < tag_param_count; i++)
		tag_vals[i] = AtoiTag(tag_params[i]);
		
	// Checking for an actual tag...
	if(StrcmpTag(tag, "VS")) {
		if(state->flags.timer_2)
			return true;
		if(tag_param_count > 0) {
			text_numerator = tag_vals[0];
			text_denominator = tag_param_count == 1 ? 1 : tag_vals[1];
			text_count = 0;
			state->text_scrolling_done = 0;
		}
		return true;
	}
	else if(StrcmpTag(tag, "VR")) {
		if(state->flags.timer_2)
			return true;
		ResetTextSpeedValues();
		state->text_scrolling_done = 0;
		return true;
	}
	else if(StrcmpTag(tag, "UNK")) {
		if(tag_param_count > 0) {
			char* font = tag_vals[0] == 0 ? "FONT/kanji_jp.dat" : "FONT/unkno_rd.dat";
			SwapFont(font, true);
		}
		return true;
	}
	else if(StrcmpTag(tag, "U")) {
		if(state->flags.timer_2)
			return true;
		if(OverlayIsLoaded(OGROUP_OVERLAY_11))
			UnlockScriptingLock(tag_vals[0]);
		return true;
	}
	return false;
}

/*
	Parses custom lowercase text tags.
		- "love" returns "なんでもできる！"
		
	Returns NULL upon error, otherwise a char buffer (which may either be hardcoded, or copied into the "buf" param).
*/
__attribute((used)) char* ParseCustomLowercaseTextTags(char* buf, const char* tag, const char** tag_params, int tag_param_count) {
	int tag_vals[4] = {0};
	if(tag_param_count > ARRAY_LENGTH(tag_vals))
		tag_param_count = ARRAY_LENGTH(tag_vals);
		
	for(int i = 0; i < tag_param_count; i++)
		tag_vals[i] = AtoiTag(tag_params[i]);
		
	if(StrcmpTag(tag, "love")) {
		strncpy(buf, MOTTO, sizeof(MOTTO));
		return buf;
	}
	return NULL;
}

/*
	Some generic function that runs every frame in Ground Mode.
*/
__attribute((used)) void YouCanDoAnything(void) {
	PlayTimerTickWrapper();
	if(participant_popup_timer >= 0) {
        if(participant_popup_timer == 0 && participant_popup_window_id >= 0) {
            CloseDialogueBox(participant_popup_window_id);
			participant_popup_window_id = -1;
		}
        participant_popup_timer--;
    }
}

/*
	Hijack some custom exceptions for Text Strings.
	Mostly used in an options menu to cleanly insert a number, rather than having a bunch of Text Strings w/ numbers.
*/
__attribute((used)) void CustomGetStringFromFile(char* buf, int string_id) {
	if(TEXT_STRING_HIJACK_OPTIONS_MENU <= string_id && string_id <= TEXT_STRING_HIJACK_OPTIONS_MENU+TOTAL_SCENES_PER_BRANCH)
		sprintf(buf, "%d", string_id-TEXT_STRING_HIJACK_OPTIONS_MENU);
	else
		GetStringFromFile(buf, string_id);
}

/*
	A function running before FileOpenInner gets called.
	Mostly used to distinguish between branch-specific files.
	Note: Does not affect LoadFileFromRom.
*/
__attribute((used)) bool HijackFileOpenInner(struct file_stream* file, char* filepath) {
	#if EVENT_FINISHED
	char filepath_hijack[0x100] = {0};
	if(strncmp(filepath, SOUND_BGM_PATH, sizeof(SOUND_BGM_PATH)-1) == 0) {
		strncpy(filepath_hijack, SOUND_BGM_PATH, sizeof(SOUND_BGM_PATH)-1);
		char* current_path = filepath_hijack + sizeof(SOUND_BGM_PATH)-1;
		// Assume we've selected the Exploration branch
		// If loading "SOUND/BGM/bgm.swd", load "SOUND/BGME/bgm.swd" instead
		*current_path = DECOI_BRANCH_PREFIXES[selected_branch];
		current_path++;
		strcpy(current_path, filepath + sizeof(SOUND_BGM_PATH)-1);
		filepath = filepath_hijack;
	}
	#endif
	return FileOpenInner(file, filepath);
}

/*
	Text speed shenanigans adapted from Irdkwia's "TestSpeed" patch. 
*/
__attribute((used)) uint32_t TryChangeTextSpeed(struct dialogue_display_state* state) {
	unsigned long long result = _s32_div_f((state->text_speed * text_numerator + text_count), text_denominator);
	text_count = result >> 32;
	return (uint32_t)result;
}

__attribute((used)) uint16_t GetExtraFontNumberIndex(char symbol) {
	return ('0' <= symbol && symbol <= '9') ? (0x40 + (symbol-'0')) : 0;
}

__attribute((naked)) void HijackTextSpeed(void) {
	asm("mov r0,r4");
	asm("b TryChangeTextSpeed");
}

__attribute((naked)) void HijackTextLoop(void) {
	asm("ldr r0,[r4,#0x80]");
	asm("cmp r0,#0x0");
	asm("bxne r14");
	// Per the original comment: "Here is the fun part"
	asm("mov r0,#0x0");
	asm("b AnalyzeTextReturn");
}

/*
	Just a little bit of setup for allowing numbers in one credits font...
*/
__attribute((naked)) void TryParseExtraFontNumber(void) {
	asm("push {r0-r2}");
	asm("mov r0,r3");
	asm("bl GetExtraFontNumberIndex");
	asm("movs r3,r0");
	asm("pop {r0-r2}");
	asm("bne ExtraFontDoRendering");
	asm("b ExtraFontReturnSix");
}
