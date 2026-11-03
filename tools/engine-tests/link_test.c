// 0.5.0: voice links (unison, chord, poly) on drums 1-3. Renders block by block on this thread, like retrig_test.
// Each drum plays a sine on its own mono jack (L1, R1, L2), so pitch and level are measured per drum.
// Run: LXR0_DIR=<state> ./link_test <dylib> <kit.SND> <card folder>
#include <dlfcn.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define PARAMS 340
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
 for(int i=121;i<127;i++)k[i]=0;for(int i=128;i<134;i++)k[i]=0;for(int i=143;i<149;i++)k[i]=0;for(int i=265;i<283;i++)k[i]=0;for(int i=313;i<331;i++)k[i]=0;
 static const int pan[]={94,95,96,99,100,101};for(int i=0;i<6;i++)k[pan[i]]=63;
 static const int WAVE[]={1,2,3},COARSE[]={8,10,12},ATK[]={49,51,53},DEC[]={50,52,54},FINE[]={9,11,13};
 for(int d=0;d<3;d++){k[WAVE[d]]=0;k[COARSE[d]]=40;k[FINE[d]]=63;k[ATK[d]]=0;k[DEC[d]]=110;k[215+d]=2+d;}
 for(int i=3;i<6;i++)k[88+i]=0;
 edit();rb(300);
 double f[3],g[3];int p[3],q[3];long t;
 puts("== unlinked: each drum alone");
 double f0[3];int p0[3];
 for(int d=0;d<3;d++){silence();start();t=note(d,60);rb(260);int best=0,bv=0;for(int c=0;c<4;c++){int x=peak(c,t,capN);if(x>bv){bv=x;best=c;}}jack[d]=best;f0[d]=hz(best,t+2205,t+11025);p0[d]=peak(best,t+2205,t+11025);printf(" drum %d: jack %d, %.2f Hz, peak %d\n",d+1,best,f0[d],p0[d]);capOn=0;}
 check("each drum on its own jack",jack[0]!=jack[1]&&jack[1]!=jack[2]&&jack[0]!=jack[2]);
 puts("== a link needs a mode and two drums");
 setlink(7,0,0,0,0);silence();start();t=note(0,60);rb(260);meas(t,f,p);show("mask 111, mode off",f,p);check("only drum 1 sounds",p[0]>p0[0]*0.95&&p[1]<50&&p[2]<50);capOn=0;
 setlink(1,1,0,0,0);silence();start();t=note(0,60);rb(260);meas(t,f,p);show("mask 001, unison",f,p);check("only drum 1 sounds, full level",p[0]>p0[0]*0.95&&p[1]<50&&p[2]<50);capOn=0;
 puts("== unison");
 setlink(7,1,0,0,0);silence();start();t=note(0,60);rb(260);meas(t,f,p);show("3 drums, detune 0",f,p);
 check("all three sound at 1/sqrt(3)",near(p[0],p0[0]/sqrtf(3),0.04)&&near(p[1],p0[1]/sqrtf(3),0.04)&&near(p[2],p0[2]/sqrtf(3),0.04));
 check("same pitch",near(f[0],f0[0],0.002)&&near(f[1],f0[0],0.002)&&near(f[2],f0[0],0.002));capOn=0;
 setlink(7,1,127,0,0);silence();start();t=note(0,60);rb(260);meas(t,f,p);show("3 drums, detune 127",f,p);
 check("-50, 0, +50 cents",near(f[0],f0[0]*pow(2,-50/1200.),0.002)&&near(f[1],f0[0],0.002)&&near(f[2],f0[0]*pow(2,50/1200.),0.002));capOn=0;
 setlink(5,1,127,0,0);silence();start();t=note(0,60);rb(260);meas(t,f,p);show("drums 1+3, detune 127",f,p);
 check("1/sqrt(2), -50 and +50 cents, drum 2 silent",near(p[0],p0[0]/sqrtf(2),0.04)&&p[1]<50&&near(f[0],f0[0]*pow(2,-50/1200.),0.002)&&near(f[2],f0[0]*pow(2,50/1200.),0.002));capOn=0;
 setlink(7,1,0,0,0);silence();start();t=note(1,60);rb(260);meas(t,f,p);show("note on drum 2 (not the lead)",f,p);check("ignored",p[0]<50&&p[1]<50&&p[2]<50);capOn=0;
 setlink(3,1,0,0,0);silence();start();t=note(2,60);rb(260);meas(t,f,p);show("mask 011, note on drum 3",f,p);check("drum 3 plays on its own",p[0]<50&&p[1]<50&&near(p[2],p0[2],0.04));capOn=0;
 setlink(7,1,0,0,0);silence();start();t=note(0,72);rb(260);meas(t,f,p);show("note 72",f,p);check("all an octave up",near(f[0],2*f0[0],0.003)&&near(f[1],2*f0[0],0.003)&&near(f[2],2*f0[0],0.003));capOn=0;
 puts("== chord: each drum keeps its own tuning");
 k[COARSE[1]]=44;k[COARSE[2]]=47;setlink(7,2,127,0,0);silence();start();t=note(0,60);rb(260);meas(t,f,p);show("coarse 40/44/47, detune 127",f,p);
 check("three pitches, no detune, 1/sqrt(3)",near(f[0],f0[0],0.002)&&near(f[1],f0[0]*pow(2,4/12.),0.003)&&near(f[2],f0[0]*pow(2,7/12.),0.003)&&near(p[1],p0[1]/sqrtf(3),0.04));capOn=0;
 silence();start();t=note(0,65);rb(260);meas(t,f,p);show("note 65",f,p);check("the chord moves up 5 semitones",near(f[0],f0[0]*pow(2,5/12.),0.003)&&near(f[2],f0[0]*pow(2,12/12.),0.003));capOn=0;
 k[COARSE[1]]=k[COARSE[2]]=40;
 puts("== width: stereo out, pans spread left to right");
 for(int d=0;d<3;d++)k[215+d]=0;
 k[COARSE[1]]=52;setlink(3,2,0,127,0);silence();start();t=note(0,60);rb(260);
 {double fl=hz(0,t+2205,t+11025),fr=hz(1,t+2205,t+11025),f2l=hz(2,t+2205,t+11025),f2r=hz(3,t+2205,t+11025);int pl=peak(0,t+2205,t+11025),pr=peak(1,t+2205,t+11025),p2l=peak(2,t+2205,t+11025),p2r=peak(3,t+2205,t+11025);
  printf(" jacks 0-3: %.2f Hz %d | %.2f Hz %d | %.2f Hz %d | %.2f Hz %d\n",fl,pl,fr,pr,f2l,p2l,f2r,p2r);
  int L=pl>p2l?0:2;double hl=hz(L,t+2205,t+11025),hr=hz(L+1,t+2205,t+11025);check("drum 1 hard left, drum 2 hard right",near(hl,f0[0],0.003)&&near(hr,f0[0]*2,0.003));}
 capOn=0;
 setlink(3,2,0,0,0);silence();start();t=note(0,60);rb(260);{int L=peak(0,t,capN)>peak(2,t,capN)?0:2;double hl=hz(L,t+2205,t+11025),hr=hz(L+1,t+2205,t+11025);printf(" width 0: L %.2f Hz R %.2f Hz\n",hl,hr);check("width 0: both centred (L = R)",near(peak(L,t+2205,t+11025),peak(L+1,t+2205,t+11025),0.03));}capOn=0;
 setlink(0,0,0,0,0);silence();start();t=note(0,60);rb(260);{int L=peak(0,t,capN)>peak(2,t,capN)?0:2;check("link off: drum 1 centred again",near(peak(L,t+2205,t+11025),peak(L+1,t+2205,t+11025),0.03));}capOn=0;
 k[COARSE[1]]=40;for(int d=0;d<3;d++)k[215+d]=2+d;edit();
 puts("== poly");
 setlink(7,3,0,0,0);silence();start();long t1=note(0,60);rb(80);long t2=note(0,64);rb(80);long t3=note(0,67);rb(260);
 meas(t3,f,p);show("60, 64, 67",f,p);
 check("one note per drum, each at full level",near(f[0],f0[0],0.002)&&near(f[1],f0[0]*pow(2,4/12.),0.003)&&near(f[2],f0[0]*pow(2,7/12.),0.003)&&near(p[2],p0[2],0.04));
 {double e=hz(jack[0],t1+2205,t2);check("drum 1 held its pitch after the next notes came",near(e,f0[0],0.003)&&near(hz(jack[0],t3+2205,t3+11025),f0[0],0.002));}
 long t4=note(0,72);rb(260);meas(t4,f,p);show("then 72, alloc oldest",f,p);check("72 takes drum 1 (oldest)",near(f[0],2*f0[0],0.003)&&near(f[1],f0[0]*pow(2,4/12.),0.003)&&near(f[2],f0[0]*pow(2,7/12.),0.003));
 long t5=note(0,48);rb(260);meas(t5,f,p);show("then 48",f,p);check("48 takes drum 2 (now oldest)",near(f[1],f0[0]/2,0.003)&&near(f[0],2*f0[0],0.003));capOn=0;
 {const double R=f0[0];int d60,d64,d67;
 setlink(7,3,0,0,1);silence();start();note(0,60);rb(60);note(0,67);rb(60);long a3=note(0,64);rb(60);meas(a3,f,p);d60=which(f,p,R);d67=which(f,p,R*pow(2,7/12.));d64=which(f,p,R*pow(2,4/12.));
 t=note(0,55);rb(260);meas(t,f,p);show("60 67 64, then 55, alloc highest",f,p);
 check("55 replaces 67 (the highest), 60 and 64 play on",d67>=0&&which(f,p,R*pow(2,-5/12.))==d67&&d60>=0&&which(f,p,R)==d60&&d64>=0&&which(f,p,R*pow(2,4/12.))==d64);capOn=0;
 setlink(7,3,0,0,0);silence();k[DEC[0]]=k[DEC[1]]=k[DEC[2]]=40;edit();start();t1=note(0,60);rb(60);meas(t1,f,p);d60=which(f,p,R);t2=note(0,64);rb(60);meas(t2,f,p);d64=which(f,p,R*pow(2,4/12.));rb(500);
 t=note(0,67);rb(260);meas(t,f,p);show("60 64, both end, then 67",f,p);d67=which(f,p,R*pow(2,7/12.));
 check("67 takes the drum 60 and 64 didn't use",d60>=0&&d64>=0&&d60!=d64&&d67>=0&&d67!=d60&&d67!=d64);
 t=note(0,69);rb(260);meas(t,f,p);show("then 69",f,p);check("69 takes the free drum used longest ago (60's)",which(f,p,R*pow(2,9/12.))==d60);capOn=0;}
 k[DEC[0]]=k[DEC[1]]=k[DEC[2]]=110;edit();
 setlink(7,3,0,0,0);silence();start();t=note(1,60);rb(200);meas(t,f,p);check("a note on drum 2 is ignored",p[0]<50&&p[1]<50&&p[2]<50);capOn=0;
 puts("== CV: cv1 to drum 1 1V/oct");
 LxrMod m;memset(&m,0,sizeof m);m.on=1;m.dst[0]=2;m.amt[0]=100;m.src[0]=0;lxr_set_mod(&m);
 for(int i=265;i<283;i++)k[i]=0;k[247]=2;k[265]=100;k[283]=0;
 lxr_fw_cv(0,3.f);rb(100);setlink(0,0,0,0,0);silence();start();t=note(0,60);rb(260);double c0=hz(jack[0],t+2205,t+11025);capOn=0;
 lxr_fw_cv(0,4.f);rb(100);silence();start();t=note(0,60);rb(260);double c1=hz(jack[0],t+2205,t+11025);capOn=0;
 printf(" unlinked drum 1: 3 V %.2f Hz, 4 V %.2f Hz (ratio %.3f)\n",c0,c1,c1/c0);check("4 V is an octave up on drum 1",near(c1/c0,2,0.01));
 lxr_fw_cv(0,3.f);rb(100);setlink(7,1,0,0,0);silence();start();t=note(0,60);rb(260);meas(t,f,p);show("unison, 3 V",f,p);check("3 V: all at drum 1's CV pitch",near(f[0],c0,0.003)&&near(f[1],c0,0.003)&&near(f[2],c0,0.003));
 lxr_fw_cv(0,4.f);rb(150);t=capN;rb(150);meas(t-2205,g,q);show("unison, CV to 4 V while held",g,q);check("all follow the CV",near(g[0],c1,0.005)&&near(g[1],c1,0.005)&&near(g[2],c1,0.005));capOn=0;
 lxr_fw_cv(0,3.f);rb(100);setlink(7,3,0,0,0);silence();start();t1=note(0,60);rb(80);lxr_fw_cv(0,4.f);rb(100);t2=note(0,60);rb(260);meas(t2,f,p);show("poly: 3 V note, then 4 V note",f,p);
 {int x=which(f,p,c0),y=which(f,p,c1);check("the first note keeps 3 V, the second plays at 4 V",x>=0&&y>=0&&x!=y);}capOn=0;
 setlink(0,0,0,0,0);silence();start();t=note(0,60);rb(260);meas(t,f,p);show("poly off, CV still 4 V",f,p);check("drum 1 is back on the CV",near(f[0],c1,0.005));capOn=0;
 lxr_fw_cv(0,3.f);rb(100);
 puts("== CV and trigger at the same moment (poly)");
 setlink(7,3,0,0,0);lxr_fw_cv(0,3.f);rb(100);silence();start();note(0,60);rb(100);
 lxr_fw_cv(0,4.f);t=note(0,60);rb(260);meas(t,f,p);show("CV change, then the trigger",f,p);{int x=which(f,p,c0),y=which(f,p,c1);check("the new note takes the new CV, the old one keeps its own",x>=0&&y>=0&&x!=y);}capOn=0;
 silence();lxr_fw_cv(0,3.f);rb(100);start();note(0,60);rb(100);
 t=note(0,60);usleep(1500);lxr_fw_cv(0,4.f);rb(260);meas(t,f,p);show("trigger, then CV 1.5 ms later",f,p);{int x=which(f,p,c0),y=which(f,p,c1);check("the CV still reaches the new note",x>=0&&y>=0&&x!=y);}capOn=0;
 lxr_fw_cv(0,3.f);rb(100);
 puts("== voice steal clicks (poly, sine, long decay)");
 {LxrMod z;memset(&z,0,sizeof z);lxr_set_mod(&z);}
 setlink(0,0,0,0,0);int own=0;for(int d=0;d<3;d++){silence();start();t=note(d,72);rb(200);int m=maxd(jack[d],t,t+132);if(m>own)own=m;capOn=0;}
 setlink(7,3,0,0,0);silence();start();note(0,60);rb(37);note(0,64);rb(41);note(0,67);rb(43);
 int worst=0;for(int i=0;i<6;i++){long ts=note(0,i&1?72:55);rb(29+7*i);for(int d=0;d<3;d++){int m=maxd(jack[d],ts,ts+132);if(m>worst)worst=m;}}rb(100);
 printf(" largest jump in the 3 ms after a steal %d; note 72 from silence, any drum %d\n",worst,own);check("a steal adds no step beyond a hit from silence",worst<=own*1.15+30);capOn=0;
 puts("== kit file");
 setlink(5,2,99,88,1);k[LV+4]=1;edit();rb(60);static uint8_t r[PARAMS];memset(r,0,PARAMS);int ok=lxr_fw_read_kit(r,0);printf(" read ok %d: %d %d %d %d %d %d\n",ok,r[LV],r[LV+1],r[LV+2],r[LV+3],r[LV+4],r[LV+5]);
 check("link bytes read back",r[LV]==5&&r[LV+1]==2&&r[LV+2]==99&&r[LV+3]==88&&r[LV+4]==1&&r[LV+5]==1);
 {static uint8_t fl[8+PARAMS],r1[PARAMS];memcpy(fl,a,8);memcpy(fl+8,r,PARAMS);lxr_fw_load_kit(fl,8+PARAMS,r);rb(400);memset(r1,0,PARAMS);ok=lxr_fw_read_kit(r1,0);int bad=0;for(int i=0;i<PARAMS;i++)if(r1[i]!=r[i])bad++;
  printf(" 340-byte reload (read ok %d): %d bytes differ, link %d %d\n",ok,bad,r1[LV],r1[LV+1]);check("a linked kit reloads exactly",ok&&!bad);
  static const int sizes[]={8+325,8+313,255};for(int i=0;i<3;i++){uint8_t p2[PARAMS];memset(p2,0,PARAMS);memcpy(p2,r,sizes[i]-8);memcpy(fl+8,r,sizes[i]-8);lxr_fw_load_kit(fl,sizes[i],p2);rb(400);memset(r1,0x55,PARAMS);ok=lxr_fw_read_kit(r1,0);int z=0;for(int j=LV;j<LV+6;j++)z+=r1[j];
   printf(" %d-byte kit after a linked one (read ok %d): link byte sum %d\n",sizes[i],ok,z);check("loads unlinked",ok&&z==0);}}
 printf("%d failed\n",fails);puts("done");_exit(fails?2:0);}
