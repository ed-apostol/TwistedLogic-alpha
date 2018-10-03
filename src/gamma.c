/********************************************/
/*Twisted Logic Chess Engine alpha          */
/*by Edsel Apostol                          */
/*Copyright 2004                            */
/*Last modified: 14 February 2005           */
/********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timeb.h>

/*********************************************/
/*MACROS                                     */
/*********************************************/
#define MAXDATA        500
#define MAXPLY 	       40
#define MAXMOVES       256
#define TRUE      1
#define FALSE     0
#define BOOL      int
#define BLACK     1
#define WHITE     0
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
#define EMT       12
#define NORMAL       1
#define CASTLE       2
#define EPCAP        4
#define PWN2         8
#define PROM         16
#define WCKS         1
#define WCQS         2
#define BCKS         4
#define BCQS         8
#define RANK(X)            (X>>4)
#define FILE(X)            (X&7)
#define MAX(A,B)            (A>B?A:B)
#define MIN(A,B)            (A<B?A:B)
#define DRAW               0
#define MATE               32500

/************************************************/
/*DATA STRUCTURES                               */
/************************************************/
typedef struct{
    int from;
    int to;
    int pc;
    int capt;
    int prom;
    int type;
    int score;
}move;
typedef struct{
    move *m;
    int mat[2];
    int kpos[2];
    int castle;
    int epsq;
    int fifty;
    int rfrom;
    int rto;
}hist;
typedef struct{
	int pc[128];
	int hply;
	int side;
	int mat[2];
	int kpos[2];
	int castle;
	int fifty;
	int epsq;
}board;

/***********************************************/
/*CONSTANTS                                    */
/***********************************************/
int castlemask[128]={
    7, 15, 15, 15,  3, 15, 15, 11, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    13, 15, 15, 15, 12, 15, 15, 14, 15, 15, 15, 15, 15, 15, 15, 15
};
int origb[128]={
    BR, BN, BB, BQ, BK, BB, BN, BR,EMT, EMT, EMT, EMT, EMT, EMT, EMT, EMT,
    BP, BP, BP, BP, BP, BP, BP, BP,EMT, EMT, EMT, EMT, EMT, EMT, EMT, EMT,
    EMT, EMT, EMT, EMT, EMT, EMT, EMT, EMT,EMT, EMT, EMT, EMT, EMT, EMT, EMT, EMT,
    EMT, EMT, EMT, EMT, EMT, EMT, EMT, EMT,EMT, EMT, EMT, EMT, EMT, EMT, EMT, EMT,
    EMT, EMT, EMT, EMT, EMT, EMT, EMT, EMT,EMT, EMT, EMT, EMT, EMT, EMT, EMT, EMT,
    EMT, EMT, EMT, EMT, EMT, EMT, EMT, EMT,EMT, EMT, EMT, EMT, EMT, EMT, EMT, EMT,
    WP, WP, WP, WP, WP, WP, WP, WP,EMT, EMT, EMT, EMT, EMT, EMT, EMT, EMT,
    WR, WN, WB, WQ, WK, WB, WN, WR,EMT, EMT, EMT, EMT, EMT, EMT, EMT, EMT
};
int pcval[13]={100, 100, 300, 300, 310, 310, 500, 500, 900,900, 0, 0, 0};
BOOL slide[12]={
    FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE,
    TRUE, TRUE, FALSE, FALSE
};
int valpcprom[13]={0,0,200,200,210,210,400,400,800,800,0,0,0};
unsigned int attmask[13]={1,2,4,4,8,8,16,16,32,32,64,64,0};

/************************************************/
/*VARIABLES                                     */
/************************************************/
move M[MAXPLY][MAXMOVES];
hist H[MAXDATA];
move pv[MAXPLY][MAXPLY];
int pvlen[MAXPLY];
int mc[MAXPLY];
board B;
int nodes;
BOOL post;
BOOL xboard;
BOOL stopsearch;
int maxtime;
int maxdepth;
int starttime;
int endtime;
char move1[6];
char *move2;
int machine;
unsigned int attb[128][128];
int history[128][128];
/**********************************************************/
/*FUNCTIONS                                               */
/**********************************************************/

