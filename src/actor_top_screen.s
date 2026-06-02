.align 4

CheckSpecialActorType:
	cmp r0,#0xC
	ldr r12,=#0x03002358
	b edit_attribute_bitfield
ManipulateActorLayering:
    mov r0,#61
    bl GetPerformanceFlagWithChecks
    cmp r0,#1
    ldr r12,[r7,#0x128]
edit_attribute_bitfield:
    orreq r12,#0x80000000 // Top Screen
    str r12,[r7,#0x128]
    ldrb r0,[r4,#0xa] // Original instruction
    b AfterAttributeBitfieldSwitchCase+0x4

.pool
.align 4