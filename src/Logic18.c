/**************************************************/
/*  Name: Twisted Logic Chess Engine              */
/*  Copyright: 2005                               */
/*  Author: Edsel Apostol                         */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/timeb.h>

/*********************************************/
/*MACROS                                     */
/*********************************************/

#define DEBUG  
/* this is used on debugging, stolen from Fruit :-) */
#ifdef DEBUG
#  define ASSERT(a) { if (!(a)) Print(4, "assertion line %d: \"" #a "\" failed\n",__LINE__); }
#else
#  define ASSERT(a)
#endif

#define TRUE  1
#define FALSE  0

#define MAXPLY 64
#define MAXDATA 1024
#define MAXMOVES 256
#define INF 10000

#define WHITE  0
#define BLACK  1

#define EMPTY  0
#define PAWN  1
#define KNIGHT  2
#define BISHOP  3
#define ROOK  4
#define QUEEN  5
#define KING  6

#define CASTLE      14
#define TWOFORWARD  17
#define PROMOTE     33
#define ENPASSANT    9

#define WCKS 1
#define WCQS 2
#define BCKS 4
#define BCQS 8

#define STARTPOS  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#define SQRANK(x)       ((x) >> 3)
#define SQFILE(x)       ((x) & 7)
#define MAX(a, b)       ((a)>(b)?(a):(b))
#define MIN(a, b)       ((a)<(b)?(a):(b))
#define ABS(a)          ((a)>0?(a):(-a))
#define DISTANCE(a, b)  (MAX((ABS(SQRANK(a)-SQRANK(b))),(ABS(SQFILE(a)-SQFILE(b)))))
    
       
/*********************************************/
/*DATA STRUCTURES                            */
/*********************************************/
/* some basic definitions */
typedef unsigned long long u64;
typedef unsigned long u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef signed long long s64;
typedef signed long s32;
typedef signed short s16;
typedef signed char s8;
/* the move structure */
typedef struct{
    u32 m;
    s32 s;
}move_t;    
/* the move list structure */
typedef struct{
    move_t list[MAXMOVES];
    u32 size; 
    u32 pos;
}movelist_t;
/* the hash type structure */
typedef union{
    u64 b;
    u32 lock;
    u32 key;
}bit64_t;
/* the undo structure */
typedef struct{
    u32 lastmove;
    u32 castle;
    u32 fifty;
    s32 epsq;
    bit64_t hash;
    bit64_t phash;
    bit64_t mathash;
}undo_t;
/* the position structure */
typedef struct{
    u64 pieces[8];
    u64 color[2];
    u32 side;
    u32 ply;
    undo_t *status;
    undo_t undos[MAXDATA];
}position_t;
/* the squares */
enum{
    a1, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8    
};

/*********************************************/
/*CONSTANTS                                  */
/*********************************************/
/* used in routines popFirstBit and getFirstBit */
const int FoldedTable[64] = { 
    63,30, 3,32,59,14,11,33, 
    60,24,50, 9,55,19,21,34, 
    61,29, 2,53,51,23,41,18, 
    56,28, 1,43,46,27, 0,35, 
    62,31,58, 4, 5,49,54, 6, 
    15,52,12,40, 7,42,45,16, 
    25,57,48,13,10,39, 8,44, 
    20,47,38,22,17,37,36,26, 
};
/* used in updating the castle status of the position */
static const int CastleMask[64]={
    13, 15, 15, 15, 12, 15, 15, 14,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    7, 15, 15, 15,  3, 15, 15, 11
};
/* the following constants are used by the Kindergarten bitboard attacks */
const u64 DiagonalMask[64]={
0x8040201008040201ULL, 0x0080402010080402ULL, 0x0000804020100804ULL, 0x0000008040201008ULL, 
0x0000000080402010ULL, 0x0000000000804020ULL, 0x0000000000008040ULL, 0x0000000000000080ULL, 
0x4020100804020100ULL, 0x8040201008040201ULL, 0x0080402010080402ULL, 0x0000804020100804ULL, 
0x0000008040201008ULL, 0x0000000080402010ULL, 0x0000000000804020ULL, 0x0000000000008040ULL, 
0x2010080402010000ULL, 0x4020100804020100ULL, 0x8040201008040201ULL, 0x0080402010080402ULL, 
0x0000804020100804ULL, 0x0000008040201008ULL, 0x0000000080402010ULL, 0x0000000000804020ULL, 
0x1008040201000000ULL, 0x2010080402010000ULL, 0x4020100804020100ULL, 0x8040201008040201ULL, 
0x0080402010080402ULL, 0x0000804020100804ULL, 0x0000008040201008ULL, 0x0000000080402010ULL, 
0x0804020100000000ULL, 0x1008040201000000ULL, 0x2010080402010000ULL, 0x4020100804020100ULL, 
0x8040201008040201ULL, 0x0080402010080402ULL, 0x0000804020100804ULL, 0x0000008040201008ULL, 
0x0402010000000000ULL, 0x0804020100000000ULL, 0x1008040201000000ULL, 0x2010080402010000ULL, 
0x4020100804020100ULL, 0x8040201008040201ULL, 0x0080402010080402ULL, 0x0000804020100804ULL, 
0x0201000000000000ULL, 0x0402010000000000ULL, 0x0804020100000000ULL, 0x1008040201000000ULL, 
0x2010080402010000ULL, 0x4020100804020100ULL, 0x8040201008040201ULL, 0x0080402010080402ULL, 
0x0100000000000000ULL, 0x0201000000000000ULL, 0x0402010000000000ULL, 0x0804020100000000ULL, 
0x1008040201000000ULL, 0x2010080402010000ULL, 0x4020100804020100ULL, 0x8040201008040201ULL
};

const u64 AntiDiagMask[64]={
0x0000000000000001ULL, 0x0000000000000102ULL, 0x0000000000010204ULL, 0x0000000001020408ULL, 
0x0000000102040810ULL, 0x0000010204081020ULL, 0x0001020408102040ULL, 0x0102040810204080ULL, 
0x0000000000000102ULL, 0x0000000000010204ULL, 0x0000000001020408ULL, 0x0000000102040810ULL, 
0x0000010204081020ULL, 0x0001020408102040ULL, 0x0102040810204080ULL, 0x0204081020408000ULL, 
0x0000000000010204ULL, 0x0000000001020408ULL, 0x0000000102040810ULL, 0x0000010204081020ULL, 
0x0001020408102040ULL, 0x0102040810204080ULL, 0x0204081020408000ULL, 0x0408102040800000ULL, 
0x0000000001020408ULL, 0x0000000102040810ULL, 0x0000010204081020ULL, 0x0001020408102040ULL, 
0x0102040810204080ULL, 0x0204081020408000ULL, 0x0408102040800000ULL, 0x0810204080000000ULL, 
0x0000000102040810ULL, 0x0000010204081020ULL, 0x0001020408102040ULL, 0x0102040810204080ULL, 
0x0204081020408000ULL, 0x0408102040800000ULL, 0x0810204080000000ULL, 0x1020408000000000ULL, 
0x0000010204081020ULL, 0x0001020408102040ULL, 0x0102040810204080ULL, 0x0204081020408000ULL, 
0x0408102040800000ULL, 0x0810204080000000ULL, 0x1020408000000000ULL, 0x2040800000000000ULL, 
0x0001020408102040ULL, 0x0102040810204080ULL, 0x0204081020408000ULL, 0x0408102040800000ULL, 
0x0810204080000000ULL, 0x1020408000000000ULL, 0x2040800000000000ULL, 0x4080000000000000ULL, 
0x0102040810204080ULL, 0x0204081020408000ULL, 0x0408102040800000ULL, 0x0810204080000000ULL, 
0x1020408000000000ULL, 0x2040800000000000ULL, 0x4080000000000000ULL, 0x8000000000000000ULL,
};

