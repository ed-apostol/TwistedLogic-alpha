/********************************************/
/*Twisted Logic Chess Engine pre-alpha      */
/*by Edsel Apostol                          */
/*Copyright 2004                            */
/*Last modified: 10 January 2005           */
/********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timeb.h>
#include <setjmp.h>

/*********************************************/
/*MACROS                                     */
/*********************************************/
typedef unsigned long u32;
#define MAXDATA        500
#define TRUE     1
#define FALSE    0
#define BOOL     int
#define BLACK    1
#define WHITE    0
#define WP        0
#define BP        1
#define WN        2
#define BN        3
#define WB        4
#define BB        5
#define WR        6
#define BR        7
#define WQ        8
#define BQ        9
#define WK        10
#define BK        11
#define EMT         12
#define NORMAL      0
#define CAPTURE     1
#define CASTLE      2
#define PROMOTE     4
#define EP          8
#define PAWNMOV     16
#define PAWNMOV2    32
#define A1    112
#define B1    113
#define C1    114
#define D1    115
#define E1    116
#define F1    117
#define G1    118
#define H1   119
#define A8    0
#define B8    1
#define C8    2
#define D8    3
#define E8    4
#define F8    5
#define G8    6
#define H8    7
#define WCKS    1
#define WCQS    2
#define BCKS    4
#define BCQS    8
#define WHITEPAWN     1
#define BLACKPAWN     2
#define KNIGHT        4
#define BISHOP        8
#define ROOK        16
#define QUEEN      32
#define KING        64
#define HASHSIZE    (0x400000)
#define HASHMASK    (0x3FFFFF)
#define RANK(X)            (X>>3)
#define FILE(X)            (X&7)
#define SQ(X, Y)            (((X)<<3)|(Y))
#define BOARD(X)            (!(X&0x88))
#define MAX(A,B)            (A>B?A:B)
#define MIN(A,B)            (A<B?A:B)
#define ABS(X)                (X<0?-X:X)
#define RD(A,B)            ABS(RANK(A)-RANK(B))
#define FD(A,B)            ABS(FILE(A)-FILE(B))
#define TAXI(A,B)            (RD(A,B)+FD(A,B))
#define TO128(X)            ((RANK(X)<<4)+(FILE(X)))
#define TO64(X)             (((X>>4)<<3)+(FILE(X)))
#define PASSEDP        0
#define PHALANX     1
#define CHAIN        2
#define RAM            3
#define DOUBLED        4
#define ISOLATED    5
#define BACKWARD    6
#define HANGING        7
#define BRDCNTRL    70
#define PSTRUCT        100
#define PLACEMENT    0
#define BLANK        1
#define FRIEND       2
#define ENEMY        3
#define THREATS        4
#define ALPHA         0
#define BETA         1
#define EXACT         2
#define DRAW         0
#define MATE        10000
#define UP(X) ((7-RANK(X))+FILE(X))

/************************************************/
/*DATA STRUCTURES                               */
/************************************************/
typedef union{
    u32 u;
    struct{
        u32 from:7;
        u32 to:7;
        u32 piece:4;
        u32 captured:4;
        u32 promoted:4;
        u32 bits:6;
    }b;
}MOVE;

typedef struct{
    MOVE move;
    signed short score;
}SMOVE;

typedef struct{
    u32 hashlock;
    signed short score;
    char depth;
    char flag;
}HASHENTRY;

typedef struct{
    int board[64];
    int fifty;
    int side;
    int ep;
    int castle;
    int hply;
    SMOVE lastmove;
    SMOVE bestmove;
    u32 hashkey;
    u32 hashlock;
    int kingpos[2];
    int piecemat[2];
    int pawnmat[2];
}POSITION;

/************************************************/
/*VARIABLES                                     */
/************************************************/
POSITION positions[MAXDATA];
HASHENTRY hashtable[HASHSIZE];
int attackboard[64][64];
u32 ephashkey[64];
u32 ephashlock[64];
u32 hashlocks[64][12];
u32 hashkeys[64][12];
jmp_buf env;
int start;
int end;
BOOL stopsearch;
SMOVE pv[40][40];
int pvlen[40];