char *mov2str(move *m){
    static char str[6];
    static char promstr[13]="ppnnbbrrqqkk ";
    char c=promstr[m->prom];
    sprintf(str, "%c%d%c%d%c%c",
        FILE(m->from) + 'a',
        8 - RANK(m->from),
        FILE(m->to) + 'a',
        8 - RANK(m->to),
        c,
        putchar('\0'));
    return str;
}

int *movptr(int attack, int pc){
    static int knightd[9]={-31, -33, -14, -18, 14, 18, 33, 31, 0};
	static int bishopd[5]={-15, -17, 15, 17, 0};
	static int rookd[5]={-16, -1, 16, 1, 0};
	static int queend[9]={-15, -17, 15, 17, -16, -1, 16, 1, 0};
	static int kingd[9]={-15, -17, 15, 17, -16, -1, 16, 1, 0};
	static int wpattd[3]={-15, -17, 0};
	static int bpattd[3]={15, 17, 0};
	static int wpmovd[4]={-15, -16, -17, 0};
	static int bpmovd[4]={15, 16, 17, 0};
	switch(pc){
		case WN: case BN: return knightd;
		case WB: case BB: return bishopd;
		case WR: case BR: return rookd;
		case WQ: case BQ: return queend;
		case WP: 
			switch(attack){
				case TRUE: return wpattd; 
				default: return wpmovd;
			}
		case BP: 
			switch(attack){
				case TRUE: return bpattd; 
				default: return bpmovd;
			}
		default: return kingd;
	}
}

void initattb(void){
    int i, m, j, *ptr;
    memset(attb, 0, sizeof(attb));
    for(i=0; i<128; ++i){
        if(i&0x88) continue;
        for(j=WP; j<EMT;++j){
			ptr=movptr(TRUE, j);
			for(; *ptr; ptr++){
				m=i;
				while(!((m+=*ptr)&0x88)){
					attb[i][m]|=attmask[j];
					if(!slide[j]) break;
				}
			}
        }
    }
}

BOOL isatt(int side, int sq){
    register int i, j, k, g, m, *ptr;
    for(j=0; j<128; j+=16){
		for(k=0; k<8; ++k){
        i=j+k;
	    g=B.pc[i];
	    if(g==EMT) continue;
	    if(i==sq) continue;
	    if((g&1)!=side) continue;
	    if(!(attb[i][sq]&attmask[g])) continue;
	    ptr=movptr(TRUE, g);
	    for(; *ptr; ptr++){
        m=i;
	    while(!((m+=*ptr)&0x88)){
	    	if(m==sq) return TRUE;
		    if(B.pc[m]!=EMT) break;
		    if(!slide[g]) break;
	   }
       }
	}
	}
	return FALSE;
}

void printresult(void){
    int c=isatt(!B.side, B.kpos[B.side]);
    if(c){
        if(B.side==WHITE) printf("0-1{Black mates}\n");
        else printf("1-0{White mates}\n");
    }
    else if(B.fifty>=100) printf("1/2-1/2{Draw by fifty move rule}\n");
    else printf("1/2-1/2{Draw}\n");
}

void sortmove(int start, int end, int ply){
    register int i, bs, bi;
    move x;
    if(start==end-1) return;
    bs=-MATE;
    bi=start;
    for(i=start; i<end; ++i){
        if(M[ply][i].score>bs){
            bs=M[ply][i].score;
            bi=i;
        }
    }
    x=M[ply][start];
    M[ply][start]=M[ply][bi];
    M[ply][bi]=x;
}

int getms(void){
  struct timeb timebuffer;
  ftime(&timebuffer);
  return (timebuffer.time * 1000) + timebuffer.millitm;
}

void initgame(void){
    int i;
    memset(M, 0, sizeof(M));
    memset(H, 0, sizeof(H));
   	maxtime=3600000;
	maxdepth=MAXPLY;
	machine=BLACK;
	for(i=0; i<128; ++i) B.pc[i]=origb[i];
	B.side=WHITE;
	B.hply=0;
	B.mat[WHITE]=B.mat[BLACK]=3920;
	B.kpos[WHITE]=116;
	B.kpos[BLACK]=4;
	B.castle=15;
	B.fifty=0;
	B.epsq=-1;
}

