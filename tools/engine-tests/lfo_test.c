// 0.5.5: LFO shapes. Reads drum 1's LFO straight from the engine each render block (the LFO updates once a block):
// the vendor's eight against their formulas, nse smooth and continuous through retriggers, xtr, the one-shots
// (one cycle per hit, then hold; sq1 a gate; offset as a delay; still with no retrigger voice), rn1.
// Run: LXR0_DIR=<state> ./lfo_test <dylib> <kit.SND> <card folder> <voiceArray offset> <lxr_render offset> (from nm)
#include <dlfcn.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define PARAMS 332
#define FREQ 115
#define WAVE 155
#define RETRIG 173
#define OFFS 185
#define AMT 121
#define OFF_LFO 204	/* DrumVoice.lfo; Lfo.phase at 0, phaseInc at 4, value at 32 */
#define S(t,n,...) t (*n)(__VA_ARGS__)
void *h;
S(void,lxr_init,void);S(void,lxr_render,int16_t[4][64]);S(void,lxr_note,int,uint8_t,uint8_t);S(void,lxr_fw_load_kit,const uint8_t*,int,const uint8_t*);S(void,lxr_fw_edit,const uint8_t*);S(void,lxr_fw_sample_root,const char*);
#define F(n) n = dlsym(h,#n); if(!n){printf("missing %s\n",#n);return 1;}
static int fails;static uint8_t k[PARAMS];static volatile uint8_t *lfo;
static void check(const char *what,int ok){printf("  %s %s\n",ok?"ok  ":"FAIL",what);if(!ok)fails++;}
static float val(void){float v;memcpy(&v,(const void*)(lfo+32),4);return v;}
static uint32_t ph(void){uint32_t p;memcpy(&p,(const void*)lfo,4);return p;}
static uint32_t inc(void){uint32_t p;memcpy(&p,(const void*)(lfo+4),4);return p;}
static void rb(int n){int16_t j[4][64];for(int b=0;b<n;b++){lxr_render(j);usleep(1100);}}
static void edit(void){lxr_fw_edit(k);rb(40);}
static void hit(void){lxr_note(0,60,100);}
// the vendor formulas, from lfo.c
static float vendor(int w,uint32_t p){float x=p/(float)0xffffffff;switch(w){case 0:return 0.5f*(sinf(0)+1);case 1:return 1.f-fabsf(x*2-1);case 2:return x;case 3:return (0xffffffff-p)/(float)0xffffffff;case 4:return p>0x7fffffff?1.f:0.f;case 6:return x*x*x;case 7:{float y=(0xffffffff-p)/(float)0xffffffff;return y*y*y;}}return -1;}
static float xtr(float x){float y=x<0.5f?2*x:2-2*x;return y*y*y;}
// one cycle after a hit: values per block, with the phase step
static int shot(float *v,int n){hit();for(int i=0;i<n;i++){rb(1);v[i]=val();}return 0;}
int main(int ac,char**v){setvbuf(stdout,0,_IONBF,0);if(ac<6){puts("args");return 1;}
 h=dlopen(v[1],RTLD_NOW);if(!h){puts(dlerror());return 1;}
 F(lxr_init);F(lxr_render);F(lxr_note);F(lxr_fw_load_kit);F(lxr_fw_edit);F(lxr_fw_sample_root);
 lfo=(volatile uint8_t*)((char*)lxr_render-strtol(v[5],0,16)+strtol(v[4],0,16)+OFF_LFO);
 static uint8_t a[400];FILE*fp=fopen(v[2],"rb");if(!fp){perror(v[2]);return 1;}int na=fread(a,1,400,fp);fclose(fp);memcpy(k,a+8,na-8);
 lxr_init();lxr_fw_sample_root(v[3]);rb(50);lxr_fw_load_kit(a,na,k);rb(2500);
 k[AMT]=0;k[FREQ]=27;k[RETRIG]=1;k[OFFS]=0;k[WAVE]=2;edit();rb(20);
 const uint32_t I=inc();const int N=(int)(4294967296.0/I)+1;printf(" phase step %u, %d blocks a cycle (%.2f Hz)\n",I,N,689.6875*I/4294967296.0);
 puts("== the vendor's shapes follow their formulas");
 for(int w=1;w<=7;w++){if(w==5)continue;k[WAVE]=w;edit();double e=0;for(int i=0;i<600;i++){rb(1);double d=fabs(val()-vendor(w,ph()));if(d>e)e=d;}printf(" wave %d: largest error %.2g\n",w,e);check("matches lfo.c",e<1e-6);}
 // the sine reads the firmware's table at phase>>20: learn the value at each index, check it's one value per index
 static float sineAt[4096];static char seen[4096];
 {k[WAVE]=0;edit();int bad=0,n=0;for(int i=0;i<N*14;i++){rb(1);int idx=ph()>>20;float x=val();if(seen[idx]&&sineAt[idx]!=x)bad++;if(!seen[idx]){seen[idx]=1;sineAt[idx]=x;n++;}}
  printf(" sine: %d table indexes seen, %d disagree\n",n,bad);check("sine reads one table value per phase, as lfo.c",!bad&&n>1000);}
 {k[WAVE]=5;edit();float last=val();int changes=0,wraps=0;uint32_t lp=ph();for(int i=0;i<N*6;i++){rb(1);uint32_t p=ph();if(p<lp)wraps++;lp=p;if(val()!=last){changes++;last=val();}}printf(" rnd: %d changes over %d wraps\n",changes,wraps);check("rnd still steps once a cycle",changes<=wraps&&changes>=wraps-1);}
 puts("== nse: smooth random");
 k[WAVE]=8;k[RETRIG]=0;edit();rb(N);
 {float p0=val(),p1=val();double md=0,m2=0,lo=1,hi=0;static float r[20000];int n=N*30;for(int i=0;i<n;i++){rb(1);r[i]=val();}
  for(int i=2;i<n;i++){double d=fabs(r[i]-r[i-1]),d2=fabs(r[i]-2*r[i-1]+r[i-2]);if(d>md)md=d;if(d2>m2)m2=d2;if(r[i]<lo)lo=r[i];if(r[i]>hi)hi=r[i];}(void)p0;(void)p1;
  printf(" over 30 cycles: range %.3f-%.3f, largest step per block %.4f, largest change of slope %.5f\n",lo,hi,md,m2);
  check("covers the range",lo<0.15&&hi>0.85);check("no steps (largest move per block under 4 cycle-steps of a full swing)",md<4.0/N);
  check("no corners (slope changes smoothly)",m2<40.0/((double)N*N));
  int clipped=0;for(int i=0;i<n;i++)if(r[i]==0.f||r[i]==1.f)clipped++;printf(" blocks at an edge (overshoot clipped): %d of %d\n",clipped,n);}
 puts("== nse through retriggers: no jump");
 k[RETRIG]=1;edit();
 {double worst=0,normal=0;float prev=val(),pp=prev;for(int h2=0;h2<12;h2++){for(int i=0;i<N/3+h2*7;i++){rb(1);float x=val();double d=fabs(x-prev);if(d>normal)normal=d;pp=prev;prev=x;}hit();rb(1);float x=val();double d=fabs(x-prev);if(d>worst)worst=d;pp=prev;prev=x;(void)pp;}
  printf(" largest step at a hit %.4f, elsewhere %.4f\n",worst,normal);check("a hit doesn't jump",worst<=normal*1.5+1e-4);}
 puts("== xtr: exponential triangle");
 k[WAVE]=9;k[RETRIG]=0;edit();{double e=0;for(int i=0;i<600;i++){rb(1);double d=fabs(val()-xtr(ph()/(float)0xffffffff));if(d>e)e=d;}printf(" largest error %.2g\n",e);check("cubic up to the top at half, cubic down",e<1e-6);}
 puts("== one-shots: one cycle per hit, then hold");
 k[RETRIG]=1;k[OFFS]=0;
 static const char *names[]={"si1","tr1","su1","sd1","sq1","xu1","xd1","xt1"};
 for(int s=0;s<8;s++){k[WAVE]=10+s;edit();hit();rb(N*2);static float r[4000];shot(r,N*2);
  // during the cycle: block i is at phase about (i+1) steps; the hold is the shape's end
  int base[]={0,1,2,3,4,6,7,-1};double e=0;for(int i=2;i<N-3;i++){uint32_t p=(uint32_t)((i+1)*(double)I);float want=s==4?(p>0x7fffffff?0.f:1.f):s==7?xtr(p/(float)0xffffffff):s==0?sineAt[p>>20]:vendor(base[s],p);
   if(s==4&&abs((int)(i+1)-N/2)<=1)continue;if(s==0&&!seen[p>>20])continue;double d=fabs(r[i]-want);if(d>e)e=d;}
  int held=1;for(int i=N+1;i<N*2;i++)if(r[i]!=r[N+1])held=0;
  printf(" %s: largest error in the cycle %.4f, start %.3f, middle %.3f, holds %.3f\n",names[s],e,r[1],r[N/2],r[N+1]);
  check("runs the shape once",e<0.02);check("then holds",held);}
 {k[WAVE]=14;edit();static float r[4000];hit();rb(N*2);shot(r,N*2);int on=1,off=1;for(int i=1;i<N/2-2;i++)if(r[i]!=1.f)on=0;for(int i=N/2+2;i<N*2;i++)if(r[i]!=0.f)off=0;check("sq1: a gate, on for the first half, then off",on&&off);}
 puts("== one-shot offset is a delay");
 {k[WAVE]=12;k[OFFS]=64;edit();static float r[4000];hit();rb(N*2);float before=val();shot(r,N*3);int still=1;for(int i=0;i<N/2-3;i++)if(r[i]!=before)still=0;
  int start=-1;for(int i=0;i<N*3;i++)if(r[i]!=before){start=i;break;}printf(" su1, offset 64: held at %.3f, starts after %d blocks (half a cycle is %d)\n",before,start,N/2);
  check("waits about half a cycle, then runs",still&&abs(start-N/2)<=2);k[OFFS]=0;}
 puts("== one-shot with no retrigger voice holds still");
 {k[WAVE]=12;k[RETRIG]=0;edit();rb(5);float x=val();int still=1;for(int i=0;i<4;i++){hit();for(int j=0;j<N/2;j++){rb(1);if(val()!=x)still=0;}}check("su1 with retrigger off: no movement",still);k[RETRIG]=1;}
 puts("== rn1: a new random value per hit");
 {k[WAVE]=18;edit();static float r[4000];float ends[6];int smooth=1,held=1;for(int h2=0;h2<6;h2++){float from=val();shot(r,N*2);float to=r[N*2-1];ends[h2]=to;for(int i=N+1;i<N*2;i++)if(r[i]!=to)held=0;
   for(int i=1;i<N;i++)if(fabs(r[i]-r[i-1])>fabs(to-from)*2.5/N+1e-5)smooth=0;double mid=r[N/2-1];printf(" hit %d: %.3f -> %.3f, middle %.3f\n",h2+1,from,to,mid);if(fabs(mid-(from+to)/2)>fabs(to-from)*0.1+0.01)smooth=0;}
  int differ=0;for(int i=1;i<6;i++)if(ends[i]!=ends[i-1])differ++;check("moves smoothly (S-curve) to the new value",smooth);check("then holds",held);check("a different value each hit",differ==5);}
 printf("%d failed\n",fails);puts("done");_exit(fails?2:0);}
