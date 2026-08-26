#include "battletholomew.h"

#include <pmdsky.h>
#include <cot.h>
#include "extern.h"

#define MONSTER_KINGAMBIT 1245 // 1202

#define ACTOR_PARTY1 173 // 69
#define ACTOR_PARTY2 174 // 70
#define ACTOR_PARTY3 175 // 71
#define ACTOR_BOSS 177 // 73
#define ACTOR_GOON2 197 // 93

#define BATTLE_MAX_TEAM_HP 420
#define BATTLE_ORAN_BERRY_HEAL 200

#define BATTLE_BOSS_HP 900
#define BATTLE_GOON_HP 350

#define ANIM_LOOP 0x0800
#define ANIM_ONCE 0x1000
#define ANIM_SPEED_NORMAL 0x0000
#define ANIM_SPEED_SLOW 0x0100
#define ANIM_SPEED_FAST 0x0200
#define ANIM_SPEED_FREEZE 0x0300

#define ANIM_WALK 0
#define ANIM_ATTACK 1
#define ANIM_SLEEP 5
#define ANIM_HURT 6
#define ANIM_IDLE 7
#define ANIM_SWING 8
#define ANIM_DOUBLE 9
#define ANIM_HOP 10
#define ANIM_CHARGE 11
#define ANIM_ROTATE 12

#define BATTLE_ANIM_IDLE (ANIM_LOOP | ANIM_IDLE)
#define BATTLE_ANIM_ATTACK (ANIM_ONCE | ANIM_ATTACK)
#define BATTLE_ANIM_HURT (ANIM_ONCE | ANIM_HURT)
#define BATTLE_ANIM_DODGE (ANIM_ONCE | ANIM_DOUBLE | ANIM_SPEED_FAST)

#define BATTLE_EFFECT_DOUBLE_SLASH 49
#define BATTLE_EFFECT_TARGET_LOCKED 81
#define BATTLE_EFFECT_EXPLOSION_SMALL 74
#define BATTLE_EFFECT_EXPLOSION_MEDIUM 76
#define BATTLE_EFFECT_EXPLOSION_BIG 66
#define BATTLE_EFFECT_PETAL 18
#define BATTLE_EFFECT_FIRE 57
#define BATTLE_EFFECT_SWEAT 62

#define BATTLE_OBJECT_ORB 376
#define BATTLE_OBJECT_9MM 610

#define BATTLE_SE_SCRATCH 3600
#define BATTLE_SE_HIT 280
#define BATTLE_SE_HURT 5121
#define BATTLE_SE_EXPLOSION 787
#define BATTLE_SE_PROJECTILE 1282
#define BATTLE_SE_WHOOSH 4629
#define BATTLE_SE_BARRIER 1554
#define BATTLE_SE_HEAL 4890

#define BATTLE_PARTY_DAMAGE_MIN 40
#define BATTLE_PARTY_DAMAGE_MAX 60
#define BATTLE_PARTY_ATTACK_FRAMES 40
#define BATTLE_ENEMY_RECOVER_FRAMES 25
#define BATTLE_DODGE_FRAMES 25
#define BATTLE_PARRY_FRAMES 8
#define BATTLE_ENEMY_FINISH_FRAMES 30
#define BATTLE_TURN_PAUSE_FRAMES 60

#define BATTLE_BOSS_MUSIC 194
#define BATTLE_GOON_MUSIC MUSIC_BOSS_BATTLE
#define BATTLE_MUSIC_LEAD_FRAMES 40

// why not in a header so I have to copy :(
struct adpcm_decoder {
    uint8_t* ptr_adpcm;
    int32_t adpcm_buf_pos;
    int32_t adpcm_buf_len;
    int16_t* ptr_pcm;
    int32_t pcm_buf_pos;
    int32_t pcm_buf_len;
    int32_t global_sample_i;
    int16_t cur_adpcm_predictor;
    uint8_t cur_adpcm_index;
    int8_t cur_nibble;
    int32_t loop_start;
    int16_t loop_adpcm_predictor;
    uint8_t loop_adpcm_index;
};

struct wave_file_streamer {
    struct file_stream fstream;
    struct adpcm_decoder adpcm_decoder;
    int32_t data_start;
    int32_t data_end;
    int32_t smplrate;
    int32_t adpcm_block_size;
    int32_t loop_start;
    int32_t cursor_pos;
    int32_t max_read_len;
    int8_t loop_skip_first_nibble;
    int8_t ofstream;
    int8_t is_looped;
    int8_t just_looped;
};

struct wave_player {
    struct wave_file_streamer wave_stream_left;
    struct wave_file_streamer wave_stream_right;
    void* snd_addr;
    int32_t timer;
    int32_t volume;
    int32_t fade_to;
    int32_t fade_time;
    int32_t fade_play;
    int32_t old_timer;
    int32_t bgm_id;
    int8_t channel_start;
    int8_t playing;
};

extern struct wave_player player[1];

int battle_current = 0;

enum battle_state {
    BATTLE_STATE_INTRO,
    BATTLE_STATE_INITIAL,
    BATTLE_STATE_MAINMENU,
    BATTLE_STATE_ITEMMENU,
    BATTLE_STATE_RUN_AWAY,
    BATTLE_STATE_ATTACK,
    BATTLE_STATE_ENEMY_START,
    BATTLE_STATE_ENEMY_WINDUP,
    BATTLE_STATE_ENEMY_STRIKE,
    BATTLE_STATE_ENEMY_FINISH,
    BATTLE_STATE_ENEMY_RECOVER,
    BATTLE_STATE_PAUSE,
    BATTLE_STATE_MESSAGE,
    BATTLE_STATE_OVER,
};