/***********************************************/
/*CONSTANTS                                    */
/***********************************************/
int castlemask[64]={
    7, 15, 15, 15,  3, 15, 15, 11,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    13, 15, 15, 15, 12, 15, 15, 14
};
int board[64]={
    BR, BN, BB, BQ, BK, BB, BN, BR,
    BP, BP, BP, BP, BP, BP, BP, BP,
    EMT, EMT, EMT, EMT, EMT, EMT, EMT, EMT,
    EMT, EMT, EMT, EMT, EMT, EMT, EMT, EMT,
    EMT, EMT, EMT, EMT, EMT, EMT, EMT, EMT,
    EMT, EMT, EMT, EMT, EMT, EMT, EMT, EMT,
    WP, WP, WP, WP, WP, WP, WP, WP,
    WR, WN, WB, WQ, WK, WB, WN, WR
};
int centerboard[64]={
    0,1,1,1,1,1,1,0,
    1,1,2,3,3,2,1,1,
    1,2,2,3,3,2,2,1,
    1,2,3,4,4,3,2,1,
    1,2,3,4,4,3,2,1,
    1,2,2,3,3,2,2,1,
    1,1,2,2,2,2,1,1,
    0,1,1,1,1,1,1,0
};
int kingboard[64]={
    -50,-50,-50,-50,-50,-50,-50,-50,
    -50,-50,-50,-50,-50,-50,-50,-50,
    -50,-50,-50,-50,-50,-50,-50,-50,
    -50,-50,-50,-50,-50,-50,-50,-50,
    -50,-50,-50,-50,-50,-50,-50,-50,
    -50,-60,-60,-60,-60,-60,-60,-50,
    -50,-60,-60,-60,-60,-60,-60,-50,
    10,30,20,0,0,0,30,10
};
int pawnboard[64]={
    0,0,0,0,0,0,0,0,
    14,15,16,25,25,16,15,14,
    13,14,15,20,20,15,14,13,
    12,13,14,17,17,14,13,12,
    1,2,3,15,15,2,1,1,
    0,1,-2,-6,-6,-5,1,0,
    0,0,2,-5,-5,3,3,2,
    0,0,0,0,0,0,0,0
};

int piecevalues[13]={100, 100, 300, 300, 310, 310, 500, 500, 900, 900, 0, 0, 0};
int pawnpbw[8]={10,3,2,2,7,4,3,5};
int knightdelta[9]={-31, -33, -14, -18, 14, 18, 33, 31, 0};
int bishopdelta[5]={-15, -17, 15, 17, 0};
int rookdelta[5]={-16, -1, 16, 1, 0};
int queendelta[9]={-15, -17, 15, 17, -16, -1, 16, 1, 0};
int kingdelta[9]={-15, -17, 15, 17, -16, -1, 16, 1, 0};
int wpattdelta[3]={-15, -17, 0};
int bpattdelta[3]={15, 17, 0};
int wpmovdelta[3]={-16, -32, 0};
int bpmovdelta[3]={16, 32, 0};
int attmask[12]={
    WHITEPAWN, BLACKPAWN,
    KNIGHT, KNIGHT,
    BISHOP, BISHOP,
    ROOK, ROOK,
    QUEEN, QUEEN,
    KING, KING
};
BOOL slide[12]={
    FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE,
    TRUE, TRUE, FALSE, FALSE
};
int *piecemovptr[12]={
    &wpmovdelta, &bpmovdelta,
    &knightdelta, &knightdelta,
    &bishopdelta, &bishopdelta,
    &rookdelta, &rookdelta,
    &queendelta, &queendelta,
    &kingdelta, &kingdelta
};
int *pieceattptr[12]={
    &wpattdelta, &bpattdelta,
    &knightdelta, &knightdelta,
    &bishopdelta, &bishopdelta,
    &rookdelta, &rookdelta,
    &queendelta, &queendelta,
    &kingdelta, &kingdelta
};

/**********************************************************/
/*FUNCTIONS                                               */
/**********************************************************/
static u32 rx=30903, ry=30903, rz=30903, rw=30903, rcarry=0;
u32 rand32(void){
    u32 t;
    rx=rx*69069+1;
    ry^=ry<<13;
    ry^=ry>>17;
    ry^=ry<<5;
    t=(rw<<1)+rz+rcarry;
    rcarry=((rz>>2)+(rw>>2)+(rcarry>>2))>>30;
    rz=rw;
    rw=t;
    return (rx+ry+rw);
}


int gettime(void){
  struct timeb timebuffer;
  ftime(&timebuffer);
  return (timebuffer.time * 1000) + timebuffer.millitm;
}

void sortmove(SMOVE *movbuf, int start, int end){
    int i, bs, bi;
    SMOVE x;
    bs=-MATE;
    bi=start;
    for(i=start; i<end; ++i){
        if(movbuf[i].score>bs){
            bs=movbuf[i].score;
            bi=i;
        }
    }
    x=movbuf[start];
    movbuf[start]=movbuf[bi];
    movbuf[bi]=x;
}

