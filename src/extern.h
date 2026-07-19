#pragma once

#include <pmdsky.h>
#include <cot.h>

#define EVENT_FINISHED 0
#define SECTION_TEXT_PLAYBILL __attribute__((section(".text.playbill"))) __attribute((used))
#define SECTION_DATA_PLAYBILL __attribute__((section(".data.playbill"))) __attribute((used))

#define TOTAL_SCENES_PER_BRANCH 11 // 10 participant scenes, 1 initial
#define TEXT_STRING_SCENE_SELECTOR_TITLE 8735
#define TEXT_STRING_MYSTERYMAIL_BLURB 8736
#define TEXT_STRING_PLAY_SCENE_QUESTION 8737
#define TEXT_STRING_MAIN_MENU_TITLE 8738
#define TEXT_STRING_MAIN_MENU_OPTION_NAME_START 8739 // Four options
#define TEXT_STRING_FLAVOR_BLURB 8743
#define TEXT_STRING_SCENE_STARTER_TITLE 8744
#define TEXT_STRING_SCENE_STARTER_DESC 8745
#define TEXT_STRING_PARTICIPANT_NAME_START 8750

#define TEXT_STRING_HIJACK_OPTIONS_MENU 31337

enum decoi_branch {
    BRANCH_INVALID = -1,
    BRANCH_EXPLORATION,
    BRANCH_CONTROL,
    BRANCH_OBLITERATION
};

// Layouts for portraits
enum portrait_layout_types {
    FACE_POS_STANDARD = 0,
    FACE_POS_ABSCOORD = 1,
    FACE_POS_ABSCOORDFACEL = 2,
    FACE_POS_ABSCOORDFACER = 3,
    FACE_POS_BOTTOM_C_FACER = 4,
    FACE_POS_BOTTOM_L_FACEINW = 5,
    FACE_POS_BOTTOM_R_FACEINW = 6,
    FACE_POS_BOTTOM_L_CENTER = 7,
    FACE_POS_BOTTOM_R_CENTER = 8,
    FACE_POS_BOTTOM_C_FACEL = 9,
    FACE_POS_BOTTOM_L_FACEOUTW = 10,
    FACE_POS_BOTTOM_R_FACEOUTW = 11,
    FACE_POS_BOTTOM_LC_FACEOUTW = 12,
    FACE_POS_BOTTOM_RC_FACEOUTW = 13,
    FACE_POS_TOP_C_FACER = 14,
    FACE_POS_TOP_L_FACEINW = 15,
    FACE_POS_TOP_R_FACEINW = 16,
    FACE_POS_TOP_L_CENTER = 17,
    FACE_POS_TOP_R_CENTER = 18,
    FACE_POS_TOP_C_FACEL = 19,
    FACE_POS_TOP_L_FACEOUTW = 20,
    FACE_POS_TOP_RC_FACER = 21,
    FACE_POS_TOP_LC_FACEOUTW = 22,
    FACE_POS_TOP_RC_FACEOUTW = 23,
};

struct performer_spawn {
    int16_t kind;
    struct direction_id_8 direction;
    uint8_t collision_box_size_x;
    uint8_t collision_box_size_y;
    uint8_t x;
    uint8_t y;
    uint8_t flags_x;
    uint8_t flags_y;
};

struct font_data {
    void* kanji_rd_data;
    void* unkno_rd_data;
    int kanji_rd_height;
    int unkno_rd_height;
    void* kanji_rd_base;
    void* unkno_rd_base;
};

// Symbols not yet documented on pmdsky-debug
extern void PlayBgm2ByIdVolumeVeneer(enum music_id music_id, int duration, int volume);
extern void ChangeSeVolumeVeneer(int se_id, int duration, int volume);
extern void ChangeVolumeBgm(int duration, int volume);
extern void ChangeVolumeBgm2(int duration, int volume);
extern void SoundStop(void);
extern void TextboxSolid(void);
extern void TextboxTransparent(void);
extern void GetSceneName(char *dst, char *src);
extern int CreateLivePerformer(int follow_idx, struct performer_spawn *performer_spawn, int hanger, int sector, bool attribute_flag);
extern void GetOptionsMenuAllChoices(int window_id, int* buf);
extern void PlayTimerTickWrapper(void);
extern void LoadMarkfont(void);
extern void LoadStaffont(void);
extern bool FileOpenInner(struct file_stream* file, char* filepath);
extern struct rgba TEXTBOX_COLOR_ATTRIBUTES;
extern struct font_data FONT_DATA;

// Generic stuff
extern void SwapFont(const char* filepath, bool swap_unkno);
extern bool SpResetTextboxColor(void);
extern int GetParticipantTextStringId(int scene);

// Playbill shenangians
extern SECTION_TEXT_PLAYBILL void CreatePlaybill(void);
extern SECTION_TEXT_PLAYBILL void ClosePlaybill(void);
extern SECTION_TEXT_PLAYBILL bool UpdateSceneSelectorMenu(void);
extern SECTION_TEXT_PLAYBILL void CreateEnvelope(void);
extern SECTION_TEXT_PLAYBILL void CloseEnvelope(void);
extern SECTION_TEXT_PLAYBILL bool UpdateMysteryMailMenu(void);

extern enum decoi_branch selected_branch;
extern int last_selected_scene;
extern bool playing_all_scenes;

// Control Scene Memory Special Process
extern int SpControlSceneMemory(int params, int scene);