int evalpos(int ply){
    int score=0;
    int diff=0;
    if(ply>1) diff=mc[ply-2]-mc[ply-1];
    score+=(B.mat[WHITE]-B.mat[BLACK]+(B.side?-diff:diff));
    return (B.side?-score:score);
}

void push(int ply, int mc, int from, int to, int type){
    register move *m=&M[ply][mc];
	m->from=from;
	m->to=to;
	m->pc=B.pc[from];
	m->capt=B.pc[to];
	m->type=type;
	if(m->capt!=EMT) m->score=(pcval[m->capt]*10)-pcval[m->pc];
    else m->score=history[m->from][m->to];
	if(type==PROM) m->prom=WQ+B.side;
	else m->prom=EMT;
}

int genmoves(int ply, int depth){
    register int i, j, k,  m, g, mc=0;
    register int *ptr;
    for(j=0; j<128; j+=16){
	for(k=0; k<8; ++k){
		i=j+k;
        g=B.pc[i];
        if(g==EMT) continue;
        if((g&1)!=B.side) continue;
        ptr=movptr(FALSE, g);
        for(;*ptr; ptr++){
            m=i;
            while(!((m+=*ptr)&0x88)){
                if(B.pc[m]==EMT){
                    if(depth==0){
                        if(g>BP) break;
                        if(*ptr==(B.side?16:-16)) break;             
                    }
                    if(g>BP) push(ply, mc++, i, m, NORMAL);
                    else if(*ptr!=(B.side?16:-16)){
                        if(m==B.epsq) push(ply, mc++, i, m, EPCAP);
                        break;
                    }
                    else if(RANK(m)==(B.side?7:0)){
                        push(ply, mc+=4, i, m, PROM);
                        break;
                    }
                    else if(RANK(i)==(B.side?1:6)){
                        push(ply, mc++, i, m, NORMAL);
                        if(B.pc[m+(B.side?16:-16)]==EMT)
                            push(ply, mc++, i, m+(B.side?16:-16), PWN2);
                        break;
                    }
                    else{
                        push(ply, mc++, i, m, NORMAL);
                        break;
                    }
                }
                else if((B.pc[m]&1)!=B.side){
                    if(g>BP){
                        push(ply, mc++, i, m, NORMAL);
                        break;
                    }
                    else if(*ptr!=(B.side?16:-16)){
                        if(RANK(m)==(B.side?7:0)) push(ply, mc++, i, m, PROM);
                        else push(ply, mc++, i, m, NORMAL);
                        break;
                    }
                }
                else break;
                if(!slide[g]) break;
            }
        }
	}
    }
    if(depth==0) return mc;
    if(B.side==WHITE){
		if(B.castle&WCKS) push(ply, mc++, 116, 118, CASTLE);
		if(B.castle&WCQS) push(ply, mc++, 116, 114, CASTLE);
	}
    else{
		if(B.castle&BCKS) push(ply, mc++, 4, 6, CASTLE);
		if(B.castle&BCQS) push(ply, mc++, 4, 2, CASTLE);
	}
    return mc;
}

void unmake(void){
	register hist *h=&H[B.hply-=1];
    B.side=(B.side==WHITE?BLACK:WHITE);
    B.mat[WHITE]=h->mat[WHITE];
    B.mat[BLACK]=h->mat[BLACK];
    B.kpos[WHITE]=h->kpos[WHITE];
    B.kpos[BLACK]=h->kpos[BLACK];
    B.castle=h->castle;
    B.fifty=h->fifty;
    B.epsq=h->epsq;
    B.pc[h->m->from]=h->m->pc;
    B.pc[h->m->to]=h->m->capt;
    if(h->m->type==EPCAP) B.pc[h->m->to+(B.side?-16:16)]=(B.side?WP:BP);
    if(h->m->type==CASTLE){
	    B.pc[h->rfrom]=B.pc[h->rto];
	    B.pc[h->rto]=EMT;
	}
}

