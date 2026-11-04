// 0.5.7: MIDI note-off, legato (lgt, per drum), per-drum glide, poly taking released keys first.
// Through the firmware's own MIDI input, as link_midi_test. Drums on their own jacks (L1, R1, L2).
// Run: LXR0_DIR=<state> ./legato_test <dylib> <kit.SND> <card folder>
#include <dlfcn.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#define PARAMS 340
#define LV 325
#define GLIDE2 333
#define TYPE2 335
#define LGT 337 /* legato, drums 1-3 */
#define S(t,n,...) t (*n)(__VA_ARGS__)
void *h;
S(void,lxr_init,void);S(void,lxr_render,int16_t[4][64]);S(void,lxr_fw_load_kit,const uint8_t*,int,const uint8_t*);S(void,lxr_fw_edit,const uint8_t*);S(int,lxr_fw_read_kit,uint8_t*,int);
#include "listen.h"
S(void,lxr_fw_sample_root,const char*);S(void,lxr_fw_lcd,char*,uint8_t*);S(void,lxr_fw_button,int,int);S(void,lxr_fw_encoder,int);S(void,lxr_fw_pot,int,int);S(void,lxr_fw_midi,const uint8_t*,int);
#define F(n) n = dlsym(h,#n); if(!n){printf("missing %s\n",#n);return 1;}
static int fails;
#define CAPN (44100*4)
static int16_t cap[4][CAPN];static volatile long capN;static volatile int capOn;
static void *aud(void *a){int16_t j[4][64];for(;;){lxr_render(j);if(capOn&&capN+64<=CAPN){for(int c=0;c<4;c++)memcpy(&cap[c][capN],j[c],128);capN+=64;}usleep(1200);}return 0;}
static void w(int ms){usleep(ms*1000);}
static void row(char *s){uint8_t cg[64];lxr_fw_lcd(s,cg);for(int i=0;i<32;i++)if((uint8_t)s[i]<32||(uint8_t)s[i]>126)s[i]='.';s[32]=0;}
static void lcd(const char *t){char s[33];row(s);printf(" %-26s [%.16s] [%.16s]\n",t,s,s+16);}
static void down(int b){lxr_fw_button(b,1);w(80);}
static void up(int b){lxr_fw_button(b,0);w(80);}
static void tap(int b){down(b);up(b);w(70);}
static void config(void){down(12);tap(13);up(12);w(150);}
static void check(const char *what,int ok){printf("  %s %s\n",ok?"ok  ":"FAIL",what);if(!ok)fails++;}
static int setTo(int i,const char *want){char s[33];for(int n=0;n<400;n++){row(s);if(!strncmp(s+16+4*i,want,strlen(want)))return 1;lxr_fw_pot(i,n<200?1:-1);w(40);}return 0;}
#define MS(x) ((long)((x)*44.1))
static int peak(int c,long a,long b){int m=0;if(a<0)a=0;if(b>capN)b=capN;for(long i=a;i<b;i++)if(abs(cap[c][i])>m)m=abs(cap[c][i]);return m;}
static double hz(int c,long a,long b){if(a<1)a=1;if(b>capN)b=capN;long f=-1,l=-1;int n=0;for(long i=a+1;i<b;i++)if(cap[c][i-1]<0&&cap[c][i]>=0){double x=i-1+(double)-cap[c][i-1]/(cap[c][i]-cap[c][i-1]);if(f<0)f=(long)(x*1000);l=(long)(x*1000);n++;}return n>1?(n-1)*44100.0*1000/(l-f):0;}
static int near(double a,double b,double tol){return fabs(a-b)<=tol*fabs(b);}
static uint8_t k[PARAMS];
static void edit(void){lxr_fw_edit(k);w(150);}
static void quiet(void){int d[3]={k[50],k[52],k[54]};k[50]=k[52]=k[54]=1;edit();w(150);k[50]=d[0];k[52]=d[1];k[54]=d[2];edit();}
// a note on or off; returns where it lands in the capture
static long midi(int on,int ch,int n){uint8_t m[3]={(uint8_t)((on?0x90:0x80)|(ch-1)),(uint8_t)n,on?100:0};long t=capN;lxr_fw_midi(m,3);return t;}
// a second note against the first: L1 the first's level, Lb just before the second, L2 just after; pitch after
typedef struct { int l1, lb, l2; double f; } Hit;
static Hit second(int d,long t1,long t2){Hit x={peak(d,t1+MS(10),t1+MS(25)),peak(d,t2-MS(15),t2),peak(d,t2+MS(10),t2+MS(25)),hz(d,t2+MS(30),t2+MS(130))};return x;}
static void show(const char *n,Hit x){printf("  %-36s first %6d, before %6d, after %6d, then %7.2f Hz\n",n,x.l1,x.lb,x.l2,x.f);}
static int legatoed(Hit x){return x.lb<0.985*x.l1&&x.l2<=1.005*x.lb;}
static int retrig(Hit x){return x.l2>0.995*x.l1;}
int main(int ac,char**v){setvbuf(stdout,0,_IONBF,0);if(ac<4){puts("args");return 1;}
 h=dlopen(v[1],RTLD_NOW);if(!h){puts(dlerror());return 1;}
 F(lxr_init);F(lxr_render);LISTEN_WRAP();F(lxr_fw_load_kit);F(lxr_fw_edit);F(lxr_fw_read_kit);F(lxr_fw_sample_root);F(lxr_fw_lcd);F(lxr_fw_button);F(lxr_fw_encoder);F(lxr_fw_pot);F(lxr_fw_midi);
 static uint8_t a[400];FILE*fp=fopen(v[2],"rb");if(!fp){perror(v[2]);return 1;}int na=fread(a,1,400,fp);fclose(fp);memcpy(k,a+8,na-8);
 lxr_init();lxr_fw_sample_root(v[3]);pthread_t t;pthread_create(&t,0,aud,0);w(300);lxr_fw_load_kit(a,na,k);w(4000);
 puts("== global page: MIDI in on, global channel 2, drum channels 1, 3, 4, notes any");
 config();config();check("MIDI in on",setTo(0,"on"));check("global channel 2",setTo(1,"  2"));
 config();check("drum 1 channel 1",setTo(0,"  1"));check("drum 2 channel 3",setTo(1,"  3"));check("drum 3 channel 4",setTo(2,"  4"));
 config();config();for(int i=0;i<3;i++){lxr_fw_pot(i,-300);w(300);}lcd("notes");
 down(4);tap(5);up(4);w(200);
 // a clean kit: sines, attack 0, a decay that falls audibly in 300 ms, each drum on its own mono jack
 lxr_fw_read_kit(k,0);
 static const int zero[]={197,198,199,200,201,202,234,235,236,238,239,240,241,242,301,302,303,304,305,306,307,28,29,30,31,32,33,34,35,36,74,75,76,77,82,84,86,102,103,104,105,106,107,0};
 for(int i=0;zero[i];i++)k[zero[i]]=0;
 for(int i=88;i<91;i++)k[i]=110;for(int i=91;i<94;i++)k[i]=0;for(int i=37;i<43;i++)k[i]=127;for(int i=43;i<49;i++)k[i]=0;for(int i=108;i<115;i++)k[i]=127;
 for(int i=121;i<127;i++)k[i]=0;for(int i=128;i<134;i++)k[i]=0;for(int i=143;i<149;i++)k[i]=0;for(int i=265;i<283;i++)k[i]=0;for(int i=313;i<PARAMS;i++)k[i]=0;
 for(int d=0;d<3;d++){k[1+d]=0;k[8+2*d]=40;k[9+2*d]=63;k[49+2*d]=0;k[50+2*d]=55;k[215+d]=2+d;k[94+d]=63;}
 edit();w(300);
 const double up7=pow(2,7/12.);
 Hit x;long t1,t2,t3;double f60;

 puts("== drum 1, lgt off: a note over a held key plays a new hit");
 quiet();capN=0;capOn=1;t1=midi(1,1,60);w(300);t2=midi(1,1,67);w(200);midi(0,1,67);midi(0,1,60);capOn=0;
 x=second(0,t1,t2);f60=hz(0,t1+MS(30),t1+MS(130));show("60 held, then 67",x);
 check("retriggered",retrig(x));check("at 67",near(x.f,f60*up7,0.004));
 printf("  (the decay falls to %.0f %% in 300 ms)\n",100.*x.lb/x.l1);

 puts("== drum 1, lgt on");
 k[LGT]=1;edit();
 quiet();capN=0;capOn=1;t1=midi(1,1,60);w(300);t2=midi(1,1,67);w(300);t3=midi(0,1,67);w(200);capOn=0;
 x=second(0,t1,t2);show("60 held, then 67",x);check("legato: no new hit",legatoed(x));check("at 67",near(x.f,f60*up7,0.004));
 x=second(0,t2,t3);show("67 let go, 60 still held",x);check("back to 60, no new hit",x.l2<=1.1*x.lb&&near(x.f,f60,0.004));
 midi(0,1,60);w(100);
 quiet();capN=0;capOn=1;t1=midi(1,1,60);w(300);midi(0,1,60);w(20);t2=midi(1,1,64);w(200);midi(0,1,64);capOn=0;
 x=second(0,t1,t2);show("60 let go, then 64",x);check("all keys up: a new hit",retrig(x));
 k[50]=30;edit();
 quiet();capN=0;capOn=1;t1=midi(1,1,60);w(1500);t2=midi(1,1,67);w(200);midi(0,1,67);midi(0,1,60);capOn=0;
 x=second(0,t1,t2);show("60 held past the end of its sound, then 67",x);check("the sound had ended: a new hit",x.lb<300&&x.l2>0.8*x.l1);
 k[50]=55;edit();
 quiet();capN=0;capOn=1;t1=midi(1,1,60);w(300);t2=midi(1,1,67);w(200);midi(0,1,67);midi(0,1,60);capOn=0;
 x=second(0,t1,t2);show("note off as note on at velocity 0 (again)",x);check("still legato",legatoed(x));
 {uint8_t m[3]={0x90,60,100};quiet();capN=0;capOn=1;t1=capN;lxr_fw_midi(m,3);w(300);m[1]=67;t2=capN;lxr_fw_midi(m,3);w(200);
  m[2]=0;lxr_fw_midi(m,3);m[1]=60;lxr_fw_midi(m,3);w(20);m[1]=64;m[2]=100;t3=capN;lxr_fw_midi(m,3);w(200);m[2]=0;lxr_fw_midi(m,3);capOn=0;
  x=second(0,t2,t3);show("velocity-0 offs, then 64",x);check("velocity 0 lets go: a new hit",x.l2>0.95*peak(0,t1+MS(10),t1+MS(25)));}

 puts("== drum 2 on its own: legato and glide per drum");
 quiet();midi(1,3,60);w(100);midi(0,3,60); // drum 2 has played 60, so its glide starts there
 k[LGT]=0;k[LGT+1]=1;k[GLIDE2]=64;k[TYPE2]=1;edit(); // lct, about 0.45 s a slide
 quiet();capN=0;capOn=1;t1=midi(1,3,60);w(300);t2=midi(1,3,67);w(700);midi(0,3,67);midi(0,3,60);capOn=0;
 {double fa=hz(1,t1+MS(30),t1+MS(130)),fm=hz(1,t2+MS(200),t2+MS(250)),fe=hz(1,t2+MS(550),t2+MS(650));x=second(1,t1,t2);
  printf("  drum 2: 60 %.2f Hz, mid-slide %.2f, end %.2f (want %.2f); drum 1 level %d\n",fa,fm,fe,fa*up7,peak(0,t1,capN));
  check("drum 2 legato",legatoed(x));check("drum 2 glides",fm>fa*1.05&&fm<fa*up7*0.97&&near(fe,fa*up7,0.004));check("drum 1 silent",peak(0,t1,capN)<300);}
 quiet();capN=0;capOn=1;t1=midi(1,1,60);w(300);t2=midi(1,1,67);w(200);midi(0,1,67);midi(0,1,60);capOn=0;
 x=second(0,t1,t2);show("drum 1 now (its lgt off, glide 0)",x);check("drum 1: new hit, no glide",retrig(x)&&near(x.f,f60*up7,0.004));
 k[GLIDE2]=0;k[LGT+1]=0;edit();

 puts("== drums 1+2 in unison: the lead's legato");
 k[LV]=3;k[LV+1]=1;k[LGT]=1;edit();w(200);
 quiet();capN=0;capOn=1;t1=midi(1,1,60);w(300);t2=midi(1,1,67);w(200);midi(0,1,67);midi(0,1,60);capOn=0;
 for(int d=0;d<2;d++){Hit y=second(d,t1,t2);char n[40];snprintf(n,40,"drum %d",d+1);show(n,y);check(d?"drum 2 legato with the group":"drum 1 legato",legatoed(y)&&near(y.f,hz(d,t1+MS(30),t1+MS(130))*up7,0.004));}
 k[LGT]=0;k[LGT+1]=1;edit(); // drum 2's own setting doesn't count while linked
 quiet();capN=0;capOn=1;t1=midi(1,1,60);w(300);t2=midi(1,1,67);w(200);midi(0,1,67);midi(0,1,60);capOn=0;
 x=second(1,t1,t2);show("lead's lgt off, drum 2's on",x);check("the group retriggers",retrig(x));
 k[LGT+1]=0;

 puts("== drums 1+2 in poly, alc old: a released key's drum goes first");
 k[LV+1]=3;k[LV+5]=0;k[50]=k[52]=110;edit();w(200);
 const double f64=f60*pow(2,4/12.),f67=f60*up7;
 quiet();capN=0;capOn=1;midi(1,1,60);w(60);midi(1,1,64);w(60);midi(0,1,64);w(30);t2=midi(1,1,67);w(300);capOn=0;
 {double f[2]={hz(0,t2+MS(100),t2+MS(250)),hz(1,t2+MS(100),t2+MS(250))};printf("  60, 64, 64 up, 67: d1 %.2f d2 %.2f\n",f[0],f[1]);
  int has60=near(f[0],f60,0.004)||near(f[1],f60,0.004),has67=near(f[0],f67,0.004)||near(f[1],f67,0.004);check("60 (held) kept, 64 (released) replaced by 67",has60&&has67);}
 midi(0,1,60);midi(0,1,67);w(50);
 quiet();capN=0;capOn=1;midi(1,1,60);w(60);midi(1,1,64);w(60);t2=midi(1,1,67);w(300);capOn=0;
 {double f[2]={hz(0,t2+MS(100),t2+MS(250)),hz(1,t2+MS(100),t2+MS(250))};printf("  60, 64 both held, 67: d1 %.2f d2 %.2f\n",f[0],f[1]);
  int has64=near(f[0],f64,0.004)||near(f[1],f64,0.004),has67=near(f[0],f67,0.004)||near(f[1],f67,0.004);check("both held: the oldest (60) replaced",has64&&has67);}
 midi(0,1,60);midi(0,1,64);midi(0,1,67);
 printf("%d failed\n",fails);puts("done");_exit(fails?2:0);}