const u8 FirstRankAttacks[64][8]={
{0xfe,0xfd,0xfb,0xf7,0xef,0xdf,0xbf,0x7f}, {0x2,0xfd,0xfa,0xf6,0xee,0xde,0xbe,0x7e}, 
{0x6,0x5,0xfb,0xf4,0xec,0xdc,0xbc,0x7c}, {0x2,0x5,0xfa,0xf4,0xec,0xdc,0xbc,0x7c}, 
{0xe,0xd,0xb,0xf7,0xe8,0xd8,0xb8,0x78}, {0x2,0xd,0xa,0xf6,0xe8,0xd8,0xb8,0x78}, 
{0x6,0x5,0xb,0xf4,0xe8,0xd8,0xb8,0x78}, {0x2,0x5,0xa,0xf4,0xe8,0xd8,0xb8,0x78}, 
{0x1e,0x1d,0x1b,0x17,0xef,0xd0,0xb0,0x70}, {0x2,0x1d,0x1a,0x16,0xee,0xd0,0xb0,0x70}, 
{0x6,0x5,0x1b,0x14,0xec,0xd0,0xb0,0x70}, {0x2,0x5,0x1a,0x14,0xec,0xd0,0xb0,0x70}, 
{0xe,0xd,0xb,0x17,0xe8,0xd0,0xb0,0x70}, {0x2,0xd,0xa,0x16,0xe8,0xd0,0xb0,0x70}, 
{0x6,0x5,0xb,0x14,0xe8,0xd0,0xb0,0x70}, {0x2,0x5,0xa,0x14,0xe8,0xd0,0xb0,0x70}, 
{0x3e,0x3d,0x3b,0x37,0x2f,0xdf,0xa0,0x60}, {0x2,0x3d,0x3a,0x36,0x2e,0xde,0xa0,0x60}, 
{0x6,0x5,0x3b,0x34,0x2c,0xdc,0xa0,0x60}, {0x2,0x5,0x3a,0x34,0x2c,0xdc,0xa0,0x60}, 
{0xe,0xd,0xb,0x37,0x28,0xd8,0xa0,0x60}, {0x2,0xd,0xa,0x36,0x28,0xd8,0xa0,0x60}, 
{0x6,0x5,0xb,0x34,0x28,0xd8,0xa0,0x60}, {0x2,0x5,0xa,0x34,0x28,0xd8,0xa0,0x60}, 
{0x1e,0x1d,0x1b,0x17,0x2f,0xd0,0xa0,0x60}, {0x2,0x1d,0x1a,0x16,0x2e,0xd0,0xa0,0x60}, 
{0x6,0x5,0x1b,0x14,0x2c,0xd0,0xa0,0x60}, {0x2,0x5,0x1a,0x14,0x2c,0xd0,0xa0,0x60}, 
{0xe,0xd,0xb,0x17,0x28,0xd0,0xa0,0x60}, {0x2,0xd,0xa,0x16,0x28,0xd0,0xa0,0x60}, 
{0x6,0x5,0xb,0x14,0x28,0xd0,0xa0,0x60}, {0x2,0x5,0xa,0x14,0x28,0xd0,0xa0,0x60}, 
{0x7e,0x7d,0x7b,0x77,0x6f,0x5f,0xbf,0x40}, {0x2,0x7d,0x7a,0x76,0x6e,0x5e,0xbe,0x40}, 
{0x6,0x5,0x7b,0x74,0x6c,0x5c,0xbc,0x40}, {0x2,0x5,0x7a,0x74,0x6c,0x5c,0xbc,0x40}, 
{0xe,0xd,0xb,0x77,0x68,0x58,0xb8,0x40}, {0x2,0xd,0xa,0x76,0x68,0x58,0xb8,0x40}, 
{0x6,0x5,0xb,0x74,0x68,0x58,0xb8,0x40}, {0x2,0x5,0xa,0x74,0x68,0x58,0xb8,0x40}, 
{0x1e,0x1d,0x1b,0x17,0x6f,0x50,0xb0,0x40}, {0x2,0x1d,0x1a,0x16,0x6e,0x50,0xb0,0x40}, 
{0x6,0x5,0x1b,0x14,0x6c,0x50,0xb0,0x40}, {0x2,0x5,0x1a,0x14,0x6c,0x50,0xb0,0x40}, 
{0xe,0xd,0xb,0x17,0x68,0x50,0xb0,0x40}, {0x2,0xd,0xa,0x16,0x68,0x50,0xb0,0x40}, 
{0x6,0x5,0xb,0x14,0x68,0x50,0xb0,0x40}, {0x2,0x5,0xa,0x14,0x68,0x50,0xb0,0x40}, 
{0x3e,0x3d,0x3b,0x37,0x2f,0x5f,0xa0,0x40}, {0x2,0x3d,0x3a,0x36,0x2e,0x5e,0xa0,0x40}, 
{0x6,0x5,0x3b,0x34,0x2c,0x5c,0xa0,0x40}, {0x2,0x5,0x3a,0x34,0x2c,0x5c,0xa0,0x40}, 
{0xe,0xd,0xb,0x37,0x28,0x58,0xa0,0x40}, {0x2,0xd,0xa,0x36,0x28,0x58,0xa0,0x40}, 
{0x6,0x5,0xb,0x34,0x28,0x58,0xa0,0x40}, {0x2,0x5,0xa,0x34,0x28,0x58,0xa0,0x40}, 
{0x1e,0x1d,0x1b,0x17,0x2f,0x50,0xa0,0x40}, {0x2,0x1d,0x1a,0x16,0x2e,0x50,0xa0,0x40}, 
{0x6,0x5,0x1b,0x14,0x2c,0x50,0xa0,0x40}, {0x2,0x5,0x1a,0x14,0x2c,0x50,0xa0,0x40}, 
{0xe,0xd,0xb,0x17,0x28,0x50,0xa0,0x40}, {0x2,0xd,0xa,0x16,0x28,0x50,0xa0,0x40}, 
{0x6,0x5,0xb,0x14,0x28,0x50,0xa0,0x40}, {0x2,0x5,0xa,0x14,0x28,0x50,0xa0,0x40}
};
/* the bit masks of Squares */
const u64 A1 = 0x0000000000000001ULL;
const u64 B1 = 0x0000000000000002ULL;
const u64 C1 = 0x0000000000000004ULL;
const u64 D1 = 0x0000000000000008ULL;
const u64 E1 = 0x0000000000000010ULL;
const u64 F1 = 0x0000000000000020ULL;
const u64 G1 = 0x0000000000000040ULL;
const u64 H1 = 0x0000000000000080ULL;
const u64 A2 = 0x0000000000000100ULL;
const u64 B2 = 0x0000000000000200ULL;
const u64 C2 = 0x0000000000000400ULL;
const u64 D2 = 0x0000000000000800ULL;
const u64 E2 = 0x0000000000001000ULL;
const u64 F2 = 0x0000000000002000ULL;
const u64 G2 = 0x0000000000004000ULL;
const u64 H2 = 0x0000000000008000ULL; 
const u64 A3 = 0x0000000000010000ULL;
const u64 B3 = 0x0000000000020000ULL;
const u64 C3 = 0x0000000000040000ULL;
const u64 D3 = 0x0000000000080000ULL;
const u64 E3 = 0x0000000000100000ULL;
const u64 F3 = 0x0000000000200000ULL;
const u64 G3 = 0x0000000000400000ULL;
const u64 H3 = 0x0000000000800000ULL;
const u64 A4 = 0x0000000001000000ULL;
const u64 B4 = 0x0000000002000000ULL;
const u64 C4 = 0x0000000004000000ULL;
const u64 D4 = 0x0000000008000000ULL;
const u64 E4 = 0x0000000010000000ULL;
const u64 F4 = 0x0000000020000000ULL;
const u64 G4 = 0x0000000040000000ULL;
const u64 H4 = 0x0000000080000000ULL; 
const u64 A5 = 0x0000000100000000ULL;
const u64 B5 = 0x0000000200000000ULL;
const u64 C5 = 0x0000000400000000ULL;
const u64 D5 = 0x0000000800000000ULL;
const u64 E5 = 0x0000001000000000ULL;
const u64 F5 = 0x0000002000000000ULL;
const u64 G5 = 0x0000004000000000ULL;
const u64 H5 = 0x0000008000000000ULL;  
const u64 A6 = 0x0000010000000000ULL;
const u64 B6 = 0x0000020000000000ULL;
const u64 C6 = 0x0000040000000000ULL;
const u64 D6 = 0x0000080000000000ULL;
const u64 E6 = 0x0000100000000000ULL;
const u64 F6 = 0x0000200000000000ULL;
const u64 G6 = 0x0000400000000000ULL;
const u64 H6 = 0x0000800000000000ULL; 
const u64 A7 = 0x0001000000000000ULL;
const u64 B7 = 0x0002000000000000ULL;
const u64 C7 = 0x0004000000000000ULL;
const u64 D7 = 0x0008000000000000ULL;
const u64 E7 = 0x0010000000000000ULL;
const u64 F7 = 0x0020000000000000ULL;
const u64 G7 = 0x0040000000000000ULL;
const u64 H7 = 0x0080000000000000ULL;
const u64 A8 = 0x0100000000000000ULL;
const u64 B8 = 0x0200000000000000ULL;
const u64 C8 = 0x0400000000000000ULL;
const u64 D8 = 0x0800000000000000ULL;
const u64 E8 = 0x1000000000000000ULL;
const u64 F8 = 0x2000000000000000ULL;
const u64 G8 = 0x4000000000000000ULL;
const u64 H8 = 0x8000000000000000ULL;
/* the bit masks of Files */
const u64 FileA = 0x0101010101010101ULL;
const u64 FileB = 0x0202020202020202ULL;
const u64 FileC = 0x0404040404040404ULL;
const u64 FileD = 0x0808080808080808ULL;
const u64 FileE = 0x1010101010101010ULL;
const u64 FileF = 0x2020202020202020ULL;
const u64 FileG = 0x4040404040404040ULL;
const u64 FileH = 0x8080808080808080ULL;

u64 FileBB[8] = {
    0x0101010101010101ULL, 0x0202020202020202ULL, 0x0404040404040404ULL, 
    0x0808080808080808ULL, 0x1010101010101010ULL, 0x2020202020202020ULL, 
    0x4040404040404040ULL, 0x8080808080808080ULL
};    
/* the bit masks of Ranks */
const u64 Rank1 = 0xFFULL;
const u64 Rank2 = 0xFF00ULL;
const u64 Rank3 = 0xFF0000ULL;
const u64 Rank4 = 0xFF000000ULL;
const u64 Rank5 = 0xFF00000000ULL;
const u64 Rank6 = 0xFF0000000000ULL;
const u64 Rank7 = 0xFF000000000000ULL;
const u64 Rank8 = 0xFF00000000000000ULL;

