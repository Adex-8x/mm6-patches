#include <pmdsky.h>
#include <cot.h>
#include "extern.h"
#include "battletholomew.h"

#define PARTICIPANT_CREDITS_DIALOGUE_BOX GLOBAL_MENU_INFO.window_ids[0]

int last_selected_scene = 0;
bool playing_all_scenes = false;
char menu_user_string[10] = {0};

const char PARTICIPANT_COLOR_TAG[] = "[CS:F]";
const char NOSHOW_COLOR_TAG[] = "[CS:B]";
const char PARTICIPANT_CREDITS_NAME_DELIMITER[] = "\n[HR][CN]";
const char PARTICIPANT_CREDITS_PARTICIPANT_TITLE[] = "[FT:3][CN]SCENE %s\n[CN]CREATED BY[FT:0][BAR]\n[HR][FT:2][CN]";
const char PARTICIPANT_CREDITS_ORGANIZER_TITLE[] = "[FT:3][CN]MYSTERYMAIL EVENT SIX\n[CN]ORGANIZED BY[FT:0][BAR]\n[HR][FT:2][CN]";
const char PARTICIPANT_CREDITS_NOSHOW[] = "[FT:0][CN][CS:B](nobody lol)[CR]";

const char* SCENE_WORDS_ENGLISH[] = {"ONE", "TWO", "THREE", "FOUR", "FIVE", "SIX", "SEVEN", "EIGHT", "NINE", "TEN"}; // mhm yeah

// The initial menu function called to show a keyboard prompt for the player to type in a string.
// This is intended to be used by a variety of menus.
void CreateSimpleKeyboardMenu(void) {
    SetupAndShowKeyboard(GLOBAL_MENU_INFO.id, NULL, NULL);
}

// The menu function called repeatedly to check if the player has finished entering a string.
// This is intended to be used by a variety of menus.
bool UpdateSimpleKeyboardMenu(void) {
    return IS_BASE_GAME_MENU_FINISHED;
}

// The Scene Selector and Main Menu logic is mostly handled in the dedicated "playbill.c" file, which has code in Overlay24.
void CreateSceneSelectorMenu(void) {
    LoadOverlay(OGROUP_OVERLAY_24);
    CreatePlaybill();
}

void CloseSceneSelectorMenu(void) {
    ClosePlaybill();
    UnloadOverlay(OGROUP_OVERLAY_24);
}


void CreateMysteryMailMenu(void) {
    LoadOverlay(OGROUP_OVERLAY_24);
    #if EVENT_FINISHED
    CreateEnvelope();
    #endif
}

void CloseMysteryMailMenu(void) {
    #if EVENT_FINISHED
    CloseEnvelope();
    #endif
    UnloadOverlay(OGROUP_OVERLAY_24);
}

struct window_params FithteaoneMailDialogueBoxParams = {
    .update = UpdateDialogueBox,
    .x_offset = 2,
    .y_offset = 2,
    .width = 28,
    .height = 20,
    .screen = {.val = SCREEN_SUB},
    .box_type = {.val = 0xFD}
};

void CreateFithteaoneMailMenu(void) {
    struct preprocessor_flags pre_flags = {};
    GLOBAL_MENU_INFO.window_ids[1] = CreateDialogueBox(&FithteaoneMailDialogueBoxParams);
    int mail_number = LoadScriptVariableValue(NULL, VAR_LOTTERY_RESULT);
    ShowStringIdInDialogueBox(GLOBAL_MENU_INFO.window_ids[1], pre_flags, 9335 + mail_number, NULL);
}

bool UpdateFithteaoneMailMenu(void) {
    return !IsDialogueBoxActive(GLOBAL_MENU_INFO.window_ids[1]) && IsAOrBPressed();
}

void CloseFithteaoneMailMenu(void) {
    CloseDialogueBox(GLOBAL_MENU_INFO.window_ids[1]);
}

void CloseGenericInputMenu(void) {
    MemZero(menu_user_string, sizeof(menu_user_string));
    strncpy(menu_user_string, (char*)GetKeyboardStringResult(), sizeof(menu_user_string));
    for(int i = 0; i < sizeof(menu_user_string) && menu_user_string[i] != '\0'; i++) {
        if(menu_user_string[i] >= 'A' && menu_user_string[i] <= 'Z')
            menu_user_string[i] += 0x20;
    }
    GLOBAL_MENU_INFO.return_val = 0;
}

void CreateParticipantCredits(void) {
    struct window_params window_params = { .x_offset = 0x3, .y_offset = 0x3, .width = 0x1A, .height = 0x10, .screen = {SCREEN_SUB}, .box_type = {0xFA} };
    LoadStaffont(0);
    SaveScriptVariableValue(NULL, VAR_DUNGEON_EVENT_LOCAL, 0);
    PARTICIPANT_CREDITS_DIALOGUE_BOX = CreateDialogueBox(&window_params);
    GLOBAL_MENU_INFO.state = 1;
}

void CloseParticipantCredits(void) {
    CloseDialogueBox(PARTICIPANT_CREDITS_DIALOGUE_BOX);
    PARTICIPANT_CREDITS_DIALOGUE_BOX = -1;
    LoadMarkfont();
    ChangeFontType(0);
}

