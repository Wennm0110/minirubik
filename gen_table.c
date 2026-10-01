/* gen_tables.c — host-only table generator for the IDA* solver.
 *
 * Builds every table the search needs, verifies them (gate H2),
 * and writes them out as:
 *   tables.h  — C arrays, for ida.c (host and rv32 gcc builds)
 *   tables.s  — assembler data, for the hand-written RV32I solver
 *
 * Everything that needs division, Lehmer ranking or state_t lives here,
 * so the search itself only ever does table lookups.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    CUBIES = 7,
    PERMUTATIONS = 5040,
    ORIENTATIONS = 729,
    NS = 49 /* s = a*7 + b: positions of cubies 0 and 1 */
};

typedef struct {
    uint8_t p[CUBIES], o[CUBIES];
} state_t;

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

/* ---- tables written to tables.h / tables.s ---- */
static uint16_t perm_move[3][PERMUTATIONS];
static uint16_t orien_move[3][ORIENTATIONS];
static uint8_t pos_move[3][NS];
static uint8_t dist_p[PERMUTATIONS];
static uint8_t dist_os[ORIENTATIONS * NS];

/* ---- helpers (host only) ---- */
static uint8_t dest[3][CUBIES]; /* position x's cubie goes to dest[f][x] */

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
        for (uint8_t j = q; j + 1U < (unsigned) (CUBIES - i); ++j)
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

/* ---- builders ---- */
static void build_perm_move(void)
{
    state_t state;
    for (uint16_t rank = 0; rank < PERMUTATIONS; ++rank) {
        unrank_state((uint32_t) rank * ORIENTATIONS, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            perm_move[face][rank] =
                (uint16_t) (rank_state(&next) / ORIENTATIONS);
        }
    }
}

static void build_orien_move(void)
{
    state_t state;
    for (uint16_t rank = 0; rank < ORIENTATIONS; ++rank) {
        unrank_state(rank, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            orien_move[face][rank] =
                (uint16_t) (rank_state(&next) % ORIENTATIONS);
        }
    }
}

static void build_pos_move(void)
{
    for (uint8_t f = 0; f < 3; ++f)
        for (uint8_t i = 0; i < CUBIES; ++i)
            dest[f][source[f][i]] = i;
    for (uint8_t f = 0; f < 3; ++f)
        for (uint8_t s = 0; s < NS; ++s) {
            uint8_t a = s / 7, b = s % 7;
            pos_move[f][s] = (uint8_t) (dest[f][a] * 7 + dest[f][b]);
        }
}

/* needs perm_move */
static void build_dist_p(void)
{
    uint16_t queue[PERMUTATIONS];
    uint32_t head = 0, tail = 0;
    memset(dist_p, 0xFF, sizeof dist_p);
    dist_p[0] = 0;
    queue[tail++] = 0;
    while (head < tail) {
        uint16_t here = queue[head++];
        for (uint8_t face = 0; face < 3; ++face) {
            uint16_t next = here;
            for (uint8_t turn = 0; turn < 3; ++turn) {
                next = perm_move[face][next];
                if (dist_p[next] == 0xFF) {
                    dist_p[next] = (uint8_t) (dist_p[here] + 1);
                    queue[tail++] = next;
                }
            }
        }
    }
}

/* needs orien_move and pos_move */
static void build_dist_os(void)
{
    static uint16_t queue[ORIENTATIONS * NS];
    uint32_t head = 0, tail = 0;
    memset(dist_os, 0xFF, sizeof dist_os);
    uint16_t start = 0 * NS + 1; /* o = 0, s = 0*7+1 */
    dist_os[start] = 0;
    queue[tail++] = start;
    while (head < tail) {
        uint16_t here = queue[head++];
        uint16_t o = here / NS;
        uint8_t s = here % NS;
        for (uint8_t face = 0; face < 3; ++face) {
            uint16_t no = o;
            uint8_t ns = s;
            for (uint8_t turn = 0; turn < 3; ++turn) {
                no = orien_move[face][no];
                ns = pos_move[face][ns];
                uint16_t next = (uint16_t) (no * NS + ns);
                if (dist_os[next] == 0xFF) {
                    dist_os[next] = (uint8_t) (dist_os[here] + 1);
                    queue[tail++] = next;
                }
            }
        }
    }
}

/* ---- H2: tables fully populated, verified max, solved entry is 0 ---- */
static int check_table(const char *name, const uint8_t *t, uint32_t n,
                       uint32_t want_filled, uint32_t solved_idx)
{
    uint32_t filled = 0;
    uint8_t mx = 0;
    for (uint32_t i = 0; i < n; ++i)
        if (t[i] != 0xFF) {
            ++filled;
            if (t[i] > mx)
                mx = t[i];
        }
    printf("%-8s filled %6u / %6u, max %u, solved entry %u\n", name,
           (unsigned) filled, (unsigned) n, mx, t[solved_idx]);
    if (filled != want_filled || t[solved_idx] != 0) {
        fprintf(stderr, "%s: H2 check FAILED\n", name);
        return 0;
    }
    return 1;
}

