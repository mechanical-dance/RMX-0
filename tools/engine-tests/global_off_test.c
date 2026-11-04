// 0.5.2/0.5.3: global MIDI channel 0 (the factory default) is off for notes. Before, it stayed on channel 1, so with drums 1-3
// linked on channel 1 and the snare selected, channel 1 also played the snare. Drums 1-3 + snare on L1, R1, L2, R2.
// Drums on their own jacks (L1, R1, L2) as in link_test. Run: LXR0_DIR=<state> ./link_midi_test <dylib> <kit.SND> <card folder>
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
#define S(t,n,...) t (*n)(__VA_ARGS__)
void *h;
S(void,lxr_init,void);S(void,lxr_render,int16_t[4][64]);S(void,lxr_fw_load_kit,const uint8_t*,int,const uint8_t*);S(void,lxr_fw_edit,const uint8_t*);S(int,lxr_fw_read_kit,uint8_t*,int);
#include "listen.h"
S(void,lxr_fw_sample_root,const char*);S(void,lxr_fw_lcd,char*,uint8_t*);S(void,lxr_fw_button,int,int);S(void,lxr_fw_encoder,int);S(void,lxr_fw_pot,int,int);S(void,lxr_fw_midi,const uint8_t*,int);
#define F(n) n = dlsym(h,#n); if(!n){printf("missing %s\n",#n);return 1;}
static int fails;
#define CAPN (44100*2)
static int16_t cap[4][CAPN];static volatile long capN;static volatile int capOn;
static void *aud(void *a){int16_t j[4][64];for(;;){lxr_render(j);if(capOn&&capN+64<=CAPN){for(int c=0;c<4;c++)memcpy(&cap[c][capN],j[c],128);capN+=64;}usleep(1200);}return 0;}
static void w(int ms){usleep(ms*1000);}
static void row(char *s){uint8_t cg[64];lxr_fw_lcd(s,cg);for(int i=0;i<32;i++)if((uint8_t)s[i]<32||(uint8_t)s[i]>126)s[i]='.';s[32]=0;}
static void lcd(const char *t){char s[33];row(s);printf(" %-26s [%.16s] [%.16s]\n",t,s,s+16);}
static void down(int b){lxr_fw_button(b,1);w(80);}
static void up(int b){lxr_fw_button(b,0);w(80);}
static void tap(int b){down(b);up(b);w(70);}
static void drum(int d){down(4);tap(5+d);up(4);w(100);}
static void config(void){down(12);tap(13);up(12);w(150);}
static void check(const char *what,int ok){printf("  %s %s\n",ok?"ok  ":"FAIL",what);if(!ok)fails++;}
// turn pot i until the value under it reads want (4 characters, as on the LCD)
static int setTo(int i,const char *want){char s[33];for(int n=0;n<400;n++){row(s);if(!strncmp(s+16+4*i,want,strlen(want)))return 1;lxr_fw_pot(i,n<200?1:-1);w(40);}return 0;}
static int peak(int c){int m=0;for(long i=4410;i<capN;i++)if(abs(cap[c][i])>m)m=abs(cap[c][i]);return m;}
static double hz(int c){long f=-1,l=-1;int n=0;for(long i=4411;i<capN;i++)if(cap[c][i-1]<0&&cap[c][i]>=0){double x=i-1+(double)-cap[c][i-1]/(cap[c][i]-cap[c][i-1]);if(f<0)f=(long)(x*1000);l=(long)(x*1000);n++;}return n>1?(n-1)*44100.0*1000/(l-f):0;}
static uint8_t k[PARAMS];
static void quiet(void){k[50]=k[52]=k[54]=1;lxr_fw_edit(k);w(250);k[50]=k[52]=k[54]=110;lxr_fw_edit(k);w(150);}
// send notes, listen 400 ms, report each drum's pitch and level
static void play(const char *what,const uint8_t *notes,int n,uint8_t ch,double *f,int *p){quiet();capN=0;capOn=1;for(int i=0;i<n;i++){uint8_t m[3]={(uint8_t)(0x90|(ch-1)),notes[i],100};lxr_fw_midi(m,3);w(60);}w(400);capOn=0;
 printf(" %-34s",what);for(int d=0;d<4;d++){f[d]=hz(d);p[d]=peak(d);printf(" o%d %7.2f Hz %6d |",d+1,f[d],p[d]);}puts("");}
