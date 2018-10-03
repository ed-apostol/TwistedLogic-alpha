/********************************************/
/*Twisted Logic Chess Engine super-alpha1   */
/*by Edsel Apostol                          */
/*Copyright 2005                            */
/*Last modified: 30 May 2005                */
/********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timeb.h>

/*********************************************/
/*MACROS                                     */
/*********************************************/

#define MAXPLY      128
#define MAXDATA     500
#define MAXMOVES    256

#define BOOL		int
#define TRUE        1
#define FALSE       0

#define RANK(X)            (X>>3)
#define FILE(X)            (X&7)
#define ABS(A)				(A<0?-A:A)
#define MAX(A,B)            (A>B?A:B)
#define MIN(A,B)            (A<B?A:B)

#define WHITE    0
#define BLACK    1
#define PCAPS     4
#define PTWOS     9

#define ALL      0
#define EMPTY    0
#define PAWN     1
#define KNIGHT   2
#define BISHOP   3
#define ROOK     4
#define QUEEN    5
#define KING     6

#define R000    0
#define R045    1
#define R090    2
#define R135    3

#define WCKS    1
#define WCQS    2
#define BCKS    4
#define BCQS    8

#define FROM(m)     ((m)&0x3f)
#define TO(m)       (((m)>>6)&0x3f)
#define PC(m)       (((m)>>12)&0x07)
#define CAPT(m)     (((m)>>15)&0x07)
#define PROM(m)     (((m)>>18)&0x07)
#define MOVE(m)     (m&0x1fffff)

#define HASHSIZE    (0x100000)
#define HASHMASK    (0xFFFFF)

#define SCODE(r,x)      (int)(~empty[r]>>shiftmask[r][x])&0xff
#define SLIDE(r,x)      slidemovs[r][x][SCODE(r,x)]
#define NEWMOVE(f,t,pc,c,pr)    ((f)|(t<<6)|(pc<<12)|(c<<15)|(pr<<18))

/*********************************************/
/*DATA STRUCTURES                            */
/*********************************************/

typedef long long u64;
typedef long u32;
typedef u32 move;
typedef struct {
    move m;
    int s;
}smove;

typedef struct {
    int mc;
    move movs[MAXPLY];
}LINE;

typedef struct {
    move m;
    int castle;
    int epsq;
    int fifty;
    int mat[2];
    u64 hash;
}hist;

typedef struct {
    u32 hash;
    move move;
    short score;
    char flag;
    char depth;
}hasht;

/*********************************************/
/*VARIABLES                                  */
/*********************************************/

smove M[MAXPLY][MAXMOVES];
hist H[MAXDATA];
int movcnt[MAXPLY];
int h[0x1000];

u64 slidemovs[4][64][256];
u64 stepmovs[64][10];

u64 pieces[2][7];
u64 empty[4];

u32 nodes;
u32 qnodes;
hist *status;
FILE *book_file;
/*hasht tt[HASHSIZE];*/

BOOL post;
BOOL xboard;
BOOL stopsearch;
BOOL check[MAXPLY];
BOOL followpv;
BOOL tryopening;
int maxtime;
int maxdepth;
int mytime;
int myotim;
int t1;
int t2;
LINE rline;

u64 bitmask[4][64];
u64 dbitmask[4][64][64];
u64 rankmask[64];
u64 filemask[64];
u64 zobrist[2][7][64];
u64 passedpawnmask[2][64];
int shiftmask[4][64];
u64 promotemask[2];

char lsb[0x10000];
struct _timeb tv;

int kingmovil[64];
int knightmovil[64];
int pawnval[2][64];

/*********************************************/
/*CONSTANTS                                  */
/*********************************************/

int board[64] = {
    ROOK,KNIGHT,BISHOP,QUEEN,KING,BISHOP,KNIGHT,ROOK,
    PAWN,PAWN,PAWN,PAWN,PAWN,PAWN,PAWN,PAWN,
    EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,
    EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,
    EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,
    EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,
    -PAWN,-PAWN,-PAWN,-PAWN,-PAWN,-PAWN,-PAWN,-PAWN,
    -ROOK,-KNIGHT,-BISHOP,-QUEEN,-KING,-BISHOP,-KNIGHT,-ROOK
};

int r045map[64] = {
    28,36,43,49,54,58,61,63,
    21,29,37,44,50,55,59,62,
    15,22,30,38,45,51,56,60,
    10,16,23,31,39,46,52,57,
    6,11,17,24,32,40,47,53,
    3,7,12,18,25,33,41,48,
    1,4,8,13,19,26,34,42,
    0,2,5,9,14,20,27,35
};

int r045shift[64] = {
    28,36,43,49,54,58,61,63,
    21,28,36,43,49,54,58,61,
    15,21,28,36,43,49,54,58,
    10,15,21,28,36,43,49,54,
    6,10,15,21,28,36,43,49,
    3,6,10,15,21,28,36,43,
    1,3,6,10,15,21,28,36,
    0,1,3,6,10,15,21,28
};

int castlemask[64] = {
    13, 15, 15, 15, 12, 15, 15, 14,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    7, 15, 15, 15,  3, 15, 15, 11
};

int pawnpos[64] = {
      9,9,9,9,9,9,9,9,
      8,8,8,8,8,8,8,8,
      6,6,6,6,6,6,6,6,
      4,4,5,5,5,5,4,4,
      2,2,4,4,4,4,2,2,
      2,2,2,2,2,2,2,2,
      1,1,1,1,1,1,1,1
};

int center[64] = {
      0,1,1,1,1,1,1,0,
      1,1,1,2,2,1,1,1,
      1,1,2,2,2,2,1,1,
      1,2,2,5,5,2,2,1,
      1,2,2,5,5,2,2,1,
      1,1,2,2,2,2,1,1,
      1,1,1,2,2,1,1,1,
      0,1,1,1,1,1,1,0
};

int flip[64] = {
     56,  57,  58,  59,  60,  61,  62,  63,
     48,  49,  50,  51,  52,  53,  54,  55,
     40,  41,  42,  43,  44,  45,  46,  47,
     32,  33,  34,  35,  36,  37,  38,  39,
     24,  25,  26,  27,  28,  29,  30,  31,
     16,  17,  18,  19,  20,  21,  22,  23,
      8,   9,  10,  11,  12,  13,  14,  15,
      0,   1,   2,   3,   4,   5,   6,   7
};

