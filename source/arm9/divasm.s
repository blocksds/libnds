#include <nds/asminc.h>
.syntax unified
.arm
#if 1
BEGIN_ASM_FUNC reciprocaldivf32_asm
        //calculates quotient r0/r1 in r0 as a 20.12 number
        //normalize denominator and find reciprocal
        clz     r2, r1
        lsl     r1, r1, r2
        mov     r3, #1u<<31
.irp x, 1,2,3,4,5,6,7 //might need more
        adds   r12,r1,r1,lsr #\x 
        movcc  r1,r12
        addcc  r3, r3,r3,lsr #\x //<-this cant overflow, I think
.endr
        lsr     r1, r1, #1
        rsb     r1, r1, #0 //2's complement abuse
        umull   r12, r3, r1, r3
        clz     r12, r0
        lsl     r0, r0, r12  //normalize numerator so we have less work later
        umull   r1, r0, r3,r0//multiply input with reciprocal
        rsb     r1, r2, r12
#if 0
        bx lr   //if you wish to return a float with mantissa in r0, and exponent in r1, use this.
#endif
        adds    r1,r1, #17        //18 w/o rounding. 

        bmi   1f //should not be taken normally
        lsr   r0, r0, r1
        adds  r0,r0, #1
        rrx   r0,r0  //if overflow, shift carry back in
        bx    lr
1:      neg   r1, r1
        lsls  r0, r0, r1 //if we shift into carry, set carry
        rrx   r0, r0 //shift carry back in   
        bx lr

#endif

#if 0
//below function should be correct, but isnt as optimized because it's C-code with minor fixes
BEGIN_ASM_FUNC reciprocaldivf32_asm
        clz     ip, r1
        rsbs    r3, ip, #1
        push    {r4, lr}
        submi     r3, ip, #1
        lslmi     r1, r1, r3
        lsrpl     r1, r1, r3
        adds    lr, r1, r1, lsr #1
        movmi   lr, r1
        add     r3, lr, lr, lsr #2
        movpl   r2, #-268435456
        movmi   r2, #-1610612736
        movmi   r1, #-2147483648
        movpl   r1, #-1073741824
        cmp     r3, #0
        movlt   r3, lr
        movlt   r2, r1
        adds    r1, r3, r3, lsr #3
        movpl   r3, r1
        addpl   r2, r2, r2, lsr #3
        adds    r1, r3, r3, lsr #4
        movpl   r3, r1
        addpl   r2, r2, r2, lsr #4
        adds    r1, r3, r3, lsr #5
        movpl   r3, r1
        addpl   r2, r2, r2, lsr #5
        adds    r1, r3, r3, lsr #6
        movpl   r3, r1
        addpl   r2, r2, r2, lsr #6
        adds    r1, r3, r3, lsr #7
        movpl   r3, r1
        addpl   r2, r2, r2, lsr #7
        adds    r1, r3, r3, lsr #8
        movpl   r3, r1
        addpl   r2, r2, r2, lsr #8
        rsb     r3, r3, #0
        umull   r1, r3, r2, r3
        umull   r2, r1, r3, r0
        rsb     r4, ip, #50
        sub     lr, ip, #18
        lsr     r0, r2, r4
        rsb     ip, ip, #18
        orr     r0, r0, r1, lsl lr
        orr     r0, r0, r1, lsr ip
        pop     {r4, pc}

#endif
