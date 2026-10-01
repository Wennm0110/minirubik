#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    CUBIES = 7,
    PERMUTATIONS = 5040,
    ORIENTATIONS = 729,
    STATES = PERMUTATIONS * ORIENTATIONS,
    MOVES = 9
};

typedef struct {
    uint8_t p[CUBIES], o[CUBIES];
} state_t;



static const char *const move_names[MOVES] = {"R",  "R2", "R'", "B", "B2",
                                              "B'", "D",  "D2", "D'"};
static const uint8_t inverse_move[MOVES] = {2, 1, 0, 5, 4, 3, 8, 7, 6};
/* Each destination takes a cubie from source[face][destination]. */
static const uint8_t source[3][CUBIES] = {
    {1, 4, 2, 0, 3, 5, 6},
    {0, 1, 2, 4, 5, 6, 3},
    {0, 2, 5, 3, 1, 4, 6},
};
static const uint8_t twist[3][CUBIES] = {
    {1, 2, 0, 2, 1, 0, 0},
    {0, 0, 0, 1, 2, 1, 2},
    {0, 0, 0, 0, 0, 0, 0},
};
static uint32_t level_count[12];
static uint8_t dist_p[PERMUTATIONS];
static uint16_t perm_move[3][PERMUTATIONS];
static uint8_t dist_o[ORIENTATIONS];
static uint16_t orien_move[3][ORIENTATIONS];
static uint8_t path[12];
static uint32_t nodes; 
enum { NS = 49 };                          /* s 的範圍：a*7+b，0～48 */
static uint8_t dest[3][CUBIES];            /* 位置 x 的角塊，轉完去哪 */
static uint8_t pos_move[3][NS];            /* s 轉一面後變成哪個 s */
static uint8_t dist_os[ORIENTATIONS * NS]; /* 新眼鏡的答案本 */


/* The three quarter-turns preserve the fixed front-upper-left corner. */

static state_t quarter_turn(state_t state, uint8_t face)
{
    state_t result;
    
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t from = source[face][i];
        result.p[i] = state.p[from];
        result.o[i] = (uint8_t) ((state.o[from] + twist[face][i]) % 3U);
    }
    return result;
}

static state_t apply_move(state_t state, uint8_t move)
{
    uint8_t turns = (uint8_t) (move % 3U + 1U);
    for (uint8_t i = 0; i < turns; ++i)
        state = quarter_turn(state, (uint8_t) (move / 3U));
    return state;
}


static uint32_t rank_state(const state_t *state)
{
    uint32_t p = 0, o = 0;
    
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t smaller = 0;
        
        for (uint8_t j = (uint8_t) (i + 1U); j < CUBIES; ++j)
            if (state->p[j] < state->p[i])
                ++smaller;
        p = p * (CUBIES - i) + smaller;
    }
    
    for (uint8_t i = 0; i < 6; ++i)
        o = o * 3U + state->o[i];
    return p * ORIENTATIONS + o;
}


static void unrank_state(uint32_t rank, state_t *state)
{
    uint8_t available[CUBIES] = {0, 1, 2, 3, 4, 5, 6};
    uint32_t p = rank / ORIENTATIONS, o = rank % ORIENTATIONS, f = 720;
    uint8_t sum = 0;
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t q = (uint8_t) (p / f);
        p %= f;
        state->p[i] = available[q];
        for (uint8_t j = q; j + 1U < CUBIES - i; ++j)
            available[j] = available[j + 1U];
        if (i < 5)
            f /= 6U - i;
    }
    for (uint8_t i = 6; i-- > 0;) {
        state->o[i] = (uint8_t) (o % 3U);
        sum = (uint8_t) (sum + state->o[i]);
        o /= 3U;
    }
    state->o[6] = (uint8_t) ((3U - sum % 3U) % 3U);
}


static int valid(const state_t *state)
{
    uint8_t sum = 0;
    
    for (uint8_t i = 0; i < CUBIES; ++i) {
        if (state->p[i] >= CUBIES || state->o[i] >= 3)
            return 0;
        
        for (uint8_t j = 0; j < i; ++j)
            if (state->p[j] == state->p[i])
                return 0;
        sum = (uint8_t) (sum + state->o[i]);
    }
    return sum % 3U == 0;
}