const u64 RankBB[8] = {
    0xFFULL, 0xFF00ULL, 0xFF0000ULL, 0xFF000000ULL, 0xFF00000000ULL,
    0xFF0000000000ULL, 0xFF000000000000ULL, 0xFF00000000000000ULL
}; 

u64 EmptyBoardBB = 0ULL;
u64 FullBoardBB  = 0xFFFFFFFFFFFFFFFFULL;

u64 WhiteSquaresBB = 0x55AA55AA55AA55AAULL;
u64 BlackSquaresBB = 0xAA55AA55AA55AA55ULL;
/* init, mixed, castle, en passant, promotion, no-nothing, 
    from the CRBMG of Sune Fischer */
char FenString[6][256] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
    "r3k2r/3q4/2n1b3/7n/1bB5/2N2N2/1B2Q3/R3K2R w KQkq - 0 1",
    "rnbq1bnr/1pppkp1p/4p3/2P1P3/p5p1/8/PP1PKPPP/RNBQ1BNR w - - 0 1",
    "rn1q1bnr/1bP1kp1P/1p2p3/p7/8/8/PP1pKPpP/RNBQ1BNR w - - 0 1",
    "r6r/3qk3/2n1b3/7n/1bB5/2N2N2/1B2QK2/R6R w - - 0 1"
}; 


/*********************************************/
/*VARIABLES                                  */
/*********************************************/
/* the precomputed static piece 64 bit attacks */
u64 KnightMoves[64];
u64 KingMoves[64];
u64 PawnCaps[64][2];
u64 PawnMoves[64][2];
u64 PawnMoves2[64][2];
/* used by the Kindergarten bitboard attacks generation */
u64 FillUpAttacks[64][8]; 
u64 AFileAttacks[64][8]; 
/* the bit mask on a given square */
u64 BitMask[64];
/* the rank and file mask indexed by square */
u64 RankMask[64];
u64 FileMask[64];
/*the 64 bit attacks on 16 different ray directions */
u64 DirA[16][64];
/* the bit mask on rank 2 and 7 according to colors */
u64 PromoteMask[2];
/* contains the delta of possible piece moves between two squares, 
zero otherwise */
s32 Direction[64][64];
/* to be used in the move evasion generator */
s32 RunTo[64][64];
/* this are used in computing hash values, inventor is someone named Zobrist */
bit64_t Zobrist[2][8][64];
bit64_t ZobMat[2][8];
bit64_t ZobEpsq[64];
bit64_t ZobCastle[16];
bit64_t ZobColor;
/* used in debugging, etc.. */
FILE *logfile;
FILE *errfile;
FILE *dumpfile;

/*********************************************/
/*FUNCTIONS                                  */
/*********************************************/
/* a utility to print output into different files */
void Print(int vb, char *fmt, ...){
/*// bit 0: stdout
// bit 1: logfile
// bit 2: errfile
// bit 3: dumpfile */
  	va_list ap;

  	va_start(ap, fmt);
    
  	if (vb&1) {
		vprintf(fmt, ap);
		fflush(stdout);
  	}

  	va_start(ap, fmt);
   	if (logfile&&((vb>>1)&1)) {
     	vfprintf(logfile, fmt, ap);
	    fflush(logfile);
	}

  	va_start(ap, fmt);
   	if (errfile&&((vb>>2)&1)) {
     	vfprintf(errfile, fmt, ap);
	    fflush(errfile);
	}

  	va_start(ap, fmt);
   	if (dumpfile&&((vb>>3)&1)) {
     	vfprintf(dumpfile, fmt, ap);
	    fflush(dumpfile);
	} 

	va_end(ap);
}

/* a utility to print the bits in a position-like format */
void displayBit(u64 a, int x){
    int i, j; 
    for(i = 56; i >= 0; i -= 8){
        Print(x, "\n%d  ",(i / 8) + 1);
        for(j = 0; j < 8; ++j){
             Print(x, "%c ", ((a & ((u64)1 << (i + j))) ? '1' : '_'));
        }    
    }
    Print(x, "\n\n");
    Print(x, "   a b c d e f g h \n\n");
}

/* utilities for move generation */
u32 GenOneForward(u32 f,u32 t)             
    {return (f | (t<<6) | (PAWN<<12))   ;}
u32 GenTwoForward(u32 f,u32 t)             
    {return (f | (t<<6) | (PAWN<<12)   | (1<<16))  ;}
u32 GenPromote(u32 f,u32 t,u32 r,u32 c)  
    {return (f | (t<<6) | (PAWN<<12)   | (c<<18)  | (r<<22) | (1<<17)) ;}
u32 GenPromoteStraight(u32 f,u32 t,u32 r) 
    {return (f | (t<<6) | (PAWN<<12)   | (r<<22)  | (1<<17))   ;}
u32 GenEnPassant(u32 f,u32 t)              
    {return (f | (t<<6) | (PAWN<<12)   | (PAWN<<18) | (1<<21)) ;}
u32 GenPawnMove(u32 f,u32 t,u32 c)        
    {return (f | (t<<6) | (PAWN<<12)   | (c<<18)) ;}
u32 GenKnightMove(u32 f,u32 t,u32 c)      
    {return (f | (t<<6) | (KNIGHT<<12) | (c<<18)) ;}
u32 GenBishopMove(u32 f,u32 t,u32 c)      
    {return (f | (t<<6) | (BISHOP<<12) | (c<<18)) ;}
u32 GenRookMove(u32 f,u32 t,u32 c)        
    {return (f | (t<<6) | (ROOK<<12)   | (c<<18)) ;}
u32 GenQueenMove(u32 f,u32 t,u32 c)       
    {return (f | (t<<6) | (QUEEN<<12)  | (c<<18)) ;}
u32 GenKingMove(u32 f,u32 t,u32 c)        
    {return (f | (t<<6) | (KING<<12)   | (c<<18)) ;}
u32 GenWhiteOO()                             
    {return (e1 | (g1<<6) | (KING<<12)  | (1<<15)) ;}
u32 GenWhiteOOO()                            
    {return (e1 | (c1<<6) | (KING<<12)  | (1<<15)) ;}
u32 GenBlackOO()                             
    {return (e8 | (g8<<6) | (KING<<12)  | (1<<15)) ;}
u32 GenBlackOOO()                            
    {return (e8 | (c8<<6) | (KING<<12)  | (1<<15)) ;}
    
u32 moveFrom(u32 m)      
    {return (63&(m))     ;}        /* Get from square */
u32 moveTo(u32 m)        
    {return (63&(m>>6))  ;}        /* Get to square */
u32 movePiece(u32 m)     
    {return  (7&(m>>12)) ;}        /* Get the piece moving */
u32 moveAction(u32 m)    
    {return (63&(m>>12)) ;}        /* Get action to do */
u32 moveCapture(u32 m)   
    {return  (7&(m>>18)) ;}        /* Get the capture piece */
u32 moveRemoval(u32 m)   
    {return (15&(m>>18)) ;}        /* Get removal to be done */
u32 movePromote(u32 m)   
    {return  (7&(m>>22)) ;}        /* Get promote value */

u32 isCastle(u32 m){ return ((m>>15)&1) ;}
u32 isPawn2Forward(u32 m){ return ((m>>16)&1) ;}
u32 isPromote(u32 m){ return ((m>>17)&1) ;}  
u32 isEnPassant(u32 m){ return ((m>>21)&1) ;}  

/* a utility to get a certain piece from a position given a square */
u32 getPiece(const position_t *pos, int sq){
    ASSERT(sq >= a1 || sq <= h8);
    u64 mask = BitMask[sq];
    if(mask & pos->pieces[EMPTY]) return EMPTY;
    else if(mask & pos->pieces[PAWN]) return PAWN;
    else if(mask & pos->pieces[KNIGHT]) return KNIGHT;
    else if(mask & pos->pieces[BISHOP]) return BISHOP;
    else if(mask & pos->pieces[ROOK]) return ROOK;
    else if(mask & pos->pieces[QUEEN]) return QUEEN;
    else{
        ASSERT(mask & pos->pieces[KING]);
        return KING;
    }    
}

/* a utility to get a certain color from a position given a square */
u32 getColor(const position_t *pos, int sq){
    ASSERT(sq >= a1 || sq <= h8);
    u64 mask = BitMask[sq];
    if(mask & pos->color[WHITE]) return WHITE;
    else{
        ASSERT(mask & pos->color[BLACK]);
        return BLACK;
    }    
}

/* a utility to convert int move to string */
char *move2Str(int m){
    static char promstr[]="\0pnbrqk";
    static char str[6];
    sprintf(str, "%c%c%c%c%c",
        SQFILE(moveFrom(m)) + 'a',
        '1' + SQRANK(moveFrom(m)),
        SQFILE(moveTo(m)) + 'a',
        '1' + SQRANK(moveTo(m)),
        promstr[movePromote(m)]
    );
    return str;
}