static int sounding(int *p){int m=0;for(int d=0;d<4;d++)if(p[d]>300)m|=1<<d;return m;}
int main(int ac,char**v){setvbuf(stdout,0,_IONBF,0);if(ac<4){puts("args");return 1;}
 h=dlopen(v[1],RTLD_NOW);if(!h){puts(dlerror());return 1;}
 F(lxr_init);F(lxr_render);LISTEN_WRAP();F(lxr_fw_load_kit);F(lxr_fw_edit);F(lxr_fw_read_kit);F(lxr_fw_sample_root);F(lxr_fw_lcd);F(lxr_fw_button);F(lxr_fw_encoder);F(lxr_fw_pot);F(lxr_fw_midi);
 static uint8_t a[400];FILE*fp=fopen(v[2],"rb");if(!fp){perror(v[2]);return 1;}int na=fread(a,1,400,fp);fclose(fp);memcpy(k,a+8,na-8);
 lxr_init();lxr_fw_sample_root(v[3]);pthread_t t;pthread_create(&t,0,aud,0);w(300);lxr_fw_load_kit(a,na,k);w(4000);

 puts("== config screens as they come");
 config();config();check("MIDI in on",setTo(0,"on"));config();config();config();for(int i=0;i<4;i++){lxr_fw_pot(i,-300);w(300);}lcd("notes 1-4 down");config();for(int i=0;i<3;i++){lxr_fw_pot(i,-300);w(300);}lcd("notes 5-7 down");
 drum(0);w(200);lxr_fw_read_kit(k,0);
 static const int zero[]={197,198,199,200,201,202,234,235,236,238,239,240,241,242,301,302,303,304,305,306,307,28,29,30,31,32,33,34,35,36,74,75,76,77,82,84,86,102,103,104,105,106,107,0};
 for(int i=0;zero[i];i++)k[zero[i]]=0;
 for(int i=88;i<92;i++)k[i]=110;for(int i=92;i<94;i++)k[i]=0;for(int i=37;i<43;i++)k[i]=127;for(int i=43;i<49;i++)k[i]=0;for(int i=108;i<115;i++)k[i]=127;
 for(int i=121;i<127;i++)k[i]=0;for(int i=128;i<134;i++)k[i]=0;for(int i=143;i<149;i++)k[i]=0;for(int i=265;i<283;i++)k[i]=0;for(int i=313;i<331;i++)k[i]=0;
 for(int d=0;d<3;d++){k[1+d]=0;k[8+2*d]=40;k[9+2*d]=63;k[49+2*d]=0;k[50+2*d]=110;k[215+d]=2+d;k[94+d]=63;}
 k[218]=5;k[97]=63;
 for(int i=219;i<221;i++)k[i]=0;
 lxr_fw_edit(k);w(400);
 double f[4];int p[4];const uint8_t n60[]={60};
 for(int i=0;i<10;i++){char z[33];row(z);if(!strncmp(z,"CH1",3))break;config();}lcd("ch screen");check("snare ch 16",setTo(3," 16"));lcd("set");
 k[LV]=7;k[LV+1]=1;lxr_fw_edit(k);w(300);
 {char z[33];config();for(int i=0;i<10;i++){row(z);if(!strncmp(z,"MID",3))break;config();}row(z);printf("  global channel shows [%.4s]\n",z+20);check("global channel untouched: off",!strncmp(z+20,"off",3));}
 for(int sel=0;sel<4;sel++){drum(sel);w(200);char t[64];sprintf(t,"1-3 unison, drum %d selected, ch 1",sel+1);play(t,n60,1,1,f,p);check("the group only",sounding(p)==7);}
 drum(3);w(200);play("snare selected, ch 16",n60,1,16,f,p);check("the snare on its own channel",sounding(p)==8);
 puts("== 0.5.3: global channel turned to 1 and back to off from the knob");
 {char z[33];config();for(int i=0;i<10;i++){row(z);if(!strncmp(z,"MID",3))break;config();}}
 check("global channel 1",setTo(1,"  1"));lcd("global 1");drum(3);w(200);play("ch 1, snare selected, global 1",n60,1,1,f,p);check("group and snare",sounding(p)==15);
 {char z[33];config();for(int i=0;i<10;i++){row(z);if(!strncmp(z,"MID",3))break;config();}}
 lxr_fw_pot(1,-40);w(300);lcd("pot down");{char z[33];row(z);check("shows off",!strncmp(z+20,"off",3));}
 drum(3);w(200);play("ch 1, snare selected, global off",n60,1,1,f,p);check("the group only",sounding(p)==7);
 printf("%d failed\n",fails);puts("done");_exit(fails?2:0);}