void sethash(register POSITION *P){
    register int i, side;
    register u32 hashlock=0;
    register u32 hashkey=0;
    side=P->side;
    for(i=0; i<64; ++i){
        if(P->board[i]!=EMT){
            hashlock^=hashlocks[i][P->board[i]];
            hashkey^=hashkeys[i][P->board[i]];
        }
    }
    hashlock^=P->side;
    hashkey^=P->side;
    if(P->ep!=-1){
        hashlock^=ephashlock[P->ep];
        hashkey^=ephashkey[P->ep];
    }
    P->hashlock=hashlock;
    P->hashkey=hashkey;
}

char *mov2str(MOVE m){
    static char str[6];
    static char piecestr[]="ppnnbbrrqqkk";
    if(m.b.promoted!=EMT){
        sprintf(str, "%c%d%c%d%c%c",
        FILE(m.b.from) + 'a',
        8 - RANK(m.b.from),
        FILE(m.b.to) + 'a',
        8 - RANK(m.b.to),
        piecestr[m.b.promoted],
        putchar('\0'));
    }
    else{
        sprintf(str, "%c%d%c%d%c%c",
        FILE(m.b.from) + 'a',
        8 - RANK(m.b.from),
        FILE(m.b.to) + 'a',
        8 - RANK(m.b.to),
        putchar(' '),
        putchar('\0'));
    }
    return str;
}


void initarrays(POSITION *P){
    register int i, k, m, p, n, q;
    int *ptr;
    memset(attackboard, 0, sizeof(attackboard));
    memset(positions, 0, sizeof(positions));
    memset(hashtable, 0, sizeof(hashtable));
    memset(ephashkey, 0, sizeof(ephashkey));
    memset(ephashlock, 0, sizeof(ephashlock));
    memset(hashlocks, 0, sizeof(hashlocks));
    memset(hashkeys, 0, sizeof(hashkeys));
	P=&positions[0];
    for(i=0; i<64; ++i){

        P->board[i]=board[i];
        ephashkey[i]=rand32();
        ephashlock[i]=rand32();


        for(k=0; k<12; ++k){

            hashlocks[i][k]=rand32();
            hashkeys[i][k]=rand32();

            ptr=pieceattptr[k];
            for(; *ptr; ptr++){
                m=TO128(i);
                q=0;
                while(TRUE){
                    m+=*ptr;
                    if(m&0x88) break;
                    attackboard[i][TO64(m)]|=attmask[k];
                    if(!slide[k]) break;
                }

            }
        }
    }

    P->fifty=0;
    P->side=WHITE;
    P->ep=-1;
    P->castle=15;
    P->hply=0;
    P->lastmove.score=0;
    P->lastmove.move.u=0;
    P->bestmove.score=0;
    P->bestmove.move.u=0;
    P->kingpos[BLACK]=TO64(E8);
    P->kingpos[WHITE]=TO64(E1);
    P->piecemat[BLACK]=4020;
    P->piecemat[WHITE]=4020;
    P->pawnmat[BLACK]=800;
    P->pawnmat[WHITE]=800;
    P->hashlock=0;
    P->hashkey=0;
    sethash(P);

}

int evalpos(POSITION *P){
    register int score, i, g, j, h, m;
    int bcs=0;
    int xbcs=0;
    int hang=0;
    int xhang=0;
    int *ptr;
    static int matonloc[64];
    memset(matonloc, 0, sizeof(matonloc));
    static int xatt[64];
    static int att[64];
    memset(xatt, 0, sizeof(xatt));
    memset(att, 0, sizeof(att));
	score=0;
    for(i=0; i<64; ++i){
        g=P->board[i];
        h=g&1;
        if(g!=EMT){
            matonloc[i]=piecevalues[g];
			if(g>9) h?(xbcs+=kingboard[i]):(bcs+=kingboard[i]);
			ptr=pieceattptr[g];
            m=TO128(i);
            while(TRUE){
                m+=*ptr;
                if(m&0x88) break;
				h?(xatt[TO64(m)]):(att[TO64(m)]);
                if(!slide[g]) break;
            }
        }
    }
    for(i=0; i<64; ++i){
        if(att[i]>xatt[i]){
			bcs+=centerboard[i];
			if((xatt[i]==0)&&(hang>matonloc[i])&&((P->board[i]&1)==WHITE))
               hang=matonloc[i];
		}
        else if(att[i]<xatt[i]){
		   xbcs+=centerboard[i];
		   if((att[i]==0)&&(xhang>matonloc[i])&&((P->board[i]&1)==BLACK))
               xhang=matonloc[i];
		}
    }
    score+=bcs-xbcs;
    score+=hang-xhang;
    score+=P->piecemat[WHITE]-P->piecemat[BLACK];
	score+=P->pawnmat[WHITE]-P->pawnmat[BLACK];
	switch(P->side){
        case WHITE: return score;
        default: return -score;
    }
}