/* a utility to print the position */
void displayBoard(const position_t *pos, int x){
    static char pcstr[] = ".PNBRQK.pnbrqk";
    int i, j, c, p; 
    for(i=56; i>=0; i-=8){
        Print(x, "\n%d  ",(i/8)+1);
        for(j=0;j<8;++j){
            c = getColor(pos, i+j);
            p = getPiece(pos, i+j);
            Print(x, "%c ", pcstr[p+(c?7:0)]);
        }    
    }
    Print(x, "\n\n");
    Print(x, "   a b c d e f g h \n\n");
    Print(x, "%d.%s%s ", (pos->status-&pos->undos[0])/2
        +(pos->side?1:0), pos->side?" ":" ..",
        move2Str(pos->status->lastmove));
    Print(x, "Castling = %d, ", pos->status->castle);
    Print(x, "En passant = %d, ", pos->status->epsq);
    Print(x, "Fifty moves = %d, ", pos->status->fifty);
    Print(x, "inCheck = %s\n", 
        isAtt(pos, pos->side^1, pos->pieces[KING]&pos->color[pos->side])
        ? "TRUE" : "FALSE");
    Print(x, "%s, ", pos->side == WHITE ? "WHITE" : "BLACK");
    Print(x, "Hash = %llu, ", pos->status->hash);
    Print(x, "Pawn Hash = %llu, ", pos->status->phash);
    Print(x, "Material Hash = %llu\n\n", pos->status->mathash);
}

/* this returns the bit mask given a certain square, 
used only in initialization */
u64 getBitMask(int f){
    ASSERT(f >= a1 && f <= h8);

    return (u64)1 << f;
}

/* this is a utility to generate 32 bit pseudo-random numbers */
static u32 rndseed = 3;
u32 rand32(void){
    u32 r = rndseed;
    r = 1664525UL * r + 1013904223UL;
    return (rndseed = r);
}
/* this count the number bits in a given bitfield, 
it is using a SWAR algorithm I think */
int bitCnt(u64 x){
    x -= (x >> 1) & 0x5555555555555555ULL;             
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL); 
    x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0fULL;       
    return (x * 0x0101010101010101ULL) >> 56;  
}
/* returns the least significant square from the 64 bitfield */
int getFirstBit(u64 b) { 
    b ^= (b - 1); 
    u32 folded = ((int)b) ^ ((int)(b >> 32)); 
    return FoldedTable[(folded * 0x78291ACF) >> 26]; 
} 
/* returns the least significant square
 and clears it from the 64 bitfield */
int popFirstBit(u64 *b) {
    u64 bb = *b ^ (*b - 1);
    u32 folded = ((int)bb ^ (int)(bb >> 32));
    *b &= (*b - 1);
    return FoldedTable[(folded * 0x78291ACF) >> 26]; 
}

/* this initializes the pseudo-constant variables used in the program */
void initArr(void){
    int i, j, m, k, n;
    u64 h;
    
    const int kingd[] = {8,-9,-1,7,8,9,1,-7,-8};
    const int knightd[] = {8,-17,-10,6,15,17,10,-6,-15};
    const int wpawnd[] = {1,8};
    const int bpawnd[] = {1,-8};
    const int wpawnc[] = {2,7,9};
    const int bpawnc[] = {2,-7,-9};
    const int wpawn2mov[] = {1,16};
    const int bpawn2mov[] = {1,-16};

    memset(KnightMoves, 0, sizeof(KnightMoves));
    memset(KingMoves, 0, sizeof(KingMoves));
    memset(PawnMoves, 0, sizeof(PawnMoves));
    memset(PawnCaps, 0, sizeof(PawnCaps));
    memset(PawnMoves2, 0, sizeof(PawnMoves2));
    
    memset(Zobrist, 0, sizeof(Zobrist));
    memset(ZobCastle, 0, sizeof(ZobCastle));
    memset(ZobMat, 0, sizeof(ZobMat));
    memset(ZobEpsq, 0, sizeof(ZobEpsq));
    
    
    memset(FillUpAttacks, 0, sizeof(FillUpAttacks));
    memset(AFileAttacks, 0, sizeof(AFileAttacks));
    
    memset(RankMask, 0, sizeof(RankMask));
    memset(FileMask, 0, sizeof(FileMask));
    memset(Direction, 0, sizeof(Direction));
    memset(DirA, 0, sizeof(DirA));
    memset(PromoteMask, 0, sizeof(PromoteMask));
              
    ZobColor.key = rand32();
    ZobColor.lock = rand32();
    for(i = 0; i < 16; i++){
        ZobCastle[i].key = rand32();
        ZobCastle[i].lock = rand32();
    }
    for(i = 0; i < 64; i++){
        ZobEpsq[i].key = rand32();
        ZobEpsq[i].lock = rand32();
    }
    for(i = 0; i < 2; i++){
        for(j = 0; j < 8; j++){ 
            ZobMat[i][j].key = rand32();
            ZobMat[i][j].lock = rand32();
        }
    }
    for(i = 0; i < 64; i++){
        for(j = 0; j < 64; j++){ 
            RunTo[i][j] = -1;
        }
    }        
    for(i = 0; i < 0x40; i++){
        for(j = 0; j < 2; j++) 
            for(k = 0; k < 7; k++){
                Zobrist[j][k][i].key = rand32();
                Zobrist[j][k][i].lock = rand32();
            }    
        for(j = 0; j < 0x40; j++){
            if(SQRANK(i) == SQRANK(j)) RankMask[i] |= getBitMask(j); 
            if(SQFILE(i) == SQFILE(j)) FileMask[i] |= getBitMask(j); 
        }    
        
        BitMask[i] = getBitMask(i);
        
        for(j = 1; j <= knightd[0]; j++){
            n = i + knightd[j];
            if(n < 64 && n >= 0 && ABS(((n & 7) - (i & 7))) <= 2) 
                KnightMoves[i] |= getBitMask(n);
        }
        for(j = 1; j <= kingd[0]; j++){
            n = i + kingd[j];
            if(n < 64 && n >= 0 && ABS(((n & 7) - (i & 7))) <= 2) 
                KingMoves[i] |= getBitMask(n);
        }
        for(j = 1; j <= wpawnd[0]; j++){
            n = i + wpawnd[j];
            if(n < 64 && n >= 0 && ABS(((n & 7) - (i & 7))) <= 2) 
                PawnMoves[i][WHITE] |= getBitMask(n);
        }
        for(j = 1; j <= bpawnd[0]; j++){
            n = i + bpawnd[j];
            if(n < 64 && n >= 0 && ABS(((n & 7) - (i & 7))) <= 2) 
                PawnMoves[i][BLACK] |= getBitMask(n);
        }
        for(j = 1; j <= wpawnc[0]; j++){
            n = i + wpawnc[j];
            if(n < 64 && n >=0 && ABS(((n & 7) - (i & 7))) <= 2) 
                PawnCaps[i][WHITE] |= getBitMask(n);
        }
        for(j = 1; j <= bpawnc[0]; j++){
            n = i + bpawnc[j];
            if(n < 64 && n >= 0 && ABS(((n & 7) - (i & 7))) <= 2) 
                PawnCaps[i][BLACK] |= getBitMask(n);
        }
        for(j = 1; j <= wpawn2mov[0]; j++){
            n = i + wpawn2mov[j];
            if(n < 64 && n >= 0 && ABS(((n & 7) - (i & 7))) <= 2) 
                PawnMoves2[i][WHITE] |= getBitMask(n);
        }
        for(j = 1; j <= bpawn2mov[0]; j++){
            n = i + bpawn2mov[j];
            if(n < 64 && n >= 0 && ABS(((n & 7) - (i & 7))) <= 2) 
                PawnMoves2[i][BLACK] |= getBitMask(n);
        }
        
        if(SQRANK(i) == 6){
            PromoteMask[WHITE] |= getBitMask(i);
        }    
        if(SQRANK(i) == 1){
            PromoteMask[BLACK] |= getBitMask(i); 
        }    
    }

    for(i = 0; i < 8; i++){
        for(j = 0; j < 64; j++) { 
            FillUpAttacks[j][i] = 0x0101010101010101ULL * FirstRankAttacks[j][i]; 
            AFileAttacks[j][i^7] = ((0x8040201008040201ULL 
                * FirstRankAttacks[j][i]) & 0x8080808080808080ULL) >> 7; 
        } 
    }
    for(i = 0; i < 64; i++){
        for(k = 1; k <= 8; k++){
            DirA[k][i] = 0;
            for(m = -1, j = i ;;){
                n = j + kingd[k];
                if (n < 0 || n > 63 || (j % 8 == 0 && n % 8 == 7) 
                || (j % 8 == 7 && n % 8 == 0)) 
				    break;
                Direction[i][n] = kingd[k];
                DirA[k][i] |= BitMask[n];
                RunTo[i][n] = m;
                m = j = n;
            }
        }
    }
}

/* returns a bitboard with all bits above b filled up (discluding b) */
u64 fillUp(u64 b){
	b |= b << 8;
	b |= b << 16;
	return (b | (b << 32)) << 8;
}

/* returns a bitboard with all bits below b filled down (discluding b) */
u64 fillDown(u64 b){
	b |= b >> 8;
	b |= b >> 16;
	return (b | (b >> 32)) >> 8;
}

/* returns a bitboard with all bits above b filled up (including b) */
u64 fillUp2(u64 b){
	b |= b << 8;
	b |= b << 16;
	return (b | (b << 32));
}

/* returns a bitboard with all bits below b filled down (including b) */
u64 fillDown2(u64 b){
	b |= b >> 8;
	b |= b >> 16;
	return (b | (b >> 32));
}

