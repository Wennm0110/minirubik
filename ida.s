    .equ    RENDER, 0
    .equ    LED_BASE, 0xf0000000
    .equ    DELAY, 20000
    .equ    TEST_FIRST, 0       # 從第幾組開始
    .equ    TEST_END, 3         # 跑到第幾組之前
    .text
main:
    addi    sp, sp, -8
    li      t0, TEST_FIRST
    sw      t0, 0(sp)               # ti = 0
    sw      x0, 4(sp)               # passed = 0
test_loop:
    lw      t0, 0(sp)               # t0 = ti
    li      t1, TEST_END
    beq     t0, t1, all_done
    slli    t3, t0, 4
    sub     t3, t3, t0              # ti * 15
    la      t1, tests
    add     t3, t3, t1              # t3 = 這一組字串的位址
    li      s11, 0
    li      t0, 0
    li      t1, 7
    la      t4, cp
    la      t5, co
parse:
    beq     t0, t1, parse_end
    add     t6, t3, t0
    lbu     t2, 0(t6)
    addi    t2, t2, -49
    add     t6, t4, t0
    sb      t2, 0(t6)
    add     t6, t3, t0
    lbu     t2, 7(t6)
    addi    t2, t2, -49
    add     t6, t5, t0
    sb      t2, 0(t6)
    addi    t0, t0, 1
    j       parse
parse_end:
    li      t0, 0
    li      t1, 6
    li      s7, 0
count_o:
    beq     t0, t1, count_o_end
    slli    t2, s7, 1
    add     s7, s7, t2
    add     t2, t5, t0
    lbu     t2, 0(t2)
    add     s7, s7, t2
    addi    t0, t0, 1
    j       count_o
count_o_end:
    li      t0, 0                       # t0 is i
    li      t1, 7                       # t1 is floor
    li      s6, 0
count_p_outer:
    beq     t0, t1, count_p_outer_end
    li      t2, 0                       # t2 is smaller
    addi    t3, t0, 1                   # t3 is j
    la      t4, cp
    add     t4, t4, t0
    lbu     t4, 0(t4)
count_p_inter:
    beq     t3, t1, count_p_inter_end
    la      t5, cp
    add     t5, t5, t3
    lbu     t5, 0(t5)
    bgeu    t5, t4, 8
    addi    t2, t2, 1
    addi    t3, t3, 1
    j       count_p_inter
count_p_inter_end:
    sub     t3, t1, t0
    li      t4, 0
mul_loop:
    beq     t3, x0, mul_loop_end
    add     t4, t4, s6
    addi    t3, t3, -1
    j       mul_loop
mul_loop_end:
    add     s6, t4, t2
    addi    t0, t0, 1
    j       count_p_outer
count_p_outer_end:
    li      t0, 0
    li      t1, 7
count_s:
    beq     t0, t1, count_s_end
    la      t2, cp
    add     t2, t2, t0
    lbu     t2, 0(t2)
    bne     t2, x0, 8
    mv      t4, t0
    li      t3, 1
    bne     t2, t3, 8
    mv      t5, t0
    addi    t0, t0, 1
    j       count_s
count_s_end:
    slli    t0, t4, 3
    sub     t4, t0, t4
    add     s8, t4, t5
    mv      a0, s6
    mv      a1, s7
    mv      a2, s8
    la      a4, dist_p
    la      a5, dist_os
    la      a6, pf
    la      a7, pt
    jal     h
    mv      s9, a0
main_loop:
    li      t0, 12
    beq     s9, t0, main_fail
    mv      a0, s6
    mv      a1, s7
    mv      a2, s8
    mv      a3, s9
    jal     dfs_iter
    bne     a0, x0, main_solved
    addi    s9, s9, 1
    j       main_loop
