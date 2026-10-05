    .text
main:
    jal     led_display
    li      a7, 10
    ecall


    .equ    RENDER, 1
    .equ    LED_BASE, 0xf0000000

    .data
# 六個面的顏色，索引 0=U 1=R 2=F 3=D 4=L 5=B
palette: .word 0x00FFFFFF, 0x00FF0000, 0x0000FF00, 0x00FFFF00, 0x00FF8000, 0x000000FF

# home[角塊][j] = 該角塊第 j 面的顏色索引
home:   .byte 0, 1, 2      # 0 UFR: U R F
        .byte 3, 2, 1      # 1 DFR: D F R
        .byte 3, 4, 2      # 2 DFL: D L F
        .byte 0, 5, 1      # 3 UBR: U B R
        .byte 3, 1, 5      # 4 DBR: D R B
        .byte 3, 5, 4      # 5 DBL: D B L
        .byte 0, 4, 5      # 6 UBL: U L B
        .byte 0, 2, 4      # 7 UFL: U F L（固定）

# 24 張貼紙：x, y, 位置, k
stickers:
        .byte  9,  2, 6, 0     # U
        .byte 13,  2, 3, 0
        .byte  9,  5, 7, 0
        .byte 13,  5, 0, 0
        .byte  9,  9, 7, 1     # F
        .byte 13,  9, 0, 2
        .byte  9, 12, 2, 2
        .byte 13, 12, 1, 1
        .byte 18,  9, 0, 1     # R
        .byte 22,  9, 3, 2
        .byte 18, 12, 1, 2
        .byte 22, 12, 4, 1
        .byte 27,  9, 3, 1     # B
        .byte 31,  9, 6, 2
        .byte 27, 12, 4, 2
        .byte 31, 12, 5, 1
        .byte  0,  9, 6, 1     # L
        .byte  4,  9, 7, 2
        .byte  0, 12, 5, 2
        .byte  4, 12, 2, 1
        .byte  9, 16, 2, 0     # D
        .byte 13, 16, 1, 0
        .byte  9, 19, 5, 0
        .byte 13, 19, 4, 0

cp:     .byte 1, 4, 2, 0, 3, 5, 6, 7
co:     .byte 1, 2, 0, 2, 1, 0, 0, 0

    .text
# led_display：依 cp/co 重畫整張展開圖
led_display:
    # --- 1. RENDER == 0 就 ret ---
    li          t0, RENDER
    beq         t0, x0, led_ret
    addi        sp, sp, -48
    sw          s0, 0(sp)
    sw          s1, 4(sp)
    sw          s2, 8(sp)
    sw          s3, 12(sp)
    sw          s4, 16(sp)
    sw          s5, 20(sp)
    sw          s6, 24(sp)
    sw          s7, 28(sp)
    sw          s8, 32(sp)
    sw          s9, 36(sp)
    sw          s10, 40(sp)
    sw          s11, 44(sp)
    # --- 2. n = 0 .. 23 ---
    #     讀 stickers[n] 的 x, y, pos, k
    #     c = cp[pos]; j = k + co[pos]; if (j >= 3) j -= 3
    #     color = palette[home[c*3 + j]]
    #     addr = LED_BASE + ((y*35 + x) << 2)
    #     畫 4x3 色塊
    li          s1, 24          # bound
    li          s2, 0           # n
    la          s0, stickers    # s0 is stickers address
    la          s3, cp          # cp address
    la          s4, co          # co address
    la          s5, home        # home address
    la          s6, palette     # palette address
    li          s7, LED_BASE    # LED base address
led_display_loop:
    beq         s1, s2, led_display_loop_end
    slli        t0, s2, 2
    add         t0, t0, s0
    lbu         t1, 0(t0)
    lbu         t2, 1(t0)
    lbu         t3, 2(t0)
    lbu         t4, 3(t0)
    addi        s2, s2, 1
    add         t5, t3, s3
    lbu         t5, 0(t5)
    add         t6, t3, s4
    lbu         t6, 0(t6)
    add         t6, t6, t4
    li          t3, 3
    blt         t6, t3, 8
    addi        t6, t6, -3
    slli        t3, t5, 1
    add         t3, t3, t5
    add         t3, t3, t6
    add         t3, t3, s5
    lbu         t3, 0(t3)               # t3 = home[c*3+j] = 面編號 0..5
    slli        t3, t3, 2               # palette 每筆 4 bytes
    add         t3, t3, s6
    lw          t3, 0(t3)               # t3 = color
    slli        t4, t2, 5               # y*32
    add         t4, t4, t2
    add         t4, t4, t2
    add         t4, t4, t2              # y*35
    add         t4, t4, t1              # + x
    slli        t4, t4, 2               # *4
    add         t4, t4, s7              # + LED base
    sw          t3, 0(t4)               # 第一列
    sw          t3, 4(t4)
    sw          t3, 8(t4)
    sw          t3, 12(t4)
    addi        t4, t4, 140             # 下一列（35*4）
    sw          t3, 0(t4)
    sw          t3, 4(t4)
    sw          t3, 8(t4)
    sw          t3, 12(t4)
    addi        t4, t4, 140
    sw          t3, 0(t4)               # 第三列
    sw          t3, 4(t4)
    sw          t3, 8(t4)
    sw          t3, 12(t4)
    j           led_display_loop
led_display_loop_end:
    # --- 3. 延遲迴圈 ---
    li          t0, 100
delay_loop:
    beq         t0, x0, delay_loop_end
    addi        t0, t0, -1
    j           delay_loop
delay_loop_end:
    lw          s0, 0(sp)
    lw          s1, 4(sp)
    lw          s2, 8(sp)
    lw          s3, 12(sp)
    lw          s4, 16(sp)
    lw          s5, 20(sp)
    lw          s6, 24(sp)
    lw          s7, 28(sp)
    lw          s8, 32(sp)
    lw          s9, 36(sp)
    lw          s10, 40(sp)
    lw          s11, 44(sp)
    addi        sp, sp, 48
led_ret:
    ret