static uint8_t *build_table(uint8_t *diameter)
{
    uint8_t *toward_solved = malloc(STATES);
    uint32_t *queue = malloc((size_t) STATES * sizeof *queue);
    uint16_t permutation[3][PERMUTATIONS], orientation[3][ORIENTATIONS];
    uint32_t head = 0, tail = 1, level_end = 1;
    state_t state;
    if (!toward_solved || !queue) {
        free(toward_solved);
        free(queue);
        return NULL;
    }
    for (uint16_t rank = 0; rank < PERMUTATIONS; ++rank) {
        unrank_state((uint32_t) rank * ORIENTATIONS, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            permutation[face][rank] =
                (uint16_t) (rank_state(&next) / ORIENTATIONS);
        }
    }
    for (uint16_t rank = 0; rank < ORIENTATIONS; ++rank) {
        unrank_state(rank, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            orientation[face][rank] =
                (uint16_t) (rank_state(&next) % ORIENTATIONS);
        }
    }
    memset(toward_solved, UINT8_MAX, STATES);
    queue[0] = 0;
    toward_solved[0] = 0;
    level_count[0] = 1;
    *diameter = 0;
    while (head < tail) {
        if (head == level_end) {
            level_end = tail;
            ++*diameter;
        }
        uint32_t here = queue[head++];
        uint16_t p = (uint16_t) (here / ORIENTATIONS);
        uint16_t o = (uint16_t) (here % ORIENTATIONS);
        for (uint8_t face = 0; face < 3; ++face) {
            uint16_t next_p = p, next_o = o;
            for (uint8_t turn = 0; turn < 3; ++turn) {
                next_p = permutation[face][next_p];
                next_o = orientation[face][next_o];
                uint32_t there = (uint32_t) next_p * ORIENTATIONS + next_o;
                if (toward_solved[there] == UINT8_MAX) {
                    uint8_t move = (uint8_t) (face * 3U + turn);
                    toward_solved[there] = inverse_move[move];
                    queue[tail++] = there;
                    level_count[*diameter+1] = (uint32_t) (level_count[*diameter+1] + 1U);
                }
            }
        }
    }
    free(queue);
    if (tail != STATES) {
        free(toward_solved);
        return NULL;
    }
    return toward_solved;
}



static void build_dist_p(void)
{
    
    uint16_t queue[PERMUTATIONS];
    uint32_t head = 0, tail = 0;
    state_t state;

    /* 1. 轉移表：從 build_table 把建 permutation[][] 的那個 for 迴圈
          整段複製過來，把 permutation 改名成 perm_move */

    for (uint16_t rank = 0; rank < PERMUTATIONS; ++rank) {
        unrank_state((uint32_t) rank * ORIENTATIONS, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            perm_move[face][rank] =
                (uint16_t) (rank_state(&next) / ORIENTATIONS);
        }
    }

    /* 2. 初始化：全部標成「還沒走過」，然後放入起點 */
    memset(dist_p, 0xFF, sizeof dist_p);
    dist_p[0] = 0;
    queue[tail++] = 0;

    /* 3. BFS */
    while (head < tail) {
        uint16_t here = queue[head++];
        for (uint8_t face = 0; face < 3; ++face) {
            uint16_t next = here;
            for (uint8_t turn = 0; turn < 3; ++turn) {
                next = perm_move[face][next];                    /* 查表：再轉一次 90° */
                if (dist_p[next] == 0xFF) {
                    dist_p[next] = dist_p[here] + 1;        /* 距離 = ? */
                    queue[tail++] = next;
                }
            }
        }
    }
}

static void build_dist_o(void)
{
    
    uint16_t queue[ORIENTATIONS];
    uint32_t head = 0, tail = 0;
    state_t state;



    for (uint16_t rank = 0; rank < ORIENTATIONS; ++rank) {
        unrank_state(rank, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            orien_move[face][rank] =
                (uint16_t) (rank_state(&next) % ORIENTATIONS);
        }
    }

    /* 2. 初始化：全部標成「還沒走過」，然後放入起點 */
    memset(dist_o, 0xFF, sizeof dist_o);
    dist_o[0] = 0;
    queue[tail++] = 0;

    /* 3. BFS */
    while (head < tail) {
        uint16_t here = queue[head++];
        for (uint8_t face = 0; face < 3; ++face) {
            uint16_t next = here;
            for (uint8_t turn = 0; turn < 3; ++turn) {
                next = orien_move[face][next];                    /* 查表：再轉一次 90° */
                if (dist_o[next] == 0xFF) {
                    dist_o[next] = dist_o[here] + 1;        /* 距離 = ? */
                    queue[tail++] = next;
                }
            }
        }
    }
}