int pawn_pcsq[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
      5,  10,  15,  20,  20,  15,  10,   5,
      4,   8,  12,  16,  16,  12,   8,   4,
      3,   6,   9,  12,  12,   9,   6,   3,
      2,   4,   6,  10,  10,   6,   4,   2,
      1,   2,   3, -10, -10,   3,   2,   1,
      0,   0,   0, -40, -40,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0
};

int knight_pcsq[64] = {
    -10, -10, -10, -10, -10, -10, -10, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,   5,   5,   5,   0, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   0,   5,   5,   5,   5,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10, -25, -10, -10, -10, -10, -25, -10
};

int bishop_pcsq[64] = {
    -10, -10, -10, -10, -10, -10, -10, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   0,   10,  8,  8,   10,   0, -10,
    -10,   0,   10,  8,  8,   10,   0, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10, -10, -20, -10, -10, -20, -10, -10
};

int king_pcsq[64] = {
    -40, -40, -40, -40, -40, -40, -40, -40,
    -40, -40, -40, -40, -40, -40, -40, -40,
    -40, -40, -40, -40, -40, -40, -40, -40,
    -40, -40, -40, -40, -40, -40, -40, -40,
    -40, -40, -40, -40, -40, -40, -40, -40,
    -40, -40, -40, -40, -40, -40, -40, -40,
    -20, -20, -20, -20, -20, -20, -20, -20,
      0,  20,  40, -60,   0, -60,  40,  20
};

int king_endpcsq[64] = {
      0,  10,  20,  30,  30,  20,  10,   0,
     10,  20,  30,  40,  40,  30,  20,  10,
     20,  30,  40,  50,  50,  40,  30,  20,
     30,  40,  50,  60,  60,  50,  40,  30,
     30,  40,  50,  60,  60,  50,  40,  30,
     20,  30,  40,  50,  50,  40,  30,  20,
     10,  20,  30,  40,  40,  30,  20,  10,
      0,  10,  20,  30,  30,  20,  10,   0
};

static int kingd[] = { 8,-9,-1,7,8,9,1,-7,-8 };
static int knightd[] = { 8,-17,-10,6,15,17,10,-6,-15 };
static int wpawnd[] = { 1,8 };
static int bpawnd[] = { 1,-8 };
static int wpawnc[] = { 2,7,9 };
static int bpawnc[] = { 2,-7,-9 };
static int wpawn2mov[] = { 1,16 };
static int bpawn2mov[] = { 1,-16 };

int pcval[7] = { 0,100,300,300,500,900,25000 };
int promval[7] = { 0,0,200,200,400,800,0 };
int posval[7] = { 0,10,4,3,2,1,5 };
char pcstr[] = " PNBRQK pnbrqk";

/*********************************************/
/*FUNCTIONS                                  */
/*********************************************/

int pow2(int pow) { return (int)1 << pow; }

int turn000to090(int f) {
    return ((~f & 0x07) << 3) + ((f >> 3) & 0x07);
}

int turn045to135(int f) {
    return (f & 0x38) + ((~f) & 0x07);
}

u64 getbitmask(int r, int f) {
    switch (r) {
    case R000: return (u64)1 << f;
    case R045: return (u64)1 << r045map[f];
    case R090: return (u64)1 << turn000to090(f);
    case R135: return (u64)1 << r045map[turn045to135(f)];
    }
    return 0;
}

u64 getshiftmask(int r, int f) {
    switch (r) {
    case R000: return f & 0x38;
    case R045: return r045shift[f];
    case R090: return turn000to090(f) & 0x38;
    case R135: return r045shift[turn045to135(f)];
    }
    return 0;
}

static u32 rndseed = 1;
u32 rand32(void) {
    u32 r = rndseed;
    r = 16807 * (r % 127773L) - 2836 * (r / 127773L);
    if (r < 0) r += 0x7fffffffL;
    return rndseed = r;
}

u64 rand64(void) {
    u32 c = rand32();
    return (u64)((rand32()) | (c << 31));
}

char slowlsb(u64 i) {
    char k = -1;
    while (i) { k++; if (i&(u64)1)break; i >>= (u64)1; }
    return k;
}

int fastlsb(u64 d) {
    register u32 n = (u32)d;
    if (n) {
        if (n & 0xffff) return (lsb[n & 0xffff]);
        else return (16 + lsb[(n >> 16) & 0xffff]);
    }
    else {
        n = (u32)(d >> 32);
        if (n & 0xffff) return (32 + lsb[n & 0xffff]);
        else return (48 + lsb[(n >> 16) & 0xffff]);
    }
}

int bitcnt(u64 d) {
    register int cnt;
    for (cnt = 0; d; cnt++) d &= d - 1;
    return cnt;
}

void initstep(int f, int p, int*m) {
    int j, n;
    for (j = 1; j <= m[0]; j++) {
        n = f + m[j];
        if (n < 64 && n >= 0 && ABS(((n & 7) - (f & 7))) <= 2) stepmovs[f][p] |= getbitmask(R000, n);
    }
}

