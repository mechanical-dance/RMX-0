// 0.5.5: the LFO page (SHIFT + MOD) reaches all 19 shapes, sin to rn1.
// Run: LXR0_DIR=<state> ./lfo_menu_test <dylib> <kit.SND> <card folder>
#include <dlfcn.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define _GNU_SOURCE
#include <string.h>
#include <unistd.h>
#define PARAMS 333
#define LV 325
#define S(t,n,...) t (*n)(__VA_ARGS__)
void *h;
S(void,lxr_init,void);S(void,lxr_render,int16_t[4][64]);S(void,lxr_fw_load_kit,const uint8_t*,int,const uint8_t*);S(void,lxr_fw_edit,const uint8_t*);
S(int,lxr_fw_read_kit,uint8_t*,int);S(void,lxr_fw_sample_root,const char*);S(void,lxr_fw_lcd,char*,uint8_t*);S(void,lxr_fw_button,int,int);S(void,lxr_fw_encoder,int);S(void,lxr_fw_encoder_button,int);S(void,lxr_fw_pot,int,int);
#define F(n) n = dlsym(h,#n); if(!n){printf("missing %s\n",#n);return 1;}
static int fails;
static void *aud(void *a){int16_t j[4][64];for(;;){lxr_render(j);usleep(1200);}return 0;}
static void w(int ms){usleep(ms*1000);}
static void lcd(const char *t){char s[33];uint8_t cg[64];lxr_fw_lcd(s,cg);char o[33];for(int i=0;i<32;i++)o[i]=((uint8_t)s[i]<32||(uint8_t)s[i]>126)?'.':s[i];o[32]=0;printf(" %-24s [%.16s] [%.16s]\n",t,o,o+16);}
static int lcdHas(const char *x){char s[33];uint8_t cg[64];lxr_fw_lcd(s,cg);s[32]=0;return strcasestr(s,x)!=0;}
static void tap(int b){lxr_fw_button(b,1);w(60);lxr_fw_button(b,0);w(150);}
static void drum(int d){lxr_fw_button(4,1);w(60);tap(5+d);lxr_fw_button(4,0);w(150);}
// a sub-page button pressed again flips to its second screen: land on the first
static void page(int sub){tap(5+sub);char s[33];uint8_t cg[64];lxr_fw_lcd(s,cg);if(s[15]=='<')tap(5+sub);}
static void check(const char *what,int ok){printf("  %s %s\n",ok?"ok  ":"FAIL",what);if(!ok)fails++;}
static uint8_t k[PARAMS],r[PARAMS];
static void rd(void){memset(r,0,PARAMS);if(!lxr_fw_read_kit(r,0))puts("  (read failed)");}
static void pot(int i,int steps){lxr_fw_pot(i,steps);w(250);}
static void enc(int n){lxr_fw_encoder(n);w(150);}
static void click(void){lxr_fw_encoder_button(1);w(60);lxr_fw_encoder_button(0);w(150);}
#define COARSE(d) (8+2*(d))
#define DEC(d) (50+2*(d))
#define VOL(d) (88+(d))
#define PAN(d) (94+(d))
#define DRIVE(d) (102+(d))
int main(int ac,char**v){setvbuf(stdout,0,_IONBF,0);if(ac<4){puts("args");return 1;}
 h=dlopen(v[1],RTLD_NOW);if(!h){puts(dlerror());return 1;}
 F(lxr_init);F(lxr_render);F(lxr_fw_load_kit);F(lxr_fw_edit);F(lxr_fw_read_kit);F(lxr_fw_sample_root);F(lxr_fw_lcd);F(lxr_fw_button);F(lxr_fw_encoder);F(lxr_fw_encoder_button);F(lxr_fw_pot);
 static uint8_t a[400];FILE*fp=fopen(v[2],"rb");if(!fp){perror(v[2]);return 1;}int na=fread(a,1,400,fp);fclose(fp);memcpy(k,a+8,na-8);
 lxr_init();lxr_fw_sample_root(v[3]);pthread_t t;pthread_create(&t,0,aud,0);w(300);lxr_fw_load_kit(a,na,k);w(4000);
 lcd("boot");check("boots on the osc page, screen 1",lcdHas("COA")&&lcdHas("pwm"));
 lxr_fw_read_kit(k,0);
 for(int d=0;d<3;d++){k[COARSE(d)]=40+10*d;k[DEC(d)]=60+10*d;k[VOL(d)]=100+d;k[PAN(d)]=60+d;}
 memset(k+LV,0,6);lxr_fw_edit(k);w(400);
 puts("== LFO page: shapes");
 drum(0);lxr_fw_button(12,1);w(60);tap(2);lxr_fw_button(12,0);w(200);lcd("shift+mod");
 for(int sc=0;sc<2;sc++){char z[33];uint8_t cg[64];lxr_fw_lcd(z,cg);for(int sl=0;sl<4;sl++)if(!strncasecmp(z+4*sl,"wav",3)){printf("  wav in slot %d, screen %d\n",sl,sc+1);
   pot(sl,-300);rd();lcd("pot down");int lo=r[155];pot(sl,600);rd();lcd("pot up");check("the knob runs from sin to rn1 (0-18)",lo==0&&r[155]==18&&lcdHas("rn1"));
   for(int i=0;i<19;i++){pot(sl,-1);}rd();printf("  back down 19 steps: %d\n",r[155]);goto done;}
  enc(4);}
 check("found the wav slot",0);done:
 printf("%d failed\n",fails);puts("done");_exit(fails?2:0);}