/* ---- emitters: C ---- */
static void emit_c_u16(FILE *f, const char *name, const uint16_t *a, int rows,
                       int cols)
{
    fprintf(f, "static const uint16_t %s[%d][%d] = {\n", name, rows, cols);
    for (int r = 0; r < rows; ++r) {
        fprintf(f, "  {");
        for (int c = 0; c < cols; ++c)
            fprintf(f, "%s%u", c ? "," : "", a[r * cols + c]);
        fprintf(f, "},\n");
    }
    fprintf(f, "};\n\n");
}

static void emit_c_u8(FILE *f, const char *name, const uint8_t *a, int rows,
                      int cols)
{
    if (rows == 1)
        fprintf(f, "static const uint8_t %s[%d] = {\n  ", name, cols);
    else
        fprintf(f, "static const uint8_t %s[%d][%d] = {\n", name, rows, cols);
    for (int r = 0; r < rows; ++r) {
        if (rows > 1)
            fprintf(f, "  {");
        for (int c = 0; c < cols; ++c)
            fprintf(f, "%s%u", c ? "," : "", a[r * cols + c]);
        if (rows > 1)
            fprintf(f, "},\n");
    }
    fprintf(f, rows == 1 ? "\n};\n\n" : "};\n\n");
}

/* ---- emitters: RV32I assembler ---- */
static void emit_s(FILE *f, const char *name, const void *data, int count,
                   int elem_bytes)
{
    const char *dir = elem_bytes == 2 ? ".half" : ".byte";
    fprintf(f, "    .align 2\n%s:\n", name);
    for (int i = 0; i < count; ++i) {
        unsigned v = elem_bytes == 2 ? ((const uint16_t *) data)[i]
                                     : ((const uint8_t *) data)[i];
        if (i % 16 == 0)
            fprintf(f, "%s    %s %u", i ? "\n" : "", dir, v);
        else
            fprintf(f, ", %u", v);
    }
    fprintf(f, "\n\n");
}

int main(void)
{
    /* order matters: dist_p needs perm_move; dist_os needs orien_move and
     * pos_move */
    build_perm_move();
    build_orien_move();
    build_pos_move();
    build_dist_p();
    build_dist_os();

    int ok = 1;
    ok &= check_table("dist_p", dist_p, PERMUTATIONS, PERMUTATIONS, 0);
    ok &= check_table("dist_os", dist_os, ORIENTATIONS * NS,
                      ORIENTATIONS * 42, 0 * NS + 1);
    if (!ok)
        return 1;

    FILE *h = fopen("tables.h", "w");
    FILE *s = fopen("tables.s", "w");
    if (!h || !s) {
        perror("fopen");
        return 1;
    }

    fprintf(h, "/* generated by gen_tables.c — do not edit */\n");
    fprintf(h, "#include <stdint.h>\n#define NS %d\n\n", NS);
    emit_c_u16(h, "perm_move", &perm_move[0][0], 3, PERMUTATIONS);
    emit_c_u16(h, "orien_move", &orien_move[0][0], 3, ORIENTATIONS);
    emit_c_u8(h, "pos_move", &pos_move[0][0], 3, NS);
    emit_c_u8(h, "dist_p", dist_p, 1, PERMUTATIONS);
    emit_c_u8(h, "dist_os", dist_os, 1, ORIENTATIONS * NS);

    fprintf(s, "# generated by gen_tables.c — do not edit\n    .data\n");
    emit_s(s, "perm_move", perm_move, 3 * PERMUTATIONS, 2);
    emit_s(s, "orien_move", orien_move, 3 * ORIENTATIONS, 2);
    emit_s(s, "pos_move", pos_move, 3 * NS, 1);
    emit_s(s, "dist_p", dist_p, PERMUTATIONS, 1);
    emit_s(s, "dist_os", dist_os, ORIENTATIONS * NS, 1);

    fclose(h);
    fclose(s);

    unsigned b_pm = sizeof perm_move, b_om = sizeof orien_move,
             b_ps = sizeof pos_move, b_dp = sizeof dist_p,
             b_do = sizeof dist_os;
    printf("perm_move  %6u bytes\n", b_pm);
    printf("orien_move %6u bytes\n", b_om);
    printf("pos_move   %6u bytes\n", b_ps);
    printf("dist_p     %6u bytes\n", b_dp);
    printf("dist_os    %6u bytes\n", b_do);
    printf("total      %6u bytes (budget 131072)\n",
           b_pm + b_om + b_ps + b_dp + b_do);
    return 0;
}