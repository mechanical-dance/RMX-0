// 0.4.13: retrigger clicks and trim fades. Renders block by block on this thread, so a trigger lands on a known sample.
// Run: LXR0_DIR=<state> ./retrig_test <dylib> <kit.SND> <card folder> <out dir>
// The card folder needs samples/SIN100.WAV, samples/MODBASS.WAV, loops/SINLP.WAV, loops/AMENX4.WAV (see make_tones.py).
// id_*.raw are for comparing builds with cmp; *.wav are for listening.
#include <dlfcn.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define PARAMS 332 // 0.5.0; older engines read only the first 325
#define BL 64
#define MAXN (44100*8)
#define TS 313
#define TE 319
#define S(t,n,...) t (*n)(__VA_ARGS__)
void *h;
S(void,lxr_init,void);S(void,lxr_render,int16_t[4][64]);S(void,lxr_hit,int,uint8_t);S(void,lxr_fw_load_kit,const uint8_t*,int,const uint8_t*);S(void,lxr_fw_edit,const uint8_t*);
S(void,lxr_fw_sample_root,const char*);S(void,lxr_fw_install_samples,void);S(int,lxr_fw_install_state,char*);S(int,lxr_fw_sample_count,void);S(void,lxr_fw_sample_info,int,char*,uint32_t*,uint32_t*,uint32_t*);
#define F(n) n = dlsym(h,#n); if(!n){printf("missing %s\n",#n);return 1;}
static int16_t cap[4][MAXN]; static long capN; static int capOn; static const char *out;
static const char *stage="boot"; static int16_t prevB[4][64]; static long dropped; static int same;
// a late firmware replays its last block (hw0.c): that isn't a new block, so it isn't captured. A long run of them is a stall.
static void rb(int n){int16_t j[4][64];for(int b=0;b<n;b++){lxr_render(j);int nz=0;for(int c=0;c<4&&!nz;c++)for(int i=1;i<64;i++)if(j[c][i]!=j[c][0]){nz=1;break;} /* a constant block can be real: a settled tail */int rep=nz&&!memcmp(j,prevB,sizeof j);memcpy(prevB,j,sizeof j);
  if(rep){dropped++;if(strcmp(stage,"boot")&&++same==200){char c[700];printf("STALL: stage [%s], captured %ld, sampling stacks\n",stage,capN);snprintf(c,sizeof c,"sample %d 3 -file %s/stall.txt >/dev/null 2>&1",getpid(),out);system(c);puts("stall stacks saved");_exit(4);}usleep(1200);continue;}same=0;
  if(capOn&&capN+BL<=MAXN){for(int c=0;c<4;c++)memcpy(&cap[c][capN],j[c],sizeof j[c]);capN+=BL;}usleep(1200);}}