/* Kogge-Stone Fills from Stefan Wescott
this routines smears the set bits of bitboard g on the direction specified 
on each routine , but only along set bits of p; a reset bit in p 
is enough to halt a smear. 
*/ 
u64 fillUpOccluded(u64 g, u64 p){
	g |= p & (g << 8);
	p &=     (p << 8);
	g |= p & (g << 16);
	p &=     (p << 16);
	return g |= p & (g << 32);
}

u64 fillDownOccluded(u64 g, u64 p){
	g |= p & (g >> 8);
	p &=     (p >> 8);
	g |= p & (g >> 16);
	p &=     (p >> 16);
	return g |= p & (g >> 32);
}

u64 fillLeftOccluded(u64 g, u64 p){
	p &= 0xfefefefefefefefeULL;
	g |= p & (g << 1);
	p &=     (p << 1);
	g |= p & (g << 2);
	p &=     (p << 2);
    return g |= p & (g << 4);
}

u64 fillRightOccluded(u64 g, u64 p){
	p &= 0x7f7f7f7f7f7f7f7fULL;
	g |= p & (g >> 1);
	p &=     (p >> 1);
	g |= p & (g >> 2);
	p &=     (p >> 2);
    return g |= p & (g >> 4);
}

u64 fillUpLeftOccluded(u64 g, u64 p){
	p &= 0xfefefefefefefefeULL;
	g |= p & (g <<  9);
	p &=     (p <<  9);
	g |= p & (g << 18);
	p &=     (p << 18);
    return g |= p & (g << 36);
}

u64 fillUpRightOccluded(u64 g, u64 p){
	p &= 0x7f7f7f7f7f7f7f7fULL;
	g |= p & (g <<  7);
	p &=     (p <<  7);
	g |= p & (g << 14);
	p &=     (p << 14);
    return g |= p & (g << 28);
}

u64 fillDownLeftOccluded(u64 g, u64 p){
	p &= 0xfefefefefefefefeULL;
	g |= p & (g >>  7);
	p &=     (p >>  7);
	g |= p & (g >> 14);
	p &=     (p >> 14);
    return g |= p & (g >> 28);
}

u64 fillDownRightOccluded(u64 g, u64 p){
	p &= 0x7f7f7f7f7f7f7f7fULL;
	g |= p & (g >>  9);
	p &=     (p >>  9);
	g |= p & (g >> 18);
	p &=     (p >> 18);
	return g |= p & (g >> 36);
} 
/* this routines are invented by Gerd Isenberg to compute sliding attacks 
the non-rotated way, he calls this the Kindergarten approach */
u64 rankAttacks(u64 occ, u32 sq){ 
    u32 f = sq & 7; 
    u32 r = sq & ~7; 
    u32 o = (u32)(occ >> (r+1)) & 63; 
    return (u64) FirstRankAttacks[o][f] << r; 
} 

u64 fileAttacks(u64 occ, u32 sq){ 
    u32 f = sq & 7; 
    occ = 0x0101010101010101ULL & (occ >> f); 
    u32 o = (0x0080402010080400ULL * occ) >> 58; 
    return (AFileAttacks[o][sq>>3]) << f; 
} 

u64 diagonalAttacks(u64 occ, u32 sq){ 
    u32 f = sq & 7; 
    occ = (DiagonalMask[sq] & occ); 
    u32 o = (0x0202020202020202ULL * occ) >> 58; 
    return (DiagonalMask[sq] & FillUpAttacks[o][f]); 
} 

u64 antiDiagAttacks(u64 occ, u32 sq){ 
    u32 f = sq & 7; 
    occ = (AntiDiagMask[sq] & occ); 
    u32 o = (0x0202020202020202ULL * occ) >> 58; 
    return (AntiDiagMask[sq] & FillUpAttacks[o][f]); 
} 

/* the following routines are used to generate specific piece moves based on 
the position of the pieces on the position structure */
u64 pawnMoves(const position_t *pos, int from){
    return (pos->pieces[EMPTY] & PawnMoves[from][pos->side]);    
}

u64 pawnMoves2(const position_t *pos, int from){
    return (pos->pieces[EMPTY] & PawnMoves2[from][pos->side]);    
}

u64 pawnCaps(const position_t *pos, int from){
    return (pos->color[pos->side^1] & PawnCaps[from][pos->side]);    
}

u64 knightAttacks(const position_t *pos, int from){
    return (~pos->color[pos->side] & KnightMoves[from]);    
}

u64 bishopAttacks(const position_t *pos, int from){
    u64 occ = ~pos->pieces[EMPTY];
    return ((diagonalAttacks(occ, from) | antiDiagAttacks(occ, from))
    & ~pos->color[pos->side]);
}

u64 rookAttacks(const position_t *pos, int from){
    u64 occ = ~pos->pieces[EMPTY];
    return ((fileAttacks(occ, from) | rankAttacks(occ, from))
    & ~pos->color[pos->side]);
}

u64 queenAttacks(const position_t *pos, int from){
    u64 occ = ~pos->pieces[EMPTY];
    return ((diagonalAttacks(occ, from) | antiDiagAttacks(occ, from)
    | fileAttacks(occ, from) | rankAttacks(occ, from))
    & ~pos->color[pos->side]);
}

u64 kingAttacks(const position_t *pos, int from){
    return (~pos->color[pos->side] & KingMoves[from]);    
}
/* the following routines returns a 64 bit attacks of pieces
on the From square with the limiting Occupied bits */
u64 bishopAttacksBB(int from, u64 occ){
    return (diagonalAttacks(occ, from) | antiDiagAttacks(occ, from));
}

u64 rookAttacksBB(int from, u64 occ){
    return (fileAttacks(occ, from) | rankAttacks(occ, from));
}

u64 queenAttacksBB(int from, u64 occ){
    return (diagonalAttacks(occ, from) | antiDiagAttacks(occ, from)
    | fileAttacks(occ, from) | rankAttacks(occ, from));
}

/* this is the attack routine, capable of multiple targets, 
can be used to determine if a position is in check*/
int isAtt(const position_t *pos, int color, u64 target){
    int from;
    while(target){
        
        from = popFirstBit(&target);
        
        if(((pos->pieces[KING]|pos->pieces[QUEEN]) & pos->color[color])
            & KingMoves[from]) return TRUE;
            
        if(((pos->pieces[PAWN]|pos->pieces[BISHOP]) & pos->color[color])
            & PawnCaps[from][color^1]) return TRUE;
            
        if((pos->pieces[KNIGHT] & pos->color[color]) 
            & KnightMoves[from]) return TRUE;
            
        if(((pos->pieces[ROOK]|pos->pieces[QUEEN]) & pos->color[color])
            & rookAttacksBB(from, ~pos->pieces[EMPTY])) return TRUE;
        
        if(((pos->pieces[BISHOP]|pos->pieces[QUEEN]) & pos->color[color])
            & bishopAttacksBB(from, ~pos->pieces[EMPTY])) return TRUE;            
    }    
    return FALSE;
}

/* a simple utility, just for simplification purposes */
u64 getPiecesBB(const position_t *pos, int piece, int color){
    return (pos->pieces[piece] & pos->color[color]);    
}
/* for debugging only */
void displayBitPieces(const position_t *pos, int x){
    Print(x, "The position:\n");
    displayBoard(pos, x);
    Print(x, "White pieces:\n");
    displayBit(pos->color[WHITE], x);
    Print(x, "Black pieces:\n");
    displayBit(pos->color[BLACK], x);
    /* Print(x, "empty pieces\n");
    displayBit(pos->pieces[EMPTY], x);
    Print(x, "pawn pieces\n");
    displayBit(pos->pieces[PAWN], x);
    Print(x, "knight pieces\n");
    displayBit(pos->pieces[KNIGHT], x);
    Print(x, "bishop pieces\n");
    displayBit(pos->pieces[BISHOP], x);
    Print(x, "rook pieces\n");
    displayBit(pos->pieces[ROOK], x);
    Print(x, "queen pieces\n");
    displayBit(pos->pieces[QUEEN], x);
    Print(x, "king pieces\n");
    displayBit(pos->pieces[KING], x); */    
}


