#include <pmdsky.h>
#include <cot.h>
#include "extern.h"

int last_selected_scene = 0;
bool playing_all_scenes = false;

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
    {
        .create = CreateSceneSelectorMenu,
        .close = CloseSceneSelectorMenu,
        .update = UpdateSceneSelectorMenu
    },
    {
        .create = CreateMysteryMailMenu,
        .close = CloseMysteryMailMenu,
        .update = UpdateMysteryMailMenu
    },
    {
        // SPECIAL: This menu will use whatever code is loaded in the scratch area
        // Code can be loaded using special process SpLoadCode
        // with a string for the first arg
        .create = (void (*)())0x23D7FF0,
        .close = (void (*)())0x23D7FF4,
        .update = (bool (*)())0x23D7FF8
    }
};

struct global_menu_info GLOBAL_MENU_INFO;
const int CUSTOM_MENU_AMOUNT = ARRAY_LENGTH(CUSTOM_MENUS);
