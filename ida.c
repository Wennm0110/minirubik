/* ida.c — IDA* with table lookups only.
 * No division, no modulo, no recursion, no malloc. */
#include <stdint.h>
#include <stdio.h>
#include "tables.h"

static const char *const names[3][3] = {
    {"R", "R2", "R'"}, {"B", "B2", "B'"}, {"D", "D2", "D'"}};

static uint8_t  pf[12], pt[12];   /* 解答：第 i 步的面、轉幾次 */
static uint32_t nodes;

static uint8_t h(uint16_t p, uint16_t o, uint8_t s)
{
    uint8_t a = dist_p[p];
    uint8_t b = dist_os[o * NS + s];
    return a > b ? a : b;
}

/* 除錯用：印出筆記本第 0 頁到第 top 頁，g 是手指位置 */
static void dump(int g, int top, const uint16_t *P, const uint16_t *O,
                 const uint8_t *S, const uint8_t *F, const uint8_t *T,
                 const uint8_t *LF)
{
    printf("      page |    P    O   S | F T | LF | pf pt\n");
    for (int i = 0; i <= top; ++i) {
        printf("      %c %2d  | %4u %4u %3u |", i == g ? '>' : ' ', i, P[i], O[i], S[i]);
        if (i <= g) printf(" %u %u | %2u |", F[i], T[i], LF[i]);
        else        printf(" - - |  - |");
        if (i < g || (i == g && top > g))
            printf("  %u  %u  (%s)\n", pf[i], pt[i], names[pf[i]][pt[i]]);
        else
            printf("  -  -\n");
    }
}


static int dfs_iter(uint16_t p0, uint16_t o0, uint8_t s0, uint8_t bound) {
    
    uint16_t P[12], O[12];
    uint8_t S[12], F[12], T[12], LF[12];
    int g = 0;

    uint8_t e = h(p0, o0, s0);

    nodes ++;
    
    if ( e > bound ) return 0;   
    if ( e == 0 ) return 1;

    P[0] = p0;  
    O[0] = o0;
    S[0] = s0;
    F[0] = 0;
    T[0] = 0;
    LF[0] = 3;
    dump(0, 0, P, O, S, F, T, LF);

    while (g >= 0) {
        if (F[g] == 3) {
            g --;
            if (g >= 0) dump(g, g, P, O, S, F, T, LF);
            continue;
        }

        if (F[g] == LF[g]) {
            F[g] ++;
            dump(g, g, P, O, S, F, T, LF);
            continue;
        }

        uint8_t f = F[g];
        uint8_t t = T[g];


        uint16_t bp = (t == 0) ? P[g] : P[g + 1];
        uint16_t bo = (t == 0) ? O[g] : O[g + 1];
        uint8_t  bs = (t == 0) ? S[g] : S[g + 1];
        P[g + 1] = perm_move[f][bp];
        O[g + 1] = orien_move[f][bo];
        S[g + 1] = pos_move[f][bs];
        pf[g] = f; 
        pt[g] = t;
        
        T[g] ++;
        if (T[g] == 3) {
            T[g] = 0;
            F[g] ++;
        }

        nodes ++;
        e = h(P[g + 1], O[g + 1], S[g + 1]);

        if (g + e + 1 > bound) {
            dump(g, g + 1, P, O, S, F, T, LF);
            continue;
        }

        if (e == 0) {
            dump(g, g + 1, P, O, S, F, T, LF);
            return 1;
        }
        g ++;
        F[g] = 0;
        T[g] = 0;
        LF[g] = f;
        dump(g, g, P, O, S, F, T, LF);
    }

    return 0;

}



int main(void)
{
    /* 21345671111111：p = 720, o = 0, s = 7 */
    uint16_t p = 720, o = 0;
    uint8_t  s = 7;

    int bound;
    for (bound = h(p, o, s); bound <= 11; ++bound)
        if (dfs_iter(p, o, s, (uint8_t) bound))
            break;

    printf("%d moves, %u nodes:", bound, nodes);
    for (int i = 0; i < bound; ++i)
        printf(" %s", names[pf[i]][pt[i]]);
    putchar('\n');
    return 0;
}