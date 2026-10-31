#include <setjmp.h>
#include <math.h>
#include <assert.h>
#include <sys/mman.h>
#include "stm32f7xx_hal.h"
#include "fatfs.h"
#include "lcd.h"
#include "lxr0_samples.h"
#include "Soundengine/DSPAudio/Oscillator.h"
struct fl flash_regs={FLASH_OPTCR_nDBANK};
int tmp_kit_result, tmp_kit_saves; int preset_saveTmpKit(void){tmp_kit_saves++;return tmp_kit_result;} void menu_lxr0RedrawScreen(void){}
FIL SDFile; uint8_t lcd_process_display_interrupt; char lcd_last[64]; int htim2,hi2s2,hi2s3; int16_t dma_buffer[256],dma_buffer2[256];
static jmp_buf out; static uint8_t* fl; static const char* root;
void NVIC_SystemReset(void){longjmp(out,1);}
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef*e,uint32_t*s){assert(e->Sector==6&&e->NbSectors==6);memset(fl,0xff,0x180000);return HAL_OK;}
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t t,uintptr_t a,uint64_t d){uint32_t*p=(uint32_t*)(uintptr_t)a; assert(*p==0xffffffff); *p=(uint32_t)d; return HAL_OK;}
static void P(char*b,const char*p){snprintf(b,300,"%s%s",root,p);}
FRESULT f_open(FIL*f,const char*p,int m){char b[300];P(b,p);f->f=fopen(b,"rb");if(!f->f)return FR_ERR;fseek(f->f,0,SEEK_END);f->size=ftell(f->f);fseek(f->f,0,SEEK_SET);return FR_OK;}
FRESULT f_close(FIL*f){fclose(f->f);return FR_OK;}
FRESULT f_read(FIL*f,void*b,UINT n,UINT*r){*r=fread(b,1,n,f->f);return FR_OK;}
FRESULT f_lseek(FIL*f,uint32_t o){fseek(f->f,o,SEEK_SET);return FR_OK;}
FRESULT f_opendir(DIR*d,const char*p){char b[300];P(b,p);d->d=opendir(b);return d->d?FR_OK:FR_NO_PATH;}
FRESULT f_readdir(DIR*d,FILINFO*i){struct dirent*e;do{e=readdir(d->d);if(!e){i->fname[0]=0;return FR_OK;}}while(e->d_name[0]=='.');strncpy(i->fname,e->d_name,12);i->fname[12]=0;i->fattrib=0;return FR_OK;}
FRESULT f_closedir(DIR*d){closedir(d->d);return FR_OK;}
static void install(void){lcd_last[0]=0;if(!setjmp(out))lxr0Samples_installFromSd();printf("lcd: %s\n",lcd_last);}
int main(int c,char**v){
 root=v[1];
 fl=mmap((void*)(uintptr_t)LXR0_SAMPLE_FLASH_BASE,0x180000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON|MAP_FIXED,-1,0);
 assert((uintptr_t)fl==LXR0_SAMPLE_FLASH_BASE); memset(fl,0xff,0x180000);
 assert(lxr0Samples_init()==0); assert(!lxr0Samples_get(6));
 install();
 int n=lxr0Samples_init(); printf("count %d\n",n); assert(n==5);
 for(int i=0;i<n;i++){const Lxr0Sample*s=lxr0Samples_get(6+i);printf("%d %.8s frames=%u rate=%u loop=%u first=%d\n",i,s->name,s->frames,s->rate,s->looped,s->data[0]);}
 const Lxr0Sample*a=lxr0Samples_get(6),*b=lxr0Samples_get(7),*d=lxr0Samples_get(8),*z=lxr0Samples_get(9),*l=lxr0Samples_get(10);
 assert(!memcmp(a->name,"ALONG   ",8)&&!memcmp(b->name,"BSTER   ",8)&&!memcmp(d->name,"C24     ",8)&&!memcmp(z->name,"ZKICK   ",8)&&!memcmp(l->name,"LOOP    ",8));
 assert(a->frames==100000&&b->frames==1001&&b->rate==22050&&d->frames==500&&d->rate==48000&&l->looped==1&&a->looped==0);
 int bexp=(16383+(-8191))/2; printf("stereo sum %d expect ~%d; pad %d %d\n",b->data[500],bexp,b->data[1001],b->data[1002]); assert(abs(b->data[500]-bexp)<=1&&b->data[1001]==0&&b->data[1002]==0);
 printf("24 bit %d expect ~8191\n",d->data[100]); assert(abs(d->data[100]-8191)<=1);
 assert(!(a->frames&0) && ((uintptr_t)b->data&3)==0);
 // pitch: note F1, 44.1 k: one frame per output sample (scaled by 44100/44140)
 OscInfo o; memset(&o,0,sizeof o); o.freq=43.6535289291f;o.cvFreq=o.pitchMod=o.modNodeValue=1;o.waveform=6;
 osc_calcUserSampleFreq(&o); printf("inc 44.1k %u (%.4f frames/sample)\n",o.phaseInc,o.phaseInc/131072.);assert(fabs(o.phaseInc/131072.-1024*43.6535289291/44140.)<1e-3);
 o.waveform=7;osc_calcUserSampleFreq(&o);printf("inc 22.05k %.4f\n",o.phaseInc/131072.);assert(fabs(o.phaseInc/131072.-0.5*1024*43.6535289291/44140.)<1e-3);
 // long sample: past the old limit, position follows the ramp
 o.waveform=6;o.phaseInc=1u<<17;int16_t buf[64];long fr=0;
 for(;fr<70000;fr+=64)calcUserSampleOscBlock(&o,buf,64,1.f);
 printf("pos at 70016: hi=%u val=%d want=%d\n",o.sampleHi,buf[63],a->data[70015]);assert(o.sampleHi==2&&buf[63]==a->data[70015]);
 for(;fr<100032;fr+=64)calcUserSampleOscBlock(&o,buf,64,1.f); assert(buf[63]==0);
 // one shot holds at end, retrigger restarts
 calcUserSampleOscBlock(&o,buf,64,1.f); assert(buf[0]==0&&buf[63]==0);
 o.phase=0;o.sampleHi=0;calcUserSampleOscBlock(&o,buf,64,1.f);assert(buf[1]==a->data[1]);
 // loop wraps: 100 frames, inc 3.5
 o.waveform=10;o.phase=0;o.sampleHi=0;o.phaseInc=(uint32_t)(3.5*131072);for(int i=0;i<20;i++)calcUserSampleOscBlock(&o,buf,64,1.f);
 uint64_t p=((uint64_t)o.sampleHi<<32)|o.phase; printf("loop pos %.1f\n",p/131072.);assert(p<(100ull<<17));
 double want=fmod(20*64*3.5,100); assert(fabs(p/131072.-want)<1e-6);
 // FM stays in range; out-of-range wave is silent
 int16_t mod[64];for(int i=0;i<64;i++)mod[i]=3000;o.fmMod=2.f;o.waveform=6;o.phase=0;o.sampleHi=0;o.phaseInc=1<<17;calcUserSampleOscFmBlock(&o,mod,buf,64,1.f);
 o.waveform=11;memset(buf,0x55,sizeof buf);calcUserSampleOscBlock(&o,buf,64,1.f);assert(buf[0]==0&&buf[63]==0);o.waveform=200;calcUserSampleOscFmBlock(&o,mod,buf,64,1.f);assert(buf[0]==0);
 // kit can't be saved: nothing happens, no reset
 uint32_t m0=*(uint32_t*)LXR0_SAMPLE_FLASH_BASE; tmp_kit_result=1; lcd_last[0]=0; lxr0Samples_installFromSd(); printf("lcd: %s\n",lcd_last); assert(!strncmp(lcd_last,"Kit not saved",13)&&*(uint32_t*)LXR0_SAMPLE_FLASH_BASE==m0&&lxr0Samples_init()==5); tmp_kit_result=0;
 assert(tmp_kit_saves>=1);
 // no usable files: flash stays
 uint32_t m=*(uint32_t*)LXR0_SAMPLE_FLASH_BASE; root=v[2]; install(); assert(*(uint32_t*)LXR0_SAMPLE_FLASH_BASE==m&&lxr0Samples_init()==5);
 // dual bank refuses
 root=v[1]; flash_regs.OPTCR=0; install(); assert(lxr0Samples_init()==5); flash_regs.OPTCR=FLASH_OPTCR_nDBANK;
 // too much: 2 big files fit only partly
 root=v[3]; install(); n=lxr0Samples_init(); printf("overflow count %d\n",n); assert(n==1);
 puts("ALL PASS");return 0;}