bool UpdateParticipantCredits(void) {
    GLOBAL_MENU_INFO.state--;
    if(GLOBAL_MENU_INFO.state > 0)
        return false;
    
    char* participant_string;
    char credits_string[0x200] = {0};
    int current_scene = LoadScriptVariableValue(NULL, VAR_DUNGEON_EVENT_LOCAL);
    struct preprocessor_flags preprocessor_flags = {.flags_1 = 0b000000010, .timer_2 = true}; // Instant text without waiting for any input!
    if(current_scene < TOTAL_SCENES_PER_BRANCH) {
        if(current_scene > 0)
            RemoveActingSector(current_scene);
            
        int next_scene = current_scene+1;
        // Either bring up the existing participant names, or a special one
        if(next_scene != TOTAL_SCENES_PER_BRANCH) {
            participant_string = StringFromId(GetParticipantTextStringId(next_scene));
            sprintf(credits_string, PARTICIPANT_CREDITS_PARTICIPANT_TITLE, SCENE_WORDS_ENGLISH[current_scene]); // NOT "next_scene", 0-indexing my beloved
        }
        else {
            participant_string = StringFromId(TEXT_STRING_EVENT_ORGANIZERS);
            strncpy(credits_string, PARTICIPANT_CREDITS_ORGANIZER_TITLE, sizeof(PARTICIPANT_CREDITS_ORGANIZER_TITLE)-1);
        }
        // Need to extract out the participant names...
        char* name_start = strstr(participant_string, PARTICIPANT_COLOR_TAG);
        
        // Could've also checked the NOSHOW arrays...but eh this works too, since we already have the strings
        if(strncmp(participant_string, NOSHOW_COLOR_TAG, sizeof(NOSHOW_COLOR_TAG)-1) == 0) {
            strncat(credits_string, PARTICIPANT_CREDITS_NOSHOW, sizeof(PARTICIPANT_CREDITS_NOSHOW)-1);
        }
        else {
            while(name_start) {
                name_start += sizeof(PARTICIPANT_COLOR_TAG)-1;
                char* name_end = strchr(name_start, '[');
                if(name_end == NULL)
                    break;
                strncat(credits_string, name_start, name_end-name_start);
                strncat(credits_string, PARTICIPANT_CREDITS_NAME_DELIMITER, sizeof(PARTICIPANT_CREDITS_NAME_DELIMITER)-1);
                participant_string = name_end+1;
                name_start = strstr(participant_string, PARTICIPANT_COLOR_TAG);
            }
        }
        ShowStringInDialogueBox(PARTICIPANT_CREDITS_DIALOGUE_BOX, preprocessor_flags, credits_string, NULL);
        LoadActingSector(next_scene);
        LoadSceneStuff(next_scene);
        SaveScriptVariableValue(NULL, VAR_DUNGEON_EVENT_LOCAL, next_scene);
        GLOBAL_MENU_INFO.state = PARTICIPANT_CREDITS_TIMER;
        return false;
    }
    else {
        GLOBAL_MENU_INFO.return_val = 1;
        return true;
    }
}



// Add your custom script menus to the list below.
// `create` is a pointer to the initial function that will run only once when a custom `message_Menu` runs. This is typically responsible for the initial creation of any windows.
// `close` is a pointer to the final function that will run only once when a custom `message_Menu` runs. This is typically responsible for the final closing of any windows, as well as setting a return value if not yet set.
// `update` is pointer to the function that will continously get called every frame when a custom `message_Menu` runs. This is typically responsible for checking the status of any menus and implementing control flow, i.e., "what happens if the player selects an option?"
// `keyboard_prompt_string_id` is the Text String ID shown when a keyboard prompt is displayed. This may not be necessary for all menus.
// `keyboard_confirm_string_id` is the Text String ID shown when confirming the player's keyboard input. This may not be necessary for all menus.
// Custom script menus use ID 80 + <array index>.
//
// Refer to menus.h for more information on the fields of `custom_menu` and `global_menu_info`!
__attribute((used)) struct custom_menu CUSTOM_MENUS[] = {
    // ID 80
    // Creates the scene selector.
    {
        .create = CreateSceneSelectorMenu,
        .close = CloseSceneSelectorMenu,
        .update = UpdateSceneSelectorMenu
    },
    // ID 81
    // Creates the main menu, only properly used once the event has finished.
    {
        .create = CreateMysteryMailMenu,
        .close = CloseMysteryMailMenu,
        .update = UpdateMysteryMailMenu
    },
    // ID 82
    {
        // SPECIAL: This menu will use whatever code is loaded in the scratch area
        // Code can be loaded using special process SpLoadCode
        // with a string for the first arg
        .create = (void (*)())0x23D7FF0,
        .close = (void (*)())0x23D7FF4,
        .update = (bool (*)())0x23D7FF8
    },
    // ID 83
    {
        .create = CreateFithteaoneMailMenu,
        .close = CloseFithteaoneMailMenu,
        .update = UpdateFithteaoneMailMenu,
    },
    // ID 84
    {
        .create = CreateBattle1,
        .close = CloseBattle,
        .update = UpdateBattle
    },
    // ID 85
    {
        .create = CreateBattle2,
        .close = CloseBattle,
        .update = UpdateBattle
    },
    // ID 86
    // Prompts the player to input a string.
    {
        .keyboard_prompt_string_id = 300,
        .keyboard_confirm_string_id = 301,
        .create = CreateSimpleKeyboardMenu,
        .close = CloseGenericInputMenu,
        .update = UpdateSimpleKeyboardMenu
    },
    // ID 87
    // Participant credits!
    {
        .create = CreateParticipantCredits,
        .close = CloseParticipantCredits,
        .update = UpdateParticipantCredits
    }
};

__attribute__((section(".data.fixed1"))) struct global_menu_info GLOBAL_MENU_INFO;
const int CUSTOM_MENU_AMOUNT = ARRAY_LENGTH(CUSTOM_MENUS);
