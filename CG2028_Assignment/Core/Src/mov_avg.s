/*
 * mov_avg.s
 *
 * Created on: 2/2/2026
 * Author: Hitesh B, Hou Linxin
 */
.syntax unified
 .cpu cortex-m4
 .thumb
 .global mov_avg
 .equ N_MAX, 8
 .bss
 .align 4

 .text
 .align 2
@ CG2028 Assignment, Sem 2, AY 2025/26
@ (c) ECE NUS, 2025
@ Write Student 1’s Name here: ABCD (A1234567R)
@ Write Student 2’s Name here: YAO XIANG (A0299826E)
@ You could create a look-up table of registers here:
@ R0 ...
@ R1 ...
@ write your program from here:
mov_avg:
    PUSH {r2-r11, lr}   @ Save registers (Teaching Team Template)

    MOV r2, #0          @ r2 = loop counter (i = 0)
    MOV r3, #0          @ r3 = accumulator (sum = 0)

loop:
    CMP r2, r0          @ Compare i with N (r0)
    BGE end_loop        @ If i >= N, exit loop

    LDR r4, [r1, r2, LSL #2]  @ Load accel_buff[i] into r4. (Offset = i * 4 bytes)
    ADD r3, r3, r4      @ sum += accel_buff[i]

    ADD r2, r2, #1      @ i++
    B loop              @ Repeat loop

end_loop:
    SDIV r0, r3, r0     @ r0 = sum / N (preserves C-parity truncating towards zero)

    POP {r2-r11, pc}    @ Restore registers and return