struct battle_attack {
    uint16_t enemy_anim;
    int16_t windup_effect;
    int16_t hit_effect;
    int16_t windup_se;
    int16_t hit_se;
    int16_t projectile_object;
    int windup_frames;
    int damage;
    bool hits_party;
    bool walk_to_target;
    char* telegraph;
};

static const struct battle_attack BOSS_ATTACKS[] = {
    {
        .enemy_anim = BATTLE_ANIM_ATTACK,
        .windup_effect = BATTLE_EFFECT_TARGET_LOCKED,
        .hit_effect = BATTLE_EFFECT_EXPLOSION_MEDIUM,
        .windup_se = BATTLE_SE_PROJECTILE,
        .hit_se = BATTLE_SE_EXPLOSION,
        .projectile_object = BATTLE_OBJECT_ORB,
        .windup_frames = 50,
        .damage = 55,
    },
    {
        .enemy_anim = BATTLE_ANIM_ATTACK,
        .windup_effect = BATTLE_EFFECT_PETAL,
        .hit_effect = BATTLE_EFFECT_EXPLOSION_MEDIUM,
        .windup_se = BATTLE_SE_WHOOSH,
        .hit_se = BATTLE_SE_EXPLOSION,
        .windup_frames = 50,
        .damage = 80,
        .hits_party = true,
    },
    {
        .enemy_anim = BATTLE_ANIM_ATTACK,
        .hit_effect = BATTLE_EFFECT_DOUBLE_SLASH,
        .hit_se = BATTLE_SE_HIT,
        .windup_frames = 55,
        .damage = 65,
        .walk_to_target = true,
    },
};

static const struct battle_attack GOON_ATTACKS[] = {
    {
        .enemy_anim = BATTLE_ANIM_ATTACK,
        .hit_effect = BATTLE_EFFECT_DOUBLE_SLASH,
        .hit_se = BATTLE_SE_HIT,
        .windup_frames = 50,
        .damage = 50,
        .walk_to_target = true,
    },
    {
        .enemy_anim = BATTLE_ANIM_ATTACK,
        .windup_effect = BATTLE_EFFECT_TARGET_LOCKED,
        .hit_effect = BATTLE_EFFECT_EXPLOSION_SMALL,
        .windup_se = BATTLE_SE_PROJECTILE,
        .hit_se = BATTLE_SE_EXPLOSION,
        .projectile_object = BATTLE_OBJECT_9MM,
        .windup_frames = 40,
        .damage = 75,
    },
};

static const int BOSS_LYRICS[] = {
    1440, 1643, 1845, 2025, 2160, 2363, 2565, 2745,
    2835, 3015, 3184, 3307, 3476, 3555, 3735, 3904,
    4027, 4230, 4320, 8640, 8843, 9045, 9225, 9360,
    9563, 9765, 9945, 10035, 10215, 10384, 10507, 10676,
    10755, 10935, 11104, 11227, 11430, 11520,
};

static const struct battle_monologue {
    int id;
    enum portrait_emotion emotion;
} BOSS_MONOLOGUE[] = {
    { 10040, PORTRAIT_PAIN },
    { 10041, PORTRAIT_SHOUTING },
    { 10042, PORTRAIT_SHOUTING },
    { 10043, PORTRAIT_ANGRY },
    { 10044, PORTRAIT_HAPPY },
    { 10045, PORTRAIT_DETERMINED },
};

enum battle_menu_option {
    BATTLE_OPTION_ATTACK,
    BATTLE_OPTION_ITEM,
    BATTLE_OPTION_RUN_AWAY,
    BATTLE_OPTION_NUM,
};

enum battle_item_option {
    BATTLE_ITEM_ORAN_BERRY,
    BATTLE_ITEM_NUM,
};

static bool fought_goon = false;

struct battle {
    enum battle_state state;

    int menu_window;
    int menu_option;
    int item_window;
    int item_option;
    int hp_window;
    int team_hp;
    int shown_hp;

    int message_window;
    int portrait_window;
    enum battle_state message_next_state;

    int damage_window;
    int damage_amount;

    int prompt_windows[4];
    bool prompt_dirty;

    int enemy_hp;
    int enemy_attacks;
    int turn;
    int monologue;
    const struct battle_attack* attack;
    int attack_target;
    int timer;
    enum battle_state pause_next_state;
    bool dodging[3];
    bool dodge_blocked[3];
    bool dodged;

    int projectile;
    struct vec2 projectile_from;
    struct vec2 projectile_to;

    struct vec2 enemy_home;
    struct vec2 enemy_dest;
    uint32_t enemy_attributes;

    int music_frame;
    int music_anchor;
    int lyric_window;
    int lyric;
    int lyric_start;
    int lyric_end;

    int invincibility_frames;
    bool parrying;
    int ran_away;
} battle;

static const int BATTLE_PARTY[] = { ACTOR_PARTY1, ACTOR_PARTY2, ACTOR_PARTY3 };

static const struct battle_prompt {
    uint8_t x_offset;
    uint8_t y_offset;
    uint8_t icon;
    char* name;
} BATTLE_PROMPTS[] = {
    { 9, 5, 4, "Dodge (Purslane)" },
    { 17, 10, 2, "Dodge (Snorton)" },
    { 9, 15, 3, "Dodge (Aquarius)" },
    { 1, 10, 5, "Parry" },
};