BOOL isattbypiece(POSITION *P, int from, int sq){
    if((attackboard[from][sq])&(attmask[P->board[from]])){
        int *ptr=pieceattptr[P->board[from]];
        int m;
        for(; *ptr; ptr++){
            m=TO128(from);
            while(TRUE){
                m=m+*ptr;
                if(m&0x88) break;
                if(TO64(m)==sq) return TRUE;
                if(P->board[TO64(m)]!=EMT) break;
                if(!slide[P->board[from]]) break;
            }
        }
    }
    return FALSE;
}


BOOL isattbyside(POSITION *P, int side, int sq){
    register int i, g;
    for(i=0; i<64; ++i){
        g=P->board[i];
        if((g!=EMT)&&((g&1)==side)&&(i!=sq))
            if(isattbypiece(P, i, sq)) return TRUE;
    }
    return FALSE;
}

void pushmove(POSITION *P, int from, int to, SMOVE *movbuf, int *movcnt, int bits){
    SMOVE m;
    int i;
    m.move.b.from=from;
    m.move.b.to=to;
    m.move.b.piece=P->board[from];
    if(bits&EP) m.move.b.captured=P->board[to+(P->side?8:-8)];
    else m.move.b.captured=P->board[to];
    m.move.b.bits=bits;
    if(bits&PROMOTE){
        for(i=WN; i<WK; i+=2){
            if(P->side==BLACK) m.move.b.promoted=i+BLACK;
            else m.move.b.promoted=i;
            if(isattbyside(P, P->side^1, to))
                m.score=piecevalues[P->board[to]]+piecevalues[P->board[i]]-piecevalues[P->board[from]]
               +centerboard[to];
            else  m.score=piecevalues[P->board[to]]+piecevalues[P->board[i]]
               +centerboard[to];

            movbuf[*movcnt]=m;
            *movcnt+=1;
        }
    }
    else{
        m.move.b.promoted=EMT;
        if(isattbyside(P, P->side^1, m.move.b.to))
            m.score=piecevalues[P->board[to]]-piecevalues[P->board[from]]
            +centerboard[to];
        else  m.score=piecevalues[P->board[to]]
              +centerboard[to];;

        movbuf[*movcnt]=m;
        *movcnt+=1;
    }
    return;
}