void initarr(void) {
    int i, j, m, k, n;
    u64 h;
    memset(slidemovs, 0, sizeof(slidemovs));
    memset(stepmovs, 0, sizeof(stepmovs));
    memset(zobrist, 0, sizeof(zobrist));
    memset(rankmask, 0, sizeof(rankmask));
    memset(filemask, 0, sizeof(filemask));
    memset(passedpawnmask, 0, sizeof(passedpawnmask));
    memset(kingmovil, 0, sizeof(kingmovil));
    memset(knightmovil, 0, sizeof(knightmovil));
    memset(pawnval, 0, sizeof(pawnval));
    memset(promotemask, 0, sizeof(promotemask));

    for (i = 0; i < 0x10000; i++) lsb[i] = slowlsb(i);
    for (i = 0; i < 0x40; i++) {
        for (j = 0; j < 2; j++) for (k = 0; k < 7; k++) zobrist[j][k][i] = rand64();
        for (j = 0; j < 0x40; j++) {
            if (RANK(i) == RANK(j)) rankmask[i] |= getbitmask(R000, j);
            if (FILE(i) == FILE(j)) filemask[i] |= getbitmask(R000, j);
        }
        for (j = i + 7; j < 0x40; j++) {
            k = j & 7; m = i & 7;
            if ((ABS(k - m) <= 1) && ((k == m - 1) || (k == m) || (k == m + 1)))
                passedpawnmask[WHITE][i] |= getbitmask(R000, j);
        }
        for (j = 0; j <= i - 7; j++) {
            k = j & 7; m = i & 7;
            if ((ABS(k - m) <= 1) && ((k == m - 1) || (k == m) || (k == m + 1)))
                passedpawnmask[BLACK][i] |= getbitmask(R000, j);
        }
        for (j = 0; j < 4; j++) {
            bitmask[j][i] = getbitmask(j, i);
            shiftmask[j][i] = getshiftmask(j, i);
            for (k = 0; k < 0x40; k++)  dbitmask[j][i][k] = bitmask[j][i] | getbitmask(j, k);
        }
        initstep(i, KNIGHT, knightd);
        initstep(i, KING, kingd);
        initstep(i, WHITE, wpawnd);
        initstep(i, BLACK, bpawnd);
        initstep(i, PCAPS | WHITE, wpawnc);
        initstep(i, PCAPS | BLACK, bpawnc);
        if (RANK(i) == 6) {
            initstep(i, PTWOS | BLACK, bpawn2mov);
            promotemask[WHITE] |= getbitmask(R000, i);
        }
        if (RANK(i) == 1) {
            initstep(i, PTWOS | WHITE, wpawn2mov);
            promotemask[BLACK] |= getbitmask(R000, i);
        }
        for (j = 0; j < 0x100; j++) {
            for (h = 0, k = i + 1; k & 7; k++) {
                h |= getbitmask(R000, k);
                if (j&pow2(k & 7)) break;
            }
            for (k = i - 1; (k & 7) != 7; k--) {
                h |= getbitmask(R000, k);
                if (j&pow2(k & 7)) break;
            }
            slidemovs[R000][i][j] = h;
            for (h = 0, k = i + 8; k < 64; k += 8) {
                h |= getbitmask(R000, k);
                if (j&pow2(k / 8)) break;
            }
            for (k = i - 8; k >= 0; k -= 8) {
                h |= getbitmask(R000, k);
                if (j&pow2(k / 8)) break;
            }
            slidemovs[R090][i][j] = h;
            for (h = 0, k = i - 9, n = 0; k >= 0 && (k & 7) != 7; k -= 9, n++);
            for (k = i + 9, m = n + 1; k < 64 && (k & 7) != 0; k += 9, m++) {
                h |= getbitmask(R000, k);
                if (j&pow2(m)) break;
            }
            for (k = i - 9, m = n - 1; k >= 0 && (k & 7) != 7; k -= 9, m--) {
                h |= getbitmask(R000, k);
                if (j&pow2(m)) break;
            }
            slidemovs[R045][i][j] = h;
            for (h = 0, k = i - 7, n = 0; k >= 0 && (k & 7) != 0; k -= 7, n++);
            for (k = i + 7, m = n + 1; k < 64 && (k & 7) != 7; k += 7, m++) {
                h |= getbitmask(R000, k);
                if (j&pow2(m)) break;
            }
            for (k = i - 7, m = n - 1; k >= 0 && (k & 7) != 0; k -= 7, m--) {
                h |= getbitmask(R000, k);
                if (j&pow2(m)) break;
            }
            slidemovs[R135][i][j] = h;
        }
    }
    for (i = 0; i < 64; i++) {
        kingmovil[i] = bitcnt(stepmovs[i][KING]);
        knightmovil[i] = bitcnt(stepmovs[i][KNIGHT]);
        pawnval[WHITE][i] = pawnpos[63 - i];
        pawnval[BLACK][i] = -pawnpos[i];
    }
}

int getpiece(u64 d, int onmove) {
    if (d&pieces[onmove][PAWN])    return PAWN;
    if (d&pieces[onmove][ROOK])    return ROOK;
    if (d&pieces[onmove][BISHOP])  return BISHOP;
    if (d&pieces[onmove][KNIGHT])  return KNIGHT;
    if (d&pieces[onmove][QUEEN])   return QUEEN;
    if (d&pieces[onmove][KING])   return KING;
    return EMPTY;
}

int getcolor(u64 d) {
    if (d&pieces[WHITE][ALL]) return WHITE;
    return BLACK;
}

void initgame(void) {
    int i, j, p;
    u64 hash = 0;
    memset(M, 0, sizeof(M));
    memset(H, 0, sizeof(H));
    memset(pieces, 0, sizeof(pieces));
    /*memset(tt, 0, sizeof(tt));*/
    memset(empty, 0, sizeof(empty));
    mytime = 3600000;
    myotim = 3600000;
    maxdepth = 5;
    tryopening = 1;
    for (i = 0; i < 64; i++) {
        p = board[i];
        if (p != 0) {
            pieces[p > 0 ? WHITE : BLACK][ABS(p)] ^= bitmask[R000][i];
            pieces[p > 0 ? WHITE : BLACK][ALL] ^= bitmask[R000][i];
        }
        else for (j = 0; j < 4; j++) empty[j] ^= bitmask[j][i];
    }
    for (i = 0; i < 64; ++i) {
        j = getcolor(bitmask[R000][i]);
        p = getpiece(bitmask[R000][i], j);
        if (p) hash ^= zobrist[j][p][i];
    }
    status = H;
    status->m = 0;
    status->castle = 15;
    status->epsq = -1;
    status->fifty = 0;
    status->mat[BLACK] = 8 * pcval[PAWN] + 2 * (pcval[ROOK] + pcval[KNIGHT] + pcval[BISHOP]) + pcval[QUEEN];
    status->mat[WHITE] = status->mat[BLACK];
    status->hash = hash;
}

int gettime(void) {
    _ftime(&tv);
    return (tv.time * 1000 + tv.millitm);
}

BOOL isatt(int onmove, u64 target) {
    register u64 x;
    register int f = fastlsb(target);
    x = (SLIDE(R000, f) | SLIDE(R090, f));
    if (x&pieces[onmove][QUEEN]) return TRUE;
    if (x&pieces[onmove][ROOK]) return TRUE;
    x = (SLIDE(R045, f) | SLIDE(R135, f));
    if (x&pieces[onmove][QUEEN]) return TRUE;
    if (x&pieces[onmove][BISHOP]) return TRUE;
    if (pieces[onmove][KNIGHT] & stepmovs[f][KNIGHT]) return TRUE;
    if (pieces[onmove][PAWN] & stepmovs[f][PCAPS | (onmove ^ 1)]) return TRUE;
    if (pieces[onmove][KING] & stepmovs[f][KING]) return TRUE;
    return FALSE;
}