BOOL makemove(register move *m){
    int rto, rfrom, i;
	hist *h=&H[B.hply];
	if(m->type==CASTLE){
		for(i=MIN(m->from, m->to); i<=MAX(m->from, m->to); ++i){
			if(isatt(!B.side, i)) return FALSE;
			if((i!=m->from)&&(B.pc[i]!=EMT)) return FALSE;
		}
		if((m->from-m->to)==2){
		    if(B.pc[m->to-1]!=EMT) return FALSE;
		    rfrom=m->to-2;
		    rto=m->from+1;
		}
		else{
		    rfrom=m->to+1;
		    rto=m->from-1;
		}
		B.pc[rto]=B.pc[rfrom];
		B.pc[rfrom]=EMT;
		h->rfrom=rfrom;
		h->rto=rto;
	}
	else{
	    h->rfrom=-1;
	    h->rto=-1;
	}
	h->castle=B.castle;
    h->epsq=B.epsq;
    h->m=m;
    h->fifty=B.fifty;
    h->mat[WHITE]=B.mat[WHITE];
	h->mat[BLACK]=B.mat[BLACK];
	h->kpos[WHITE]=B.kpos[WHITE];
	h->kpos[BLACK]=B.kpos[BLACK];
	B.castle=castlemask[m->from]&castlemask[m->to];
	if(m->prom!=EMT){
	    B.pc[m->to]=m->prom;
	    B.mat[B.side]+=valpcprom[m->prom];
	}
	else B.pc[m->to]=B.pc[m->from];
	B.pc[m->from]=EMT;
	if((m->pc<WN)||(m->capt!=EMT)) B.fifty=0;
	else B.fifty+=1;
	if(m->type==EPCAP) B.mat[!B.side]-=pcval[WP];
	if(m->type==PWN2) B.epsq=m->to+(B.side?-16:16);
	else B.epsq=-1;
	if(m->capt!=EMT) B.mat[!B.side]-=pcval[m->capt];
	if(m->pc>BQ) B.kpos[B.side]=m->to;
	B.hply+=1;
	B.side=(B.side==WHITE?BLACK:WHITE);
	if(isatt(B.side, B.kpos[!B.side])){
	    unmake();
	    return FALSE;
	}
	else return TRUE;
}

int search(int alpha, int beta, int depth, int ply){
	int i, j, x, legal, c;
	nodes+=1;
	if(nodes&1024){
	    if(getms()>endtime){
		    stopsearch=TRUE;
			return alpha;
		}
	}

	pvlen[ply]=ply;
	c=isatt(!B.side, B.kpos[B.side]);
	if(c) depth+=1;
    if(depth==0){
        x=evalpos(ply);
        if(x>=beta) return beta;
        if(x>alpha) alpha=x;
    }
    legal=FALSE;
    mc[ply]=genmoves(ply, depth);
    for(i=0; i<mc[ply]; ++i){
        sortmove(i, mc[ply], ply);
	    if(!makemove(&M[ply][i])) continue;
		if(depth>0) depth-=1;
        x=-search(-beta, -alpha, depth, ply+1);
        unmake();
		legal=TRUE;
        if(x>alpha){
            if(x>=beta) return beta;
			alpha=x;
            pv[ply][ply]=M[ply][i];
            history[M[ply][i].from][M[ply][i].to]+=ply;
		    for(j=ply+1; j<pvlen[ply+1]; ++j) pv[ply][j]=pv[ply+1][j];
		    pvlen[ply]=pvlen[ply+1];
	    }
		if(stopsearch) return alpha;
    }
	if(!legal){
	    if(depth==0) return alpha;
	    if(c) return -MATE+ply;
		else return DRAW+ply;
	}
    return alpha;
}