main_solved:
    li      s5, 1
    li      s3, 0               # s3 is k
    la      s0, pf
    la      s1, pt
    jal     led_display
    # --- 重播：k = 0 .. s9-1 ---
    #     f = pf[k]
    #     n = pt[k] + 1            （要做幾次 90°）
    #     重複 n 次：turn(f)
chk_loop:
    beq     s3, s9, chk_loop_end
    add     t2, s1, s3
    lbu     t2, 0(t2)               #t2 is n
    addi    s2, t2, 1               
chk_inner_loop:
    beq     s2, x0, chk_inner_loop_end
    add     t1, s0, s3
    lbu     t1, 0(t1)
    mv      a0, t1
    jal     turn
    addi    s2, s2, -1
    j       chk_inner_loop
chk_inner_loop_end:
    jal     led_display
    addi    s3, s3, 1
    j       chk_loop
    # --- 檢查：i = 0 .. 6 ---
    #     if (cp[i] != i) s5 = 0
    #     if (co[i] != 0) s5 = 0
chk_loop_end:
    li      s0, 0               # s0 is i
    li      s1, 7               # s1 is 7
chk_second_loop:
    beq     s0, s1, chk_second_loop_end
    la      t0, cp
    add     t0, t0, s0
    lbu     t0, 0(t0)
    la      t1, co
    add     t1, t1, s0
    lbu     t1, 0(t1)
    beq     t0, s0, 8
    mv      s5, x0
    beq     t1, x0, 8
    mv      s5, x0
    addi    s0, s0, 1
    j       chk_second_loop
chk_second_loop_end:
    j       test_end
main_fail:
    li      s5, 0
test_end:
    lw      t0, 0(sp)               # t0 = ti
    la      t1, expect
    add     t1, t1, t0
    lbu     t1, 0(t1)               # t1 = expect[ti]
    beq     s5, x0, test_next       # 重播驗證沒過
    bne     s9, t1, test_next       # 步數不是最短
    lw      t2, 4(sp)
    addi    t2, t2, 1
    sw      t2, 4(sp)               # passed++
test_next:
    addi    t0, t0, 1
    sw      t0, 0(sp)               # ti++
    j       test_loop
all_done:
    lw      a0, 4(sp)               # x10 = 通過的組數
    addi    sp, sp, 8
    li      a7, 10
    ecall

# h(p, o, s) → a0
# 輸入：a0 = p, a1 = o, a2 = s
# 輸出：a0 = max(dist_p[p], dist_os[o*49 + s])
h:
    # 1. 讀 dist_p[p]，存到某個 t 暫存器
    add     t0, a0, a4
    lbu     t0, 0(t0)
    # 2. 算出 o * 49
    slli    t1, a1, 5
    slli    t2, a1, 4
    add     t1, t1, t2
    add     t1, t1, a1
    # 3. 加上 s，得到 dist_os 的索引
    add     t1, t1, a2
    # 4. 讀 dist_os[索引]
    add     t2, t1, a5
    lbu     t1, 0(t2)
    # 5. 取兩者較大的放進 a0
    addi    a0, t0, 0
    bgeu    t0, t1, h_end
    addi    a0, t1, 0
h_end:
    ret

# apply(p, o, s, f) → 轉一次 90°
# 輸入：a0 = p, a1 = o, a2 = s, a3 = f
# 輸出：a0 = 新的 p, a1 = 新的 o, a2 = 新的 s
apply:
    la      t0, perm_move
    la      t1, orien_move
    la      t2, pos_move
    beq     a3, x0, apply_do
    li      t3, 1
    li      t4, 10080
    add     t0, t0, t4
    addi    t1, t1, 1458
    addi    t2, t2, 49
    beq     a3, t3, apply_do
    add     t0, t0, t4
    addi    t1, t1, 1458
    addi    t2, t2, 49
