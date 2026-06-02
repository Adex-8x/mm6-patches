#include <pmdsky.h>
#include <cot.h>
#include "extern.h"

#define MAIN_SIMPLE_MENU GLOBAL_MENU_INFO.window_ids[0]
#define MAIN_OPTIONS_MENU GLOBAL_MENU_INFO.window_ids[3]

#define SCENE_SELECTOR_ADV_MENU GLOBAL_MENU_INFO.window_ids[0]
#define SCENE_SELECTOR_PARTICIPANT_LISTING GLOBAL_MENU_INFO.window_ids[1]
#define SCENE_SELECTOR_TOP_CHART GLOBAL_MENU_INFO.window_ids[2]
#define SCENE_SELECTOR_YESNO_MENU GLOBAL_MENU_INFO.window_ids[3]

SECTION_DATA_PLAYBILL struct preprocessor_flags PREPROC_INSTANT_NO_INPUT = {.flags_1 = 0b000000010, .timer_2 = true}; // Instant text without waiting for any input!
SECTION_DATA_PLAYBILL char SCENE_FORMATTER[] = "[M:S3] Scene %d";
SECTION_DATA_PLAYBILL char SCENE_INITIAL[] = "[M:S3] Initial";

SECTION_DATA_PLAYBILL uint16_t SCENE_CHOICES_STRING_IDS[TOTAL_SCENES+1];
SECTION_DATA_PLAYBILL int SCENE_ITEM_STATES[2];

SECTION_TEXT_PLAYBILL void SetPlaybillColors(void) {
    TEXTBOX_COLOR_ATTRIBUTES.r = 0xAA;
    TEXTBOX_COLOR_ATTRIBUTES.g = 0x34;
    TEXTBOX_COLOR_ATTRIBUTES.b = 0xA8;
    TextboxTransparent();
    SetBothScreensWindowsColor(3);
}

// Playbill (Scene Selector)

SECTION_TEXT_PLAYBILL char* SceneOptionEntryFn(char* buffer, int option_id) {
    if(option_id != 0)
        sprintf(buffer, SCENE_FORMATTER, option_id);
    else
        return SCENE_INITIAL;
    return buffer;
}

SECTION_TEXT_PLAYBILL void CreateCustomControlsChart(int text_string_id) {
    struct window_params topchart_params = { .x_offset = 0x3, .y_offset = 0x9, .width = 0x1A, .height = 0x9, .screen = {SCREEN_SUB}, .box_type = {0xFA} };
    struct window_flags topchart_flags;
    SCENE_SELECTOR_TOP_CHART = CreateControlsChart(&topchart_params, topchart_flags, NULL, text_string_id);
}

SECTION_TEXT_PLAYBILL void CreatePlaybill(void) {
    int option_id = last_selected_scene >= 0 && last_selected_scene < TOTAL_SCENES ? last_selected_scene : 0;
    struct window_params menu_params = { .x_offset = 2, .y_offset = 3, .box_type = {0xFC} };
    struct window_params dbox_params = { .x_offset = 0x9, .y_offset = 0x12, .width = 0x15, .height = 0x4, .box_type = {0xFC} };
    struct window_flags menu_flags = { .a_accept = true, .se_on = true, .set_choice = true, .partial_menu = true, .menu_title = true, .menu_lower_bar = true, .no_accept_button = false };
    #if EVENT_FINISHED
    menu_flags.b_cancel = true;
    #endif
    struct window_extra_info menu_info = {.set_choice_id = option_id, .title_string_id = TEXT_STRING_SCENE_SELECTOR_TITLE, .title_height = 0x10 };
    int menu_options = TOTAL_SCENES; int menu_options_pp = 5;
    SetPlaybillColors();
    SCENE_SELECTOR_ADV_MENU = CreateAdvancedMenu(&menu_params, menu_flags, &menu_info, SceneOptionEntryFn, menu_options, menu_options_pp);
    SCENE_SELECTOR_PARTICIPANT_LISTING = CreateDialogueBox(&dbox_params);
    CreateCustomControlsChart(TEXT_STRING_FLAVOR_BLURB);
    ShowStringIdInDialogueBox(SCENE_SELECTOR_PARTICIPANT_LISTING, PREPROC_INSTANT_NO_INPUT, TEXT_STRING_PARTICIPANT_NAME_START+option_id, NULL);
    GLOBAL_MENU_INFO.previous_option = option_id;
}

SECTION_TEXT_PLAYBILL void CreateYesNoSubMenu(void) {
    struct window_params menu_params = { .x_offset = 0x14, .y_offset = 3, .width = 0xA, .box_type = {0xFC} };
    struct window_flags menu_flags = { .a_accept = true, .b_cancel = true, .se_on = true, .menu_title = true, .menu_lower_bar = true, .no_accept_button = true};
    struct window_extra_info menu_info = {.title_height = 0x10, .title_string_id = TEXT_STRING_PLAY_SCENE_QUESTION};
    struct simple_menu_id_item simple_options[3];
    for(int i = 0; i < 2; i++) {
        simple_options[i].string_id = i+422;
        simple_options[i]._padding = 0;
        simple_options[i].result_value = i+1;
    }
    simple_options[2].string_id = NULL;
    simple_options[2]._padding = NULL;
    simple_options[2].result_value = NULL;
    SCENE_SELECTOR_YESNO_MENU = CreateSimpleMenuFromStringIds(&menu_params, menu_flags, &menu_info, simple_options, 3);
}

