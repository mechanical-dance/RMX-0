#include <dlfcn.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
void *h;
#define S(t,n,...) t (*n)(__VA_ARGS__)
S(void,lxr_init,void);S(void,lxr_render,int16_t[4][64]);S(void,lxr_hit,int,uint8_t);S(void,lxr_fw_load_kit,const uint8_t*,int,const uint8_t*);S(void,lxr_fw_edit,const uint8_t*);S(void,lxr_fw_sample_root,const char*);S(void,lxr_fw_install_samples,void);S(int,lxr_fw_install_state,char*);S(int,lxr_fw_sample_count,void);S(void,lxr_fw_sample_info,int,char*,uint32_t*,uint32_t*,uint32_t*);
#define F(n) n = dlsym(h,#n); if(!n){printf("missing %s\n",#n);return 1;}
static int16_t cap[44100]; volatile long capN; volatile int capOn;
void *aud(void *a){int16_t j[4][64];for(;;){lxr_render(j);if(capOn)for(int n=0;n<64&&capN<44100;n++)cap[capN++]=j[0][n];usleep(1200);}}
void w(int ms){usleep(ms*1000);}
double hz(void){int zc=0,pv=0;long a=2205,b=capN;for(long n=a;n<b;n++){int x=cap[n];if((x>40&&pv<=0)||(x<-40&&pv>=0)){zc++;pv=x;}}return b>a?zc/2.0/((b-a)/44100.0):0;}
int main(int c,char**v){setvbuf(stdout,0,_IONBF,0);h=dlopen(v[1],RTLD_NOW);if(!h){puts(dlerror());return 1;}
 F(lxr_init);F(lxr_render);F(lxr_hit);F(lxr_fw_load_kit);F(lxr_fw_edit);F(lxr_fw_sample_root);F(lxr_fw_install_samples);F(lxr_fw_install_state);F(lxr_fw_sample_count);F(lxr_fw_sample_info);
 uint8_t a[400]={0},k[400]={0};FILE*f=fopen(v[2],"rb");int na=fread(a,1,400,f);fclose(f);memcpy(k,a+8,na-8);
 lxr_init();lxr_fw_sample_root(v[3]);lxr_fw_load_kit(a,na,k);pthread_t t;pthread_create(&t,0,aud,0);w(4000);
 lxr_fw_install_samples();char m[33]={0};int s=0;while(lxr_fw_install_state(m)!=2&&s<120){w(500);s++;}printf("install [%s]\n",m);w(5000);printf("samples %d\n",lxr_fw_sample_count());
 static const int off[]={82,74,102,197,121,234,235,236,238,239,240,241,242,0};for(int i=0;off[i];i++)k[off[i]]=0;k[37]=127;k[43]=0;k[128]=0;k[88]=110;k[35]=k[36]=0;k[50]=127;k[62]=127;k[9]=63;k[20]=0;for(int i=115;i<121;i++)k[i]=0;
 static const int co[]={22,40,63,64,76,100};
 for(int wv=0;wv<2;wv++){k[1]=wv?6:0;for(int i=0;i<6;i++){k[8]=co[i];lxr_fw_edit(k);w(250);capN=0;capOn=1;lxr_hit(0,127);w(1000);capOn=0;double d=hz();printf("%s coarse %3d: %8.1f Hz%s",wv?"sample":"sine  ",co[i],d,wv?"":"\n");if(wv)printf("  speed %.2fx\n",d/441.0);}}
 _exit(0);}
