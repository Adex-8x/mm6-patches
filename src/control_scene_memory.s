.align 4
	.global SpControlSceneMemory
// ---------------------------------------------------------------------------------------------------------------
// control_scene_memory.s
// By happylappy
// Loads the save file, bitors a specified bit in a bitfield of whether a scene has been played, saves the
// game, and returns if the bit is true.
// param_1: (1 if should_load) + (2 if should_write) + (4 if should_save) + (8 if should_read)
// param_2: (Scene scn 1 = 1, scn 2 = 2, etc.) + 16 * (Branch: E = 0, C = 1, O = 2)
// Returns: If should_read is true: 1 if the specified branch has been played, 0 if not. Else Nothing.
// ----------------------------------------------------------------------------------------------------------------

	SpControlSceneMemory:
		push {r6-r11,lr}
		mov r7, r0
		mov r6, r1
		ands r10, r6, #0xF; // Put the Scene number in r10.
		subne r10, #0x1; // If non-zero, subtract 1 from the scene number to be zero-indexed.
		mov r9, r6, lsr #0x4; // Retrieve the branch index.
		// The halfwords for chansey's corner of GAME_STATE_VALUES only stores 12, 12, and 8 bits from their respective half-words.
		// In order for it to actually save, branch O's scenes 9 and 10 will be stored at the end of the half-word of branch E like so:
		// X = Not stored to save, ? = Unused, E = Branch E bit, C = Branch C bit, O = Branch O Bit
		// Halfword 0: XXXX OOEE EEEE EEEE
		// Halfword 1: XXXX ??CC CCCC CCCC
		// Halfword 2: XXXX XXXX OOOO OOOO		
		cmp r9, #0x1; // If branch is Obliteration... (Greater than Control)
		cmpgt r10, #0x8; // If scene is 9 or 10... (Greater than 8)
		addgt r10, #0x2; // XXXX XXoo OOOO OOOO -> XXXX ooXX OOOO OOOO
		movgt r9, #0x0;  // XXXX ooXX OOOO OOOO -> XXXX ooEE EEEE EEEE
		ldr r11, =GAME_STATE_VALUES
		ldr r11, [r11]
		add r11, #0x1300
		add r11, #0xA4
		add r11, r9, lsl #0x1
		tst r7, #0x1
		beq ShouldNotLoad
		mov r0, #0x3
		bl NoteLoadBase
	ShouldNotLoad:
		tst r7, #0x2
		beq ShouldNotWrite
		ldrh r8, [r11, #0x0]
		mov r0, #0x1
		orr r8, r0, lsl r10
		strh r8, [r11, #0x0]
	ShouldNotWrite:
		tst r7, #0x4
		beq ShouldNotSave
		mov r0, #0x3
		bl NoteSaveBase
	ShouldNotSave:
		ldrh r8, [r11, #0x0]
		tst r7, #0x8
		moveq r0, #0x0
		popeq {r6-r11,pc}; // Should Not Read
		mov r0, #0x1
		and r0, r0, r8, lsr r10
		pop {r6-r11,pc}
		
