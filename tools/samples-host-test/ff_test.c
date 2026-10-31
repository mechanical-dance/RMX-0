// Real FatFs (the firmware's ff.c and ffconf.h) on a RAM disk: format, copy files in, run the installer.
// usage: ff_test host=card/PATH ...
#include <assert.h>
#include <setjmp.h>
#include <sys/mman.h>
#include "stm32f7xx_hal.h"
#include "fatfs.h"
#include "diskio.h"
#include "lcd.h"
#include "lxr0_samples.h"
struct fl flash_regs={FLASH_OPTCR_nDBANK};
FIL SDFile; FATFS SDFatFS; uint8_t lcd_process_display_interrupt; char lcd_last[64]; int htim2,hi2s2,hi2s3; int16_t dma_buffer[256],dma_buffer2[256];
int preset_saveTmpKit(void){FIL f;if(f_open(&f,"tmp.snd",FA_CREATE_ALWAYS|FA_WRITE))return 1;UINT w;f_write(&f,"x",1,&w);f_close(&f);return 0;}
void menu_lxr0RedrawScreen(void){}
static jmp_buf out; static uint8_t* fl; static uint8_t disk[64*1024*1024];
void NVIC_SystemReset(void){longjmp(out,1);}
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef*e,uint32_t*s){memset(fl,0xff,0x180000);return HAL_OK;}
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t t,uintptr_t a,uint64_t d){*(uint32_t*)a=(uint32_t)d;return HAL_OK;}
DSTATUS disk_initialize(BYTE d){return 0;} DSTATUS disk_status(BYTE d){return 0;}
DRESULT disk_read(BYTE d,BYTE*b,DWORD s,UINT c){memcpy(b,disk+s*512ull,c*512);return RES_OK;}
DRESULT disk_write(BYTE d,const BYTE*b,DWORD s,UINT c){memcpy(disk+s*512ull,b,c*512);return RES_OK;}
DRESULT disk_ioctl(BYTE d,BYTE c,void*b){if(c==GET_SECTOR_COUNT)*(DWORD*)b=sizeof(disk)/512;else if(c==GET_BLOCK_SIZE)*(DWORD*)b=1;else if(c==GET_SECTOR_SIZE)*(WORD*)b=512;return RES_OK;}
DWORD get_fattime(void){return 0;}
static void cp(const char*host,const char*card){FILE*h=fopen(host,"rb");FIL f;assert(h);assert(!f_open(&f,card,FA_CREATE_ALWAYS|FA_WRITE));static char b[8192];size_t n;UINT w;while((n=fread(b,1,8192,h))>0)assert(!f_write(&f,b,n,&w));f_close(&f);fclose(h);}
int main(int c,char**v){
 fl=mmap((void*)(uintptr_t)LXR0_SAMPLE_FLASH_BASE,0x180000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON|MAP_FIXED,-1,0);memset(fl,0xff,0x180000);
 static BYTE work[4096];assert(!f_mkfs("",FM_FAT32,0,work,sizeof work));assert(!f_mount(&SDFatFS,"",1));f_mkdir("samples");f_mkdir("loops");
 for(int i=1;i<c;i++){char*e=strchr(v[i],'=');*e=0;cp(v[i],e+1);}
 lcd_last[0]=0;int reset=setjmp(out);if(!reset)lxr0Samples_installFromSd();
 printf("reset=%d lcd: %s\n",reset,lcd_last);
 int n=lxr0Samples_init();printf("installed %d\n",n);
 for(int i=0;i<n;i++){const Lxr0Sample*s=lxr0Samples_get(6+i);printf("s%d %.8s frames=%u rate=%u loop=%u\n",i,s->name,s->frames,s->rate,s->looped);}
 return 0;}