static void build_pos_move(void)
{
    /* 2a. 從 source 反推 dest */
    for (uint8_t f = 0; f < 3; ++f)
        for (uint8_t i = 0; i < CUBIES; ++i)
            dest[f][source[f][i]] = i;      

    /* 2b. 對每個 s、每一面，算出轉完後的 s */
    for (uint8_t f = 0; f < 3; ++f)
        for (uint8_t s = 0; s < NS; ++s) {
            uint8_t a = s / 7;          /* 空格 ③：從 s 解出角塊 0 的位置 */
            uint8_t b = s % 7;          /* 空格 ④：從 s 解出角塊 1 的位置 */
            pos_move[f][s] = (uint8_t) (dest[f][a] * 7 + dest[f][b]);   /* 空格 ⑤ */
        }
}

static void build_dist_os(void)
{
    static uint16_t queue[ORIENTATIONS * NS];
    uint32_t head = 0, tail = 0;

    memset(dist_os, 0xFF, sizeof dist_os);      /* 全部標成「還沒走過」 */
    uint16_t start = 1;                       /* ① */
    dist_os[start] = 0;
    queue[tail++] = start;

    while (head < tail) {
        uint16_t here = queue[head++];
        uint16_t o = here / NS;                       /* ② */
        uint8_t  s = here % NS;                       /* ③ */
        for (uint8_t face = 0; face < 3; ++face) {
            uint16_t no = o;
            uint8_t  ns = s;
            for (uint8_t turn = 0; turn < 3; ++turn) {
                no = orien_move[face][no];                       /* ④ */
                ns = pos_move[face][ns];                       /* ⑤ */
                uint16_t next = no * NS + ns;            /* ⑥ */
                if (dist_os[next] == 0xFF) {
                    dist_os[next] = dist_os[here] + 1;
                    queue[tail++] = next;
                }
            }
        }
    }
}

static uint8_t h(uint16_t p, uint16_t o) {
    return dist_p[p] > dist_o[o] ? dist_p[p] : dist_o[o];
}

static int dfs(uint16_t p, uint16_t o, uint8_t g, uint8_t bound, int last_face) {
    nodes ++;
    uint8_t est = h(p, o);
    if (bound < g + est)
        return 0;
    if(est == 0)
        return 1;
    for (int face = 0 ; face < 3 ; ++ face) {
        if (last_face == face)
            continue;
        uint16_t np = p, no = o;
        for ( int turn = 0 ; turn < 3 ; ++ turn ) {
            np = perm_move[face][np];
            no = orien_move[face][no];
            path[g] = (uint8_t) (face * 3 + turn);
            if (dfs(np, no, g + 1, bound, face))
                return 1;
        }
    }

    return 0;
}

static int ida_star(uint16_t p, uint16_t o)
{
    for (uint8_t bound = h(p, o); bound <= 11; ++bound) {   /* E. 第一輪的 bound 從多少開始？ */
        if (dfs(p, o, 0, bound, -1))
            return bound;
    }
    return -1;
}

static uint8_t depth_of(const uint8_t *table, uint32_t rank)
{
    uint16_t p = (uint16_t) (rank / ORIENTATIONS);
    uint16_t o = (uint16_t) (rank % ORIENTATIONS);
    uint8_t d = 0;
    while (p != 0 || o != 0) {
        uint8_t m = table[(uint32_t) p * ORIENTATIONS + o];
        uint8_t face = m / 3;          /* 從動作編號 m 取出哪一面 */
        uint8_t turns =  m % 3 + 1;         /* 從 m 取出轉幾次 */
        for (uint8_t t = 0; t < turns; ++t) {
            p = perm_move[face][p];
            o = orien_move[face][o];
        }
        ++d;
    }
    return d;
}

static int parse_state(const char *input, state_t *state)
{
    
    for (int i = 0; i < 14; ++i) {
        int limit = i < 7 ? 7 : 3;
        if (input[i] < '1' || input[i] > '0' + limit)
            return 0;
        (i < 7 ? state->p : state->o)[i % 7] = (uint8_t) (input[i] - '1');
    }
    return input[14] == '\0' && valid(state);
}

/* stdout is fully buffered off a terminal, so a write error surfaces at the
 * flush, not at the printf that queued the bytes. Every exit path that has
 * produced output goes through here.
 */
static int output_failed(void)
{
    return fflush(stdout) != 0 || ferror(stdout);
}