int genmoves(POSITION *P, SMOVE *movbuf){
    register int i, m, g, h;
    int movcnt=0;
    h=P->side;
    int *ptr;
    for(i=0; i<64; ++i){
        g=P->board[i];
        if((g!=EMT)&&((g&1)==h)){
            if((g!=WP)&&(g!=BP)){
                ptr=piecemovptr[g];
                for(; *ptr; ptr++){
                    m=TO128(i);
                    while(TRUE){
                        m=m+*ptr;
                        if(m&0x88) break;
                        if(P->board[TO64(m)]==EMT){
                            pushmove(P, i, TO64(m), movbuf, &movcnt, NORMAL);
                        }
                        if(P->board[TO64(m)]!=EMT){
                            if((P->board[TO64(m)]&1)!=P->side){
                                pushmove(P, i, TO64(m), movbuf, &movcnt, CAPTURE);
                                break;
                            }
                            else break;
                        }
                        if(!slide[g]) break;
                    }
                }
            }
            else if((g==WP)||(g==BP)){
                if(h==WHITE){
                       if(P->board[i-8]==EMT){
                    if(RANK((i-8))==0)
                        pushmove(P, i, i-8, movbuf, &movcnt, PAWNMOV|PROMOTE);
                              else pushmove(P, i, i-8, movbuf, &movcnt, PAWNMOV);
                              if((P->board[i-16]==EMT)&&(RANK(i)==6))
                                       pushmove(P, i, i-16, movbuf, &movcnt, PAWNMOV|PAWNMOV2);
                       }
                       if((P->board[i-7]!=EMT)&&((P->board[i-7]&1)!=P->side)&&(FILE(i)!=7)){
                                if(RANK((i-7))==0)
                                    pushmove(P, i, i-7, movbuf, &movcnt, PAWNMOV|CAPTURE|PROMOTE);
                           else pushmove(P, i, i-7, movbuf, &movcnt, PAWNMOV|CAPTURE);
                         }
                         if((P->board[i-9]!=EMT)&&((P->board[i-9]&1)!=P->side)&&(FILE(i)!=0)){
                               if(RANK((i-9))==0)
                                       pushmove(P, i, i-9, movbuf, &movcnt, PAWNMOV|CAPTURE|PROMOTE);
                               else pushmove(P, i, i-9, movbuf, &movcnt, PAWNMOV|CAPTURE);
                         }
                     }
                  else if(h==BLACK){
                    if(P->board[i+8]==EMT){
                         if(RANK((i+8))==7)
                            pushmove(P, i, i+8, movbuf, &movcnt, PAWNMOV|PROMOTE);
                        else pushmove(P, i, i+8, movbuf, &movcnt, PAWNMOV);
                        if((P->board[i+16]==EMT)&&(RANK(i)==1))
                            pushmove(P, i, i+16, movbuf, &movcnt, PAWNMOV|PAWNMOV2);
                    }
                    if((P->board[i+7]!=EMT)&&((P->board[i+7]&1)!=P->side)&&(FILE(i)!=0)){
                        if(RANK((i+7))==7)
                            pushmove(P, i, i+7, movbuf, &movcnt, PAWNMOV|CAPTURE|PROMOTE);
                        else pushmove(P, i, i+7, movbuf, &movcnt, PAWNMOV|CAPTURE);
                    }
                    if((P->board[i+9]!=EMT)&&((P->board[i+9]&1)!=P->side)&&(FILE(i)!=7)){
                        if(RANK((i+9))==7)
                            pushmove(P, i, i+9, movbuf, &movcnt, PAWNMOV|CAPTURE|PROMOTE);
                        else pushmove(P, i, i+9, movbuf, &movcnt, PAWNMOV|CAPTURE);
                    }
                }
            }
        }
    }
    if(P->ep!=-1){
            if(h==BLACK){
                if((FILE(P->ep)!=0)&&(P->board[i-9]==BP))
                    pushmove(P, i, i+9, movbuf, &movcnt, PAWNMOV|EP);
                if((FILE(P->ep)!=7)&&(P->board[i-7]==BP))
                    pushmove(P, i, i+7, movbuf, &movcnt, PAWNMOV|EP);
            }
            else if(h==WHITE){
                if((FILE(P->ep)!=0)&&(P->board[i+9]==WP))
                    pushmove(P, i, i-9, movbuf, &movcnt, PAWNMOV|EP);
                if((FILE(P->ep)!=7)&&(P->board[i+7]==WP))
                    pushmove(P, i, i-7, movbuf, &movcnt, PAWNMOV|EP);
            }
        }
        if(P->side==WHITE){
            if(P->castle&WCKS)
                pushmove(P, TO64(E1), TO64(G1), movbuf, &movcnt, CASTLE);
            if(P->castle&WCQS)
                pushmove(P, TO64(E1), TO64(C1), movbuf, &movcnt, CASTLE);
        }
        else if(h==BLACK){
            if(P->castle&BCKS)
                pushmove(P, TO64(E8), TO64(G8), movbuf, &movcnt, CASTLE);
            if(P->castle&BCQS)
                pushmove(P, TO64(E8), TO64(C8), movbuf, &movcnt, CASTLE);
        }
        return movcnt;
}


