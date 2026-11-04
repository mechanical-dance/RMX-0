// 0.5.0: voice links from the buttons. DRUM (button 4) held: a second drum pressed while the first is held forms
// the group, a double-tap edits one linked drum on its own, a single press goes back to the group. Group LEDs.
// Run: LXR0_DIR=<state> ./link_buttons_test <dylib> <kit.SND> <card folder>
#define _GNU_SOURCE
#include <dlfcn.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define PARAMS 340
#define LV 325
#define DRUM 4
#define S(t,n,...) t (*n)(__VA_ARGS__)
void *h;
S(void,lxr_init,void);S(void,lxr_render,int16_t[4][64]);S(void,lxr_fw_load_kit,const uint8_t*,int,const uint8_t*);S(void,lxr_fw_edit,const uint8_t*);
#include "listen.h"
S(int,lxr_fw_read_kit,uint8_t*,int);S(void,lxr_fw_sample_root,const char*);S(void,lxr_fw_lcd,char*,uint8_t*);S(void,lxr_fw_button,int,int);S(void,lxr_fw_encoder,int);S(void,lxr_fw_pot,int,int);S(uint32_t,lxr_fw_leds,void);
#define F(n) n = dlsym(h,#n); if(!n){printf("missing %s\n",#n);return 1;}
static int fails;
static void *aud(void *a){int16_t j[4][64];for(;;){lxr_render(j);usleep(1200);}return 0;}
static void w(int ms){usleep(ms*1000);}
static void lcd(const char *t){char s[33];uint8_t cg[64];lxr_fw_lcd(s,cg);char o[33];for(int i=0;i<32;i++)o[i]=((uint8_t)s[i]<32||(uint8_t)s[i]>126)?'.':s[i];o[32]=0;printf(" %-28s [%.16s] [%.16s]  leds %04x\n",t,o,o+16,lxr_fw_leds());}
static int lcdHas(const char *x){char s[33];uint8_t cg[64];lxr_fw_lcd(s,cg);s[32]=0;return strstr(s,x)!=0;}
static void down(int b){lxr_fw_button(b,1);w(80);}
static void up(int b){lxr_fw_button(b,0);w(80);}
static void tap(int b){down(b);up(b);w(70);}
static void drum(int d){down(DRUM);tap(5+d);up(DRUM);w(100);}
static void page(int sub){tap(5+sub);char s[33];uint8_t cg[64];lxr_fw_lcd(s,cg);if(s[15]=='<')tap(5+sub);}
static void check(const char *what,int ok){printf("  %s %s\n",ok?"ok  ":"FAIL",what);if(!ok)fails++;}
static uint8_t k[PARAMS],r[PARAMS],b[PARAMS];
static void rd(void){memset(r,0,PARAMS);if(!lxr_fw_read_kit(r,0))puts("  (read failed)");}
static void pot(int i,int steps){lxr_fw_pot(i,steps);w(250);}
static int led(int d){return (lxr_fw_leds()>>(5+d))&1;}
#define COARSE(d) (8+2*(d))
// a coarse edit on the page shown: which drums moved
static int moved(void){rd();memcpy(b,r,PARAMS);pot(0,8);rd();int m=0;for(int d=0;d<3;d++)if(r[COARSE(d)]!=b[COARSE(d)])m|=1<<d;pot(0,-8);return m;}
int main(int ac,char**v){setvbuf(stdout,0,_IONBF,0);if(ac<4){puts("args");return 1;}
 h=dlopen(v[1],RTLD_NOW);if(!h){puts(dlerror());return 1;}
 F(lxr_init);F(lxr_render);LISTEN_WRAP();F(lxr_fw_load_kit);F(lxr_fw_edit);F(lxr_fw_read_kit);F(lxr_fw_sample_root);F(lxr_fw_lcd);F(lxr_fw_button);F(lxr_fw_encoder);F(lxr_fw_pot);F(lxr_fw_leds);
 static uint8_t a[400];FILE*fp=fopen(v[2],"rb");if(!fp){perror(v[2]);return 1;}int na=fread(a,1,400,fp);fclose(fp);memcpy(k,a+8,na-8);
 lxr_init();lxr_fw_sample_root(v[3]);pthread_t t;pthread_create(&t,0,aud,0);w(300);lxr_fw_load_kit(a,na,k);w(4000);
 lxr_fw_read_kit(k,0);for(int d=0;d<3;d++)k[COARSE(d)]=40+10*d;memset(k+LV,0,6);lxr_fw_edit(k);w(400);
 puts("== plain selection is unchanged");
 drum(0);page(0);lcd("drum 1 osc");rd();check("no link",r[LV]==0&&r[LV+1]==0);check("edits move drum 1 only",moved()==1);
 down(DRUM);w(300);lcd("DRUM held, no link");check("only drum 1 lit",led(0)&&!led(1)&&!led(2));up(DRUM);
 puts("== hold drum 1, press drum 2");
 down(DRUM);down(5);lcd("drum 1 held");down(6);lcd("drum 2 pressed");rd();
 check("drums 1+2 linked, unison",r[LV]==3&&r[LV+1]==1);
 w(200);check("LEDs: drums 1 and 2 lit, 3 off",led(0)&&led(1)&&!led(2));
 up(6);up(5);up(DRUM);w(200);lcd("all released");check("then the link screen, on mod",lcdHas("MOD")&&lcdHas("uni"));
 page(0);lcd("osc");check("group edit moves drums 1 and 2",moved()==3);
 puts("== double-tap drum 2: solo");
 w(500);down(DRUM);tap(6);tap(6);lcd("double-tapped drum 2");
 int on=0,off=0,flips=0,last=-1,steady=1;for(int i=0;i<20;i++){int x=led(1);if(x)on++;else off++;if(last>=0&&x!=last)flips++;last=x;if(!led(0))steady=0;w(100);}
 printf("  drum 2 LED over 2 s: on %d, off %d, changes %d\n",on,off,flips);check("drum 2 blinks about once a second",flips>=3&&flips<=5&&on>5&&off>5);check("drum 1 stays lit",steady);
 up(DRUM);w(200);page(0);lcd("drum 2 osc");check("solo: edits move drum 2 only",moved()==2);
 puts("== single press drum 2: the group again");
 w(500);drum(1);page(0);check("group edit moves drums 1 and 2",moved()==3);
 down(DRUM);w(300);check("no blink: drum 2 steady",({int s=1;for(int i=0;i<8;i++){if(!led(1))s=0;w(100);}s;}));up(DRUM);
 puts("== a drum outside the group");
 drum(2);page(0);lcd("drum 3 osc");rd();check("the link keeps running",r[LV]==3&&r[LV+1]==1);check("edits move drum 3 only",moved()==4);
 down(DRUM);w(300);check("LEDs: only the selected drum lit (0.5.2)",!led(0)&&!led(1)&&led(2));up(DRUM);
 puts("== three drums, and the mode kept");
 rd();memcpy(k,r,PARAMS);k[LV+1]=2;lxr_fw_edit(k);w(300);
 down(DRUM);down(5);down(6);down(7);rd();lcd("1, 2, 3 held");check("all three linked, chord kept",r[LV]==7&&r[LV+1]==2);up(7);up(6);up(5);up(DRUM);
 page(0);check("group edit moves all three",moved()==7);
 puts("== re-forming: hold drum 2, press drum 3");
 down(DRUM);down(6);down(7);rd();check("drums 2+3",r[LV]==6&&r[LV+1]==2);up(7);up(6);up(DRUM);w(200);lcd("released");check("drum 2's link screen",lcdHas("MOD")&&lcdHas("chd"));
 down(DRUM);w(300);check("LEDs: drums 2 and 3, drum 2 selected",!led(0)&&led(1)&&led(2));up(DRUM);
 printf("%d failed\n",fails);puts("done");_exit(fails?2:0);}
