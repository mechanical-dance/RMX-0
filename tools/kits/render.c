// render a kit through the native lxr-0 engine: notes from a script, St1 (L1/R1) to a WAV, plus levels.
// render <dylib> <kit.SND> <card> <out.wav> <seconds> "ms:voice:note:vel ..."
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#define PARAMS 340
#define S(t,n,...) t (*n)(__VA_ARGS__)
S(void,lxr_init,void);S(void,lxr_render,int16_t[4][64]);S(void,lxr_note,int,uint8_t,uint8_t);S(void,lxr_fw_load_kit,const uint8_t*,int,const uint8_t*);S(void,lxr_fw_sample_root,const char*);
#define F(n) n=dlsym(h,#n);if(!n){printf("missing %s\n",#n);return 1;}
typedef struct{long ms;int v,n,vel;}Ev;
int main(int ac,char**v){if(ac<7){puts("args");return 1;}void*h=dlopen(v[1],RTLD_NOW);if(!h){puts(dlerror());return 1;}
 F(lxr_init);F(lxr_render);F(lxr_note);F(lxr_fw_load_kit);F(lxr_fw_sample_root);
 static uint8_t a[400],k[PARAMS];FILE*fp=fopen(v[2],"rb");if(!fp){perror(v[2]);return 1;}int na=fread(a,1,400,fp);fclose(fp);memcpy(k,a+8,na-8);
 Ev ev[256];int ne=0;char*s=strdup(v[6]);for(char*t=strtok(s," ");t&&ne<256;t=strtok(0," ")){sscanf(t,"%ld:%d:%d:%d",&ev[ne].ms,&ev[ne].v,&ev[ne].n,&ev[ne].vel);ne++;}
 double secs=atof(v[5]);long nb=(long)(secs*44100/64),N=nb*64;int16_t*out=malloc(N*4);
 lxr_init();lxr_fw_sample_root(v[3]);int16_t j[4][64];for(int b=0;b<50;b++){lxr_render(j);usleep(1200);}
 lxr_fw_load_kit(a,na,k);for(int b=0;b<1700;b++){lxr_render(j);usleep(1200);}
 int e=0;long clip=0;int pk=0;double win[64]={0};int nw=(int)(secs*2);if(nw>64)nw=64;
 for(long b=0;b<nb;b++){long ms=b*64*1000/44100;while(e<ne&&ev[e].ms<=ms){lxr_note(ev[e].v,ev[e].n,ev[e].vel);e++;}
  lxr_render(j);usleep(1200);
  for(int i=0;i<64;i++){int L=j[0][i],R=j[1][i];out[(b*64+i)*2]=L;out[(b*64+i)*2+1]=R;int m=abs(L)>abs(R)?abs(L):abs(R);if(m>pk)pk=m;if(m>=32700)clip++;int w=(int)((b*64+i)/22050);if(w<nw&&m>win[w])win[w]=m;}}
 FILE*o=fopen(v[4],"wb");uint32_t ds=N*4,r=36+ds,sr=44100,br=44100*4;uint16_t f1=1,ch=2,ba=4,bits=16;uint32_t fs=16;
 fwrite("RIFF",1,4,o);fwrite(&r,4,1,o);fwrite("WAVEfmt ",1,8,o);fwrite(&fs,4,1,o);fwrite(&f1,2,1,o);fwrite(&ch,2,1,o);fwrite(&sr,4,1,o);fwrite(&br,4,1,o);fwrite(&ba,2,1,o);fwrite(&bits,2,1,o);fwrite("data",1,4,o);fwrite(&ds,4,1,o);fwrite(out,4,N,o);fclose(o);
 printf("peak %d (%.1f dBFS), clipped samples %ld\n half-second peaks:",pk,20*log10(pk/32768.0+1e-9),clip);for(int w=0;w<nw;w++)printf(" %d",(int)win[w]);puts("");fflush(stdout);_exit(0);}