BOOL makemove(POSITION *P, POSITION *N, SMOVE m){
    register int i;
    N=&positions[P->hply+1];
    /*memcpy(N, P, sizeof(*N));*/
    N->fifty=P->fifty;
    N->castle=P->castle;
    N->piecemat[BLACK]=P->piecemat[BLACK];
    N->piecemat[WHITE]=P->piecemat[WHITE];
    N->pawnmat[BLACK]=P->pawnmat[BLACK];
    N->pawnmat[WHITE]=P->pawnmat[WHITE];
    for(i=0; i<64; ++i) N->board[i]=P->board[i];
	if(m.move.b.bits&CASTLE){
        int to, from;
        if(isattbyside(P, P->side^1, P->kingpos[P->side])) return FALSE;
        switch(m.move.b.to){
            case TO64(G1):
                if((P->board[TO64(F1)]!=EMT)||(P->board[TO64(G1)]!=EMT)||
                isattbyside(P, P->side^1, TO64(F1))||isattbyside(P, P->side^1, TO64(G1)))
                    return FALSE;
                from=TO64(H1);
                to=TO64(F1);
                break;
            case TO64(C1):
                if((P->board[TO64(D1)]!=EMT)||(P->board[TO64(C1)]!=EMT)||(P->board[TO64(B1)]!=EMT)||
                isattbyside(P, P->side^1, TO64(D1))||isattbyside(P, P->side^1, TO64(C1)))
                    return FALSE;
                from=TO64(A1);
                to=TO64(D1);
                break;
            case TO64(G8):
                if((P->board[TO64(F8)]!=EMT)||(P->board[TO64(G8)]!=EMT)||
                isattbyside(P, P->side^1, TO64(F8))||isattbyside(P, P->side^1, TO64(G8)))
                    return FALSE;
                from=TO64(H8);
                to=TO64(F8);
                break;
            case TO64(C8):
                if((P->board[TO64(D8)]!=EMT)||(P->board[TO64(C8)]!=EMT)||(P->board[TO64(B8)]!=EMT)||
                isattbyside(P, P->side^1, TO64(D8))||isattbyside(P, P->side^1, TO64(C8)))
                    return FALSE;
                from=TO64(A8);
                to=TO64(D8);
                break;
            default: break;
        }
        N->board[to]=N->board[from];
        N->board[from]=EMT;
    }

    N->castle&=castlemask[m.move.b.from]&castlemask[m.move.b.to];
    if(m.move.b.bits&PAWNMOV2){
        if(P->side==WHITE) N->ep=m.move.b.to+8;
        else N->ep=m.move.b.to-8;
    }
    else N->ep=-1;

    if(m.move.b.bits&(PAWNMOV|CAPTURE)) N->fifty=0;
    else N->fifty+=1;

    if(m.move.b.bits&PROMOTE) N->board[m.move.b.to]=m.move.b.promoted;
    else N->board[m.move.b.to]=N->board[m.move.b.from];
    N->board[m.move.b.from]=EMT;

    if(m.move.b.bits&EP){
        if(P->side==WHITE) N->board[m.move.b.to+8]=EMT;
        else N->board[m.move.b.to-8]=EMT;
    }

    if(P->board[m.move.b.from]==(P->side?BK:WK)){
        N->kingpos[P->side]=m.move.b.to;
        N->kingpos[P->side^1]=P->kingpos[P->side^1];
    }
    else{
        N->kingpos[BLACK]=P->kingpos[BLACK];
        N->kingpos[WHITE]=P->kingpos[WHITE];
    }

    if(isattbyside(N, P->side^1, N->kingpos[P->side])){return FALSE;}

    N->side=P->side^1;
    N->hply=P->hply+1;
    if(m.move.b.bits&CAPTURE){
        if(m.move.b.captured!=(P->side?BP:WP))
            N->piecemat[N->side]-=piecevalues[m.move.b.captured];
        else
            N->pawnmat[N->side]-=piecevalues[m.move.b.captured];
    }
    N->lastmove=m;
    sethash(N);
	return TRUE;
}

BOOL probehash(POSITION *P, int *alpha, int *beta, int *score, int depth){
    HASHENTRY *ptr=&hashtable[P->hashkey&HASHMASK];
    if(ptr->hashlock==P->hashlock){
        if(ptr->depth>=depth){
            if(ptr->flag==EXACT){ *score=ptr->score; return TRUE;}
            if(ptr->flag==ALPHA){
                if(ptr->score<=*alpha){*score=*alpha; return TRUE;}
            }
            if(ptr->flag==BETA){
                if(ptr->score>=*beta){ *score=*beta; return TRUE;}
            }
        }
    }
    return FALSE;
}

void storehash(POSITION *P, int score, int type, int depth){
    HASHENTRY *ptr=&hashtable[P->hashkey&HASHMASK];
    ptr->hashlock=P->hashlock;
    ptr->score=score;
    ptr->depth=depth;
    ptr->flag=type;
}

int isdraw(POSITION *P){
    int i; int r=0;
    if(P->hply>=400) return TRUE;
    for(i=(P->hply-P-> fifty); i<P->hply; ++i)
    if((positions[i].hashkey==P->hashkey)&&(positions[i].hashlock==P->hashlock))
        ++r;
    return r;
}