static int GetEnemyActor(void) {
    return battle_current == BATTLE_BOSS ? ACTOR_BOSS : ACTOR_GOON2;
}

static struct live_actor* GetBattleActor(int entity_id) {
    if (GROUND_STATE_PTRS.actors == NULL)
        return NULL;
    int index = GetLiveActorIdxFromScriptEntityId(entity_id);
    if (index < 0)
        return NULL;
    return &GROUND_STATE_PTRS.actors->actors[index];
}

static void PlayBattleAnimation(int entity_id, uint16_t animation) {
    struct live_actor* actor = GetBattleActor(entity_id);
    if (actor == NULL)
        return;
    actor->second_bitfield = animation;
    actor->direction_should_change = true;
}

static void PlayBattleEffect(int entity_id, int16_t effect_id) {
    struct live_actor* actor = GetBattleActor(entity_id);
    if (actor != NULL && effect_id > 0)
        SetEffectLiveActor(actor, false, effect_id, 3);
}

static void GetEntityCenter(struct vec2* coord_min, struct vec2* coord_max, struct vec2* center) {
    center->x = (coord_min->x + coord_max->x) / 2;
    center->y = (coord_min->y + coord_max->y) / 2;
}

static void LerpVec2(struct vec2* from, struct vec2* to, int elapsed, int total, struct vec2* out) {
    out->x = from->x + _s32_div_f((to->x - from->x) * elapsed, total);
    out->y = from->y + _s32_div_f((to->y - from->y) * elapsed, total);
}

static struct live_object* GetProjectile(void) {
    if (battle.projectile < 0 || GROUND_STATE_PTRS.objects == NULL)
        return NULL;
    return &GROUND_STATE_PTRS.objects->objects[battle.projectile];
}

static void DespawnProjectile(void) {
    if (battle.projectile >= 0) {
        DeleteLiveObject(battle.projectile);
        battle.projectile = -1;
    }
}

static void SpawnProjectile(int16_t object_id, int entity_from, int entity_to) {
    struct live_actor* from = GetBattleActor(entity_from);
    struct live_actor* to = GetBattleActor(entity_to);
    if (from == NULL || to == NULL)
        return;

    GetEntityCenter(&from->coord_min, &from->coord_max, &battle.projectile_from);
    GetEntityCenter(&to->coord_min, &to->coord_max, &battle.projectile_to);

    struct object_spawn spawn = {
        .kind = object_id,
        .direction = {DIR_DOWN},
        .collision_box_size_x = 1,
        .collision_box_size_y = 1,
        .x = battle.projectile_from.x / 8,
        .y = battle.projectile_from.y / 8,
        .link = -1,
    };
    battle.projectile = CreateLiveObject(-1, &spawn, from->entity.hanger, from->entity.sector, false);

    struct live_object* object = GetProjectile();
    if (object != NULL)
        SetPositionLiveObject(object, &battle.projectile_from);
}

static void StartEnemyWalk(int entity_from, int entity_to) {
    struct live_actor* from = GetBattleActor(entity_from);
    struct live_actor* to = GetBattleActor(entity_to);

    struct vec2 target;
    GetEntityCenter(&from->coord_min, &from->coord_max, &battle.enemy_home);
    GetEntityCenter(&to->coord_min, &to->coord_max, &target);

    battle.enemy_dest.x = target.x + (battle.enemy_home.x - target.x) / 4;
    battle.enemy_dest.y = target.y + (battle.enemy_home.y - target.y) / 4;
    PlayBattleAnimation(entity_from, ANIM_LOOP | ANIM_WALK);

    battle.enemy_attributes = from->attribute_bitfield;
    from->attribute_bitfield &= ~0x7C0; // disable collision
}

static void EndEnemyWalk(void) {
    struct live_actor* enemy = GetBattleActor(GetEnemyActor());
    if (enemy != NULL)
        enemy->attribute_bitfield = battle.enemy_attributes;
}

static void MoveEnemy(struct vec2* from, struct vec2* to, int elapsed, int total) {
    struct live_actor* enemy = GetBattleActor(GetEnemyActor());
    if (enemy == NULL)
        return;

    struct vec2 pos;
    LerpVec2(from, to, elapsed, total, &pos);
    SetPositionLiveActor(enemy, &pos);
}

static void MoveProjectile(int elapsed, int total) {
    struct live_object* object = GetProjectile();
    if (object == NULL)
        return;

    struct vec2 pos;
    LerpVec2(&battle.projectile_from, &battle.projectile_to, elapsed, total, &pos);
    SetPositionLiveObject(object, &pos);
}

static void PlayPartyAnimation(uint16_t animation) {
    for (int i = 0; i < ARRAY_LENGTH(BATTLE_PARTY); i++)
        PlayBattleAnimation(BATTLE_PARTY[i], animation);
}

static int GetOranBerryAmount(void) {
    return LoadScriptVariableValue(NULL, VAR_CRYSTAL_COLOR_03);
}

static void SetOranBerryAmount(int amount) {
    SaveScriptVariableValue(NULL, VAR_CRYSTAL_COLOR_03, amount);
}

static void CloseHpBox(void) {
    if (battle.hp_window >= 0) {
        CloseTextBox(battle.hp_window);
        battle.hp_window = -1;
    }
}

static void DrawHpBox(int window_id) {
    char buffer[0x40];
    snprintf(buffer, sizeof(buffer), "HP[CLUM_SET:32][CS:G]%d[CR]/%d", battle.team_hp, BATTLE_MAX_TEAM_HP);
    DrawTextInWindow(window_id, 4, 4, buffer);
}

