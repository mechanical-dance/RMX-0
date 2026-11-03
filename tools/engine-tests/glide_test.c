// 0.5.4/0.5.6: glide. Types lcr (T per octave), lct (T per slide), exp (RC lag, T to 1%); always glides, through
// silence and repeated hits; the group in unison and chord, drum 1 with no link, not in poly. Sines on their own jacks, pitch measured in short windows.
// Run: LXR0_DIR=<state> ./glide_test <dylib> <kit.SND> <card folder>
#include <dlfcn.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define PARAMS 333
#define GL 331 /* glide */
#define GT 332 /* glide type: 0 lcr, 1 lct, 2 exp */
#define BL 64
#define MAXN (44100*6)
#define LV 325 /* link bytes: voices, mode, detune, width, sync, alloc */
#define S(t,n,...) t (*n)(__VA_ARGS__)
void *h;
typedef struct { uint8_t on; uint8_t dst[18], amt[18], src[18]; float cv[5]; uint8_t acc[6]; } LxrMod;
S(void,lxr_init,void);S(void,lxr_render,int16_t[4][64]);S(void,lxr_note,int,uint8_t,uint8_t);S(void,lxr_fw_load_kit,const uint8_t*,int,const uint8_t*);S(void,lxr_fw_edit,const uint8_t*);
S(void,lxr_fw_sample_root,const char*);S(void,lxr_fw_cv,int,float);S(void,lxr_set_mod,const LxrMod*);S(int,lxr_fw_read_kit,uint8_t*,int);
#define F(n) n = dlsym(h,#n); if(!n){printf("missing %s\n",#n);return 1;}
static int16_t cap[4][MAXN]; static long capN; static int capOn; static int16_t prevB[4][64]; static int fails;
static void rb(int n){int16_t j[4][64];for(int b=0;b<n;b++){lxr_render(j);int nz=0;for(int c=0;c<4&&!nz;c++)for(int i=1;i<64;i++)if(j[c][i]!=j[c][0]){nz=1;break;}int rep=nz&&!memcmp(j,prevB,sizeof j);memcpy(prevB,j,sizeof j);
  if(!rep&&capOn&&capN+BL<=MAXN){for(int c=0;c<4;c++)memcpy(&cap[c][capN],j[c],sizeof j[c]);capN+=BL;}usleep(1200);}}