u64 updatehash(move m, int onmove) {
    register int from = FROM(m);
    register int to = TO(m);
    register int pc = PC(m);
    register int c = CAPT(m);
    register int pr = PROM(m);
    register u64 hash = zobrist[onmove][pc][from];
    if (c) hash ^= zobrist[onmove ^ 1][c][to];
    if (pr) hash ^= zobrist[onmove][pr][to];
    else hash ^= zobrist[onmove][pc][to];
    if ((pc == PAWN) && (to == status->epsq)) hash ^= zobrist[onmove ^ 1][PAWN][to + onmove ? 8 : -8];
    else if ((pc == KING) && (ABS(from - to) == 2)) {
        if (from - to == 2) {
            hash ^= zobrist[onmove][ROOK][(onmove ? 56 : 0)];
            hash ^= zobrist[onmove][ROOK][(onmove ? 59 : 3)];
        }
        else {
            hash ^= zobrist[onmove][ROOK][(onmove ? 63 : 7)];
            hash ^= zobrist[onmove][ROOK][(onmove ? 61 : 5)];
        }
    }
    return hash ^ onmove;
}

void updateboards(move m, int onmove) {
    register int from = FROM(m);
    register int to = TO(m);
    register int pc = PC(m);
    register int capt = CAPT(m);
    register int prom = PROM(m);

    pieces[onmove][pc] ^= dbitmask[R000][from][to];
    pieces[onmove][ALL] ^= dbitmask[R000][from][to];
    if (capt) {
        pieces[onmove ^ 1][capt] ^= bitmask[R000][to];
        pieces[onmove ^ 1][ALL] ^= bitmask[R000][to];
        empty[R000] ^= bitmask[R000][from];
        empty[R045] ^= bitmask[R045][from];
        empty[R090] ^= bitmask[R090][from];
        empty[R135] ^= bitmask[R135][from];
    }
    else {
        empty[R000] ^= dbitmask[R000][from][to];
        empty[R045] ^= dbitmask[R045][from][to];
        empty[R090] ^= dbitmask[R090][from][to];
        empty[R135] ^= dbitmask[R135][from][to];
    }

    if (pc == PAWN) {
        if (prom) {
            pieces[onmove][PAWN] ^= bitmask[R000][to];
            pieces[onmove][prom] ^= bitmask[R000][to];
        }
        else if (to == status->epsq) {
            register int i = onmove ? (to + 8) : (to - 8);
            pieces[onmove ^ 1][PAWN] ^= bitmask[R000][i];
            pieces[onmove ^ 1][ALL] ^= bitmask[R000][i];
            empty[R000] ^= bitmask[R000][i];
            empty[R045] ^= bitmask[R045][i];
            empty[R090] ^= bitmask[R090][i];
            empty[R135] ^= bitmask[R135][i];
        }
    }
    else if (pc == KING) {
        register move rm;
        if (from - to == 2) rm = (onmove ? ((56) | (59 << 6) | (ROOK << 12)) : ((0) | (3 << 6) | (ROOK << 12)));
        else if (from - to == -2) rm = (onmove ? ((63) | (61 << 6) | (ROOK << 12)) : ((7) | (5 << 6) | (ROOK << 12)));
        else return;
        updateboards(rm, onmove);
    }
}

void undomove(int onmove) {
    --status;
    updateboards(status->m, onmove);
}

BOOL domove(move m, int onmove) {
    updateboards(m, onmove);
    if (isatt(onmove ^ 1, pieces[onmove][KING])) {
        updateboards(m, onmove);
        return FALSE;
    }
    else {
        status->m = m;
        *(status + 1) = *status;
        status++;
        status->hash ^= updatehash(m, onmove);
        status->castle &= castlemask[FROM(m)] & castlemask[TO(m)];
        status->epsq = -1;
        if (PC(m) == PAWN) {
            if (RANK(FROM(m)) == 1 && RANK(TO(m)) == 3) status->epsq = TO(m) - 8;
            else if (RANK(FROM(m)) == 6 && RANK(TO(m)) == 4) status->epsq = TO(m) + 8;
        }
        if ((CAPT(m)) || (PC(m) == PAWN)) status->fifty = 0;
        else status->fifty += 1;
        if (CAPT(m)) status->mat[onmove ^ 1] -= pcval[CAPT(m)];
        else if ((PC(m) == PAWN) && ((status - 1)->epsq == TO(m)))
            status->mat[onmove ^ 1] -= pcval[PAWN];
        if (PROM(m)) status->mat[onmove] += promval[PROM(m)];
        return TRUE;
    }
}

BOOL islegalcastle(int from, int to, int onmove) {
    register int i;
    for (i = MIN(from, to); i <= MAX(from, to); ++i) {
        if (isatt(onmove ^ 1, bitmask[R000][i])) return FALSE;
        if ((i != from) && (~empty[R000] & bitmask[R000][i])) return FALSE;
    }
    if (((from - to) == 2) && (~empty[R000] & bitmask[R000][to - 1])) return FALSE;
    return TRUE;
}

int genmoves(int onmove, int ply) {
    int n = 0;
    u64 h = pieces[onmove][ALL];
    u64 piecemask = ~h;
    while (h) {
        register int f = fastlsb(h);
        register u64 m = 0, d = bitmask[R000][f];
        register int p = getpiece(d, onmove);
        register int q = 0;
        h ^= d;
        switch (p) {
        case PAWN:
            if (promotemask[onmove] & d) q = 1;
            m = empty[R000] & stepmovs[f][onmove];
            if ((promotemask[onmove ^ 1] & d) && m) m |= empty[R000] & stepmovs[f][PTWOS | onmove];
            m |= stepmovs[f][PCAPS | onmove] & pieces[onmove ^ 1][ALL];
            break;
        case ROOK: m = piecemask & (SLIDE(R000, f) | SLIDE(R090, f)); break;
        case BISHOP: m = piecemask & (SLIDE(R045, f) | SLIDE(R135, f)); break;
        case QUEEN:
            m = piecemask & (SLIDE(R000, f) | SLIDE(R090, f) | SLIDE(R045, f) | SLIDE(R135, f));
            break;
        case KNIGHT: m = (piecemask&stepmovs[f][KNIGHT]); break;
        case KING: m = (piecemask&stepmovs[f][KING]); break;
        }
        while (m) {
            register int t = fastlsb(m);
            register u64 a = bitmask[R000][t];
            m ^= a;
            if (q) {
                register int x = getpiece(a, onmove ^ 1);
                register int i;
                for (i = KNIGHT; i <= QUEEN; i++) M[ply][n++].m = NEWMOVE(f, t, p, x, i);
            }
            else M[ply][n++].m = NEWMOVE(f, t, p, getpiece(a, onmove ^ 1), 0);
        }
    }
    if (onmove) {
        if ((status->castle&BCKS) && (islegalcastle(60, 62, onmove)))
            M[ply][n++].m = NEWMOVE(60, 62, KING, 0, 0);
        if ((status->castle&BCQS) && (islegalcastle(60, 58, onmove)))
            M[ply][n++].m = NEWMOVE(60, 58, KING, 0, 0);
    }
    else {
        if ((status->castle&WCKS) && (islegalcastle(4, 6, onmove)))
            M[ply][n++].m = NEWMOVE(4, 6, KING, 0, 0);
        if ((status->castle&WCQS) && (islegalcastle(4, 2, onmove)))
            M[ply][n++].m = NEWMOVE(4, 2, KING, 0, 0);
    }
    if (status->epsq != -1) {
        h = pieces[onmove][PAWN] & stepmovs[status->epsq][PCAPS | (onmove ^ 1)];
        while (h) {
            register int f = fastlsb(h);
            register u64 d = bitmask[R000][f];
            h ^= d;
            M[ply][n++].m = NEWMOVE(f, status->epsq, PAWN, 0, 0);
        }
    }
    return n;
}

