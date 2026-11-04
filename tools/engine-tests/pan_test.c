// 0.5.8: pan follows its value while a note sounds, on all six voices: the pan knob, an LFO through the mod matrix,
// and the link's width. Drums on the stereo outs (St1, and St2 for drum 2), balance measured as R/(L+R) in short windows.
// Run: LXR0_DIR=<state> ./pan_test <dylib> <kit.SND> <card folder>
#include <dlfcn.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define PARAMS 340
#define BL 64
#define MAXN (44100*8)
#define LV 325
#define S(t,n,...) t (*n)(__VA_ARGS__)
void *h;
S(void,lxr_init,void);S(void,lxr_render,int16_t[4][64]);S(void,lxr_note,int,uint8_t,uint8_t);S(void,lxr_fw_load_kit,const uint8_t*,int,const uint8_t*);S(void,lxr_fw_edit,const uint8_t*);
S(void,lxr_fw_sample_root,const char*);S(int,lxr_fw_read_kit,uint8_t*,int);
#define F(n) n = dlsym(h,#n); if(!n){printf("missing %s\n",#n);return 1;}
static int16_t cap[4][MAXN]; static long capN; static int capOn; static int fails;
// LISTEN=<file.wav>: everything rendered, St1 and St2 mixed to stereo, for listening
static int16_t *lis;static long lisN,lisMax=44100*60;
static void lisAdd(int16_t j[4][64]){if(!lis)return;for(int i=0;i<64&&lisN<lisMax;i++,lisN++){int L=j[0][i]+j[2][i],R=j[1][i]+j[3][i];lis[lisN*2]=L>32767?32767:L<-32768?-32768:L;lis[lisN*2+1]=R>32767?32767:R<-32768?-32768:R;}}
static void lisSave(void){const char*f=getenv("LISTEN");if(!f||!lis)return;FILE*o=fopen(f,"wb");uint32_t ds=lisN*4,r=36+ds,sr=44100,br=44100*4,fs=16;uint16_t f1=1,ch=2,ba=4,bits=16;
 fwrite("RIFF",1,4,o);fwrite(&r,4,1,o);fwrite("WAVEfmt ",1,8,o);fwrite(&fs,4,1,o);fwrite(&f1,2,1,o);fwrite(&ch,2,1,o);fwrite(&sr,4,1,o);fwrite(&br,4,1,o);fwrite(&ba,2,1,o);fwrite(&bits,2,1,o);fwrite("data",1,4,o);fwrite(&ds,4,1,o);fwrite(lis,4,lisN,o);fclose(o);}