/* the move generator, this generates all pseudo-legal moves
castling is generated legally */
void genMoves(const position_t *pos, movelist_t *ml){
    int from, to;
    u64 pc_bits, mv_bits;
    
    ASSERT(!(pos->pieces[EMPTY] & pos->color[WHITE]));
    ASSERT(!(pos->pieces[EMPTY] & pos->color[BLACK]));
    ASSERT(!(pos->color[BLACK] & pos->color[WHITE]));
    if((pos->color[BLACK] & pos->color[WHITE]))
        displayBitPieces(pos, 8);
    
    ASSERT(pos->side == WHITE || pos->side == BLACK);
    ASSERT(pos->status->castle >= 0 || pos->status->castle <= 15);
    if(pos->status->epsq != -1)
        ASSERT(pos->status->epsq >= h3 || pos->status->epsq <= h6);

    ml->size = 0;    
    
    pc_bits = pos->pieces[PAWN] & pos->color[pos->side];
    while(pc_bits){
        from = popFirstBit(&pc_bits);
        /* pawn move 1 forward */
        mv_bits = pawnMoves(pos, from);    
        while(mv_bits){
            to = popFirstBit(&mv_bits);
            if((PromoteMask[pos->side] & BitMask[from])){
                ml->list[ml->size++].m = GenPromoteStraight(from, to, KNIGHT);
                ml->list[ml->size++].m = GenPromoteStraight(from, to, BISHOP);
                ml->list[ml->size++].m = GenPromoteStraight(from, to, ROOK);
                ml->list[ml->size++].m = GenPromoteStraight(from, to, QUEEN);
            }else
                ml->list[ml->size++].m = GenOneForward(from, to);
        }
        /* pawn captures */
        mv_bits = pawnCaps(pos, from);    
        while(mv_bits){
            to = popFirstBit(&mv_bits);
            if((PromoteMask[pos->side] & BitMask[from])){
                ml->list[ml->size++].m = GenPromote(from, to, KNIGHT, getPiece(pos, to));
                ml->list[ml->size++].m = GenPromote(from, to, BISHOP, getPiece(pos, to));
                ml->list[ml->size++].m = GenPromote(from, to, ROOK, getPiece(pos, to));
                ml->list[ml->size++].m = GenPromote(from, to, QUEEN, getPiece(pos, to));
            }else
                ml->list[ml->size++].m = GenPawnMove(from, to, getPiece(pos, to));
        }
        /* pawn moves 2 forward */
        mv_bits = 0;
        if((pawnMoves(pos, from) && (PromoteMask[pos->side^1] & BitMask[from]))){
            mv_bits = pawnMoves2(pos, from);
        } 
        while(mv_bits){
            to = popFirstBit(&mv_bits);
            ml->list[ml->size++].m = GenTwoForward(from, to);
        }
    }
    
    pc_bits = pos->pieces[KNIGHT] & pos->color[pos->side];
    while(pc_bits){
        from = popFirstBit(&pc_bits);
        mv_bits = knightAttacks(pos, from); 
        while(mv_bits){
            to = popFirstBit(&mv_bits);
            ml->list[ml->size++].m = GenKnightMove(from, to, getPiece(pos, to));  
        }
    }
    
    pc_bits = pos->pieces[BISHOP] & pos->color[pos->side];
    while(pc_bits){
        from = popFirstBit(&pc_bits);
        mv_bits = bishopAttacks(pos, from); 
        while(mv_bits){
            to = popFirstBit(&mv_bits);
            ml->list[ml->size++].m = GenBishopMove(from, to, getPiece(pos, to));
        }
    }
    
    pc_bits = pos->pieces[ROOK] & pos->color[pos->side];
    while(pc_bits){
        from = popFirstBit(&pc_bits);
        mv_bits = rookAttacks(pos, from); 
        while(mv_bits){
            to = popFirstBit(&mv_bits);
            ml->list[ml->size++].m = GenRookMove(from, to, getPiece(pos, to));
        }
    }
    
    pc_bits = pos->pieces[QUEEN] & pos->color[pos->side];
    while(pc_bits){
        from = popFirstBit(&pc_bits);
        mv_bits = queenAttacks(pos, from); 
        while(mv_bits){
            to = popFirstBit(&mv_bits);
            ml->list[ml->size++].m = GenQueenMove(from, to, getPiece(pos, to));
        }
    }
    
    pc_bits = pos->pieces[KING] & pos->color[pos->side];
    while(pc_bits){
        from = popFirstBit(&pc_bits);
        mv_bits = kingAttacks(pos, from); 
        while(mv_bits){
            to = popFirstBit(&mv_bits);
            ml->list[ml->size++].m = GenKingMove(from, to, getPiece(pos, to));
        }
    }
    if(pos->side == BLACK){
        if((pos->status->castle&BCKS) && (!(~pos->pieces[EMPTY]&(F8|G8)))){
            if(!isAtt(pos, pos->side^1, E8) && !isAtt(pos, pos->side^1, F8)
            && !isAtt(pos, pos->side^1, G8))
             ml->list[ml->size++].m = GenBlackOO();
        }
        if((pos->status->castle&BCQS) && (!(~pos->pieces[EMPTY]&(B8|C8|D8)))){
            if(!isAtt(pos, pos->side^1, E8) && !isAtt(pos, pos->side^1, D8)
            && !isAtt(pos, pos->side^1, C8))
            ml->list[ml->size++].m = GenBlackOOO();
        }
    }else{    
        if((pos->status->castle&WCKS) && (!(~pos->pieces[EMPTY]&(F1|G1)))){
            if(!isAtt(pos, pos->side^1, E1) && !isAtt(pos, pos->side^1, F1)
            && !isAtt(pos, pos->side^1, G1))
            ml->list[ml->size++].m = GenWhiteOO();
        }
        if((pos->status->castle&WCQS) && (!(~pos->pieces[EMPTY]&(B1|C1|D1)))){
            if(!isAtt(pos, pos->side^1, E1) && !isAtt(pos, pos->side^1, D1)
            && !isAtt(pos, pos->side^1, C1))
            ml->list[ml->size++].m = GenWhiteOOO();
        }
    }
    
    if((pos->status->epsq != -1)){
        mv_bits = pos->pieces[PAWN] & pos->color[pos->side] 
        & PawnCaps[pos->status->epsq][pos->side^1];
        while(mv_bits){
            from = popFirstBit(&mv_bits);
            ml->list[ml->size++].m = GenEnPassant(from, pos->status->epsq);
        }
    }    
}
/* this returns the pinned pieces to the King of the side Color, 
idea stolen from Glaurung*/
u64 pinnedPieces(const position_t *pos, int color){
  u64 b1, b2, pinned, pinners, sliders;
  int ksq = getFirstBit((pos->pieces[KING]&pos->color[color])), temp;
  
  pinned = EmptyBoardBB;
  b1 = ~pos->pieces[EMPTY];

  sliders = (pos->pieces[QUEEN]|pos->pieces[ROOK]) & pos->color[color^1];
  if(sliders) {
    b2 = rookAttacksBB(ksq, b1) & pos->color[color];
    pinners = rookAttacksBB(ksq, b1 ^ b2) & sliders;
    while(pinners) {
      temp = popFirstBit(&pinners);
      pinned |= (rookAttacksBB(temp, b1) & b2);
    }
  }

  sliders = (pos->pieces[QUEEN]|pos->pieces[BISHOP]) & pos->color[color^1];
  if(sliders) {
    b2 = bishopAttacksBB(ksq, b1) & pos->color[color];
    pinners = bishopAttacksBB(ksq, b1 ^ b2) & sliders;
    while(pinners) {
      temp = popFirstBit(&pinners);
      pinned |= (bishopAttacksBB(temp, b1) & b2);
    }
  }

  return pinned;
}

/* this enables checking for move legality without updating the position structure,
idea stolen from Glaurung */
int moveIsLegal(const position_t *pos, int m, u64 pinned){
  int us, them;
  int ksq, from, to;

  /* If we're in check, all pseudo-legal moves are legal, because our
  // check evasion generator only generates TRUE legal moves.
  if(this->is_check()) return TRUE; */
  
  if(isCastle(m)) return TRUE;
  
  us = pos->side;
  them = us ^ 1;

  from = moveFrom(m);
  to = moveTo(m);
  ksq = getFirstBit(getPiecesBB(pos, KING, us));
  
  /* En passant captures are a tricky special case.  Because they are
  // rather uncommon, we do it simply by testing whether the king is attacked
  // after the move is made: */
  if(isEnPassant(m)) {
    int capsq = (SQRANK(from) << 3) + SQFILE(to);
    u64 b = ~pos->pieces[EMPTY];
    
    b ^= from ^ capsq ^ to;

    return
      (!(rookAttacksBB(ksq, b) & (getPiecesBB(pos, QUEEN, them)
       | getPiecesBB(pos, ROOK, them))) &&
       !(bishopAttacksBB(ksq, b) & (getPiecesBB(pos, QUEEN, them)
       | getPiecesBB(pos, ROOK, them))));
  }
  
  /* If the moving piece is a king, check whether the destination 
  // square is attacked by the opponent. */
  if(from == ksq) return !(isAtt(pos, them, BitMask[to]));

  /* A non-king move is legal if and only if it is not pinned or it
  // is moving along the ray towards or away from the king. */
  if(!(pinned & BitMask[from])) return TRUE;
  if(Direction[from][ksq] == Direction[to][ksq])
    return TRUE;

  return FALSE;
}