int gencaps(int onmove, int ply) {
    int n = 0;
    u64 h = pieces[onmove][ALL];
    u64 piecemask = pieces[onmove ^ 1][ALL];
    while (h) {
        register int f = fastlsb(h);
        register u64 m = 0, d = bitmask[R000][f];
        register int p = getpiece(d, onmove);
        register int q = 0;
        h ^= d;
        switch (p) {
        case PAWN:
            m = stepmovs[f][PCAPS | onmove] & pieces[onmove ^ 1][ALL];
            if (promotemask[onmove] & d) {
                m |= empty[R000] & stepmovs[f][onmove];
                q = 1;
            }
            break;
        case ROOK: m = piecemask & (SLIDE(R000, f) | SLIDE(R090, f)); break;
        case BISHOP: m = piecemask & (SLIDE(R045, f) | SLIDE(R135, f)); break;
        case QUEEN:
            m = piecemask & (SLIDE(R000, f) | SLIDE(R090, f) | SLIDE(R045, f) | SLIDE(R135, f));
            break;
        case KNIGHT: m = (piecemask&stepmovs[f][KNIGHT]); break;
        case KING: m = (piecemask&stepmovs[f][KING]); break;
        }
        while (m) {
            register int t = fastlsb(m);
            register u64 a = bitmask[R000][t];
            m ^= a;
            if (q) {
                register int x = getpiece(a, onmove ^ 1);
                register int i;
                for (i = KNIGHT; i <= QUEEN; i++) M[ply][n++].m = NEWMOVE(f, t, p, x, i);
            }
            else M[ply][n++].m = NEWMOVE(f, t, p, getpiece(a, onmove ^ 1), 0);
        }
    }
    if (status->epsq != -1) {
        h = pieces[onmove][PAWN] & stepmovs[status->epsq][PCAPS | (onmove ^ 1)];
        while (h) {
            register int f = fastlsb(h);
            register u64 d = bitmask[R000][f];
            h ^= d;
            M[ply][n++].m = NEWMOVE(f, status->epsq, PAWN, 0, 0);
        }
    }
    return n;
}

void attcnt(int *x, int *y) {
    register u64 m = 0;
    register int h = ~empty[R000];
    register int target = ~empty[R000];
    while (h) {
        register int f = fastlsb(h);
        register u64 d = bitmask[R000][f];
        register int c = getcolor(d);
        register int p = getpiece(d, c);
        h ^= d;
        switch (p) {
        case PAWN: m |= target & stepmovs[f][PCAPS | c]; break;
        case ROOK: m |= target & (SLIDE(R000, f) | SLIDE(R090, f)); break;
        case BISHOP: m |= target & (SLIDE(R045, f) | SLIDE(R135, f)); break;
        case QUEEN:
            m |= target & (SLIDE(R000, f) | SLIDE(R090, f) | SLIDE(R045, f) | SLIDE(R135, f));
            break;
        case KNIGHT: m |= target & stepmovs[f][KNIGHT]; break;
        case KING: m |= target & stepmovs[f][KING]; break;
        }
        while (m) {
            register int t = fastlsb(m);
            register u64 a = bitmask[R000][t];
            m ^= a;
            if (c) y[t] += 1;
            else x[t] += 1;
        }
    }
    return;
}

