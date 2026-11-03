// 0.5.0: voice links from the panel. Link settings on the drum transient page, group knobs, limits, exclusions, sync.
// Run: LXR0_DIR=<state> ./link_menu_test <dylib> <kit.SND> <card folder>
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
 puts("== link settings on the transient page");
 drum(0);page(4);lcd("drum 1 transient");check("screen 1 ends in mod",lcdHas("mod"));
 enc(4);lcd("2nd screen");check("screen 2: det wid syn alc",lcdHas("det")&&lcdHas("wid")&&lcdHas("syn")&&lcdHas("alc"));
 enc(-4);lcd("back");
 rd();int m0=r[LV+1];pot(3,40);rd();lcd("pot 4 up");printf("  mode %d -> %d\n",m0,r[LV+1]);check("pot 4 sets the link mode",r[LV+1]!=m0&&r[LV+1]<=3);
 pot(3,-200);rd();check("mode back to off",r[LV+1]==0);lcd("mode off");
 drum(2);page(4);pot(3,15);rd();lcd("drum 3, pot 4");check("drum 3's page edits the same mode byte",r[LV+1]>0);int m3=r[LV+1];
 drum(1);page(4);lcd("drum 2 transient");
 pot(3,-200);rd();check("drum 2's page too",r[LV+1]==0&&m3>0);
 puts("== group knobs: drums 1+2 in unison");
 rd();memcpy(k,r,PARAMS);k[LV]=3;k[LV+1]=1;lxr_fw_edit(k);w(300);
 drum(0);page(0);lcd("drum 1 osc");
 rd();uint8_t b[PARAMS];memcpy(b,r,PARAMS);pot(0,20);rd();
 int d1=r[COARSE(0)]-b[COARSE(0)],d2=r[COARSE(1)]-b[COARSE(1)],d3=r[COARSE(2)]-b[COARSE(2)];printf("  coarse moved %d %d %d (from %d %d %d)\n",d1,d2,d3,b[COARSE(0)],b[COARSE(1)],b[COARSE(2)]);
 check("drum 2 moves by the same amount, drum 3 (unlinked) stays",d1>0&&d2==d1&&d3==0);
 check("the interval survives",r[COARSE(1)]-r[COARSE(0)]==b[COARSE(1)]-b[COARSE(0)]);
 for(int i=0;i<8&&r[COARSE(0)]<127;i++){pot(0,30);rd();}printf("  coarse after turning up: %d %d\n",r[COARSE(0)],r[COARSE(1)]);check("each stops at its own limit",r[COARSE(0)]==127&&r[COARSE(1)]==127);
 pot(0,-15);rd();printf("  and back down: %d %d\n",r[COARSE(0)],r[COARSE(1)]);check("both move down together from the limit",r[COARSE(0)]==r[COARSE(1)]&&r[COARSE(0)]<127);
 drum(1);page(1);rd();memcpy(b,r,PARAMS);pot(1,-20);rd();lcd("drum 2 aeg, pot 2");
 printf("  decay moved %d %d %d\n",r[DEC(0)]-b[DEC(0)],r[DEC(1)]-b[DEC(1)],r[DEC(2)]-b[DEC(2)]);check("an edit on drum 2's page moves drum 1 too",r[DEC(1)]<b[DEC(1)]&&r[DEC(0)]-b[DEC(0)]==r[DEC(1)]-b[DEC(1)]&&r[DEC(2)]==b[DEC(2)]);
 puts("== mix page: level, pan and outputs stay per drum, drive is shared");
 drum(0);page(6);lcd("drum 1 mix");rd();memcpy(b,r,PARAMS);
 pot(0,-20);rd();check("volume: drum 1 only",r[VOL(0)]!=b[VOL(0)]&&r[VOL(1)]==b[VOL(1)]);
 pot(1,20);rd();check("pan: drum 1 only",r[PAN(0)]!=b[PAN(0)]&&r[PAN(1)]==b[PAN(1)]);
 rd();memcpy(b,r,PARAMS);pot(3,30);rd();printf("  drive %d %d -> %d %d\n",b[DRIVE(0)],b[DRIVE(1)],r[DRIVE(0)],r[DRIVE(1)]);check("drive: both",r[DRIVE(0)]-b[DRIVE(0)]==r[DRIVE(1)]-b[DRIVE(1)]&&r[DRIVE(0)]!=b[DRIVE(0)]);
 puts("== encoder edits");
 drum(0);page(0);rd();memcpy(b,r,PARAMS);click();enc(-3);click();rd();lcd("encoder -3 on coarse");
 printf("  coarse %d %d -> %d %d\n",b[COARSE(0)],b[COARSE(1)],r[COARSE(0)],r[COARSE(1)]);check("the encoder moves both by the same amount",r[COARSE(0)]!=b[COARSE(0)]&&r[COARSE(0)]-b[COARSE(0)]==r[COARSE(1)]-b[COARSE(1)]);
 puts("== sync");
 rd();memcpy(k,r,PARAMS);k[COARSE(0)]=50;k[COARSE(1)]=70;k[DEC(0)]=33;k[DEC(1)]=99;lxr_fw_edit(k);w(300);
 drum(0);page(4);enc(4);lcd("drum 1 link screen 2");pot(2,40);rd();lcd("sync on");
 printf("  sync %d; coarse %d %d, decay %d %d, vol %d %d, pan %d %d\n",r[LV+4],r[COARSE(0)],r[COARSE(1)],r[DEC(0)],r[DEC(1)],r[VOL(0)],r[VOL(1)],r[PAN(0)],r[PAN(1)]);
 check("turning sync on copies drum 1's sound to drum 2",r[LV+4]==1&&r[COARSE(1)]==50&&r[DEC(1)]==33);
 check("level and pan stay",r[VOL(1)]==k[VOL(1)]&&r[PAN(1)]==k[PAN(1)]);
 check("drum 3 stays",r[COARSE(2)]==k[COARSE(2)]&&r[DEC(2)]==k[DEC(2)]);
 enc(-4);page(0);rd();memcpy(k,r,PARAMS);k[COARSE(1)]=90;lxr_fw_edit(k);w(300);pot(0,6);rd();lcd("osc, pot 1 with sync");
 printf("  coarse %d %d\n",r[COARSE(0)],r[COARSE(1)]);check("with sync an edit copies the value",r[COARSE(0)]==r[COARSE(1)]);
 puts("== no link, no group edits");
 rd();memcpy(k,r,PARAMS);k[LV+1]=0;k[LV+4]=0;lxr_fw_edit(k);w(300);rd();memcpy(b,r,PARAMS);pot(0,-10);rd();
 check("mode off: drum 1 only",r[COARSE(0)]!=b[COARSE(0)]&&r[COARSE(1)]==b[COARSE(1)]);
 rd();memcpy(k,r,PARAMS);k[LV]=1;k[LV+1]=1;lxr_fw_edit(k);w(300);rd();memcpy(b,r,PARAMS);pot(0,10);rd();
 check("one drum in the mask: drum 1 only",r[COARSE(0)]!=b[COARSE(0)]&&r[COARSE(1)]==b[COARSE(1)]);
 puts("== glide on the pitch mod page, screen 2 (0.5.4)");
 drum(1);page(2);enc(4);lcd("drum 2 pitch mod, 2nd screen");check("gld on screen 2",lcdHas("gld"));
 rd();memcpy(b,r,PARAMS);pot(0,30);rd();lcd("pot 1 up");{int n=0;for(int i=0;i<PARAMS;i++)if(r[i]!=b[i])n++;printf("  glide %d -> %d, %d bytes changed\n",b[331],r[331],n);check("pot 1 sets glide, nothing else",r[331]>b[331]&&n==1);}
 drum(0);page(2);enc(4);rd();int g1=r[331];pot(0,-200);rd();check("drum 1's page edits the same byte, down to 0",g1>0&&r[331]==0);
 puts("== glide type next to it (0.5.6)");
 lcd("drum 1 pitch mod, 2nd screen");check("typ on screen 2, lcr",lcdHas("typ")&&lcdHas("lcr"));
 pot(1,-200);rd();int t0=r[332];pot(1,200);rd();lcd("pot 2 up");int t1=r[332];check("pot 2 runs lcr to exp (0-2)",t0==0&&t1==2&&lcdHas("exp"));
 pot(1,-200);rd();check("back to lcr",r[332]==0);enc(-4);
 printf("%d failed\n",fails);puts("done");_exit(fails?2:0);}