static void rb(int n){int16_t j[4][64];for(int b=0;b<n;b++){lxr_render(j);lisAdd(j);if(capOn&&capN+BL<=MAXN){for(int c=0;c<4;c++)memcpy(&cap[c][capN],j[c],sizeof j[c]);capN+=BL;}usleep(1200);}}
static void start(void){capN=0;capOn=1;}
static long note(int v,int n){lxr_note(v,n,127);usleep(300);return capN+2*BL;}
static void check(const char *what,int ok){printf("  %s %s\n",ok?"ok  ":"FAIL",what);if(!ok)fails++;}
static double rms(int c,long a,long b){if(b>capN)b=capN;double s=0;for(long i=a;i<b;i++)s+=(double)cap[c][i]*cap[c][i];return b>a?sqrt(s/(b-a)):0;}
// balance of the stereo pair starting at jack c (0: St1, 2: St2), over 20 ms at t
static double bal(int c,long t){double l=rms(c,t,t+882),r=rms(c+1,t,t+882);return r/(l+r+1e-9);}
static double lvl(int c,long t){return rms(c,t,t+882)+rms(c+1,t,t+882);}
static uint8_t k[PARAMS];
static void edit(void){lxr_fw_edit(k);}
#define MS(x) ((long)((x)*44.1))
int main(int ac,char**v){setvbuf(stdout,0,_IONBF,0);if(ac<4){puts("args");return 1;}
 h=dlopen(v[1],RTLD_NOW);if(!h){puts(dlerror());return 1;}
 F(lxr_init);F(lxr_render);F(lxr_note);F(lxr_fw_load_kit);F(lxr_fw_edit);F(lxr_fw_sample_root);F(lxr_fw_read_kit);
 static uint8_t a[400];FILE*fp=fopen(v[2],"rb");if(!fp){perror(v[2]);return 1;}int na=fread(a,1,400,fp);fclose(fp);memcpy(k,a+8,na-8);
 if(getenv("LISTEN"))lis=malloc(lisMax*4);
 lxr_init();lxr_fw_sample_root(v[3]);rb(50);lxr_fw_load_kit(a,na,k);rb(2500);lisN=0;
 // a clean kit: sines, long decay, FX and reverb off, no LFO routes; drum 1 and the snare on St1, drum 2 on St2
 static const int zero[]={197,198,199,200,201,202,234,235,236,238,239,240,241,242,301,302,303,304,305,306,307,28,29,30,31,32,33,34,35,36,74,75,76,77,82,84,86,102,103,104,105,106,107,0};
 for(int i=0;zero[i];i++)k[zero[i]]=0;
 for(int i=37;i<43;i++)k[i]=127;for(int i=43;i<49;i++)k[i]=0;for(int i=108;i<115;i++)k[i]=127;for(int i=128;i<134;i++)k[i]=0;
 for(int i=247;i<301;i++)k[i]=0;for(int i=313;i<PARAMS;i++)k[i]=0;
 for(int d=0;d<3;d++){k[1+d]=0;k[8+2*d]=57;k[9+2*d]=63;k[49+2*d]=0;k[50+2*d]=112;k[88+d]=100;k[94+d]=0;}
 k[215]=0;k[216]=1;k[217]=1;k[89+1]=0;	// drum 3 silent
 k[91]=0;k[92]=0;k[93]=0;k[218]=0;k[56]=100;k[99]=0;	// the snare: silent until its test
 edit();rb(300);
 long t;double b0,b1,l0,l1;

 puts("== drum 1: the pan knob moves a ringing note");
 k[94]=0;edit();rb(60);start();t=note(0,60);rb(300);b0=bal(0,t+MS(200));l0=lvl(0,t+MS(200));
 k[94]=127;edit();rb(150);b1=bal(0,capN-MS(30));l1=lvl(0,capN-MS(30));capOn=0;
 printf("  hard left %.2f, after the knob %.2f; level %.0f -> %.0f\n",b0,b1,l0,l1);
 check("starts hard left",b0<0.03);check("moves hard right without a new hit",b1>0.97&&l1<l0*1.02);

 puts("== drum 1: an LFO through the mod matrix sweeps a ringing note");
 k[94]=63;k[155]=0;k[115]=21;k[173]=0;	// lf1: sine, about 1 Hz, free
 k[283]=5;k[247]=2+32;k[265]=50;	// slot 1: lf1 -> drum 1 pan, 50 %
 {static uint8_t fl[8+PARAMS];memcpy(fl,a,8);memcpy(fl+8,k,PARAMS);lxr_fw_load_kit(fl,8+PARAMS,k);rb(400);}	// routes take effect when a kit loads
 start();t=note(0,60);rb(1500);{double lo=1,hi=0;for(long x=t+MS(100);x<capN-MS(30);x+=MS(20)){double q=bal(0,x);if(q<lo)lo=q;if(q>hi)hi=q;}capOn=0;
  printf("  balance swings %.2f to %.2f over 2 s\n",lo,hi);check("swings end to end during one note",lo<0.1&&hi>0.9);}
 k[265]=0;{static uint8_t fl[8+PARAMS];memcpy(fl,a,8);memcpy(fl+8,k,PARAMS);lxr_fw_load_kit(fl,8+PARAMS,k);rb(400);}

 puts("== link width moves a ringing unison group");
 k[94]=63;k[95]=63;k[89]=100;k[LV]=3;k[LV+1]=1;k[LV+2]=0;k[LV+3]=0;edit();rb(100);
 start();t=note(0,60);rb(300);b0=bal(0,t+MS(200));double c0=bal(2,t+MS(200));
 k[LV+3]=127;edit();rb(150);b1=bal(0,capN-MS(30));double c1=bal(2,capN-MS(30));capOn=0;
 printf("  drum 1 %.2f -> %.2f, drum 2 %.2f -> %.2f\n",b0,b1,c0,c1);
 check("centred, then spread to pans 0 and 126",fabs(b0-0.5)<0.03&&fabs(c0-0.5)<0.03&&b1<0.03&&c1>0.9);
 k[LV]=0;k[LV+1]=0;k[LV+3]=0;k[89]=0;edit();rb(60);

 puts("== the snare too");
 k[88]=0;k[91]=110;k[99]=0;edit();rb(100);start();t=note(3,60);rb(100);b0=bal(0,t+MS(40));
 k[99]=127;edit();rb(40);b1=bal(0,capN-MS(25));capOn=0;
 printf("  snare %.2f -> %.2f\n",b0,b1);check("the snare's pan knob moves a ringing hit",b0<0.05&&b1>0.95);

 printf("%d failed\n",fails);puts("done");lisSave();_exit(fails?2:0);}