static void CreateHpBox(void) {
    struct window_params hp_params = { .x_offset = 20, .y_offset = 1, .width = 10, .height = 2, .box_type = {0xFE} };
    battle.hp_window = CreateTextBox(&hp_params, DrawHpBox);
    battle.shown_hp = battle.team_hp;
}

static void CloseDamageBox(void) {
    if (battle.damage_window >= 0) {
        CloseTextBox(battle.damage_window);
        battle.damage_window = -1;
    }
}

static void DrawDamageBox(int window_id) {
    char buffer[0x20];
    snprintf(buffer, sizeof(buffer), "[BS]-%d[BR]", battle.damage_amount);
    DrawTextInWindow(window_id, 0, 2, buffer);
}

static void ShowDamageBox(int amount, bool enemy) {
    CloseDamageBox();
    battle.damage_amount = amount;

    struct window_params damage_params = { .x_offset = enemy ? 20 : 10, .y_offset = 16, .width = 6, .height = 2, .box_type = {0} };
    battle.damage_window = CreateTextBox(&damage_params, DrawDamageBox);
}

static void ClosePromptBoxes(void) {
    for (int i = 0; i < ARRAY_LENGTH(BATTLE_PROMPTS); i++) {
        if (battle.prompt_windows[i] >= 0) {
            CloseTextBox(battle.prompt_windows[i]);
            battle.prompt_windows[i] = -1;
        }
    }
}

static void DrawPromptBox(int window_id) {
    for (int i = 0; i < ARRAY_LENGTH(BATTLE_PROMPTS); i++) {
        if (battle.prompt_windows[i] != window_id)
            continue;

        bool used = i < ARRAY_LENGTH(BATTLE_PARTY) ? battle.dodging[i] : battle.parrying;
        char buffer[0x40];
        snprintf(buffer, sizeof(buffer), "[CLUM_SET:1][M:B%d][CS:%c] %s[CR]",
                 BATTLE_PROMPTS[i].icon, used ? 'K' : 'A', BATTLE_PROMPTS[i].name);
        DrawTextInWindow(window_id, 0, 2, buffer);
        return;
    }
}

static void RedrawPromptBoxes(void) {
    for (int i = 0; i < ARRAY_LENGTH(BATTLE_PROMPTS); i++) {
        if (battle.prompt_windows[i] < 0)
            continue;
        ClearWindow(battle.prompt_windows[i]);
        DrawPromptBox(battle.prompt_windows[i]);
        UpdateWindow(battle.prompt_windows[i]);
    }
}

static void CreatePromptBoxes(void) {
    ClosePromptBoxes();
    for (int i = 0; i < ARRAY_LENGTH(BATTLE_PROMPTS); i++) {
        if (i < ARRAY_LENGTH(BATTLE_PARTY) && !battle.attack->hits_party && i != battle.attack_target)
            continue;

        struct window_params prompt_params = { .x_offset = BATTLE_PROMPTS[i].x_offset,
                                               .y_offset = BATTLE_PROMPTS[i].y_offset,
                                               .width = 14, .height = 2,
                                               .screen = {SCREEN_SUB}, .box_type = {0xFF} };
        battle.prompt_windows[i] = CreateTextBox(&prompt_params, DrawPromptBox);
    }
    battle.prompt_dirty = false;
    RedrawPromptBoxes();
}

static void CloseLyricBox(void) {
    if (battle.lyric_window >= 0) {
        CloseTextBox(battle.lyric_window);
        battle.lyric_window = -1;
    }
}

static const char* GetLyricText(void) {
    return battle.lyric >= 0 ? StringFromId(10051 + battle.lyric) : "";
}

static void DrawLyricBox(int window_id) {
    const char* text = GetLyricText();
    int length = strlen(text);
    if (length == 0)
        return;

    int total = battle.lyric_end - battle.lyric_start - 12;
    int elapsed = battle.music_frame - battle.lyric_start;
    int sung = length;
    if (total > 0 && elapsed < total)
        sung = elapsed > 0 ? _s32_div_f(length * elapsed, total) : 0;

    char sung_text[0x80];
    int i;
    for (i = 0; i < sung && i < sizeof(sung_text) - 1; i++)
        sung_text[i] = text[i];
    sung_text[i] = '\0';

    char buffer[0x80];
    snprintf(buffer, sizeof(buffer), "[CN][CS:K]%s[CR][CS:C]%s[CR]", sung_text, text + i);
    DrawTextInWindow(window_id, 0, 2, buffer);
}

static void CreateLyricBox(void) {
    struct window_params lyric_params = { .x_offset = 1, .y_offset = 21, .width = 30, .height = 2, .box_type = {0}, .screen = {SCREEN_SUB}};
    battle.lyric_window = CreateTextBox(&lyric_params, DrawLyricBox);
}

static int GetMusicFrame(void) {
    struct wave_file_streamer* stream = &player[0].wave_stream_left;
    if (stream->smplrate <= 0 || stream->adpcm_block_size <= 0)
        return battle.music_frame;

    int bytes = stream->cursor_pos;
    int samples = (bytes - _s32_div_f(bytes, stream->adpcm_block_size) * 4) * 2;
    return _s32_div_f(samples * 60, stream->smplrate);
}

static int GetCurrentLyric(void) {
    int index = -1;
    battle.lyric_start = 0;
    battle.lyric_end = battle.music_frame;
    for (int i = 0; i < ARRAY_LENGTH(BOSS_LYRICS); i++) {
        if (BOSS_LYRICS[i] > battle.music_frame) {
            battle.lyric_end = BOSS_LYRICS[i];
            break;
        }
        index = i;
        battle.lyric_start = BOSS_LYRICS[i];
    }
    return index;
}

