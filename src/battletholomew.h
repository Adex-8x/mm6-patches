#pragma once

#include <pmdsky.h>
#include <cot.h>

#define BATTLE_BOSS 1
#define BATTLE_GOON 2

#define BATTLE_RESULT_LOST 0
#define BATTLE_RESULT_WON 1

extern int battle_current;
extern int battle_team_hp;

void CreateBattle(void);
void CloseBattle(void);
bool UpdateBattle(void);

void CreateBattle1(void);
void CreateBattle2(void);
