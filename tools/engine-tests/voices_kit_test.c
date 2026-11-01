#include <dlfcn.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define PARAMS 325
#define TS 313
#define TE 319
void *h;
#define S(t,n,...) t (*n)(__VA_ARGS__)
S(void,lxr_init,void);S(void,lxr_render,int16_t[4][64]);S(void,lxr_hit,int,uint8_t);S(void,lxr_fw_load_kit,const uint8_t*,int,const uint8_t*);S(void,lxr_fw_edit,const uint8_t*);
S(int,lxr_fw_read_kit,uint8_t*,int);S(void,lxr_fw_sample_root,const char*);S(void,lxr_fw_install_samples,void);S(int,lxr_fw_install_state,char*);S(int,lxr_fw_boots,void);S(int,lxr_fw_sample_count,void);
S(void,lxr_fw_sample_info,int,char*,uint32_t*,uint32_t*,uint32_t*);S(void,lxr_fw_lcd,char*,uint8_t*);S(void,lxr_fw_button,int,int);S(void,lxr_fw_encoder,int);S(void,lxr_fw_encoder_button,int);S(void,lxr_fw_pot,int,int);S(void,lxr_set_verb,const uint8_t*);
#define F(n) n = dlsym(h,#n); if(!n){printf("missing %s\n",#n);return 1;}
static int16_t cap[44100*8]; volatile long capN; volatile int capOn; volatile int bp[4]; volatile int hzv[64],hzn,hzOn,hzC,hzF,hzP,hzL; volatile int lg[4000],logn,logOn,blk,blkp; volatile long total, lastLoud, loudOn; volatile int firstP, lastP;
void *aud(void *a){int16_t j[4][64];for(;;){lxr_render(j);int pk=0;for(int c=0;c<4;c++)for(int n=0;n<64;n++)if(abs(j[c][n])>pk)pk=abs(j[c][n]);for(int c=0;c<4;c++){int q=0;for(int n=0;n<64;n++)if(abs(j[c][n])>q)q=abs(j[c][n]);if(q>bp[c])bp[c]=q;}if(logOn&&logn<4000){if(pk>blkp)blkp=pk;if(++blk==7){lg[logn++]=blkp;blk=0;blkp=0;}}if(hzOn){for(int n=0;n<64;n++){int x=j[0][n];if(abs(x)>hzL)hzL=abs(x);if((x>40&&hzP<=0)||(x<-40&&hzP>=0)){hzC++;hzP=x;}}hzF+=64;if(hzF>=11025){if(hzn<64)hzv[hzn++]=hzL>100?hzC*2:0;hzF=0;hzC=0;hzL=0;}}if(capOn)for(int n=0;n<64&&capN<44100*8;n++)cap[capN++]=j[0][n];total+=64;if(pk>30){if(!loudOn)loudOn=total;lastLoud=total;lastP=pk;if(total-loudOn<640&&pk>firstP)firstP=pk;}usleep(1200);}}
void w(int ms){usleep(ms*1000);}
void lcd(const char *t){char s[33];uint8_t cg[64];lxr_fw_lcd(s,cg);char o[33];for(int i=0;i<32;i++)o[i]=((uint8_t)s[i]<32||(uint8_t)s[i]>126)?'.':s[i];o[32]=0;printf("%-22s [%.16s] [%.16s]\n",t,o,o+16);}
void row(char *o){uint8_t cg[64];char s[33];lxr_fw_lcd(s,cg);memcpy(o,s,32);o[32]=0;}
void tap(int b){lxr_fw_button(b,1);w(60);lxr_fw_button(b,0);w(120);}
void click(void){lxr_fw_encoder_button(1);w(60);lxr_fw_encoder_button(0);w(120);}
int diff(uint8_t*a,uint8_t*b){int at=-1;for(int i=0;i<PARAMS;i++)if(a[i]!=b[i]){if(at<0)at=i;else printf("  also par %d %d->%d\n",i,a[i],b[i]);}return at;}
long len(int v){total=0;lastLoud=0;loudOn=0;firstP=0;lastP=0;lxr_hit(v,127);w(3000);return loudOn?lastLoud-loudOn:0;}
void trace(const char*t,int hit){w(500);printf("%s:\n",t);if(hit)lxr_hit(0,127);for(int i=0;i<6;i++){bp[0]=bp[1]=bp[2]=bp[3]=0;w(200);printf("  %4d ms: %6d %6d %6d %6d\n",200*(i+1),bp[0],bp[1],bp[2],bp[3]);}}
int rd(const char*p,uint8_t*b){FILE*f=fopen(p,"rb");if(!f){perror(p);exit(1);}int n=fread(b,1,400,f);fclose(f);return n;}
int main(int c,char**v){setvbuf(stdout,0,_IONBF,0);
 h=dlopen(v[1],RTLD_NOW);if(!h){puts(dlerror());return 1;}
 F(lxr_init);F(lxr_render);F(lxr_hit);F(lxr_fw_load_kit);F(lxr_fw_edit);F(lxr_fw_read_kit);F(lxr_fw_sample_root);F(lxr_fw_install_samples);F(lxr_fw_install_state);F(lxr_fw_boots);F(lxr_fw_sample_count);F(lxr_fw_sample_info);F(lxr_fw_lcd);F(lxr_fw_button);F(lxr_fw_encoder);F(lxr_fw_encoder_button);F(lxr_fw_pot);F(lxr_set_verb);
 uint8_t a[400]={0},k[PARAMS]={0},r0[PARAMS],r1[PARAMS];int na=rd(v[2],a);memcpy(k,a+8,na-8);printf("kit file %d bytes\n",na);
 lxr_init();lxr_fw_sample_root(v[3]);lxr_fw_load_kit(a,na,k);pthread_t t;pthread_create(&t,0,aud,0);w(4000);
 int b0=lxr_fw_boots();lxr_fw_install_samples();char m[33]={0};int st,s=0;while((st=lxr_fw_install_state(m))!=2&&s<120){w(500);s++;}printf("install [%s]\n",m);w(5000);printf("boots %d->%d samples %d\n",b0,lxr_fw_boots(),lxr_fw_sample_count());
 int ns=lxr_fw_sample_count(),best=0;uint32_t bf=0;for(int i=0;i<ns;i++){char n[9]={0};uint32_t f,r,l;lxr_fw_sample_info(i,n,&f,&r,&l);printf(" %d [%s] %u frames looped %u\n",i,n,f,l);if(!l&&f>bf){bf=f;best=i;}}
 lcd("start");
 lxr_fw_read_kit(k,0);
 static const int off[]={197,198,199,200,201,202,234,235,236,238,239,240,241,242,0};for(int i=0;off[i];i++)k[off[i]]=0;
 for(int i=88;i<94;i++)k[i]=110;for(int i=37;i<43;i++)k[i]=127;for(int i=43;i<49;i++)k[i]=0;k[31]=k[32]=k[35]=k[36]=0;
 for(int m=28;m<31;m++)k[m]=0;for(int m=33;m<37;m++)k[m]=0;lxr_fw_edit(k);w(300);
 static const int wv[]={4,6,7,7};static const int vo[]={3,4,5,6};static const int tv[]={3,4,5,5};
 static const int dc[]={56,58,60,61};
 for(int i=0;i<4;i++){uint8_t was=k[wv[i]],wd=k[dc[i]];k[wv[i]]=7;k[dc[i]]=127;
  for(int x=0;x<3;x++){k[TS+tv[i]]=x==1?128:x==2?200:0;k[TE+tv[i]]=x==2?200:0;lxr_fw_edit(k);w(200);capN=0;capOn=1;long t0=total;lxr_hit(vo[i],127);w(1500);capOn=0;long e=0;for(long n=0;n<capN;n++)e+=abs(cap[n]);
   printf("voice %d wave par %d start %3d end %3d: rendered %ld, mean %ld, Hz/250ms:",vo[i],wv[i],k[TS+tv[i]],k[TE+tv[i]],(long)(total-t0),capN?e/capN:0);for(long o=0;o+11025<=capN&&o<5*11025;o+=11025){int zc=0,pv=0,pk=0;for(long n=o;n<o+11025;n++){int y=cap[n];if(abs(y)>pk)pk=abs(y);if((y>40&&pv<=0)||(y<-40&&pv>=0)){zc++;pv=y;}}printf(" %d",pk>100?zc*2:0);}puts("");}
  k[wv[i]]=was;k[dc[i]]=wd;k[TS+tv[i]]=0;k[TE+tv[i]]=0;lxr_fw_edit(k);w(100);}
 puts("== kit file round trip");
 k[TS]=33;k[TS+3]=44;k[TE]=55;k[TE+5]=66;lxr_fw_edit(k);w(200);memset(r0,0,PARAMS);int ok=lxr_fw_read_kit(r0,0);printf("read ok %d: %d %d %d %d\n",ok,r0[TS],r0[TS+3],r0[TE],r0[TE+5]);
 uint8_t f[8+PARAMS];memcpy(f,a,8);memcpy(f+8,r0,PARAMS);lxr_fw_load_kit(f,8+PARAMS,r0);w(300);ok=lxr_fw_read_kit(r1,0);int bad=0;for(int i=0;i<PARAMS;i++)if(r1[i]!=r0[i])bad++;printf("333-byte reload (read ok %d): %d bytes differ\n",ok,bad);
 static const int sizes[]={8+313,8+301,255};for(int i=0;i<3;i++){memset(f,0xAA,sizeof f);memcpy(f,a,8);memcpy(f+8,r0,sizes[i]-8);uint8_t p[PARAMS];memset(p,0,PARAMS);memcpy(p,f+8,sizes[i]-8);lxr_fw_load_kit(f,sizes[i],p);w(300);memset(r1,0x55,PARAMS);ok=lxr_fw_read_kit(r1,0);int z=0;for(int j=TS;j<TS+12;j++)z+=r1[j];printf("%d-byte kit (read ok %d): trim sum %d, verb amount %d\n",sizes[i],ok,z,r1[307]);}
 _exit(0);}
