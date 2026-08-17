.nds
.include "symbols.asm"

.open "arm9.bin", arm9_start
    .org SetFlushTypeDefaultCase
    .area 0x4
	    b GrayscaleFlush
    .endarea

    .org SetFlushTypeReturn+0x4
    .area 0x4
	    b TurnOffGrayscale
    .endarea

    .org SetFlushTypeQuickExit
    .area 0x4
	    beq custom_flush_return
    .endarea

    .org UpdatePalettesFlush
    .area 0x4
	    bcc CheckGrayscaleHook
    .endarea
.close
