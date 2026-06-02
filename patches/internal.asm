.nds
.include "symbols.asm"

.open "arm9.bin", arm9_start
  .ifdef HookScriptMenuRequestCheck
  .org ShowKeyboard
    push {r3-r8,lr}
    bl HookKeyboardCheck

  .org ShowKeyboardTypeCase3
    b HookKeyboardCustomPrompt

  .org ShowKeyboardReturn
    pop {r3-r8,pc}

  .org PreprocessStringFromIdCallsite
    bl CustomPreprocessStringFromId
  .endif
.close

.open "overlay11.bin", overlay11_start
  .org ScriptSpecialProcessCall
    b cotInternalTrampolineScriptSpecialProcessCall

  .ifdef HookScriptMenuRequestCheck
  .org ScriptMenuRequestDefaultCase
    b HookScriptMenuRequestCheck
  
  .org ScriptMenuUpdateDefaultCase
    b HookScriptMenuUpdateCheck
  .endif
.close 