/* this updates the position structure from the move being played */
void makeMove(position_t *pos, int m){
	u32 rook_from, rook_to, epsq;
    int from = moveFrom(m);
    int to = moveTo(m);
        
    int side = pos->side;
    int xside = side^1;
    undo_t *temp;    
    
    temp = ++pos->status;
    temp->lastmove = m;
    temp->epsq = -1;
    temp->castle = (temp - 1)->castle & CastleMask[from] & CastleMask[to];
    temp->fifty = (temp - 1)->fifty + 1;
    temp->hash.b = (temp - 1)->hash.b;
    temp->phash.b = (temp - 1)->phash.b;
    temp->mathash.b = (temp - 1)->mathash.b;
    
    temp->hash.b ^= ZobCastle[temp->castle].b;
    temp->hash.b ^= ZobCastle[(temp-1)->castle].b;
    temp->hash.b ^= ZobColor.b; 

    switch(moveAction(m)) {
		case PAWN:   
			pos->pieces[PAWN] ^= BitMask[from] ^ BitMask[to];
            break;
		case KNIGHT: 
			pos->pieces[KNIGHT] ^= BitMask[from] ^ BitMask[to];
            break;
		case BISHOP: 
			pos->pieces[BISHOP] ^= BitMask[from] ^ BitMask[to];
            break;
		case ROOK:   
			pos->pieces[ROOK] ^= BitMask[from] ^ BitMask[to];
            break;
		case QUEEN:  
			pos->pieces[QUEEN] ^= BitMask[from] ^ BitMask[to];
            break;
		case KING:   
			pos->pieces[KING] ^= BitMask[from] ^ BitMask[to];
            break;
		case CASTLE:
			pos->pieces[KING] ^= BitMask[from] ^ BitMask[to];
            switch (to) {
				case c1: rook_from = a1; rook_to = d1; break;
				case g1: rook_from = h1; rook_to = f1; break;
				case c8: rook_from = a8; rook_to = d8; break;
				case g8: rook_from = h8; rook_to = f8; break;
			}					
			pos->pieces[ROOK] ^= BitMask[rook_from] ^ BitMask[rook_to];
			pos->color[side] ^= BitMask[rook_from] ^ BitMask[rook_to];
			break;
		case TWOFORWARD:
			pos->pieces[PAWN] ^= BitMask[from] ^ BitMask[to];
            switch(side){
                case WHITE: temp->epsq = to - 8; break;    
                case BLACK: temp->epsq = to + 8; break;
            }
			break;
		case PROMOTE:
			pos->pieces[PAWN] ^= BitMask[from]; 
			switch(movePromote(m)) {
				case KNIGHT: 
                    pos->pieces[KNIGHT] ^= BitMask[to];  
                    break;
				case BISHOP: 
                    pos->pieces[BISHOP] ^= BitMask[to];  
                    break;
				case ROOK:   
                    pos->pieces[ROOK] ^= BitMask[to];  
                    break;
				case QUEEN:  
                    pos->pieces[QUEEN] ^= BitMask[to];  
                    break;
			}
			break;		
	}

	switch(moveRemoval(m)) {
		case PAWN:   
		    pos->pieces[PAWN] ^= BitMask[to];  
		    pos->color[xside] ^= BitMask[to];  
			break;
		case KNIGHT:
			pos->pieces[KNIGHT] ^= BitMask[to];  
		    pos->color[xside] ^= BitMask[to]; 
			break;
		case BISHOP: 
			pos->pieces[BISHOP] ^= BitMask[to];  
		    pos->color[xside] ^= BitMask[to]; 
			break;
		case ROOK:
			pos->pieces[ROOK] ^= BitMask[to];  
		    pos->color[xside] ^= BitMask[to]; 
			break;
		case QUEEN:
			pos->pieces[QUEEN] ^= BitMask[to];  
		    pos->color[xside] ^= BitMask[to]; 
			break;
		case ENPASSANT:
			switch(side){
                case WHITE: epsq = to - 8; break;    
                case BLACK: epsq = to + 8; break;
            } 
            pos->pieces[PAWN] ^= BitMask[epsq]; 
			pos->color[xside] ^= BitMask[epsq]; 
			break;
	}               

	pos->color[side] ^= BitMask[from] ^ BitMask[to];
	pos->pieces[EMPTY] = (~(pos->color[side]|pos->color[xside]));
	
	ASSERT(!(pos->color[side]&pos->color[xside]));
	
	++pos->ply;					
	pos->side = xside;
}
    
/* this undos the move done */
void unmakeMove(position_t *pos){
    u32 rook_from, rook_to, epsq;
    int from;
    int to;
    int m;        
    int side; 
    int xside;
    
    m = pos->status->lastmove;
    --pos->status;
	--pos->ply;					
	xside = pos->side;
	side = xside ^ 1;
	pos->side = side;
	from = moveFrom(m);
    to = moveTo(m);

    switch(moveAction(m)) {
		case PAWN:   
			pos->pieces[PAWN] ^= BitMask[from] ^ BitMask[to];
            break;
		case KNIGHT: 
			pos->pieces[KNIGHT] ^= BitMask[from] ^ BitMask[to];
            break;
		case BISHOP: 
			pos->pieces[BISHOP] ^= BitMask[from] ^ BitMask[to];
            break;
		case ROOK:   
			pos->pieces[ROOK] ^= BitMask[from] ^ BitMask[to];
            break;
		case QUEEN:  
			pos->pieces[QUEEN] ^= BitMask[from] ^ BitMask[to];
            break;
		case KING:   
			pos->pieces[KING] ^= BitMask[from] ^ BitMask[to];
            break;
		case CASTLE:
			pos->pieces[KING] ^= BitMask[from] ^ BitMask[to];
            switch (to) {
				case c1: rook_from = a1; rook_to = d1; break;
				case g1: rook_from = h1; rook_to = f1; break;
				case c8: rook_from = a8; rook_to = d8; break;
				case g8: rook_from = h8; rook_to = f8; break;
			}					
			pos->pieces[ROOK] ^= BitMask[rook_from] ^ BitMask[rook_to];
			pos->color[side] ^= BitMask[rook_from] ^ BitMask[rook_to];
			break;
		case TWOFORWARD:
			pos->pieces[PAWN] ^= BitMask[from] ^ BitMask[to];
			break;
		case PROMOTE:
			pos->pieces[PAWN] ^= BitMask[from]; 
			switch(movePromote(m)) {
				case KNIGHT: 
                    pos->pieces[KNIGHT] ^= BitMask[to];  
                    break;
				case BISHOP: 
                    pos->pieces[BISHOP] ^= BitMask[to];  
                    break;
				case ROOK:   
                    pos->pieces[ROOK] ^= BitMask[to];  
                    break;
				case QUEEN:  
                    pos->pieces[QUEEN] ^= BitMask[to];  
                    break;
			}
			break;		
	}

    switch(moveRemoval(m)) {
		case PAWN:   
		    pos->pieces[PAWN] ^= BitMask[to];  
		    pos->color[xside] ^= BitMask[to];  
			break;
		case KNIGHT:
			pos->pieces[KNIGHT] ^= BitMask[to];  
		    pos->color[xside] ^= BitMask[to]; 
			break;
		case BISHOP: 
			pos->pieces[BISHOP] ^= BitMask[to];  
		    pos->color[xside] ^= BitMask[to]; 
			break;
		case ROOK:
			pos->pieces[ROOK] ^= BitMask[to];  
		    pos->color[xside] ^= BitMask[to]; 
			break;
		case QUEEN:
			pos->pieces[QUEEN] ^= BitMask[to];  
		    pos->color[xside] ^= BitMask[to]; 
			break;
		case ENPASSANT:
			switch(side){
                case WHITE: epsq = to - 8; break;    
                case BLACK: epsq = to + 8; break;
            } 
            pos->pieces[PAWN] ^= BitMask[epsq]; 
			pos->color[xside] ^= BitMask[epsq]; 
			break;
	}               

	pos->color[side] ^= BitMask[from] ^ BitMask[to];
	pos->pieces[EMPTY] = (~(pos->color[side]|pos->color[xside]));
	
	ASSERT(!(pos->color[side]&pos->color[xside]));
}