apply_do:
    add     t0, t0, a0
    add     t1, t1, a1
    add     t0, t0, a0
    add     t1, t1, a1
    add     t2, t2, a2
    # 1. 新的 p = perm_move[f][p]
    lhu     a0, 0(t0)
    # 2. 新的 o = orien_move[f][o]
    lhu     a1, 0(t1)
    # 3. 新的 s = pos_move[f][s]
    lbu     a2, 0(t2)
    ret

# dfs_iter(p0, o0, s0, bound)
# 輸入：a0 = p0, a1 = o0, a2 = s0, a3 = bound
# 輸出：a0 = 1 找到解 / 0 沒找到；nodes 累加
#
# 暫存器配置
#   s0  = 目前這一頁的指標（或 g，看你選排法 A 還是 B）
#   s1  = g（深度）
#   s2  = bound
#   s3  = f（這一圈試的面）
#   s4  = t（這一圈試的轉幾次）
#   s5  = e（h 的結果）
#   s10 = 保存的 ra
#   s11 = nodes
dfs_iter:
    # --- 進場：保存 ra、bound；把起點寫進第 0 頁 ---
    mv      s10, ra
    mv      s2, a3
    la      s0, pages
    sh      a0, 0(s0)
    sh      a1, 2(s0)
    sb      a2, 4(s0)
    # --- nodes++；e = h(p0, o0, s0) ---
    addi    s11, s11, 1
    jal     h        
    # --- if (e > bound) 回傳 0；if (e == 0) 回傳 1 ---
    bltu    s2, a0, dfs_fail
    beq     a0, x0, dfs_found
    # --- F[0] = 0; T[0] = 0; LF[0] = 3; g = 0 ---
    sb      x0, 5(s0)
    sb      x0, 6(s0)
    li      t0, 3
    sb      t0, 7(s0)
    mv      s1, x0

dfs_loop:
    # --- if (g < 0) 跳到 dfs_fail ---
    blt     s1, x0, dfs_fail
    # --- if (F[g] == 3) { g--; 回 dfs_loop } ---
    li      t0, 3
    lbu     t1, 5(s0)
    bne     t0, t1, 16
    addi    s1, s1, -1
    addi    s0, s0, -8
    j       dfs_loop
    # --- if (F[g] == LF[g]) { F[g]++; 回 dfs_loop } ---
    lbu     t0, 7(s0)
    bne     t0, t1, 16 
    addi    t0, t0, 1
    sb      t0, 5(s0)
    j       dfs_loop     
    # --- f = F[g]; t = T[g] ---
    # --- 選出發點：t == 0 用第 g 頁，否則用第 g+1 頁 ---
    # --- 呼叫 apply，結果寫入第 g+1 頁 ---
    # --- pf[g] = f; pt[g] = t ---
    lbu     s3, 5(s0)
    lbu     s4, 6(s0)
    lhu     a0, 0(s0)
    lhu     a1, 2(s0)
    lbu     a2, 4(s0)
    beq     s4, x0, 16
    lhu     a0, 8(s0)
    lhu     a1, 10(s0)
    lbu     a2, 12(s0)
    mv      a3, s3
    jal     ra, apply
    sh      a0, 8(s0)
    sh      a1, 10(s0)
    sb      a2, 12(s0)
    add     t0, a6, s1
    add     t1, a7, s1
    sb      s3, 0(t0)
    sb      s4, 0(t1)
    # --- 書籤前進：T[g]++；到 3 就歸零、F[g]++ ---
    addi    t1, s4, 1
    li      t0, 3
    bne     t1, t0, 16
    addi    t2, s3, 1
    mv      t1, x0
    sb      t2, 5(s0)
    sb      t1, 6(s0)
    # --- nodes++；e = h(第 g+1 頁的 p, o, s) ---
    addi    s11, s11, 1
    jal     h
    mv      s5, a0 
    # --- if (g + 1 + e > bound) 回 dfs_loop ---
    add     t0, s1, s5
    addi    t0, t0, 1
    bltu    s2, t0, dfs_loop
    # --- if (e == 0) 跳到 dfs_found ---
    beq     s5, x0, dfs_found
    # --- 往下翻：g++; F[g] = 0; T[g] = 0; LF[g] = f; 回 dfs_loop ---
    addi    s1, s1, 1
    addi    s0, s0, 8
    sb      x0, 5(s0)
    sb      x0, 6(s0)
    sb      s3, 7(s0)
    j       dfs_loop