static int self_test(void)
{
    const state_t solved = {{0, 1, 2, 3, 4, 5, 6}, {0}};
    state_t state;
    for (uint8_t move = 0; move < MOVES; ++move) {
        state = solved;
        state = apply_move(state, move);
        state = apply_move(state, inverse_move[move]);
        if (memcmp(&solved, &state, sizeof solved))
            return 0;
    }
    for (uint32_t rank = 0; rank < STATES; ++rank) {
        unrank_state(rank, &state);
        if (!valid(&state) || rank_state(&state) != rank)
            return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    state_t state;
    uint8_t diameter;
    if (argc == 2 && !strcmp(argv[1], "--self-test")) {
        if (!self_test()) {
            fputs("self-test failed\n", stderr);
            return 1;
        }
        uint8_t *table = build_table(&diameter);
        build_dist_p();
        build_dist_o();
        uint32_t dcount[16] = {0};          /* 每個距離各幾個排列 */
        uint8_t  dmax = 0;
        for (uint16_t r = 0; r < PERMUTATIONS; ++r) {
            if (dist_p[r] == 0xFF) {        /* 沒走到 → 表沒填滿 */
                fprintf(stderr, "dist_p[%u] unfilled\n", (unsigned) r);
                return 1;
            }
            dcount[dist_p[r]]++;
            if (dist_p[r] > dmax)                        /* 更新最大值 */
                dmax = dist_p[r];
        }
        for (uint8_t d = 0; d <= dmax; ++d)
            printf("dist_p %u: %u perms\n", (unsigned) d, (unsigned) dcount[d]);
        printf("dist_p max = %u\n", (unsigned) dmax);

        dmax = 0;
        memset(dcount, 0, sizeof dcount);
        for (uint16_t r = 0; r < ORIENTATIONS; ++r) {
            if (dist_o[r] == 0xFF) {        /* 沒走到 → 表沒填滿 */
                fprintf(stderr, "dist_o[%u] unfilled\n", (unsigned) r);
                return 1;
            }
            dcount[dist_o[r]]++;
            if (dist_o[r] > dmax)                        /* 更新最大值 */
                dmax = dist_o[r];
        }
        for (uint8_t d = 0; d <= dmax; ++d)
            printf("dist_o %u: %u orien\n", (unsigned) d, (unsigned) dcount[d]);
        printf("dist_o max = %u\n", (unsigned) dmax);

        for (uint8_t i = 0; i <= diameter; ++i)
            printf("level %u: %u states\n", (unsigned) i, (unsigned) level_count[i]);
        if (!table) {
            fputs("could not build complete state table\n", stderr);
            return 1;
        }
        free(table);
        if (diameter != 11) {
            fputs("BFS check failed\n", stderr);
            return 1;
        }
        puts("3674160 states; diameter 11");
        return output_failed();
    }
    if (argc == 2 && !strcmp(argv[1], "--sweep")) {
        uint8_t *table = build_table(&diameter);
        build_dist_p();
        build_dist_o();
        uint32_t count = 0, worst_nodes = 0, worst_rank = 0;
        for (uint32_t r = 0; r < STATES; ++r) {
            if (depth_of(table, r) != 11)
                continue;
            nodes = 0;
            int len = ida_star((uint16_t) (r / ORIENTATIONS), (uint16_t) (r % ORIENTATIONS));
            if (len != 11) {
                printf("WRONG: rank %u got %d moves\n", r, len);
                return 1;
            }
            ++count;
            if (nodes > worst_nodes) {
                worst_nodes = nodes;
                worst_rank = r;
            }
        }
        printf("%u states at depth 11, worst = %u nodes (rank %u)\n",
            count, worst_nodes, worst_rank);
        free(table);
        return 0;
    }
    if (argc != 2 || !parse_state(argv[1], &state)) {
        /* C99 5.1.2.2.1 lets argv[0] be null when argc is 0. */
        fprintf(stderr, "usage: %s PPPPPPPOOOOOOO\n",
                argc > 0 && argv[0] ? argv[0] : "solver");
        return 2;
    }


    build_dist_p();
    printf("h_p = %u\n", (unsigned) dist_p[rank_state(&state) / ORIENTATIONS]);
    build_dist_o();
    printf("h_o = %u\n", (unsigned) dist_o[rank_state(&state) % ORIENTATIONS]);

    uint32_t r = rank_state(&state);
    int len = ida_star((uint16_t) (r / ORIENTATIONS), (uint16_t) (r % ORIENTATIONS));
    printf("IDA*: %d moves, %u nodes:", len, nodes);
    for (int i = 0; i < len; ++i)
        printf(" %s", move_names[path[i]]);
    putchar('\n');

    uint8_t *table = build_table(&diameter);
    if (!table) {
        fputs("could not build complete state table\n", stderr);
        return 1;
    }
    const char *separator = "";
    for (uint32_t rank = rank_state(&state); rank; rank = rank_state(&state)) {
        uint8_t move = table[rank];
        printf("%s%s", separator, move_names[move]);
        separator = " ";
        state = apply_move(state, move);
    }
    putchar('\n');
    free(table);
    return output_failed();
}