int eval(int onmove) {
    int x, y, q = (status - H), score = status->mat[WHITE] - status->mat[BLACK];
    /*int att[64], xatt[64];
    memset(att, 0, sizeof(att));
    memset(xatt, 0, sizeof(xatt));*/
    u64 lp = pieces[WHITE][PAWN];
    u64 dp = pieces[BLACK][PAWN];
    u64 h = ~empty[R000], z, s;
    u64 m = ~h;
    /*attcnt(att, xatt);
    for(x=0; x<64; x++){
        if(att[x]>xatt[x]) score+=center[x];
        if(xatt[x]>att[x]) score-=center[x];
    }*/
    while (h) {
        register int f = fastlsb(h);
        register u64 d = bitmask[R000][f];
        register int c = getcolor(d);
        register int p = getpiece(d, c);
        register u64 pawns = (c ? dp : lp);
        register u64 xpawns = (c ? lp : dp);
        h ^= d;
        switch (p) {
        case PAWN:
            score += c ? -pawn_pcsq[f] : pawn_pcsq[flip[f]];
            if (!(passedpawnmask[c][f] & xpawns)) {
                score += pawnval[c][f] * 20;
                if (SLIDE(R090, f)&pieces[c][ROOK] & passedpawnmask[c ^ 1][f])
                    score += pawnval[c][f] * 3;
            }
            /*if(!((((FILE(f)<7)?filemask[f+1]:0)|((FILE(f)>0)?filemask[f-1]:0))&pawns))
                score+=c?30:-30; */
            if (filemask[f] & (pawns^bitmask[R000][f])) score += pawnval[c ^ 1][f] * 2;
            if (!((pawns^bitmask[R000][f])&passedpawnmask[c ^ 1][f + (c ? -8 : 8)]))
                score += pawnval[c ^ 1][f];
            break;
        case KNIGHT:
            score += c ? -knight_pcsq[f] : knight_pcsq[flip[f]];
            if (!(passedpawnmask[c][f] & (~filemask[f])&xpawns))
                score += c ? -knight_pcsq[f] * 3 : knight_pcsq[flip[f]] * 3;
            break;
        case BISHOP:
            score += c ? -bishop_pcsq[f] : bishop_pcsq[flip[f]];
            if ((RANK(f) == c ? 5 : 2) && (passedpawnmask[c ^ 1][f] & filemask[f] & pawns) && (FILE(f) == 3 | FILE(f) == 4))
                score += c ? 20 : -20;
            break;
        case KING:
            if (pieces[c ^ 1][QUEEN] && bitcnt(pieces[c ^ 1][ROOK]) > 1) {
                score += c ? -king_pcsq[f] : king_pcsq[flip[f]];
                z = passedpawnmask[c][f] & pawns;
                x = bitcnt(z&rankmask[f + (c ? -8 : 8)]);
                score += x * (c ? -25 : 25);
                if (FILE(f - 1) > 0) {
                    if (!(xpawns&pawns&filemask[f - 1])) score += (c ? 30 : -30);
                    else if (!(xpawns&filemask[f - 1])) score += (c ? 20 : -20);
                    else if (!(pawns&filemask[f - 1])) score += (c ? 20 : -20);
                }
                if (!(xpawns&pawns&filemask[f])) score += (c ? 30 : -30);
                else if (!(xpawns&filemask[f])) score += (c ? 20 : -20);
                else if (!(pawns&filemask[f])) score += (c ? 20 : -20);
                if (FILE(f + 1) < 7) {
                    if (!(xpawns&pawns&filemask[f + 1])) score += (c ? 30 : -30);
                    else if (!(xpawns&filemask[f + 1])) score += (c ? 20 : -20);
                    else if (!(pawns&filemask[f + 1])) score += (c ? 20 : -20);
                }
            }
            else score += c ? -king_endpcsq[f] : king_endpcsq[flip[f]];
            break;
        case ROOK:
            /*x=bitcnt(m&(SLIDE(R000, f)|SLIDE(R090, f)));
            score+=x*(c?-1:1);*/
            if (!(pawns&xpawns&filemask[f])) score += c ? -30 : 30;
            else if (!(pawns&filemask[f])) score += c ? -20 : 20;
            if (RANK(f) == (c ? 1 : 6)) score += c ? -20 : 20;
            break;
            /*case QUEEN:
                x=bitcnt(m&(SLIDE(R045, f)|SLIDE(R135, f)|SLIDE(R000, f)|SLIDE(R090, f)));
                score+=x*(c?-1:1);
                break; */
        }
    }

    if (!lp&&score > 0) {
        if (!(pieces[WHITE][QUEEN] | pieces[WHITE][ROOK]) && bitcnt(pieces[WHITE][ALL]) < 3)
            return 0;
    }
    else if (!dp&&score < 0) {
        if (!(pieces[BLACK][QUEEN] | pieces[BLACK][ROOK]) && bitcnt(pieces[BLACK][ALL]) < 3)
            return 0;
    }
    return onmove ? -score : +score;
}

char *displaymove(move m) {
    static char promstr[7] = "\0pnbrqk";
    static char str[6];
    sprintf(str, "%c%c%c%c%c",
        (int)FILE(FROM(m)) + 'a',
        '1' + (int)RANK(FROM(m)),
        (int)FILE(TO(m)) + 'a',
        '1' + (int)RANK(TO(m)),
        promstr[PROM(m)]
    );
    return str;
}

void displayb(int o) {
    int i, j, c;
    for (i = 56; i >= 0; i -= 8) {
        printf("\n  +---+---+---+---+---+---+---+---+\n%d |", (i / 8) + 1);
        for (j = 0; j < 8; ++j) {
            c = getcolor(bitmask[R000][i + j]);
            printf(" %c |", pcstr[getpiece(bitmask[R000][i + j], c) + (c ? 7 : 0)]);
            /*printf(" %d |", c+1);
            printf(" %d |",getpiece(bitmask[R000][i+j],c)); */
        }
    }
    printf("\n  +---+---+---+---+---+---+---+---+\n");
    printf("    a   b   c   d   e   f   g   h  \n");
    printf("%d.%s%s  c: %d  castle: %d  ep square: %d  eval: %d  fifty: %d  \n",
        (status - H) / 2 + (o ? 1 : 0), o ? " " : " ...", displaymove((status - 1)->m),
        isatt(o ^ 1, pieces[o][KING]),
        status->castle, status->epsq, eval(o), status->fifty);
}

int reps(void) {
    register hist *i = status - status->fifty;
    register int r = 0;
    while (i < status) {
        if (i->hash == status->hash) ++r;
        ++i;
    }
    return r;
}

void open_book() {
    srand(gettime() / 12345);
    book_file = fopen("Book.txt", "r");
    if (!book_file) printf("Opening book missing.\n");
}

void close_book() {
    if (book_file) fclose(book_file);
    book_file = NULL;
}

move parsemove(char *s, int onmove, int mc) {
    int m, from, to, i, p;
    from = (s[0] - 'a') + (8 * (s[1] - '1'));
    to = (s[2] - 'a') + (8 * (s[3] - '1'));
    m = (from) | (to << 6);
    for (i = 0; i < mc; ++i) {
        if (m == (M[127][i].m & 0xfff)) {
            p = EMPTY;
            if (PROM(M[127][i].m)) {
                switch (s[4]) {
                case 'n':case 'N':p = KNIGHT; break;
                case 'b':case 'B':p = BISHOP; break;
                case 'r':case 'R':p = ROOK; break;
                default:p = QUEEN; break;
                }
            }
            if (p == (int)PROM(M[127][i].m)) return M[127][i].m;
        }
    }
    return FALSE;
}

move get_opening(char *hist, char *opening, int onmove, int mc) {
    register int i = 0;
    char tmp[6];
    while (hist[i] != '\n') {
        if (hist[i] != opening[i])
            return 0;
        i++;
    }
    if (opening[i] != '\n') {
        tmp[0] = opening[i];
        tmp[1] = opening[i + 1];
        tmp[2] = opening[i + 2];
        tmp[3] = opening[i + 3];
        tmp[4] = opening[i + 4];
        return parsemove(tmp, onmove, mc);
    }
    return 0;
}