void printresult(POSITION *P){
    if(isattbyside(P, P->side^1, P->kingpos[P->side])){
            if(P->side==WHITE) printf("0-1{Black mates}\n");
            else printf("1-0{White mates}\n");
    }
    else if(isdraw(P)==3) printf("1/2-1/2{Draw by repetition}\n");
    else if(P->fifty>=100) printf("1/2-1/2{Draw by ffty move rule}\n");
    else printf("1/2-1/2{Stalemate}\n");
}

int search(register POSITION *P, int alpha, int beta, u32 *nodes, int depth, int ply){
    int c, i, movcnt, x, y, j;
    SMOVE movbuf[140];
    POSITION *N=&positions[P->hply+1];
    BOOL legalmove=FALSE;
    pvlen[ply]=ply;
    int hashtype=ALPHA;
    int bestscore=-MATE;
    if(*nodes&1024){
        if(gettime()>end){
            stopsearch=TRUE;
            longjmp(env, 0);
        }
    }
    if(isdraw(P))
        return (DRAW-depth);
    c=isattbyside(P, P->side^1, P->kingpos[P->side]);
    if(c) ++depth;
    *nodes+=1;
    if(probehash(P, &alpha, &beta, &x, depth)) return x;
    if(depth==0){
        x= evalpos(P);
        storehash(P, x, EXACT, depth);
        if(x>beta) return beta;
        if(x>alpha) alpha=x;
		return alpha;
    }
    movcnt=genmoves(P, movbuf);
    for(i=0; i<movcnt; ++i){
        sortmove(movbuf, i, movcnt);
        if(!makemove(P, N, movbuf[i])) continue;
        N->lastmove=movbuf[i];
        legalmove=TRUE;
        /*if(depth<1){
            if(movbuf[i].move.b.captured==EMT)
                continue;
        }*/
		if(depth>0) depth-=1;
        x=-search(N, -beta, -alpha, nodes, depth, ply+1);
        if(x>bestscore){
            P->bestmove=movbuf[i];
            bestscore=x;
        }
        if(x>alpha){
            if(x>beta){
               storehash(P, x, BETA, depth);
                return beta;
            }
			hashtype=EXACT;
            alpha=x;
            pv[ply][ply]=movbuf[i];
		    for(j=ply+1; j<pvlen[ply+1]; ++j) pv[ply][j]=pv[ply+1][j];
		    pvlen[ply]=pvlen[ply+1];
        }
    }
    if(!legalmove){
		if(depth==0) return alpha;
		if(c) return -MATE+depth;
        else return DRAW+depth;
    }
    storehash(P, alpha, hashtype, depth);
    return alpha;
}

BOOL rootsearch(register POSITION *P, int maxtime, int maxdepth, BOOL post, int xboard){
    POSITION *N=&positions[P->hply+1];
    SMOVE movbuf[140];
    int i, j, k, x=0;
    int ply=0;
    int movcnt=genmoves(P, movbuf);
    start=gettime();
    end=start+maxtime;
    u32 nodes=0;
    int alpha=-MATE;
    int beta=MATE;
    stopsearch=FALSE;
    memset(pv, 0, sizeof(pv));
	memset(pvlen, 0, sizeof(pvlen));
	pvlen[ply]=ply;
    setjmp(env);
    if(stopsearch) return TRUE;
    BOOL legalmove=FALSE;
    int bestscore=-MATE;
    for(i=1; i<=maxdepth; ++i){
        alpha=-MATE;
        beta=MATE;
        for(j=0; j<movcnt; ++j){
            sortmove(movbuf, j, movcnt);
            if(!makemove(P, N, movbuf[j])) continue;
            isattbyside(N, N->side^1, N->kingpos[N->side]);
            legalmove=TRUE;
            x=-search(N, -beta, -alpha, &nodes, i, ply+1);
            movbuf[j].score=x;
            if(x>bestscore){
                P->bestmove=movbuf[j];
                bestscore=x;

            }
			if(x>alpha){
                alpha=x;
                pv[ply][ply]=movbuf[j];
		        for(j=ply+1; j<pvlen[ply+1]; ++j) pv[ply][j]=pv[ply+1][j];
		        pvlen[ply]=pvlen[ply+1];
		    }
		}
        if(!legalmove) break;
        if(post){
            if(xboard) printf("%d %d %d %d ", i, bestscore, (gettime()-start)/10, nodes);
            else printf("%3d %9d %5d ", i, nodes, bestscore);
            for(k=0; k<pvlen[0]; ++k) printf("%s", mov2str(pv[0][k].move));
            printf("\n");
            fflush(stdout);
        }
    }
    if(!legalmove){ printresult(P); return FALSE;}
    return TRUE;
}