static void start(void){capN=0;capOn=1;}
// two blocks are already rendered or queued when a hit arrives: its trigger starts the second block from here
static long hit(int v){lxr_hit(v,127);usleep(300);return capN+2*BL;}
static int loud(void){int best=0;long bv=-1;for(int c=0;c<4;c++){long e=0;for(long i=0;i<capN;i++)e+=abs(cap[c][i]);if(e>bv){bv=e;best=c;}}return best;}
static void wav(const char *name,int c){char p[600];snprintf(p,sizeof p,"%s/%s.wav",out,name);FILE*f=fopen(p,"wb");uint32_t d=capN*2,r=44100,br=88200,sz=36+d,fs=16;uint16_t fmt=1,ch=1,ba=2,bits=16;fwrite("RIFF",1,4,f);fwrite(&sz,4,1,f);fwrite("WAVEfmt ",1,8,f);fwrite(&fs,4,1,f);fwrite(&fmt,2,1,f);fwrite(&ch,2,1,f);fwrite(&r,4,1,f);fwrite(&br,4,1,f);fwrite(&ba,2,1,f);fwrite(&bits,2,1,f);fwrite("data",1,4,f);fwrite(&d,4,1,f);fwrite(cap[c],2,capN,f);fclose(f);}
static void raw(const char *name){char p[600];snprintf(p,sizeof p,"%s/id_%s.raw",out,name);FILE*f=fopen(p,"wb");for(int c=0;c<4;c++)fwrite(cap[c],2,capN,f);fclose(f);}
static int maxd(const int16_t *x,long a,long b){int m=0;if(a<1)a=1;if(b>capN)b=capN;for(long i=a;i<b;i++){int d=abs(x[i]-x[i-1]);if(d>m)m=d;}return m;}
static int peak(const int16_t *x,long a,long b){int m=0;if(a<0)a=0;if(b>capN)b=capN;for(long i=a;i<b;i++)if(abs(x[i])>m)m=abs(x[i]);return m;}
static double hz(const int16_t *x,long a,long b){int zc=0,pv=0;for(long n=a;n<b&&n<capN;n++){int y=x[n];if((y>40&&pv<=0)||(y<-40&&pv>=0)){zc++;pv=y;}}return zc/2.0/((b-a)/44100.0);}
static void clean(uint8_t *k){
 static const int zero[]={197,198,199,200,201,202,234,235,236,238,239,240,241,242,301,302,303,304,305,306,307,28,29,30,31,32,33,34,35,36,74,75,76,77,82,84,86,102,103,104,105,106,107,0};
 for(int i=0;zero[i];i++)k[zero[i]]=0;
 for(int i=88;i<94;i++)k[i]=110;for(int i=37;i<43;i++)k[i]=127;for(int i=43;i<49;i++)k[i]=0;for(int i=108;i<115;i++)k[i]=127;
 for(int i=121;i<127;i++)k[i]=0;for(int i=128;i<134;i++)k[i]=0;for(int i=143;i<149;i++)k[i]=0;for(int i=265;i<283;i++)k[i]=0;
 for(int i=TS;i<TS+12;i++)k[i]=0;
 static const int pan[]={94,95,96,99,100,101};for(int i=0;i<6;i++)k[pan[i]]=63;
}
static void edit(uint8_t *k){lxr_fw_edit(k);rb(120);}
static const int WAVE[]={1,2,3,4,6,7},COARSE[]={8,10,12,14,16,18},ATK[]={49,51,53,55,57,59},DEC[]={50,52,54,56,58,60},VO[]={0,1,2,3,4,5};
// end whatever a voice is playing, so the next hit really is from silence
static const int DEC2[]={50,52,54,56,58,60};
static void quiet(uint8_t *k,int v){static uint8_t t[PARAMS];memcpy(t,k,PARAMS);if(v>=5){t[60]=t[61]=2;}else t[DEC2[v]]=2;lxr_fw_edit(t);rb(120);if(v>=5){hit(5);}rb(330);int16_t j[4][64];lxr_render(j);int p=0;for(int c=0;c<4;c++)for(int i=0;i<64;i++)if(abs(j[c][i])>p)p=abs(j[c][i]);usleep(1200);if(p)printf("  (not silent before hit: voice %d peak %d)\n",v,p);lxr_fw_edit(k);rb(120);}
// a voice's own note from silence: largest jump in its first 3 ms, and over the note
static int own(uint8_t *k,int v,int *first){quiet(k,v);start();long t=hit(v);rb(260);int c=loud();*first=maxd(cap[c],t,t+132);int m=maxd(cap[c],t,capN);capOn=0;return m;}
// sounding note, retriggered at n points in its cycle; worst jump on the trigger sample and in the 6 ms after
static void sweep(const char *name,int v,int n,int spaced,int own1,int own2,const char *save){
 int w0=0,w1=0,lv=0,wo=-1;start();
 long tr[40];hit(v);rb(40);
 for(int i=0;i<n;i++){rb(spaced+(i%14));tr[i]=hit(v);}
 rb(80);int c=loud();
 for(int i=0;i<n;i++){long t=tr[i];int j0=abs(cap[c][t]-cap[c][t-1]);int j1=maxd(cap[c],t,t+264);if(j0>w0){w0=j0;wo=i;}if(j1>w1)w1=j1;int l=abs(cap[c][t-1]);if(l>lv)lv=l;}
 printf("%-26s jack %d: trigger-sample jump %6d  next 6 ms %6d  | level before %6d | silence: first 3 ms %5d, whole note %5d\n",name,c,w0,w1,lv,own1,own2);
 if(save)wav(save,c);capOn=0;}