move *think(void){
	int i, x, k, alpha, beta;
	printf("\n");
	nodes=0;
	memset(pv, 0, sizeof(pv));
	memset(pvlen, 0, sizeof(pvlen));
	memset(history, 0, sizeof(history));
	starttime=getms();
    endtime=starttime+maxtime;
	stopsearch=FALSE;
	alpha=-MATE;
	beta=MATE;
	for(i=1; i<=maxdepth; ++i){
        x=search( alpha, beta, i, 0);
        /*while(x<alpha||x>beta){
            if(x<alpha) alpha=x;
            if(x>beta) beta=x;
            x=search(alpha, beta, i, 0);
        }
        alpha=x-30;
        beta=x+30;*/
        if(post){
		    if(xboard) printf("%d %d %d %d ", i, x, (getms()-starttime)/10, nodes);
            else printf("i=%d nodes=%d scr=%d ", i, nodes, x);
            for(k=0; k<pvlen[0]; ++k) printf("%s ", mov2str(&pv[0][k]));
            printf("\n");
            fflush(stdout);
		}
		if(stopsearch) return &pv[0][0];
    }
    return &pv[0][0];
}

int main(void){
    int i, j, movcnt, legal;
	char s[256];
    char line[256];
    move *m;
    printf("Twisted Logic Chess Engine pre-alpha\n");
	printf("by Edsel Apostol\n");
	printf("Copyright 2005\n");
    printf("use xboard commands or get a GUI\n\n");
    xboard=FALSE;
    post=TRUE;
    initgame();
    initattb();
    while(B.hply<MAXDATA){
        fflush(stdout);
        if(B.side==machine){
            m=think();
            printf("move %s\n", mov2str(m));
            if(m->score<-700){
			    printf("resign\n");
			    machine=EMT;
			    continue;
			}
			if(!makemove(m)){
			    printresult();
                machine=EMT;
                continue;
            }
            continue;
        }
        if(!xboard) printf("command> ");
        if (!fgets(line, 256, stdin))    return 1;
        if(line[0]=='\n') continue;
        sscanf(line, "%s", s);
        if(!strcmp(s, "xboard")){
            xboard=TRUE;
            printf("feature playother=1 usermove=1 myname=TwistedLogic ping=1 done=1\n");
            continue;
        }
        if(!strcmp(s, "quit")) exit(0);
        if(!strcmp(s, "new")){
            initgame();
			continue;
        }
        if(!strcmp(s, "force")){
            machine=EMT;
            continue;
        }
        if(!strcmp(s, "go")){
            machine=B.side;
            continue;
        }
        if(!strcmp(s, "playother")){
            if(machine==EMT) machine=!B.side;
            else machine=(B.side==WHITE?BLACK:WHITE);
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
            maxdepth=MAXPLY;
            continue;
        }
        if(!strcmp(s, "sd")){
            sscanf(line, "sd %d", &maxdepth);
            maxtime=3600000;
            if(maxdepth>MAXPLY) maxdepth=MAXPLY;
            continue;
        }
        if(!strcmp(s, "time")){
            sscanf(line, "time %d", &maxtime);
            maxtime*=10;
            maxtime/=30;
            maxdepth=MAXPLY;
            continue;
        }
        if(!strcmp(s, "undo")){
            if(B.hply==0) continue;
            unmake();
            continue;
        }
        if(!strcmp(s, "remove")){
            if(B.hply<2) continue;
            unmake();
            unmake();
            continue;
        }
        if(!strcmp(s, "ping")){
            sscanf(line, "ping %d", &i);
            printf("pong %d\n", i);
            continue;
        }
        if(!strcmp(s, "usermove")){
            sscanf(line, "usermove %s", &move1);
            switch(move1[4]){
                case 'n': case 'b': case 'r': case 'q': break;
                default: move1[4]=' '; break;
            }
            move1[5]='\0';
            movcnt=genmoves(0, 1);
            for(i=0; i<movcnt; ++i) printf("%d %s\n", i, mov2str(&M[0][i]));
            legal=FALSE;
            for(i=0; i<movcnt; ++i){
                move2=mov2str(&M[0][i]);
                for(j=0; j<6; ++j){
                	if(move1[j]!=*(move2+j)) break;
                }
                if(j==6){
                    legal=TRUE;
                    if(!makemove(&M[0][i])){
                        printf("Illegalmove(the move can't be executed):%s\n", move1);
                        break;
                    }
                }
            }
            if(!legal) printf("Illegalmove(not found on movelist):%s\n", move1);
            continue;
        }
        printf("Error(unknown command):%s\n", s);
    }
    return 1;
}
