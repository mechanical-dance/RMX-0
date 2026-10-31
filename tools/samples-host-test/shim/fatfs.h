#pragma once
#include <stdint.h>
#include <stdio.h>
#include <dirent.h>
typedef unsigned UINT;
typedef enum {FR_OK,FR_ERR,FR_NO_FILE,FR_NO_PATH} FRESULT;
typedef struct {FILE*f; uint32_t size;} FIL;
typedef struct {DIR*d; char p[300];} XDIR;
#define DIR XDIR
typedef struct {char fname[13]; uint8_t fattrib;} FILINFO;
#define AM_DIR 0x10
#define AM_HID 0x02
#define FA_READ 1
extern FIL SDFile;
FRESULT f_open(FIL*,const char*,int); FRESULT f_close(FIL*);
FRESULT f_read(FIL*,void*,UINT,UINT*); FRESULT f_lseek(FIL*,uint32_t);
#define f_tell(fp) ((uint32_t)ftell((fp)->f))
#define f_size(fp) ((fp)->size)
FRESULT f_opendir(DIR*,const char*); FRESULT f_readdir(DIR*,FILINFO*); FRESULT f_closedir(DIR*);
