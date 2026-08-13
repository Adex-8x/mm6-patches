#include <pmdsky.h>
#include <cot.h>
#include "pan.h"

volatile int TextSoundPan = 0;

// defining it like this is NOT my strongest work but whatever
inline void SetTextSoundPan(int val){
    TextSoundPan = val;
}

void __attribute__((naked)) AdexIAmSoSorry(){
    asm("mov r2,r1");
    asm("ldr r3,=TextSoundPan");
    asm("mov r1,#0");
    asm("ldr r3,[r3]");
    asm("b PlaySeFull");
}