SECTION_TEXT_PLAYBILL void ClosePlaybill(void) {
    playing_all_scenes = false; // Just to be extra sure lol
    if(SCENE_SELECTOR_ADV_MENU >= 0)
        CloseAdvancedMenu(SCENE_SELECTOR_ADV_MENU);
    if(SCENE_SELECTOR_PARTICIPANT_LISTING >= 0)
        CloseDialogueBox(SCENE_SELECTOR_PARTICIPANT_LISTING);
    if(SCENE_SELECTOR_TOP_CHART >= 0)
        CloseControlsChart(SCENE_SELECTOR_TOP_CHART);
    if(SCENE_SELECTOR_YESNO_MENU >= 0)
        CloseSimpleMenu(SCENE_SELECTOR_YESNO_MENU);
    TextboxSolid();
    SpResetTextboxColor();
    SetBothScreensWindowsColor(2);
}

SECTION_TEXT_PLAYBILL bool UpdateSceneSelectorMenu(void) {
    int main_menu_id = SCENE_SELECTOR_ADV_MENU;
    int sub_menu_id = SCENE_SELECTOR_YESNO_MENU;
    int result;
    int current_menu_option;
    switch(GLOBAL_MENU_INFO.state) {
        case 0:;
            // Initial state; check for main scene select menu activity...
            if(!IsAdvancedMenuActive2(main_menu_id)) {
                result = GetAdvancedMenuResult(main_menu_id);
                GLOBAL_MENU_INFO.menu_results[0] = result;
                if(result >= 0)
                    GLOBAL_MENU_INFO.state++;
                else {
                    GLOBAL_MENU_INFO.return_val = -1;
                    return true;
                }
            }
            // If active, change the bottom window to show participants' names!
            else {
                current_menu_option = GetAdvancedMenuCurrentOption(main_menu_id);
                if(current_menu_option != GLOBAL_MENU_INFO.previous_option) {
                    ShowStringIdInDialogueBox(SCENE_SELECTOR_PARTICIPANT_LISTING, PREPROC_INSTANT_NO_INPUT, TEXT_STRING_PARTICIPANT_NAME_START+current_menu_option, NULL);
                    GLOBAL_MENU_INFO.previous_option = current_menu_option;
                }
            }
            break;
        case 1:;
            // smol state, just create the yes/no submenu and go on
            CreateYesNoSubMenu();
            GLOBAL_MENU_INFO.state++;
            break;
        case 2:;
            // Yes/no submenu activity check...
            if(!IsSimpleMenuActive(sub_menu_id)) {
                result = GetSimpleMenuResult(sub_menu_id);
                CloseSimpleMenu(sub_menu_id);
                SCENE_SELECTOR_YESNO_MENU = -1;
                if(result == 1) {
                    // If "yes" pressed, save the result and quit
                    result = GLOBAL_MENU_INFO.menu_results[0];
                    last_selected_scene = result;
                    GLOBAL_MENU_INFO.return_val = result;
                    return true;
                }
                else {
                    // If "no" pressed or exited otherwise, go back to the initial state
                    ResumeAdvancedMenu(main_menu_id);
                    GLOBAL_MENU_INFO.state = 0;
                }
            }
            break;
            
    }
    return false;
}

// Envelope (Main Menu)
SECTION_TEXT_PLAYBILL void CreateSceneStarterMenu(void) {
    struct window_params menu_params = { .x_offset = 0x12, .y_offset = 3, .box_type = {0xFC} };
    struct window_params dbox_params = { .x_offset = 0x9, .y_offset = 0x12, .width = 0x15, .height = 0x4, .box_type = {0xFC} };
    struct window_flags menu_flags = {.a_accept = true, .b_cancel = true, .se_on = true, .menu_title = true, .menu_lower_bar = true, .no_up_down = true, .invisible_cursor = true};
    struct window_extra_info menu_info = {.title_string_id = TEXT_STRING_SCENE_STARTER_TITLE, .title_height = 0x10};
    for(int i = 0; i < TOTAL_SCENES; i++)
        SCENE_CHOICES_STRING_IDS[i] = i+TEXT_STRING_SCENE_STARTER_CHOICES;
    SCENE_CHOICES_STRING_IDS[TOTAL_SCENES] = 0;
    struct options_menu_id_item option_items[2] = {
        {.string_id = TEXT_STRING_SCENE_STARTER_DESC, ._padding = 0, .n_choices = 0, .choices = SCENE_CHOICES_STRING_IDS},
        {.string_id = 0, ._padding = 0, .n_choices = 0, .choices = NULL}
    };
    SCENE_ITEM_STATES[0] = 0;
    SCENE_ITEM_STATES[1] = 0;
    MAIN_OPTIONS_MENU = CreateOptionsMenu(&menu_params, menu_flags, &menu_info, option_items, 2, SCENE_ITEM_STATES);
    SCENE_SELECTOR_PARTICIPANT_LISTING = CreateDialogueBox(&dbox_params);
    ShowStringIdInDialogueBox(SCENE_SELECTOR_PARTICIPANT_LISTING, PREPROC_INSTANT_NO_INPUT, TEXT_STRING_PARTICIPANT_NAME_START, NULL);
}

