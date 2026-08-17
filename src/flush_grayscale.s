CheckGrayscaleHook:
	ldr   r12,GRAY_FLAG
	cmp   r12,#0
	beq   VanillaFlush
	// This is where the fun begins...
	mov   r9,#0
	b     grayscale_loop_next_iter
grayscale_loop:
	ldrb  r0,[r4,#+0x0]
	ldrb  r1,[r4,#+0x1]
	ldrb  r2,[r4,#+0x2]
	add   r0,r0,r1
	add   r0,r0,r2
	mov   r1,#3
	bl    _s32_div_f
	mov   r6,r0 // Equal grayscale byte!
	ldr   r7,=GRAY_BYTES
	mov   r8,#2
calculate_gray_differences:
	ldrb  r11,[r4,r8]
	sub   r0,r11,r6
	bl    abs
	ldrh  r1,[r10,#+0xA]
	mov   r2,#0xFF
	sub   r1,r2,r1
	bl    UMultiplyByFixedPoint
	ldrb  r1,[r4,r8]
	cmp   r1,r6 // Compare current byte to grayscale...
	subgt r0,r1,r0
	addle r0,r1,r0
	strb  r0,[r7,r8]
	subs  r8,r8,#1
	bpl   calculate_gray_differences
	// Calculate halfword
	ldrb  r1,[r7,#+0x1]
	ldrb  r2,[r7,#+0x2]
	ldrb  r3,[r7]
	and   r1,r1,#0xF8
	and   r2,r2,#0xF8
	lsl   r1,r1,#0x2
	and   r3,r3,#0xF8
	orr   r1,r1,r2, lsl #0x7
	orr   r1,r1,r3, asr #0x3
	add   r4,r4,#0x4
	strh  r1,[r5],#0x2
	add   r9,r9,#0x1
grayscale_loop_next_iter:
	ldr   r0,[r10,#+0x4]
	cmp   r9,r0
	blt   grayscale_loop
	b     UpdatePalettesReturn

TurnOffGrayscale:
	mov   r12,#0
	str   r12,GRAY_FLAG
	pop   {r3,r4,r15} // Original instruction

GrayscaleFlush:
	mov   r3,#1
	str   r3,GRAY_FLAG
	ldrsh r1,[r4,#+0x14]
	add   r0,r4,#0x1C
	add   r2,r4,#0x16
	rsb   r1,r1,#0x100
	lsl   r1,r1,#0x10
	lsr   r1,r1,#0x10
	bl    InitFlush
custom_flush_return:
	add   r13,r13,#0x4
	pop   {r3,r4,r15}

.pool
GRAY_FLAG:
	.word 0x0
GRAY_BYTES:
	.byte 0x0, 0x0, 0x0
.align