dfs_found:
    # a0 = 1，還原 ra，ret
    mv      ra, s10
    li      a0, 1
    ret

dfs_fail:
    # a0 = 0，還原 ra，ret
    mv      ra, s10
    mv      a0, x0
    ret


# turn(a0 = f)：對 cp/co 做一次 90°
turn:
    # --- base = f * 7 ---
    slli    t0, a0, 3
    sub     t0, t0, a0              # t0 is f times 7 = base
    li      t1, 0                   # t1 is i
    li      t2, 7                   # t2 is 7
    # --- 迴圈一：i = 0..6 ---
    #     from  = source[base + i]
    #     np[i] = cp[from]
    #     x     = co[from] + twist[base + i]
    #     if (x >= 3) x -= 3
    #     no[i] = x
turn_loop:
    beq     t1, t2, turn_loop_end
    la      t3, source
    add     t4, t1, t0              # t4 is base + i
    add     t3, t3, t4
    lbu     t3, 0(t3)               # t3 is source[t4] = from
    la      t5, cp
    add     t5, t5, t3              
    lbu     t5, 0(t5)               # t5 is cp[from]
    la      t6, np
    add     t6, t6, t1
    sb      t5, 0(t6)
    la      t5, co
    add     t5, t5, t3
    lbu     t5, 0(t5)
    la      t6, twist
    add     t6, t6, t4
    lbu     t6, 0(t6)
    add     t5, t5, t6
    li      t6, 3
    bltu    t5, t6, 8
    addi    t5, t5, -3
    la      t6, no
    add     t6, t6, t1
    sb      t5, 0(t6)
    addi    t1, t1, 1
    j       turn_loop
turn_loop_end:
    # --- 迴圈二：i = 0..6 ---
    #     cp[i] = np[i]
    #     co[i] = no[i]
    li      t0, 0                   # t0 is i
    li      t3, 7                   # t3 is 7
turn_second_loop:
    beq     t0, t3, turn_second_loop_end
    la      t1, np                  # t1 is np
    la      t2, cp                  # t2 is cp
    add     t1, t1, t0
    lbu     t1, 0(t1)
    add     t2, t2, t0
    sb      t1, 0(t2)
    la      t1, no                  # t1 is no
    la      t2, co                  # t2 is co
    add     t1, t1, t0
    lbu     t1, 0(t1)
    add     t2, t2, t0
    sb      t1, 0(t2)
    addi    t0, t0, 1               
    j       turn_second_loop
turn_second_loop_end:
    ret

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
    li          t0, DELAY
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
code_end:

    .data
pages:  .zero 96           # 12 頁 × 8 bytes
pf:     .zero 12
pt:     .zero 12

source: .byte 1, 4, 2, 0, 3, 5, 6      # source[0]
        .byte 0, 1, 2, 4, 5, 6, 3      # source[1]
        .byte 0, 2, 5, 3, 1, 4, 6      # source[2]
twist:  .byte 1, 2, 0, 2, 1, 0, 0      # twist[0]
        .byte 0, 0, 0, 1, 2, 1, 2      # twist[1]
        .byte 0, 0, 0, 0, 0, 0, 0      # twist[2]
np:     .zero 7
no:     .zero 7

tests:  .string "12345671111111"
        .string "23745612123332"
        .string "21345671111111"
expect: .byte 0, 3, 11
cp:     .byte 0, 0, 0, 0, 0, 0, 0, 7
co:     .zero 8


    .align 2
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