static void UpdateLyrics(void) {
    if (battle_current != BATTLE_BOSS)
        return;

    int audio = GetMusicFrame();
    if (audio != battle.music_anchor) {
        battle.music_anchor = audio;
        battle.music_frame = audio > BATTLE_MUSIC_LEAD_FRAMES ? audio - BATTLE_MUSIC_LEAD_FRAMES : 0;
    } else {
        battle.music_frame++;
    }

    int index = GetCurrentLyric();
    if (index != battle.lyric) {
        battle.lyric = index;
        CloseLyricBox();
        if (GetLyricText()[0] != '\0')
            CreateLyricBox();
    } else if (battle.lyric_window >= 0) {
        ClearWindow(battle.lyric_window);
        DrawLyricBox(battle.lyric_window);
        UpdateWindow(battle.lyric_window);
    }
}

static char* BattleMenuEntryFn(char* buffer, int option_id) {
    switch (option_id) {
        case BATTLE_OPTION_ATTACK:
            return "ATTACK";
        case BATTLE_OPTION_ITEM:
            return "ITEM";
        case BATTLE_OPTION_RUN_AWAY:
            return "RUN AWAY";
    }
    return "";
}

static char* BattleItemMenuEntryFn(char* buffer, int option_id) {
    switch (option_id) {
        case BATTLE_ITEM_ORAN_BERRY:
            sprintf(buffer, "Oran Berry[CLUM_SET:88]x%d", GetOranBerryAmount());
            return buffer;
    }
    return "";
}

static void CloseBattleMenu(void) {
    if (battle.menu_window >= 0) {
        CloseAdvancedMenu(battle.menu_window);
        battle.menu_window = -1;
    }
}

static void CreateBattleMenu(void) {
    struct window_params menu_params = { .x_offset = 2, .y_offset = 1, .box_type = {0xFF} };
    struct window_flags menu_flags = { .a_accept = true, .se_on = true, .set_choice = true };
    struct window_extra_info menu_info = { .set_choice_id = battle.menu_option };
    battle.menu_window = CreateAdvancedMenu(&menu_params, menu_flags, &menu_info, BattleMenuEntryFn, BATTLE_OPTION_NUM, BATTLE_OPTION_NUM);
}

static void CloseBattleItemMenu(void) {
    if (battle.item_window >= 0) {
        CloseAdvancedMenu(battle.item_window);
        battle.item_window = -1;
    }
}

static void CreateBattleItemMenu(void) {
    struct window_params item_params = { .x_offset = 2, .y_offset = 2, .width = 14, .box_type = {0xFF} };
    struct window_flags item_flags = { .a_accept = true, .b_cancel = true, .se_on = true, .set_choice = true };
    struct window_extra_info item_info = { .set_choice_id = battle.item_option };
    battle.item_window = CreateAdvancedMenu(&item_params, item_flags, &item_info, BattleItemMenuEntryFn, BATTLE_ITEM_NUM, BATTLE_ITEM_NUM);
}

static void CloseBattleMessage(void) {
    if (battle.message_window >= 0) {
        CloseDialogueBox(battle.message_window);
        battle.message_window = -1;
    }
    if (battle.portrait_window >= 0) {
        ClosePortraitBox(battle.portrait_window);
        battle.portrait_window = -1;
    }
}

static void CreateBattleMessage(char* message, enum monster_id speaker, enum battle_state next_state) {
    if (speaker != MONSTER_NONE) {
        struct portrait_params* portrait = &GLOBAL_MENU_INFO.portrait_params;
        InitPortraitParamsWithMonsterId(portrait, speaker);
        SetPortraitEmotion(portrait, PORTRAIT_NORMAL);
        SetPortraitLayout(portrait, FACE_POS_STANDARD);
        battle.portrait_window = CreatePortraitBox(SCREEN_MAIN, 0, true);
        ShowPortraitInPortraitBox(battle.portrait_window, portrait);
    }

    struct preprocessor_flags flags = { .flags_1 = 0b000001110, .flags_11 = 0b10 };
    battle.message_window = CreateDialogueBox(NULL);
    ShowStringInDialogueBox(battle.message_window, flags, message, NULL);
    battle.message_next_state = next_state;
    battle.state = BATTLE_STATE_MESSAGE;
}

static void CreateBattleMessageForBoss(char* message, enum portrait_emotion emotion) {
    int speaker = MONSTER_SHAYMIN_LAND;
    struct portrait_params* portrait = &GLOBAL_MENU_INFO.portrait_params;
    InitPortraitParamsWithMonsterId(portrait, speaker);
    SetPortraitEmotion(portrait, emotion);
    SetPortraitLayout(portrait, FACE_POS_BOTTOM_R_FACEINW);
    battle.portrait_window = CreatePortraitBox(SCREEN_MAIN, 0, true);
    ShowPortraitInPortraitBox(battle.portrait_window, portrait);

    struct preprocessor_flags flags = { .flags_1 = 0b000001110, .flags_11 = 0b10 };
    battle.message_window = CreateDialogueBox(NULL);
    ShowStringInDialogueBox(battle.message_window, flags, message, NULL);

    battle.message_next_state = BATTLE_STATE_ENEMY_START;
    battle.state = BATTLE_STATE_MESSAGE;
}

