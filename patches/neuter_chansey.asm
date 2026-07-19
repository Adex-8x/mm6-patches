.nds
.include "symbols.asm"

// Control has it's price. Unfortunately, Chansey will never be able to interact with eggs again.

.open "arm9.bin", arm9_start
	.org SetEggSpecies
	.area 0x4
		bx lr
	.endarea

	.org SetUnkGameState0x13a6
	.area 0x4
		bx lr
	.endarea

	.org SetEggHatchTimer
	.area 0x4
		bx lr
	.endarea

	.org DecrementEggHatchTimer
	.area 0x4
		bx lr
	.endarea
.close