int main(int ac,char**v){setvbuf(stdout,0,_IONBF,0);if(ac<5){puts("args");return 1;}out=v[4];
 h=dlopen(v[1],RTLD_NOW);if(!h){puts(dlerror());return 1;}
 F(lxr_init);F(lxr_render);F(lxr_hit);F(lxr_fw_load_kit);F(lxr_fw_edit);F(lxr_fw_sample_root);F(lxr_fw_install_samples);F(lxr_fw_install_state);F(lxr_fw_sample_count);F(lxr_fw_sample_info);
 static uint8_t a[400],k[PARAMS];FILE*f=fopen(v[2],"rb");if(!f){perror(v[2]);return 1;}int na=fread(a,1,400,f);fclose(f);memcpy(k,a+8,na-8);
 lxr_init();lxr_fw_sample_root(v[3]);rb(50);lxr_fw_load_kit(a,na,k);rb(2500);
 lxr_fw_install_samples();char m[33]={0};int s=0;while(lxr_fw_install_state(m)!=2&&s<80){rb(350);s++;}printf("install [%s]\n",m);rb(3500);
 int ns=lxr_fw_sample_count(),sOne=-1,sLoop=-1,sBass=-1,sAmen=-1;uint32_t fOne=0,fLoop=0;
 for(int i=0;i<ns;i++){char n[9]={0};uint32_t fr,r,l;lxr_fw_sample_info(i,n,&fr,&r,&l);for(int e=7;e>=0&&n[e]==32;e--)n[e]=0;printf(" s%d [%s] %u frames rate %u looped %u\n",i,n,fr,r,l);if(!strcmp(n,"SIN100")){sOne=i;fOne=fr;}if(!strcmp(n,"SINLP")){sLoop=i;fLoop=fr;}if(!strcmp(n,"MODBASS"))sBass=i;if(!strcmp(n,"AMENX4"))sAmen=i;}
 if(sOne<0||sLoop<0||sBass<0||sAmen<0){puts("samples missing");_exit(1);}
 stage="identity";
 puts("== identity: first hits, kit as stored but with no noise sources (the RNG's start differs per run)");
 {static uint8_t q[PARAMS];memcpy(q,k,PARAMS);static const int wv[]={1,2,3,4,6,7,20,21,22,24,25,26,27,0};for(int i=0;wv[i];i++)if(q[wv[i]]==4)q[wv[i]]=0;q[28]=0;q[32]=0;q[36]=0;lxr_fw_edit(q);rb(300);start();for(int i=0;i<6;i++){hit(i);rb(700);}rb(1400);hit(6);rb(700);raw("kit_nonoise");capOn=0;lxr_fw_edit(k);rb(1400);}
 if(getenv("KIT_ID_ONLY")){puts("done");_exit(0);}
 puts("== identity: first hits, kit as stored, all voices, then the open hat after a long gap");
 start();for(int i=0;i<6;i++){hit(i);rb(700);}rb(1400);hit(6);rb(700);raw("kit_all");capOn=0;
 clean(k);
 puts("== identity: clean kit, sines from silence, each voice");
 for(int i=0;i<6;i++){k[WAVE[i]]=0;k[COARSE[i]]=40;k[ATK[i]]=0;k[DEC[i]]=40;}k[61]=40;edit(k);rb(700);
 start();for(int i=0;i<6;i++){hit(VO[i]);rb(500);}raw("sine_all");capOn=0;
 puts("== identity: untrimmed samples from silence, and a hit after a one shot ended with the envelope still open");
 k[WAVE[0]]=6+sOne;k[COARSE[0]]=63;k[9]=63;k[DEC[0]]=127;k[ATK[0]]=0;edit(k);quiet(k,0);
 start();hit(0);rb(700);double one=hz(cap[loud()],4410,30000);hit(0);rb(800);raw("oneshot_untrimmed");printf("SIN100 at raw coarse 63, fine 63: %.1f Hz (file is 100 Hz)\n",one);capOn=0;
 k[WAVE[0]]=6+sLoop;edit(k);quiet(k,0);start();hit(0);rb(900);raw("loop_untrimmed");capOn=0;
 k[WAVE[0]]=6+sAmen;edit(k);quiet(k,0);start();hit(0);rb(900);raw("amen_untrimmed");capOn=0;
 k[WAVE[0]]=6+sBass;edit(k);quiet(k,0);start();hit(0);rb(700);raw("modbass_untrimmed");capOn=0;
 stage="retrigger";
 puts("== retrigger: long-decay sine, drum 1");
 k[WAVE[0]]=0;k[DEC[0]]=127;k[ATK[0]]=0;
 static const int cs[]={28,31,35};int o1,o2;
 for(int i=0;i<3;i++){k[COARSE[0]]=cs[i];edit(k);quiet(k,0);start();hit(0);rb(700);double d=hz(cap[loud()],4410,capN);capOn=0;o2=own(k,0,&o1);char nm[32],sv[32];snprintf(nm,sizeof nm,"sine %.0f Hz",d);snprintf(sv,sizeof sv,"retrig_sine_%.0fhz",d);sweep(nm,0,28,24,o1,o2,sv);}
 k[COARSE[0]]=31;k[ATK[0]]=40;edit(k);o2=own(k,0,&o1);sweep("sine 49 Hz, attack 40",0,28,60,o1,o2,"retrig_sine_attack40");
 k[ATK[0]]=0;
 puts("== fade length on the sine, level at full scale of this note (superposed on the note from silence)");
 k[COARSE[0]]=28;edit(k);quiet(k,0);start();long t=hit(0);rb(120);{int c=loud();int L=peak(cap[c],t,capN);int N[]={44,88,132,176,220,0};for(int n=0;N[n];n++){int w=0;for(int sg=-1;sg<=1;sg+=2){int prev=sg*L;for(long i=0;i<400&&t+i<capN;i++){double x=(i+1.0)/(N[n]+1);double g=x>=1?0:1-x*x*(3-2*x);int y=cap[c][t+i]+(int)(sg*L*g);if(abs(y-prev)>w)w=abs(y-prev);prev=y;}}printf(" %3d samples (%.1f ms): largest jump %d (note alone %d)\n",N[n],N[n]/44.1,w,maxd(cap[c],t,capN));}}capOn=0;
 puts("== retrigger: MODBASS bassline, coarse 0, drum 1");
 k[WAVE[0]]=6+sBass;k[COARSE[0]]=63;k[9]=63;k[DEC[0]]=127;k[ATK[0]]=0;edit(k);o2=own(k,0,&o1);sweep("MODBASS attack 0",0,16,110,o1,o2,"bassline_attack0");
 k[ATK[0]]=20;edit(k);o2=own(k,0,&o1);sweep("MODBASS attack 20",0,16,110,o1,o2,"bassline_attack20");k[ATK[0]]=0;
 puts("== retrigger: resonant filter, drum 1 (the SVF_reset question)");
 k[WAVE[0]]=2;k[COARSE[0]]=36;k[37]=50;k[43]=120;k[DEC[0]]=127;edit(k);o2=own(k,0,&o1);sweep("saw, LP cutoff 50, reso 120",0,28,24,o1,o2,"retrig_resonant");k[37]=127;k[43]=0;
 puts("== retrigger: decimated, drum 1");
 k[WAVE[0]]=0;k[COARSE[0]]=31;k[108]=100;edit(k);o2=own(k,0,&o1);sweep("sine 49 Hz, decimation 100",0,28,24,o1,o2,NULL);k[108]=127;
 puts("== retrigger: other voices, sine at coarse 31, long decay");
 static const char*nv[]={"drum 2","drum 3","snare","cymbal"};
 k[COARSE[0]]=40;k[DEC[0]]=20;k[24]=k[25]=0;edit(k);
 for(int i=1;i<5;i++){k[WAVE[i]]=0;k[COARSE[i]]=31;k[DEC[i]]=127;k[ATK[i]]=0;edit(k);o2=own(k,VO[i],&o1);sweep(nv[i-1],VO[i],28,24,o1,o2,NULL);k[DEC[i]]=20;k[COARSE[i]]=40;edit(k);}
 k[WAVE[5]]=0;k[COARSE[5]]=31;k[60]=120;k[61]=120;edit(k);o2=own(k,5,&o1);sweep("closed hat",5,28,24,o1,o2,NULL);o2=own(k,6,&o1);sweep("open hat",6,28,24,o1,o2,NULL);k[60]=k[61]=20;edit(k);quiet(k,5);
 puts("== retrigger: rolls faster than the fade");
 k[WAVE[0]]=0;k[COARSE[0]]=31;k[DEC[0]]=127;edit(k);quiet(k,0);start();for(int i=0;i<40;i++){hit(0);rb(1);}rb(60);{int c=loud();printf("every block: peak %d, largest jump %d\n",peak(cap[c],0,capN),maxd(cap[c],0,capN));wav("roll",c);}capOn=0;
 stage="trims";
 puts("== trims: SIN100 one shot, start near a sine peak");
 k[WAVE[0]]=6+sOne;k[COARSE[0]]=63;k[9]=63;k[DEC[0]]=127;
 int ts=0;{double bd=9;for(int x=1;x<60;x++){double ph=fmod((double)((uint64_t)fOne*x/255u)/441.0,1.0);if(fabs(ph-0.25)<bd){bd=fabs(ph-0.25);ts=x;}}}
 edit(k);quiet(k,0);start();t=hit(0);rb(300);{int c=loud();int nat=maxd(cap[c],t,capN);printf("untrimmed: largest jump %d, peak %d\n",nat,peak(cap[c],t,capN));}capOn=0;
 k[TS]=ts;edit(k);quiet(k,0);start();t=hit(0);rb(200);{int c=loud();printf("start trim %d: first sample %d, largest jump in 1.5 ms %d, then %d, level after %d\n",ts,abs(cap[c][t]),maxd(cap[c],t,t+66),maxd(cap[c],t+66,capN),peak(cap[c],t+66,t+882));wav("trim_start",c);}capOn=0;
 k[TS]=0;k[TE]=100;edit(k);quiet(k,0);start();t=hit(0);rb(600);{int c=loud();printf("end trim 100: largest jump %d, tail peak %d\n",maxd(cap[c],t,capN),peak(cap[c],capN-100,capN));wav("trim_end",c);}capOn=0;
 puts("== trims: SINLP loop");
 const int lt[][2]={{ts,0},{0,100},{ts,100},{1,1},{120,120}};
 for(int i=0;i<5;i++){k[WAVE[0]]=6+sLoop;k[TS]=lt[i][0];k[TE]=lt[i][1];edit(k);quiet(k,0);start();t=hit(0);rb(1000);{int c=loud();printf("loop start %3d end %3d: largest jump after 2 ms %d, peak %d\n",lt[i][0],lt[i][1],maxd(cap[c],t+88,capN),peak(cap[c],t+88,capN));char nm[32];snprintf(nm,sizeof nm,"trim_loop_%d_%d",lt[i][0],lt[i][1]);wav(nm,c);}capOn=0;}
 puts("== trims: for listening");
 k[WAVE[0]]=6+sAmen;k[TS]=40;k[TE]=60;edit(k);quiet(k,0);start();hit(0);rb(2200);wav("amen_trimmed",loud());capOn=0;
 k[WAVE[0]]=6+sBass;k[TS]=20;k[TE]=80;edit(k);o2=own(k,0,&o1);sweep("MODBASS trimmed",0,16,110,o1,o2,"bassline_trimmed");
 printf("replayed blocks dropped: %ld\n",dropped);
 puts("done");_exit(0);}