static void EndBattle(bool won) {
    GLOBAL_MENU_INFO.return_val = won ? BATTLE_RESULT_WON : BATTLE_RESULT_LOST;
    PlayPartyAnimation(won ? BATTLE_ANIM_IDLE : BATTLE_ANIM_HURT);
    PlayBattleAnimation(GetEnemyActor(), won ? BATTLE_ANIM_HURT : BATTLE_ANIM_IDLE);

    StopBgm(60);

    if (won) {
        CloseLyricBox();
        battle.lyric = -1;
    }


    char buffer[0x64];
    if (won)
        snprintf(buffer, sizeof(buffer), StringFromId(10027));
    else
        snprintf(buffer, sizeof(buffer), StringFromId(10028));

    if (won && battle_current == BATTLE_BOSS)
        battle.state = BATTLE_STATE_OVER;
    else
        CreateBattleMessage(buffer, MONSTER_KINGAMBIT, BATTLE_STATE_OVER);
}

static bool CheckBattleOver(void) {
    if (battle.enemy_hp <= 0) {
        EndBattle(true);
        return true;
    }
    if (battle.team_hp <= 0) {
        EndBattle(false);
        return true;
    }
    return false;
}

static void StartPartyAttack(void) {
    PlayPartyAnimation(BATTLE_ANIM_ATTACK);
    PlayBattleEffect(GetEnemyActor(), BATTLE_EFFECT_DOUBLE_SLASH);
    PlaySeByIdVolumeWrapper(BATTLE_SE_SCRATCH);
    battle.timer = BATTLE_PARTY_ATTACK_FRAMES;
    battle.state = BATTLE_STATE_ATTACK;
}

static void StartEnemyAttack(void) {
    const struct battle_attack* attacks = battle_current == BATTLE_BOSS ? BOSS_ATTACKS : GOON_ATTACKS;
    int amount = battle_current == BATTLE_BOSS ? ARRAY_LENGTH(BOSS_ATTACKS) : ARRAY_LENGTH(GOON_ATTACKS);

    battle.attack = &attacks[RandRange(0, amount)];
    battle.attack_target = RandRange(0, ARRAY_LENGTH(BATTLE_PARTY));
    for (int i = 0; i < ARRAY_LENGTH(BATTLE_PARTY); i++) {
        battle.dodging[i] = false;
        battle.dodge_blocked[i] = false;
    }
    battle.dodged = false;
    battle.parrying = false;
    battle.enemy_attacks++;

    DespawnProjectile();
    if (battle.attack->projectile_object != 0)
        SpawnProjectile(battle.attack->projectile_object, GetEnemyActor(),
                        BATTLE_PARTY[battle.attack_target]);
    if (battle.attack->walk_to_target)
        StartEnemyWalk(GetEnemyActor(), BATTLE_PARTY[battle.attack_target]);
    else
        PlayBattleAnimation(GetEnemyActor(), battle.attack->enemy_anim);

    if (battle.attack->hits_party) {
        for (int i = 0; i < ARRAY_LENGTH(BATTLE_PARTY); i++)
            PlayBattleEffect(BATTLE_PARTY[i], battle.attack->windup_effect);
    } else {
        PlayBattleEffect(BATTLE_PARTY[battle.attack_target], battle.attack->windup_effect);
    }

    PlaySeByIdVolumeWrapper(battle.attack->windup_se);
    CreatePromptBoxes();
    battle.timer = battle.attack->windup_frames;
    battle.state = BATTLE_STATE_ENEMY_WINDUP;
}

static bool UpdateDodgeInput(struct buttons* held, bool in_window) {
    bool buttons[] = { held->x, held->a, held->b };
    bool dodged = true;

    for (int i = 0; i < ARRAY_LENGTH(BATTLE_PARTY); i++) {
        if (!battle.attack->hits_party && i != battle.attack_target)
            continue;
        if (!in_window)
            battle.dodge_blocked[i] = buttons[i];
        else if (!buttons[i])
            battle.dodge_blocked[i] = false;
        else if (!battle.dodging[i] && !battle.dodge_blocked[i]) {
            battle.dodging[i] = true;
            battle.prompt_dirty = true;
            PlayBattleAnimation(BATTLE_PARTY[i], BATTLE_ANIM_DODGE);
        }
        dodged = dodged && battle.dodging[i];
    }
    if (battle.prompt_dirty)
        PlaySeByIdVolumeWrapper(BATTLE_SE_BARRIER);
    return dodged;
}

static void UpdateBattleInput(int frames_to_impact) {
    if (battle.dodged || battle.parrying)
        return;

    struct buttons pressed = {0};
    struct buttons held = {0};
    GetPressedButtons(0, &pressed);
    GetHeldButtons(0, &held);

    if (pressed.y) {
        if (frames_to_impact <= BATTLE_PARRY_FRAMES) {
            battle.parrying = true;
            battle.prompt_dirty = true;
            PlaySeByIdVolumeWrapper(BATTLE_SE_BARRIER);
            PlayPartyAnimation(BATTLE_ANIM_ATTACK);
        }
    } else if (UpdateDodgeInput(&held, frames_to_impact <= BATTLE_DODGE_FRAMES)) {
        battle.dodged = true;
    }

    if (battle.prompt_dirty) {
        battle.prompt_dirty = false;
        RedrawPromptBoxes();
    }
}

static void StartBattlePause(enum battle_state next_state) {
    battle.timer = BATTLE_TURN_PAUSE_FRAMES;
    battle.pause_next_state = next_state;
    battle.state = BATTLE_STATE_PAUSE;
}

