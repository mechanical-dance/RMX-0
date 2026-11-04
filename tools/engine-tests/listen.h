// LISTEN=<file.wav>: a copy of everything a test renders (St1 and St2 mixed to stereo), for listening.
// Include after the lxr_render declaration; call LISTEN_WRAP() right after F(lxr_render).
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
static FILE *lisF; static uint32_t lisN; static void (*lisReal)(int16_t[4][64]);
static void lisHdr(void){uint32_t ds=lisN*4,r=36+ds,sr=44100,br=44100*4,fs=16;uint16_t f1=1,ch=2,ba=4,bits=16;fseek(lisF,0,SEEK_SET);
 fwrite("RIFF",1,4,lisF);fwrite(&r,4,1,lisF);fwrite("WAVEfmt ",1,8,lisF);fwrite(&fs,4,1,lisF);fwrite(&f1,2,1,lisF);fwrite(&ch,2,1,lisF);fwrite(&sr,4,1,lisF);fwrite(&br,4,1,lisF);fwrite(&ba,2,1,lisF);fwrite(&bits,2,1,lisF);fwrite("data",1,4,lisF);fwrite(&ds,4,1,lisF);fseek(lisF,0,SEEK_END);}
static void lisRender(int16_t j[4][64]){lisReal(j);if(!lisF)return;int16_t o[128];for(int i=0;i<64;i++){int L=j[0][i]+j[2][i],R=j[1][i]+j[3][i];o[2*i]=L>32767?32767:L<-32768?-32768:L;o[2*i+1]=R>32767?32767:R<-32768?-32768:R;}
 fwrite(o,2,128,lisF);lisN+=64;if(!(lisN&0x3fff))lisHdr();}
#define LISTEN_WRAP() do{const char*lf=getenv("LISTEN");if(lf&&(lisF=fopen(lf,"wb"))){lisHdr();lisReal=lxr_render;lxr_render=lisRender;}}while(0)
static void lisClose(void){if(lisF){FILE*f=lisF;lisHdr();lisF=0;fflush(f);}}
#define _exit(x) (lisClose(),_exit(x))