move book_move(int onmove, int mc) {
    char line[512], book_line[512];
    move m, move[128] = { 0 };
    int count[128] = { 0 }, moves = 0, total_count = 0, i, j = 0;
    if (!book_file) return 0;
    line[0] = '\0';
    for (i = 0; i < (status - H); i++) j += sprintf(line + j, "%s", displaymove(H[i].m));
    line[strlen(line)] = '\n';
    fseek(book_file, 0, SEEK_SET);
    while (fgets(book_line, 512, book_file)) {
        m = get_opening(line, book_line, onmove, mc);
        if (m) {
            for (j = 0; j < moves; ++j)
                if (move[j] == m) {
                    ++count[j];
                    break;
                }
            if (j == moves) {
                move[moves] = m;
                count[moves] = 1;
                ++moves;
            }
            ++total_count;
        }
    }
    if (moves == 0) return 0;
    j = rand() % total_count;
    for (i = 0; i < moves; ++i) {
        j -= count[i];
        if (j < 0) return move[i];
    }
    return 0;
}
/*
#define EXACT 0
#define ALPHA 1
#define VETA 2

int checkhash(move *m, int a, int v, int *scr, int d){
    register hasht *ptr=&tt[status->hash&HASHMASK];
    if(ptr->hash==status->hash){
        if(ptr->depth>=d){
            if(ptr->flag==EXACT){ *scr=ptr->score; return 1;}
            else if(ptr->flag==ALPHA&&ptr->score<=a){*scr=ptr->score; return 1;}
            else if(ptr->flag==VETA&&ptr->score>=v){*scr=ptr->score; return 1;}
        }else{
            *m=ptr->move;
            return 2;
        }
    }else return 0;
}

int storehash(int d, int scr, int f, move *m){
    register hasht *ptr=&tt[status->hash&HASHMASK];
    if(d<ptr->depth) return 0;
    ptr->hash=status->hash;
    ptr->move=*m;
    ptr->depth=d;
    ptr->score=scr;
    ptr->flag=f;
    return 1;
}*/

void sortmove(int start, int end, int ply) {
    register int i, s, bs, bi;
    register smove x;
    bs = -99999;
    bi = start;
    for (i = start; i < end; ++i) {
        s = M[ply][i].s;
        if (s > bs) {
            bs = s;
            bi = i;
        }
    }
    if (bi == start) return;
    x = M[ply][start];
    M[ply][start] = M[ply][bi];
    M[ply][bi] = x;
}

void prescore(int ply) {
    register int i;
    register smove *m;
    for (i = 0; i < movcnt[ply]; ++i) {
        m = &M[ply][i];
        if (CAPT(m->m)) m->s = m->m ^ 0x7000;
        else m->s = h[m->m & 0xfff];
    }
}

int qsearch(int alpha, int veta, int onmove, int ply) {
    int i, w, c;
    nodes++;
    qnodes++;
    w = eval(onmove);
    if (w >= veta) return w;
    if (w > alpha) alpha = w;
    movcnt[ply] = gencaps(onmove, ply);
    prescore(ply);
    for (i = 0; i < movcnt[ply]; ++i) {
        sortmove(i, movcnt[ply], ply);
        if (!domove(M[ply][i].m, onmove)) continue;
        w = -qsearch(-veta, -alpha, onmove ^ 1, ply + 1);
        undomove(onmove);
        if (w >= veta) return w;
        if (w > alpha) alpha = w;
    }
    return alpha;
}

void sortpv(int ply) {
    register int i;
    followpv = FALSE;
    for (i = 0; i < movcnt[ply]; ++i)
        if (MOVE(M[ply][i].m) == MOVE(rline.movs[ply])) {
            followpv = TRUE;
            M[ply][i].s += 1000000;
            return;
        }
}

int search(int alpha, int veta, int onmove, int depth, int ply, int null, LINE *pline) {
    int i, w, j = 0, c, a, v,/* f=ALPHA, */legal = 0;
    LINE line;
    if (ply >= MAXPLY - 1) return eval(onmove);
    if ((++nodes & 1023) == 0) {
        if (gettime() > t2) {
            stopsearch = TRUE;
            return alpha;
        }
    }
    if (ply&&reps()) return 0;
    /*j=checkhash(&rline.movs[ply], alpha, veta, &w, depth);
    if(j==1&&ply) return w;
    else if(j==2) sortpv(ply);*/
    c = isatt(onmove ^ 1, (pieces[onmove][KING]));
    if (c) depth += 16;
    if (depth < 16) {
        pline->mc = 0;
        return qsearch(alpha, veta, onmove, ply);
    }
    else if (!c&&null&&depth >= 48 && ply&&bitcnt(~pieces[onmove][PAWN] & pieces[onmove][ALL]) > 4) {
        w = -search(-veta, 1 - veta, onmove ^ 1, depth - 32, ply + 1, 0, &line);
        if (w >= veta) return w;
        if (w <= -30000) depth += 16;
    }
    movcnt[ply] = genmoves(onmove, ply);
    prescore(ply);
    if (followpv) sortpv(ply);
    for (i = 0; i < movcnt[ply]; ++i) {
        sortmove(i, movcnt[ply], ply);
        if (!domove(M[ply][i].m, onmove)) continue;
        w = -search(-veta, -alpha, onmove ^ 1, depth - 16, ply + 1, 1, &line);
        undomove(onmove);
        if (stopsearch) return alpha;
        legal = 1;
        if (w > alpha) {
            if (w >= veta) {/*storehash(depth, veta, VETA, &M[ply][i]);*/ return w; }
            alpha = w;
            /*f=EXACT;*/
            h[M[ply][i].m & 0xfff] += depth;
            pline->movs[0] = M[ply][i].m;
            memcpy(pline->movs + 1, line.movs, line.mc * sizeof(move));
            pline->mc = line.mc + 1;
            if (ply == 0 && post) {
                if (xboard) printf("%d %d %d %d ", (depth / 16), w, (gettime() - t1) / 10, (int)nodes);
                else printf("%3d  %9d  %9d  %6d  %5d  ", (depth / 16), (int)(nodes - qnodes), (int)qnodes, (gettime() - t1), w);
                for (j = 0; j < pline->mc; ++j) printf("%s ", displaymove(pline->movs[j]));
                printf("\n");
                fflush(stdout);
            }
        }
    }
    if (!legal) {
        if (c) return -40000 + ply;
        else return ply;
    }
    if (status->fifty >= 100) return 0;
    /*storehash(depth, alpha, f, &pline->movs[0]);*/
    return alpha;
}

