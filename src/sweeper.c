#include <pmdsky.h>
#include <cot.h>
#include "extern.h"
#include "sweeper.h"
//Pinesweeper
int status[16];
int ds[8] = {-1, -1, -1,  0,  0,  1,  1,  1};
int dz[8] = {-1,  0,  1, -1,  1, -1,  0,  1};
int aufgedeckt[16];

int blockedSolutions(){
    if (status[1] == 1 && status[7] == 1 && status[8] == 1 && status[14] == 1 || status[2] == 1 && status[4] == 1 && status[11] == 1 && status[13] == 1){
        return 0;
    }
    else {
        return 1;
    }
}

void zufallsgeneration(){//+ init
    int counter;
    for (int i = 0; i < 18; i++) {
    counter = 200 + i;
    SaveScriptVariableValueAtIndex(NULL, VAR_SCENARIO_TALK_BIT_FLAG, counter, 0);//init, 1 -> Tagged(0-15blocks), 16 Mode toggle
    }
    do {
        for (int i = 0; i < 16; i++) {
            aufgedeckt[i] = 0;
        }
        for (int i = 0; i < 16; i++) {
            status[i] = 0;
        }
        for (int i = 0; i < 4; i++) {
            status[i+1] = 1;
        }
        for (int i = 16 - 1; i > 0; i--) {
        int j = RandRange(0, i + 1);
        int temp = status[i];
        status[i] = status[j];
        status[j] = temp;
        }
    } while (!blockedSolutions());
}

int getflaechenInfo(int posIndex){
    int spalte = posIndex / 4;
    int zeile = posIndex % 4;
    int zähler = 0;
    if (status[posIndex] == 1){
        return 6;//Tanza
    }
    for (int i = 0; i < 8; i++) {
        int nr = spalte + ds[i];
        int nc = zeile + dz[i];

        if (nr >= 0 && nr < 4 && nc >= 0 && nc < 4) {
            if (status[nr * 4 + nc] == 1) {
                zähler++;
            }
        }
    }
    return zähler;
}

void tanzaPos(){
    int xOffset;
    int yOffset;
    if (TSXPosLastMitDrag < 96 && TSXPosLastMitDrag > 64){
        xOffset = 0;
    }
    else if (TSXPosLastMitDrag < 128 && TSXPosLastMitDrag > 96){
        xOffset = 1;
    }
    else if (TSXPosLastMitDrag < 160 && TSXPosLastMitDrag > 128){
        xOffset = 2;
    }
    else if (TSXPosLastMitDrag < 192 && TSXPosLastMitDrag > 160){
        xOffset = 3;
    }

    if (TSYPosLastMitDrag < 64 && TSYPosLastMitDrag > 32){
        yOffset = 0;
    }
    else if (TSYPosLastMitDrag < 96 && TSYPosLastMitDrag > 64){
        yOffset = 1;
    }
    else if (TSYPosLastMitDrag < 128 && TSYPosLastMitDrag > 96){
        yOffset = 2;
    }
        else if (TSYPosLastMitDrag < 160 && TSYPosLastMitDrag > 128){
        yOffset = 3;
    }
    SaveScriptVariableValueAtIndex(NULL, VAR_POSITION_X, 2, (64 + xOffset * 32 + 16) *256);
    SaveScriptVariableValueAtIndex(NULL, VAR_POSITION_Y, 2, (32 + yOffset * 32 + 20) *256);
}

int checkFlaechen(){//Links Nach Rechts, Oben nach Unten, Erst Grid(1-16), dann Sonderknöpfe(17-19)
    int TSX = TSXPosLive;
    int TSY = TSYPosLive;
    switch(TSPressed){
        case 0:
        return 0;
        break;
        case 1:
        if (TSX >= 64 && TSX < 192 && TSY >= 32 && TSY < 160){
            int spalte = (TSX - 64) / 32;
            int zeile = (TSY - 32) / 32;
            int indexFlaeche = zeile * 4 + spalte; 
            tanzaPos();
            SaveScriptVariableValueAtIndex(NULL, VAR_CRYSTAL_COLOR_01, 0, getflaechenInfo(indexFlaeche));//Info über Tanza u in 8 Block Radius
            return indexFlaeche+1;
        }
        //Sonderzeug
        else if (TSX < 35 && TSX > 3 && TSY < 35 && TSY > 3){
            return 17;
        }
        else if (TSX < 253 && TSX > 221 && TSY < 35 && TSY > 3){
            return 18;
        }
        else if (TSX < 253 && TSX > 221 && TSY < 189 && TSY > 157){
            return 19;
        }
        default:
        return 128;
        break;
    }
    return 128; 
}

int checkTSDruck(){
    switch(TSPressed){
        case 0:
        return 0;
        case 1:
        return 1;
    }
    return 128;
}

int checkTagFlag(int posIndex){
    int flag = LoadScriptVariableValueAtIndex(NULL, VAR_SCENARIO_TALK_BIT_FLAG, posIndex);
    int mode = LoadScriptVariableValueAtIndex(NULL, VAR_SCENARIO_TALK_BIT_FLAG, 216);
    if (flag == 0){
        if (mode == 1){
            SaveScriptVariableValueAtIndex(NULL, VAR_SCENARIO_TALK_BIT_FLAG, posIndex, 1);
        }
        return 0;
    }
    else if(flag == 1){
        if (mode == 1){
            SaveScriptVariableValueAtIndex(NULL, VAR_SCENARIO_TALK_BIT_FLAG, posIndex, 0);
        }
        return 1;
    }
    else {
        return 128;
    }
}

void aufdecken(int posIndex){
    aufgedeckt[posIndex - 200] = 1;
}

int checkAufgedeckt(int posIndex){
    int i = posIndex - 200;
    if (aufgedeckt[i] == 0){
        return 0;
    }
    else if (aufgedeckt[i] == 1){
        return 1;
    }
    else {
        return 128;
    }
}

int checkWin(){
    for (int i = 0; i < 16; i++){
        if (status[i] == 1){
            int tagged = LoadScriptVariableValueAtIndex(NULL, VAR_SCENARIO_TALK_BIT_FLAG, 200 + i);
            if (tagged != 1){
                return 0;
            }
        }
        else {
            if (aufgedeckt[i] != 1){
                return 0;
            }
        }
    }
    return 1; 
}

void positionAnpassen(){
    int x = LoadScriptVariableValueAtIndex(NULL, VAR_POSITION_X, 0);
    int y = LoadScriptVariableValueAtIndex(NULL, VAR_POSITION_Y, 0);
    SaveScriptVariableValueAtIndex(NULL, VAR_POSITION_X, 0, x-256);
    SaveScriptVariableValueAtIndex(NULL, VAR_POSITION_Y, 0, y+3*256);
    x = LoadScriptVariableValueAtIndex(NULL, VAR_POSITION_X, 1);
    y = LoadScriptVariableValueAtIndex(NULL, VAR_POSITION_Y, 1);
    SaveScriptVariableValueAtIndex(NULL, VAR_POSITION_X, 1, x+256);
    SaveScriptVariableValueAtIndex(NULL, VAR_POSITION_Y, 1, y+3*256);
    x = LoadScriptVariableValueAtIndex(NULL, VAR_POSITION_X, 2);
    y = LoadScriptVariableValueAtIndex(NULL, VAR_POSITION_Y, 2);
    SaveScriptVariableValueAtIndex(NULL, VAR_POSITION_X, 2, x+256);
    SaveScriptVariableValueAtIndex(NULL, VAR_POSITION_Y, 2, y+2*256);
}