static void ResolveEnemyStrike(void) {
    int enemy = GetEnemyActor();
    int target = BATTLE_PARTY[battle.attack_target];

    DespawnProjectile();
    ClosePromptBoxes();
    PlayBattleAnimation(enemy, BATTLE_ANIM_IDLE);

    if (battle.attack->hits_party) {
        for (int i = 0; i < ARRAY_LENGTH(BATTLE_PARTY); i++)
            PlayBattleEffect(BATTLE_PARTY[i], battle.attack->hit_effect);
    } else {
        PlayBattleEffect(target, battle.attack->hit_effect);
    }
    if (!battle.dodged)
        PlaySeByIdVolumeWrapper(battle.attack->hit_se);

    if (battle.parrying) {
        battle.enemy_hp -= battle.attack->damage / 2;
        ShowDamageBox(battle.attack->damage / 2, true);
        PlayBattleAnimation(enemy, BATTLE_ANIM_HURT);
        PlayBattleEffect(enemy, battle.attack->hit_effect);
    } else if (!battle.dodged) {
        battle.team_hp -= battle.attack->damage;
        ShowDamageBox(battle.attack->damage, false);
        PlaySeByIdVolumeWrapper(BATTLE_SE_HURT);
        if (battle.attack->hits_party)
            PlayPartyAnimation(BATTLE_ANIM_HURT);
        else
            PlayBattleAnimation(target, BATTLE_ANIM_HURT);
    }

    battle.timer = BATTLE_ENEMY_FINISH_FRAMES;
    battle.state = BATTLE_STATE_ENEMY_FINISH;
}

void CreateBattle(void) {
    battle.state = BATTLE_STATE_INTRO;
    battle.team_hp = BATTLE_MAX_TEAM_HP;
    battle.enemy_hp = battle_current == BATTLE_BOSS ? BATTLE_BOSS_HP : BATTLE_GOON_HP;
    battle.attack = NULL;
    battle.timer = 60;
    battle.dodged = false;
    battle.projectile = -1;
    battle.damage_window = -1;
    for (int i = 0; i < ARRAY_LENGTH(BATTLE_PROMPTS); i++)
        battle.prompt_windows[i] = -1;
    battle.music_frame = 0;
    battle.lyric_window = -1;
    battle.lyric = -1;
    battle.invincibility_frames = 0;
    battle.parrying = false;
    battle.menu_window = -1;
    battle.menu_option = BATTLE_OPTION_ATTACK;
    battle.item_window = -1;
    battle.item_option = BATTLE_ITEM_ORAN_BERRY;
    battle.hp_window = -1;
    battle.message_window = -1;
    battle.portrait_window = -1;
    battle.ran_away = 0;
    battle.enemy_attacks = 0;
    battle.turn = 0;
    battle.monologue = 0;
    CreateHpBox();
    StopBgm(0);
    PlayBgmByIdVeneer(battle_current == BATTLE_BOSS ? BATTLE_BOSS_MUSIC : BATTLE_GOON_MUSIC);
}

void CreateBattle1(void) { battle_current = BATTLE_BOSS; CreateBattle(); }
void CreateBattle2(void) { battle_current = BATTLE_GOON; CreateBattle(); }

void CloseBattle(void) {
    CloseBattleMenu();
    CloseBattleItemMenu();
    CloseBattleMessage();
    CloseHpBox();
    CloseDamageBox();
    ClosePromptBoxes();
    CloseLyricBox();
    DespawnProjectile();
    if (battle_current == BATTLE_BOSS) {
        fought_goon = false;
    }
    battle_current = -1;
}