SECTION_TEXT_PLAYBILL void CreateEnvelope(void) {
    last_selected_scene = 0x0;
    struct window_params menu_params = { .x_offset = 2, .y_offset = 3, .box_type = {0xFF} };
    struct window_flags menu_flags = { .a_accept = true, .se_on = true, .menu_title = true, .partial_menu = true};
    struct window_extra_info menu_info = {.title_string_id = TEXT_STRING_MAIN_MENU_TITLE, .title_height = 0x10};
    struct simple_menu_id_item simple_options[4] = {0};
    for(int i = 0; i < ARRAY_LENGTH(simple_options)-1; i++) {
        simple_options[i].string_id = i+TEXT_STRING_MAIN_MENU_OPTION_NAME_START;
        simple_options[i]._padding = 0;
        simple_options[i].result_value = i+1;
    }
    SetPlaybillColors();
    CreateCustomControlsChart(TEXT_STRING_MYSTERYMAIL_BLURB);
    MAIN_SIMPLE_MENU = CreateSimpleMenuFromStringIds(&menu_params, menu_flags, &menu_info, simple_options, ARRAY_LENGTH(simple_options));
    GLOBAL_MENU_INFO.previous_option = 0;
}

SECTION_TEXT_PLAYBILL void CloseEnvelope(void) {
    playing_all_scenes = GLOBAL_MENU_INFO.return_val == 1;
    if(MAIN_SIMPLE_MENU >= 0)
        CloseSimpleMenu(MAIN_SIMPLE_MENU);
    if(MAIN_OPTIONS_MENU >= 0)
        CloseOptionsMenu(MAIN_OPTIONS_MENU);
    if(SCENE_SELECTOR_PARTICIPANT_LISTING >= 0)
        CloseDialogueBox(SCENE_SELECTOR_PARTICIPANT_LISTING);
    if(SCENE_SELECTOR_TOP_CHART >= 0)
        CloseControlsChart(SCENE_SELECTOR_TOP_CHART);
    TextboxSolid();
    SpResetTextboxColor();
    SetBothScreensWindowsColor(2);
}

SECTION_TEXT_PLAYBILL bool UpdateMysteryMailMenu(void) {
    #if EVENT_FINISHED
    int main_menu_id = MAIN_SIMPLE_MENU;
    int sub_menu_id = MAIN_OPTIONS_MENU;
    int result;
    int current_menu_option;
    switch(GLOBAL_MENU_INFO.state) {
        case 0:;
            // Initial state; your standard menu
            if(!IsSimpleMenuActive(main_menu_id)) {
                result = GetSimpleMenuResult(main_menu_id);
                if(result == 1) {
                    // Open a submenu; which scene to start from?
                    GLOBAL_MENU_INFO.state++;
                }
                else {
                    // Welp we're done here
                    GLOBAL_MENU_INFO.return_val = result;
                    return true;
                }
            }
            break;
        case 1:;
            // Transitional state; just create the options menu
            CreateSceneStarterMenu();
            GLOBAL_MENU_INFO.state++;
            break;
        case 2:;
            // Options menu; spooky stuff
            // AKA "start from which scene"
            if(!IsOptionsMenuActive(sub_menu_id)) {
                result = GetOptionsMenuResult(sub_menu_id);
                CloseOptionsMenu(sub_menu_id);
                MAIN_OPTIONS_MENU = -1;
                if(result != 0) {
                    last_selected_scene = SCENE_ITEM_STATES[0];
                    GLOBAL_MENU_INFO.return_val = 1;
                    return true;
                }
                else {
                    ResumeSimpleMenu(main_menu_id);
                    CloseDialogueBox(SCENE_SELECTOR_PARTICIPANT_LISTING);
                    SCENE_SELECTOR_PARTICIPANT_LISTING = -1;
                    GLOBAL_MENU_INFO.state = 0;
                }
            }
            else {
                GetOptionsMenuAllChoices(sub_menu_id, SCENE_ITEM_STATES);
                current_menu_option = SCENE_ITEM_STATES[0];
                if(current_menu_option != GLOBAL_MENU_INFO.previous_option) {
                    ShowStringIdInDialogueBox(SCENE_SELECTOR_PARTICIPANT_LISTING, PREPROC_INSTANT_NO_INPUT, TEXT_STRING_PARTICIPANT_NAME_START+current_menu_option, NULL);
                    GLOBAL_MENU_INFO.previous_option = current_menu_option;
                }
            }
            break;
    }
    return false;
    #else
    GLOBAL_MENU_INFO.return_val = 2;
    return true;
    #endif
}