static void start(void){capN=0;capOn=1;}
static long note(int v,int n){lxr_note(v,n,127);usleep(300);return capN+2*BL;}
static int peak(int c,long a,long b){int m=0;if(b>capN)b=capN;for(long i=a;i<b;i++)if(abs(cap[c][i])>m)m=abs(cap[c][i]);return m;}
static double hz(int c,long a,long b){if(b>capN)b=capN;long f=-1,l=-1;int n=0;for(long i=a+1;i<b;i++)if(cap[c][i-1]<0&&cap[c][i]>=0){double x=i-1+(double)-cap[c][i-1]/(cap[c][i]-cap[c][i-1]);if(f<0)f=(long)(x*1000);l=(long)(x*1000);n++;}return n>1?(n-1)*44100.0*1000/(l-f):0;}
static void check(const char *what,int ok){printf("  %s %s\n",ok?"ok  ":"FAIL",what);if(!ok)fails++;}
static int maxd(int c,long a,long b){int m=0;if(a<1)a=1;if(b>capN)b=capN;for(long i=a;i<b;i++){int d=abs(cap[c][i]-cap[c][i-1]);if(d>m)m=d;}return m;}
static int near(double a,double b,double tol){return fabs(a-b)<=tol*fabs(b);}
static int which(double *f,int *p,double hz){for(int d=0;d<3;d++)if(p[d]>200&&near(f[d],hz,0.004))return d;return -1;}
static uint8_t k[PARAMS];
static void edit(void){lxr_fw_edit(k);rb(60);}
static const int JACK[3]={0,1,2}; /* drum 1-3 jack, found at the start */
static int jack[3];
// level and pitch of each drum, 50-250 ms after t
static void meas(long t,double *f,int *p){for(int d=0;d<3;d++){f[d]=hz(jack[d],t+2205,t+11025);p[d]=peak(jack[d],t+2205,t+11025);}}
static void show(const char *n,double *f,int *p){printf(" %-34s",n);for(int d=0;d<3;d++)printf(" d%d %7.2f Hz %6d |",d+1,f[d],p[d]);puts("");}
static void silence(void){k[50]=k[52]=k[54]=1;edit();rb(150);k[50]=k[52]=k[54]=110;edit();}
static void setlink(int mask,int mode,int det,int wid,int alloc){k[LV]=mask;k[LV+1]=mode;k[LV+2]=det;k[LV+3]=wid;k[LV+4]=0;k[LV+5]=alloc;edit();}
int main(int ac,char**v){setvbuf(stdout,0,_IONBF,0);if(ac<4){puts("args");return 1;}
 h=dlopen(v[1],RTLD_NOW);if(!h){puts(dlerror());return 1;}
 F(lxr_init);F(lxr_render);F(lxr_note);F(lxr_fw_load_kit);F(lxr_fw_edit);F(lxr_fw_sample_root);F(lxr_fw_cv);F(lxr_set_mod);F(lxr_fw_read_kit);
 static uint8_t a[400];FILE*fp=fopen(v[2],"rb");if(!fp){perror(v[2]);return 1;}int na=fread(a,1,400,fp);fclose(fp);memcpy(k,a+8,na-8);
 lxr_init();lxr_fw_sample_root(v[3]);rb(50);lxr_fw_load_kit(a,na,k);rb(2500);
 // a clean kit: no LFOs, filters open, drive off, sines, attack 0, long decay, centred
 static const int zero[]={197,198,199,200,201,202,234,235,236,238,239,240,241,242,301,302,303,304,305,306,307,28,29,30,31,32,33,34,35,36,74,75,76,77,82,84,86,102,103,104,105,106,107,0};
 for(int i=0;zero[i];i++)k[zero[i]]=0;
 for(int i=88;i<94;i++)k[i]=110;for(int i=37;i<43;i++)k[i]=127;for(int i=43;i<49;i++)k[i]=0;for(int i=108;i<115;i++)k[i]=127;
 for(int i=121;i<127;i++)k[i]=0;for(int i=128;i<134;i++)k[i]=0;for(int i=143;i<149;i++)k[i]=0;for(int i=265;i<283;i++)k[i]=0;for(int i=313;i<332;i++)k[i]=0;
 static const int pan[]={94,95,96,99,100,101};for(int i=0;i<6;i++)k[pan[i]]=63;
 static const int WAVE[]={1,2,3},COARSE[]={8,10,12},ATK[]={49,51,53},DEC[]={50,52,54},FINE[]={9,11,13};
 for(int d=0;d<3;d++){k[WAVE[d]]=0;k[COARSE[d]]=64;k[FINE[d]]=63;k[ATK[d]]=0;k[DEC[d]]=110;k[215+d]=2+d;}
 for(int i=3;i<6;i++)k[88+i]=0;
 edit();rb(300);
 long t;
 puts("== each drum alone");
 double f0[3];
 for(int d=0;d<3;d++){silence();start();t=note(d,60);rb(260);int best=0,bv=0;for(int c=0;c<4;c++){int x=peak(c,t,capN);if(x>bv){bv=x;best=c;}}jack[d]=best;f0[d]=hz(best,t+2205,t+11025);printf(" drum %d: jack %d, %.2f Hz\n",d+1,best,f0[d]);capOn=0;}
 check("each drum on its own jack",jack[0]!=jack[1]&&jack[1]!=jack[2]&&jack[0]!=jack[2]);
 // pitch around sample s, over +-10 ms
 #define AT(c,s) hz(c,(s)-441,(s)+441)
 #define MS(x) ((long)((x)*44.1))
 const double tOct=0.025*pow(320.,(64-1)/126.); printf(" glide 64: T %.3f s\n",tOct);
 const double R=f0[0];
 puts("== glide 0: no slide (unchanged)");
 silence();start();note(0,60);rb(200);t=note(0,72);rb(100);
 printf("  20 ms after: %.2f Hz (target %.2f)\n",AT(jack[0],t+MS(20)),2*R);check("on pitch at once",near(AT(jack[0],t+MS(20)),2*R,0.01));capOn=0;
 puts("== unlinked, glide 64, lcr: drum 1 slides an octave");
 k[GL]=64;k[GT]=0;edit();silence();start();note(0,60);rb(450);t=note(0,72);rb((int)(tOct*1000/1.45)+200);
 double h=AT(jack[0],t+MS(tOct*500)),e=AT(jack[0],t+MS(tOct*1000+60)),b=AT(jack[0],t+MS(tOct*250));
 printf("  quarter %.2f (want %.2f), half %.2f (want %.2f), end %.2f (want %.2f)\n",b,R*pow(2,.25),h,R*sqrt(2),e,2*R);
 check("a quarter of the way",near(b,R*pow(2,.25),0.03));check("half way at half the time: even in pitch",near(h,R*sqrt(2),0.03));check("lands on the octave",near(e,2*R,0.01));capOn=0;
 puts("== constant rate: a semitone is 12 times faster");
 silence();start();note(0,60);rb(450);t=note(0,61);rb(200);
 double s1=AT(jack[0],t+MS(tOct*1000/24)),s2=AT(jack[0],t+MS(tOct*1000/12+40));
 printf("  half way %.2f (want %.2f), after %.0f ms %.2f (want %.2f)\n",s1,R*pow(2,1/24.),tOct*1000/12+40,s2,R*pow(2,1/12.));
 check("half a semitone at 1/24 of the octave time",near(s1,R*pow(2,1/24.),0.012));check("there by 1/12 of the octave time",near(s2,R*pow(2,1/12.),0.006));capOn=0;
 puts("== after silence: still slides (always glide)");
 silence();start();note(0,60);rb(400);silence();start();t=note(0,72);rb((int)(tOct*1000/1.45)+100);
 printf("  half way %.2f (want %.2f)\n",AT(jack[0],t+MS(tOct*500)),R*sqrt(2));check("slides from the last pitch",near(AT(jack[0],t+MS(tOct*500)),R*sqrt(2),0.03));capOn=0;
 puts("== repeated hits during a slide: it keeps going and catches up");
 silence();start();note(0,60);rb(400);t=note(0,72);for(int i=0;i<6;i++){rb((int)(tOct*1000/1.45/8));note(0,72);}rb((int)(tOct*1000/1.45)+100);
 {// measured between hits (a hit resets the oscillator phase): 9/16 of the way at 9/16 of T
  double hw=AT(jack[0],t+MS(tOct*1000*9/16)),e2=AT(jack[0],t+MS(tOct*1000+60));printf("  9/16 of T %.2f (want %.2f), end %.2f\n",hw,R*pow(2,9/16.),e2);check("re-hits don't restart or stall it",near(hw,R*pow(2,9/16.),0.03)&&near(e2,2*R,0.01));}capOn=0;
 puts("== chasing: a new note mid-slide slides from where it is");
 silence();start();note(0,60);rb(400);t=note(0,72);rb((int)(tOct*500/1.45));long tb=note(0,60);rb((int)(tOct*1000/1.45)+100);
 {double a=AT(jack[0],tb+MS(10)),b=AT(jack[0],tb+MS(tOct*250));printf("  turned back at %.2f, a quarter octave later %.2f (want %.2f)\n",a,b,R*pow(2,.25));check("turns back from mid-slide",near(a,R*sqrt(2),0.03)&&near(b,R*pow(2,.25),0.03));}capOn=0;
 puts("== lct: every slide takes T");
 k[GT]=1;edit();
 silence();start();note(0,60);rb(400);t=note(0,72);rb((int)(tOct*1000/1.45)+100);
 {double h1=AT(jack[0],t+MS(tOct*500)),e1=AT(jack[0],t+MS(tOct*1000+60));printf("  octave: half %.2f (want %.2f), end %.2f\n",h1,R*sqrt(2),e1);check("an octave in T",near(h1,R*sqrt(2),0.03)&&near(e1,2*R,0.01));}
 silence();start();note(0,60);rb(400);t=note(0,61);rb((int)(tOct*1000/1.45)+100);
 {double h1=AT(jack[0],t+MS(tOct*500)),e1=AT(jack[0],t+MS(tOct*1000+60));printf("  semitone: half %.2f (want %.2f), end %.2f\n",h1,R*pow(2,1/24.),e1);check("a semitone in T too",near(h1,R*pow(2,1/24.),0.006)&&near(e1,R*pow(2,1/12.),0.004));}capOn=0;
 puts("== exp: fast, then easing in");
 k[GT]=2;edit();
 silence();start();note(0,60);rb(400);t=note(0,72);rb((int)(tOct*1000/1.45)+100);
 {double q=AT(jack[0],t+MS(tOct*1000/4.6052)),e1=AT(jack[0],t+MS(tOct*1000+30)),want=R*pow(2,1-exp(-1.));printf("  after T/4.6 %.2f (want %.2f, 63%%), at T %.2f (want %.2f)\n",q,want,e1,2*R);check("RC curve: 63%% of the way after one time constant",near(q,want,0.03));check("within 1%% by T",near(e1,2*R,0.012));}capOn=0;
 k[GT]=0;edit();
 puts("== unlinked drum 2 doesn't glide");
 silence();start();note(1,60);rb(200);t=note(1,72);rb(100);
 check("drum 2 on pitch at once",near(AT(jack[1],t+MS(20)),2*f0[1],0.01));capOn=0;
 puts("== unison 1-3: all slide");
 k[LV]=7;k[LV+1]=1;k[LV+2]=0;edit();silence();start();note(0,60);rb(450);t=note(0,72);rb((int)(tOct*1000/1.45)+200);
 {int ok=1;for(int d=0;d<3;d++){double x=AT(jack[d],t+MS(tOct*500)),y=AT(jack[d],t+MS(tOct*1000+60));printf("  drum %d half %.2f end %.2f\n",d+1,x,y);if(!near(x,R*sqrt(2),0.03)||!near(y,2*R,0.01))ok=0;}check("all three slide together",ok);}capOn=0;
 puts("== chord 1-3 (0, +4, +7): the chord slides");
 k[COARSE[1]]=68;k[COARSE[2]]=71;k[LV+1]=2;edit();silence();start();note(0,60);rb(450);t=note(0,72);rb((int)(tOct*1000/1.45)+200);
 {int ok=1;double iv[3]={0,4,7};for(int d=0;d<3;d++){double x=AT(jack[d],t+MS(tOct*500)),y=AT(jack[d],t+MS(tOct*1000+60)),w=R*pow(2,iv[d]/12.);printf("  drum %d half %.2f (want %.2f) end %.2f (want %.2f)\n",d+1,x,w*sqrt(2),y,2*w);if(!near(x,w*sqrt(2),0.03)||!near(y,2*w,0.01))ok=0;}check("intervals kept while sliding",ok);}capOn=0;
 puts("== poly: no glide");
 k[COARSE[1]]=64;k[COARSE[2]]=64;k[LV+1]=3;edit();silence();start();t=note(0,60);rb(100);long t2=note(0,72);rb(100);
 {int ok=0;for(int d=0;d<3;d++){double y=AT(jack[d],t2+MS(20));printf("  drum %d %.2f\n",d+1,y);if(near(y,2*R,0.01))ok=1;}check("the new note on pitch at once",ok);}capOn=0;
 puts("== linked 2+3: drum 1 doesn't glide");
 k[LV]=6;k[LV+1]=1;edit();silence();start();note(0,60);rb(200);t=note(0,72);rb(100);check("drum 1 on pitch at once",near(AT(jack[0],t+MS(20)),2*R,0.01));
 start();note(1,60);rb(450);t=note(1,72);rb((int)(tOct*1000/1.45)+100);check("the group slides",near(AT(jack[1],t+MS(tOct*500)),R*sqrt(2),0.03)&&near(AT(jack[2],t+MS(tOct*500)),R*sqrt(2),0.03));capOn=0;
 puts("== CV jumps glide (drum 1, no link)");
 k[LV]=0;k[LV+1]=0;edit();
 {LxrMod m;memset(&m,0,sizeof m);m.on=1;m.dst[0]=2;m.amt[0]=100;m.src[0]=0;lxr_set_mod(&m);lxr_fw_cv(0,4.f);rb(100);
  // 5 V's pitch with no glide, as the target (the CV scale isn't exactly 1 V per octave)
  k[GL]=0;edit();lxr_fw_cv(0,5.f);rb(100);silence();start();t=note(0,60);rb(450);double c5=AT(jack[0],capN-MS(30));capOn=0;
  k[GL]=64;edit();lxr_fw_cv(0,4.f);rb(100);
  silence();start();t=note(0,60);rb(450);double c4=AT(jack[0],capN-MS(30));lxr_fw_cv(0,5.f);long tc=capN;rb((int)(tOct*1000*1.2/1.45)+150);
  double oc=log2(c5/c4),x=AT(jack[0],tc+MS(tOct*500*oc)),y=AT(jack[0],tc+MS(tOct*1000*oc+80));
  printf("  4 V %.2f, 5 V %.2f (%.3f octaves); half %.2f (want %.2f), end %.2f\n",c4,c5,oc,x,sqrt(c4*c5),y);
  check("a 1 V jump slides",near(x,sqrt(c4*c5),0.03)&&near(y,c5,0.01));memset(&m,0,sizeof m);lxr_set_mod(&m);}capOn=0;
 {static uint8_t r[PARAMS],r1[PARAMS],fl[8+PARAMS];lxr_fw_read_kit(r,0);check("glide in the kit",r[GL]==64&&r[GT]==0);
  // a 0.5.0-0.5.3 kit (339 bytes) with a link: keeps the link, loads with glide off
  r[LV]=3;r[LV+1]=2;memcpy(fl,a,8);memcpy(fl+8,r,PARAMS-2);uint8_t p2[PARAMS];memcpy(p2,r,PARAMS);p2[GL]=0;p2[GT]=0;lxr_fw_load_kit(fl,8+PARAMS-2,p2);rb(400);memset(r1,0x55,PARAMS);lxr_fw_read_kit(r1,0);
  printf("  339-byte kit: link %d %d, glide %d\n",r1[LV],r1[LV+1],r1[GL]);check("339-byte kit: link kept, glide off",r1[LV]==3&&r1[LV+1]==2&&r1[GL]==0&&r1[GT]==0);
  // a 0.5.4-0.5.5 kit (340 bytes) with glide: keeps it, type lcr
  r[GL]=70;r[GT]=2;memcpy(fl+8,r,PARAMS-1);memcpy(p2,r,PARAMS);p2[GT]=0;lxr_fw_load_kit(fl,8+PARAMS-1,p2);rb(400);memset(r1,0x55,PARAMS);lxr_fw_read_kit(r1,0);
  printf("  340-byte kit: glide %d, type %d\n",r1[GL],r1[GT]);check("340-byte kit: glide kept, type lcr",r1[GL]==70&&r1[GT]==0);}
 printf("%d failed\n",fails);puts("done");_exit(fails?2:0);}