bool UpdateBattle(void) {
    UpdateLyrics();

    if (battle.hp_window >= 0 && battle.shown_hp != battle.team_hp) {
        CloseHpBox();
        CreateHpBox();
    }

    switch (battle.state) {
        case BATTLE_STATE_INTRO:
            if (--battle.timer > 0)
                break;
            battle.state = BATTLE_STATE_RUN_AWAY;

            break;
        case BATTLE_STATE_INITIAL:
            CreateBattleMenu();
            battle.state = BATTLE_STATE_MAINMENU;
            break;
        case BATTLE_STATE_MAINMENU:
            if (IsAdvancedMenuActive2(battle.menu_window)) {
                battle.menu_option = GetAdvancedMenuCurrentOption(battle.menu_window);
                break;
            }

            switch (GetAdvancedMenuResult(battle.menu_window)) {
                case BATTLE_OPTION_ATTACK:
                    StartPartyAttack();
                    break;
                case BATTLE_OPTION_ITEM:
                    battle.state = BATTLE_STATE_ITEMMENU;
                    break;
                case BATTLE_OPTION_RUN_AWAY:
                    battle.state = BATTLE_STATE_RUN_AWAY;
                    break;
                default:
                    battle.state = BATTLE_STATE_INITIAL;
                    break;
            }
            CloseBattleMenu();
            break;
        case BATTLE_STATE_ITEMMENU:
            if (battle.item_window < 0) {
                CreateBattleItemMenu();
                break;
            }
            if (IsAdvancedMenuActive2(battle.item_window)) {
                battle.item_option = GetAdvancedMenuCurrentOption(battle.item_window);
                break;
            }

            switch (GetAdvancedMenuResult(battle.item_window)) {
                case BATTLE_ITEM_ORAN_BERRY: {
                    int amount = GetOranBerryAmount();
                    CloseBattleItemMenu();
                    if (amount <= 0) {
                        char buffer[0x64];
                        snprintf(buffer, sizeof(buffer), StringFromId(10033));
                        CreateBattleMessage(buffer, MONSTER_KINGAMBIT, BATTLE_STATE_ITEMMENU);
                        break;
                    }
                    SetOranBerryAmount(amount - 1);
                    PlaySeByIdVolumeWrapper(BATTLE_SE_HEAL);
                    battle.team_hp += BATTLE_ORAN_BERRY_HEAL;
                    if (battle.team_hp > BATTLE_MAX_TEAM_HP)
                        battle.team_hp = BATTLE_MAX_TEAM_HP;
                    battle.state = BATTLE_STATE_INITIAL;
                    break;
                }
                default:
                    CloseBattleItemMenu();
                    battle.state = BATTLE_STATE_INITIAL;
                    break;
            }
            break;
        case BATTLE_STATE_RUN_AWAY:
            char buffer[0x80];
            if (battle.ran_away > 1) {
                snprintf(buffer, sizeof(buffer), StringFromId(10032));
            } else if (battle_current == BATTLE_GOON && fought_goon && battle.ran_away == 0) {
                snprintf(buffer, sizeof(buffer), StringFromId(10031));
            } else {
                snprintf(buffer, sizeof(buffer), StringFromId(10030));
            }
            fought_goon = true;
            CreateBattleMessage(buffer, MONSTER_KINGAMBIT, BATTLE_STATE_INITIAL);
            battle.ran_away++;
            break;
        case BATTLE_STATE_MESSAGE:
            if (!IsDialogueBoxActive(battle.message_window)) {
                CloseBattleMessage();
                battle.state = battle.message_next_state;
            }
            break;
        case BATTLE_STATE_ATTACK:
            if (--battle.timer > 0)
                break;

            PlayPartyAnimation(BATTLE_ANIM_IDLE);
            battle.turn++;
            battle.enemy_attacks = 0;
            int damage = RandRange(BATTLE_PARTY_DAMAGE_MIN, BATTLE_PARTY_DAMAGE_MAX + 1);
            battle.enemy_hp -= damage;
            ShowDamageBox(damage, true);
            PlaySeByIdVolumeWrapper(BATTLE_SE_HIT);
            PlayBattleAnimation(GetEnemyActor(), BATTLE_ANIM_HURT);

            if (!CheckBattleOver())
                StartBattlePause(BATTLE_STATE_ENEMY_START);
            break;
        case BATTLE_STATE_ENEMY_START:
            if (battle_current == BATTLE_BOSS && battle.monologue < ARRAY_LENGTH(BOSS_MONOLOGUE) && battle.turn >= battle.monologue * 2 + 1) {
                PlayBattleAnimation(GetEnemyActor(), BATTLE_ANIM_IDLE);
                const char* text = StringFromId(BOSS_MONOLOGUE[battle.monologue].id);
                if (text != NULL) {
                    CreateBattleMessageForBoss(text, BOSS_MONOLOGUE[battle.monologue].emotion);
                }
                battle.monologue++;
                break;
            }
            StartEnemyAttack();
            break;
        case BATTLE_STATE_PAUSE:
            if (--battle.timer > 0)
                break;

            CloseDamageBox();
            battle.state = battle.pause_next_state;
            break;
        case BATTLE_STATE_ENEMY_WINDUP:
            if (battle.attack->walk_to_target)
                MoveEnemy(&battle.enemy_home, &battle.enemy_dest,
                          battle.attack->windup_frames - battle.timer, battle.attack->windup_frames);
            UpdateBattleInput(battle.timer + BATTLE_DODGE_FRAMES);

            if (--battle.timer > 0)
                break;

            if (battle.attack->walk_to_target)
                PlayBattleAnimation(GetEnemyActor(), BATTLE_ANIM_IDLE);
            battle.timer = BATTLE_DODGE_FRAMES;
            battle.state = BATTLE_STATE_ENEMY_STRIKE;

            break;
        case BATTLE_STATE_ENEMY_STRIKE:
            if (battle.attack->walk_to_target && battle.timer == BATTLE_DODGE_FRAMES)
                PlayBattleAnimation(GetEnemyActor(), battle.attack->enemy_anim);
            MoveProjectile(BATTLE_DODGE_FRAMES - battle.timer, BATTLE_DODGE_FRAMES);
            UpdateBattleInput(battle.timer);

            if (--battle.timer > 0)
                break;
            ResolveEnemyStrike();

            break;
        case BATTLE_STATE_ENEMY_FINISH:
            if (--battle.timer > 0)
                break;

            battle.timer = BATTLE_ENEMY_RECOVER_FRAMES;
            battle.state = BATTLE_STATE_ENEMY_RECOVER;

            break;
        case BATTLE_STATE_ENEMY_RECOVER:
            if (battle.attack->walk_to_target)
                MoveEnemy(&battle.enemy_dest, &battle.enemy_home, BATTLE_ENEMY_RECOVER_FRAMES - battle.timer, BATTLE_ENEMY_RECOVER_FRAMES);

            if (--battle.timer > 0)
                break;

            if (battle.attack->walk_to_target)
                EndEnemyWalk();
            PlayPartyAnimation(BATTLE_ANIM_IDLE);
            PlayBattleAnimation(GetEnemyActor(), BATTLE_ANIM_IDLE);
            if (!CheckBattleOver()) {
                bool enraged = battle_current == BATTLE_BOSS && battle.enemy_hp < BATTLE_BOSS_HP / 2;
                StartBattlePause(enraged && battle.enemy_attacks < 2
                                     ? BATTLE_STATE_ENEMY_START
                                     : BATTLE_STATE_INITIAL);
            }
            break;
        case BATTLE_STATE_OVER:
            return true;
    }
    return false;
}