int main(void){
    POSITION *rootpos=&positions[0];
    POSITION *newpos=&positions[1];
    int xboard=FALSE;
    BOOL bench=FALSE;
    BOOL legalmove;
    char s[256];
    char line[256];
    char move[6];
    char *move2;
    int from, to, i, j;
    int machine=BLACK;
    int maxdepth=32;
    int maxtime=120000;
    SMOVE movbuf[140];
    int movcnt=0;
    int post=TRUE;
    SMOVE bestmove;
    xboard=FALSE;
    printf("Twisted Logic Chess Engine pre-alpha\nby Edsel Apostol\n");
    printf("Copyright 2004\n");
    initarrays(rootpos);
    while(rootpos->hply<MAXDATA){
        fflush(stdout);
        if(bench) machine=rootpos->side;
        if(rootpos->side==machine){
            if(!rootsearch(rootpos, maxtime, maxdepth, post, xboard)){
                machine=EMT;
                continue;
            }
            printf("move %s\n", mov2str(rootpos->bestmove.move));
            newpos=&positions[rootpos->hply+1];
            makemove(rootpos, newpos, rootpos->bestmove);
            rootpos=newpos;
            movcnt=genmoves(rootpos, movbuf);
            continue;
        }
        if(!xboard) printf("command> ");
        if (!fgets(line, 256, stdin))    return 1;
        if(line[0]=='\n') continue;
        sscanf(line, "%s", s);
        if(!strcmp(s, "xboard")){
            xboard=TRUE;
            printf("feature playother=1 usermove=1 ping=1 done=1\n");
            continue;
        }
        if(!strcmp(s, "bench")){ bench=TRUE; maxdepth=12; maxtime=8000; continue;}
        if(!strcmp(s, "quit")) exit(0);
        if(!strcmp(s, "new")){
            rootpos=&positions[0];
            movcnt=genmoves(rootpos, movbuf);
            machine=BLACK;
            continue;
        }
        if(!strcmp(s, "force")){
            machine=EMT;
            continue;
        }
        if(!strcmp(s, "go")){
            machine=rootpos->side;
            continue;
        }
        if(!strcmp(s, "playother")){
            machine=WHITE?BLACK:WHITE;
            continue;
        }
        if(!strcmp(s, "post")){
            post=TRUE;
            continue;
        }
        if(!strcmp(s, "nopost")){
            post=FALSE;
            continue;
        }
        if(!strcmp(s, "st")){
            sscanf(line, "st %d", &maxtime);
            maxtime*=1000;
            maxdepth=32;
            continue;
        }
        if(!strcmp(s, "sd")){
            sscanf(line, "sd %d", &maxdepth);
            maxtime=120000;
            continue;
        }
        if(!strcmp(s, "time")){
            sscanf(line, "time %d", &maxtime);
            maxtime*=10;
            maxtime/=30;
            maxdepth=32;
            continue;
        }
        if(!strcmp(s, "undo")){
            if(rootpos->hply==0) continue;
            rootpos=&positions[rootpos->hply-1];
            movcnt=genmoves(rootpos, movbuf);
            continue;
        }
        if(!strcmp(s, "remove")){
            if(rootpos->hply<2) continue;
            rootpos=&positions[rootpos->hply-2];
            movcnt=genmoves(rootpos, movbuf);
            continue;
        }
        if(!strcmp(s, "ping")){
            sscanf(line, "ping %d", &i);
            printf("pong %d\n", i);
            continue;
        }
        if(!strcmp(s, "usermove")){
        	sscanf(line, "usermove %s", &move);
            legalmove=FALSE;
            switch(move[4]){
                case 'n': case 'b': case 'r': case 'q': break;
                default: move[4]=' '; break;
            }
            move[5]='\0';
            for(i=0; i<movcnt; ++i){
                move2=mov2str(movbuf[i].move);
                for(j=0; j<6; ++j){
                	if(move[j]!=*(move2+j)) break;
                }
                if(j==6){
                   	newpos=&positions[rootpos->hply+1];
                    if(!makemove(rootpos, newpos, movbuf[i])){
                        break;
                    }
                    else{
                        legalmove=TRUE;
                        rootpos=newpos;
                        break;
                    }
                }

            }
            if(!legalmove) printf("Illegalmove(not found on movelist):%s\n", move);
            continue;
        }
        printf("Error(unknown command):%s\n", s);
    }
    return 1;
}














