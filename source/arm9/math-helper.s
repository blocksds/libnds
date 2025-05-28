.syntax  unified
.arm
.text
.balign 4
.global start_div64_64
.type   start_div64_64, %function
start_div64_64:
    mov r12, #0x04000000;
    add r12, r12, #0x290;
    stmia r12, {r0-r3};
    ldr r0, [r12, #-0x10];
    mov r1, #2;
    and r0, r0, #3;
    cmp r0, #2;
    strne r1, [r12, #-0x10];
    bx lr;
.arm
.text
.balign 4
.global finish_div64_64
.type finish_div64_64, %function
finish_div64_64:
    mov r1, #0x04000000;
    add r1, r1, #0x290;
    ldr r0, [r1, #-0x10];
1:
    tst r0, #0x8000;
    ldrne r0, [r1, #-0x10];
    bne 1b;
    ldrd r0, r1,[r1, #0x10];
    bx lr