/* sets position from a FEN string*/
void setPosition(position_t *pos, char *fen) {
    int rank = 7, file = 0, pc = 0, color = 0, count = 0, i, sq;
    undo_t *temp;
    
    pos->pieces[EMPTY] = EmptyBoardBB;
    pos->pieces[PAWN] = EmptyBoardBB;
    pos->pieces[KNIGHT] = EmptyBoardBB;
    pos->pieces[BISHOP] = EmptyBoardBB;
    pos->pieces[ROOK] = EmptyBoardBB;
    pos->pieces[QUEEN] = EmptyBoardBB;
    pos->pieces[KING] = EmptyBoardBB;
    
    pos->color[WHITE] = EmptyBoardBB;
    pos->color[BLACK] = EmptyBoardBB;
    
    pos->side = WHITE;
    pos->ply = 0;
    
    for(temp = &pos->undos[0]; temp < &pos->undos[MAXDATA]; temp++){
        temp->lastmove = EMPTY;
        temp->epsq = -1;
        temp->castle = 0;
        temp->fifty = 0;
        temp->hash.b = EmptyBoardBB;
        temp->phash.b = EmptyBoardBB;
        temp->mathash.b = EmptyBoardBB;
    }    
    
    pos->status = &pos->undos[0];      
    
    while((rank >= 0) && *fen){
        count = 1; pc = EMPTY;   
        switch(*fen) {
            case 'K': pc = KING; color = WHITE; break;
            case 'k': pc = KING; color = BLACK; break;
            case 'Q': pc = QUEEN; color = WHITE; break;
            case 'q': pc = QUEEN; color = BLACK; break;
            case 'R': pc = ROOK; color = WHITE; break;
            case 'r': pc = ROOK; color = BLACK; break;
            case 'B': pc = BISHOP; color = WHITE; break;
            case 'b': pc = BISHOP; color = BLACK; break;
            case 'N': pc = KNIGHT; color = WHITE; break;
            case 'n': pc = KNIGHT; color = BLACK; break;
            case 'P': pc = PAWN; color = WHITE; break;
            case 'p': pc = PAWN; color = BLACK; break;
            case '/': case ' ': rank--; file = 0; fen++; continue;
            case '1': case '2': case '3': case '4': case '5': case '6': 
            case '7': case '8': count = *fen - '0'; break;
            default: Print(3, "info string FEN Error 1!"); return; 
        }
        for(i = 0; i < count; i++, file++){
            sq = (rank * 8) + file;
            
            ASSERT(pc >= EMPTY && pc <= KING);
            ASSERT(sq >= a1 && sq <= h8);
            ASSERT(color == BLACK || color == WHITE);
            
            if(pc != EMPTY){
                pos->pieces[pc] ^= BitMask[sq];
                pos->color[color] ^= BitMask[sq];
                
                pos->status->hash.b ^= Zobrist[color][pc][sq].b;
                pos->status->mathash.b ^= ZobMat[color][pc].b;
                
                if(pc == PAWN) pos->status->phash.b ^= Zobrist[color][pc][sq].b; 
            }else{
                pos->pieces[EMPTY] ^= BitMask[sq];
            }  
        }
        fen++;
    }    
    while(isspace(*fen)) fen++;
    switch(tolower(*fen)) {
        case 'w': pos->side = WHITE; break;
        case 'b': pos->side = BLACK; break;
        default: Print(3, "info string FEN Error 2!\n"); return; 
    }
    do {fen++;} while(isspace(*fen));
    
    while(*fen != '\0' && !isspace(*fen)) {
        if(*fen == 'K') pos->status->castle |= WCKS;
        else if(*fen == 'Q') pos->status->castle |= WCQS;
        else if(*fen == 'k') pos->status->castle |= BCKS;
        else if(*fen == 'q') pos->status->castle |= BCQS;
        fen++;
    }
    while(isspace(*fen)) fen++;
    
    if(*fen!='\0') {
        if(*fen!='-'){
            if(fen[0] >= 'a' && fen[0] <= 'h' && fen[1] >= '1' && fen[1] <= '8')
            pos->status->epsq = fen[0] - 'a' + (fen[1] - '1') * 8;
            do{fen++;} while(!isspace(*fen));
        }
        do{fen++;} while(isspace(*fen));
        if(isdigit(*fen)) sscanf(fen, "%d", &pos->status->fifty); 
    }
    pos->status->hash.b ^= ZobCastle[pos->status->castle].b;
    if(pos->status->epsq != -1) pos->status->hash.b ^= ZobEpsq[pos->status->epsq].b;
    if(pos->side == BLACK) pos->status->hash.b ^= ZobColor.b;
}
/* the recursive perft routine, it execute make_move on all moves*/
void perft(position_t *pos, u32 maxply, u64 nodesx[]){
    int move; u64 pinned;
    movelist_t movelist;
    if(pos->ply+1 > maxply) return;
    genMoves(pos, &movelist);
    pinned = pinnedPieces(pos, pos->side);
    for(movelist.pos = 0; movelist.pos < movelist.size; movelist.pos++){
        move = movelist.list[movelist.pos].m;
        if(!moveIsLegal(pos, move, pinned)) continue; 
        nodesx[pos->ply+1]++;
        makeMove(pos, move);
        perft(pos, maxply, nodesx);
        unmakeMove(pos); 
    }   
}

/* returns time in milli-seconds */
int getTime(void){
    static struct _timeb tv;
    _ftime(&tv);
    return (tv.time*1000+tv.millitm);
}
/* this is the perft controller */
void runPerft(int max_depth){
    position_t pos;
    movelist_t ml;
    int depth, i, x;
    u64 nodes, time_start, nodesx[MAXPLY], duration;
    
    Print(3, "See the logfile.txt on the same directory to view\n");
    Print(3, "the detailed output after running.\n");
        
    for(x = 0; x < 6; x++){
    	setPosition(&pos, FenString[x]);        
    	
        Print(3, "FEN %d = %s\n", x+1, FenString[x]);
        
    	displayBoard(&pos, 3);
    	
    	genMoves(&pos, &ml);
    	
    	Print(3, "pseudo-legalmoves = %d", ml.size);
    	for(ml.pos = 0; ml.pos < ml.size; ml.pos++){
	        if(!(ml.pos%12)) Print(3, "\n");
    	    Print(3, "%s ", move2Str(ml.list[ml.pos].m));
    	}    
        Print(3, "\n\n");
    	        				
    	for(depth = 1; depth <= max_depth; depth++) {     
    		    
            Print(3, "perft %d ", depth);          
        
            memset(nodesx, 0, sizeof(nodesx));
            			
            time_start = getTime();
            			
            perft(&pos, depth, nodesx);
            			
            duration = getTime() - time_start;
            					
            nodes = 0;
            if(duration == 0) duration = 1;
            for(i = 1; i <= depth; i++) nodes += nodesx[i];
            			
            Print(3, "%llu\t\t", nodesx[depth]);
            Print(3, "[%llu ms - ", duration);
            Print(3, "%llu KNPS]\n", nodes/duration); 
    	}
    	Print(3, "\nDONE DOING PERFT ON FEN %d\n", x+1);
    	Print(3, "\n\n\n");
    }      
}
/* parse the move from string and returns a move from the
move list of generated moves if the move string matched 
one of them */
int parseMove(char *s, movelist_t *ml){
    int m, from, to, p;
    
    from = (s[0] - 'a') + (8 * (s[1] - '1'));
	to = (s[2] - 'a') + (8 * (s[3] - '1'));
	m = (from) | (to << 6);

   	for(ml->pos = 0; ml->pos < ml->size; ml->pos++){
        if(m == (ml->list[ml->pos].m & 0xfff)){
            p = EMPTY;
            if(movePromote(ml->list[ml->pos].m)){
                switch(s[4]){
                    case 'n': case 'N': p = KNIGHT; break;
                    case 'b': case 'B': p = BISHOP; break;
                    case 'r': case 'R': p = ROOK; break;
                    default: p = QUEEN; break;
                }
            }
            if(p == movePromote(ml->list[ml->pos].m)) return ml->list[ml->pos].m;
        }
    }
    return 0;
}
/* this is the main loop of the program */
int main(int argc, char *argv[]){
    position_t pos;
    movelist_t ml;
    char command[256];
    char temp[256];
    int move;
    int i;
    
    Print(3, "Twisted Logic Revenge alpha by Edsel Apostol\n");
    Print(3, "under development, please type help for commands\n\n");
    
    logfile = fopen("logfile.txt", "w+");
    errfile = fopen("errfile.txt", "w+");
    dumpfile = fopen("dumpfile.txt", "w+");
    
    initArr();
    
    setPosition(&pos, FenString[0]);
    
    while(TRUE){
        displayBoard(&pos, 3);
        genMoves(&pos, &ml);
        Print(3, "pseudo-legal moves = %d:", ml.size);
        for(ml.pos = 0; ml.pos < ml.size; ml.pos++){
            if(!(ml.pos%12)) Print(3, "\n");
            Print(3, "%s ", move2Str(ml.list[ml.pos].m));
        }    
        Print(3, "\n\n");
        Print(1, "Logic >>");
       	if(!fgets(command, 256, stdin)) break;
        if(command[0]=='\n') continue;
        sscanf(command, "%s", temp);
        if(!strcmp(temp, "new")){
            setPosition(&pos, FenString[0]);
        }else if(!strcmp(temp, "undo")){
            if(pos.status > &pos.undos[0]) unmakeMove(&pos);
        }else if(!strcmp(temp, "perft")){
            sscanf(command, "perft %d", &move);
            runPerft(move);
        }else if(!strcmp(temp, "quit")){
            break;
        }else if(!strcmp(temp, "help")){
            Print(3, "help - displays this texts\n");
            Print(3, "new - initialize to the starting position\n");
            Print(3, "undo - undo the last move done\n");
            Print(3, "perft X - do a perft test from a set of test positions for depth X\n");
            Print(3, "quit - quits the program\n");
            Print(3, "this are the only commands as of now\n");
            Print(3, "press any key to continue...\n");
            getch();
        }else{    
            move = parseMove(command, &ml);
            if(move){
                makeMove(&pos, move);
            }else Print(3, "Illegal move: %s\n", command);    
        }    
    }    
	fflush(stdout);
	fflush(logfile);
	fflush(errfile);
	fflush(dumpfile);
	fclose(logfile);
	fclose(errfile);
	fclose(dumpfile);
	logfile = NULL;
	errfile = NULL;
	dumpfile = NULL;
	
	return EXIT_SUCCESS;
}

/*
this are the standard results, to be used in comparing with the 
program output

Perft tests:

Initial position: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -

1                20 
2               400
3              8902 
4            197281 
5           4865609 
6         119060324 
7        3195901860
8       84998978956
9     2439530234167

mixed test: r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -

1                48
2              2039     
3             97862 
4           4085603 
5         193690690
6        8031647685


castle test:   r3k2r/3q4/2n1b3/7n/1bB5/2N2N2/1B2Q3/R3K2R w KQkq - 0 1

1                47
2              2409
3            111695
4           5664262
5         269506799
6       13523904666
  
ep test:  rnbq1bnr/1pppkp1p/4p3/2P1P3/p5p1/8/PP1PKPPP/RNBQ1BNR w - - 0 1

1                22 
2               491 
3             12571 
4            295376 
5           8296614 
6         205958173
  
promotion test:  rn1q1bnr/1bP1kp1P/1p2p3/p7/8/8/PP1pKPpP/RNBQ1BNR w - - 0 1

1                37
2              1492
3             48572
4           2010006
5          67867493
6        2847190653
  
No nothing test: r6r/3qk3/2n1b3/7n/1bB5/2N2N2/1B2QK2/R6R w - - 0 1

1                62 
2              3225
3            176531
4           8773247
5         461252378
*/
