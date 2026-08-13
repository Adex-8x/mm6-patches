.nds
.include "symbols.asm"

.open "overlay36.bin", overlay36_start
    .org 0x023A7080+0xB20+0x68
    .area 0x4
        b AdexIAmSoSorry
    .endarea
.close
