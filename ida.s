    .text
main:
    li      s11, 0
    li      s6, 720
    li      s7, 0
    li      s8, 7
    mv      a0, s6
    mv      a1, s7
    mv      a2, s8
    jal     h
    mv      s9, a0
main_loop:
    mv      a0, s6
    mv      a1, s7
    mv      a2, s8
    mv      a3, s9
    jal     dfs_iter
    bne     a0, x0, main_solved
    addi    s9, s9, 1
    j       main_loop
main_solved:
    li      a7, 10
    ecall

# h(p, o, s) → a0
# 輸入：a0 = p, a1 = o, a2 = s
# 輸出：a0 = max(dist_p[p], dist_os[o*49 + s])
h:
    # 1. 讀 dist_p[p]，存到某個 t 暫存器
    la      t0, dist_p
    add     t0, a0, t0
    lbu     t0, 0(t0)
    # 2. 算出 o * 49
    slli    t1, a1, 5
    slli    t2, a1, 4
    add     t1, t1, t2
    add     t1, t1, a1
    # 3. 加上 s，得到 dist_os 的索引
    add     t1, t1, a2
    # 4. 讀 dist_os[索引]
    la      t2, dist_os
    add     t2, t2, t1
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
    la      t0, pf
    la      t1, pt
    add     t0, t0, s1
    add     t1, t1, s1
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
    lhu     a0, 8(s0)
    lhu     a1, 10(s0)
    lbu     a2, 12(s0)
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

    .data
pages:  .zero 96           # 12 頁 × 8 bytes
pf:     .zero 12
pt:     .zero 12