move think(int depth, int onmove) {
    int alpha, veta, i, x, j;
    if (tryopening) {
        int mc = genmoves(onmove, 127);
        rline.movs[0] = book_move(onmove, mc);
    }
    else rline.movs[0] = 0;
    if (rline.movs[0]) return rline.movs[0];
    else tryopening = 0;
    nodes = qnodes = 0;
    stopsearch = 0;
    memset(h, 0, sizeof(h));
    maxtime = (((mytime * 2) - myotim) * 10) / 30;
    t1 = gettime();
    t2 = maxtime + t1;
    alpha = -99999; veta = 99999;
    depth *= 16;
    if (!xboard) printf("depth    nodes     qnodes    time  score  pv\n");
    for (i = 16; i <= depth; i += 16) {
        followpv = 1;
        x = search(alpha, veta, onmove, i, 0, 0, &rline);
        if (stopsearch) break;
        if (x > 30000 || x < -30000) break;
        while ((x < alpha) || (x > veta)) {
            if (x < alpha) alpha = x - 100;
            if (x > veta) veta = x + 100;
            x = search(alpha, veta, onmove, i, 0, 0, &rline);
        }
        alpha = x - 70;
        veta = x + 70;
    }
    return rline.movs[0];
}

void isgameover(int onmove) {
    int i;
    int mc = genmoves(onmove, 127);
    for (i = 0; i < mc; ++i) {
        if (domove(M[127][i].m, onmove)) {
            undomove(onmove);
            break;
        }
    }
    if (i == mc) {
        if (isatt(onmove ^ 1, (pieces[onmove][KING]))) {
            switch (onmove) {
            case WHITE:printf("0-1 {Black mates}\n"); break;
            default:printf("1-0 {White mates}\n"); break;
            }
        }
        else printf("1/2-1/2 {Stalemate}\n");
    }
    else if (status->fifty >= 100) printf("1/2-1/2 {Draw by fifty move rule}\n");
    else if (reps() > 1) printf("1/2-1/2 {Draw by repetition}\n");
}

int main(void) {
    char fff[256];
    char s[256];
    char strmov[6];
    move m; int i, mc;
    int onmove = WHITE, machine = BLACK;
    printf("Twisted Logic Chess Engine super-alpha\n");
    printf("by Edsel Apostol, Philippines\n");
    printf("use xboard commands or get Arena GUI\n\n");
    xboard = FALSE;
    post = TRUE;
    initarr();
    initgame();
    open_book();
    do {
        fflush(stdout);
        if (!xboard) displayb(onmove);
        if (onmove == machine) {
            m = think(maxdepth, onmove);
            if (!m) {
                printf("Illegal move(game over): %s\n", displaymove(m));
                machine = -1;
                continue;
            }
            domove(m, onmove);
            onmove ^= 1;
            printf("move %s\n", displaymove(m));
            isgameover(onmove);
            continue;
        }
        if (!xboard) printf("command> ");
        if (!fgets(fff, 255, stdin)) return 1;
        if (fff[0] == '\n') continue;
        sscanf(fff, "%s", s);
        if (!strcmp(s, "xboard")) {
            xboard = TRUE;
            printf("feature playother=1 usermove=1 myname=TwistedLogic ping=1 done=1\n");
            continue;
        }
        if (!strcmp(s, "quit")) exit(0);
        if (!strcmp(s, "new")) {
            initgame();
            onmove = WHITE;
            machine = BLACK;
            continue;
        }
        if (!strcmp(s, "force")) {
            machine = -1;
            continue;
        }
        if (!strcmp(s, "go")) {
            machine = onmove;
            continue;
        }
        if (!strcmp(s, "playother")) {
            if (machine == -1) machine = onmove;
            else machine = (onmove ? WHITE : BLACK);
            continue;
        }
        if (!strcmp(s, "post")) {
            post = TRUE;
            continue;
        }
        if (!strcmp(s, "nopost")) {
            post = FALSE;
            continue;
        }
        if (!strcmp(s, "st")) {
            sscanf(fff, "st %d", &mytime);
            mytime *= 30000;
            myotim = mytime;
            maxdepth = MAXPLY;
            continue;
        }
        if (!strcmp(s, "sd")) {
            sscanf(fff, "sd %d", &maxdepth);
            maxtime = 7200000;
            if (maxdepth > MAXPLY) maxdepth = MAXPLY;
            continue;
        }
        if (!strcmp(s, "time")) {
            sscanf(fff, "time %d", &mytime);
            maxdepth = MAXPLY;
            continue;
        }
        if (!strcmp(s, "otim")) {
            sscanf(fff, "otim %d", &myotim);
            continue;
        }
        if (!strcmp(s, "moves")) {
            for (i = 0; i < genmoves(onmove, 127); i++)
                printf("%s ", displaymove(M[127][i].m));
            printf("\n");
            continue;
        }
        if (!strcmp(s, "undo")) {
            if ((status - H) < 1) continue;
            onmove ^= 1;
            undomove(onmove);
            continue;
        }
        if (!strcmp(s, "d")) {
            displayb(onmove);
            continue;
        }
        if (!strcmp(s, "remove")) {
            if ((status - H) < 2) continue;
            onmove ^= 1;
            undomove(onmove);
            onmove ^= 1;
            undomove(onmove);
            continue;
        }
        if (!strcmp(s, "ping")) {
            sscanf(fff, "ping %d", &i);
            printf("pong %d\n", i);
            continue;
        }
        if (!strcmp(s, "usermove")) {
            sscanf(fff, "usermove %s", strmov);
            mc = genmoves(onmove, 127);
            m = parsemove(strmov, onmove, mc);
            if ((!m) || (!domove(m, onmove))) {
                printf("Illegalmove(the move can't be executed):%s\n", strmov);
                isgameover(onmove);
                continue;
            }
            else onmove ^= 1;
            isgameover(onmove);
            continue;
        }
        mc = genmoves(onmove, 127);
        m = parsemove(fff, onmove, mc);
        if (m) {
            if (!domove(m, onmove)) {
                printf("Illegalmove(the move can't be executed):%s\n", fff);
                isgameover(onmove);
                continue;
            }
            else onmove ^= 1;
            isgameover(onmove);
            continue;
        }
        printf("Error(unknown command): %s\n", s);
    } while (1);
    close_book();
    return 0;
}