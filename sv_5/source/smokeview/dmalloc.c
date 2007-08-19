#include "options.h"
#ifdef pp_MEM2
#define INDMALLOC
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "MALLOC.h"
#include "ASSERT.h"
#ifdef _DEBUG
static int checkmemoryflag=1;
#endif
#ifdef WIN32
#include <windows.h>
#endif

#ifdef pp_memstatus
#ifdef WIN32
void _memorystatus(unsigned int size,unsigned int *availmem,unsigned int *physmemused, unsigned int *totalmem){
  MEMORYSTATUS stat;

    GlobalMemoryStatus (&stat);
    if(availmem!=NULL)*availmem=stat.dwMemoryLoad;
    if(totalmem!=NULL)*totalmem=stat.dwTotalPhys/(1024*1024);
    if(physmemused!=NULL)*physmemused=(stat.dwTotalPhys-stat.dwAvailPhys)/(1024*1024);
#ifdef _DEBUG
    if(size!=0&&size<=stat.dwAvailPhys-0.1*stat.dwTotalPhys){
      printf("*** Available Memory: %i M \n",
            stat.dwAvailPhys/(1024*1024));
    }
#endif
    if(size!=0&&size>stat.dwAvailPhys-0.1*stat.dwTotalPhys){
      printf("*** Low Memory Warning. Only %i M available for viewing data.\n",
            stat.dwAvailPhys/(1024*1024));
      printf("    Unload datafiles or system performance may degrade.\n");
    }
}
#endif
#endif

#ifdef pp_MEM2
void initMM(void){
  MMfirst=malloc(sizeof(MMdata));
  MMlast=malloc(sizeof(MMdata));

  MMfirst->prev=NULL;
  MMfirst->next=MMlast;

  MMlast->prev=MMfirst;
  MMlast->next=NULL;
}
#endif

/* ------------------ _NewMemory ------------------------ */

mallocflag _NewMemory(void **ppv, size_t size){
  void **ppb=(void **)ppv;
#ifdef _DEBUG
  char *c;
#endif
#ifdef pp_MEM2
  MMdata *this_ptr, *prev_ptr, *next_ptr;
  MMdata *infoblock;
  int infoblocksize;
#endif

  ASSERT(ppv != NULL && size != 0);
#ifdef pp_MEM2
  infoblocksize=(sizeof(MMdata)+3)/4;
  infoblocksize*=4;

  this_ptr = (void *)malloc(infoblocksize+size+sizeofDebugByte);
  if(this_ptr!=NULL){
    infoblock=(MMdata *)this_ptr;
    prev_ptr=MMfirst;
    next_ptr=MMfirst->next;

    prev_ptr->next=this_ptr;
    next_ptr->prev=this_ptr;

    infoblock->prev=prev_ptr;
    infoblock->next=next_ptr;
    infoblock->marker=markerByte;
    *ppb=this_ptr+infoblocksize;
  }
  else{
    *ppb=NULL;
  }
#else
  *ppb = (void *)malloc(size+sizeofDebugByte);
#endif

#ifdef _DEBUG
  {
    CheckMemory;
    if(*ppb != NULL){
      if(sizeofDebugByte!=0){
       c = (char *)(*ppb) + size;
       *c=(char)debugByte;
      }
      memset(*ppb, memGarbage, size);
      if(!CreateBlockInfo(*ppb, size)){
        free(*ppb);
        *ppb=NULL;
      }
    }
    ASSERT(*ppb !=NULL);
  }
#endif
  return (*ppb != NULL);
}

/* ------------------ FreeMemory ------------------------ */

void FreeMemory(void *pv){
#ifdef _DEBUG
  int len_memory;
#endif
  int infoblocksize;
#ifdef pp_MEM2
  MMdata *this_ptr, *prev_ptr, *next_ptr;
#endif

  ASSERT(pv != NULL);
#ifdef pp_MEM2
  infoblocksize=(sizeof(MMdata)+3)/4;
  infoblocksize*=4;
#else
  infoblocksize=0;
#endif
#ifdef _DEBUG
  {
    CheckMemory;
    len_memory=sizeofBlock((char *)pv+infoblocksize);
    memset((char *)pv+infoblocksize, memGarbage, len_memory);
    FreeBlockInfo((char *)pv+infoblocksize);
  }
#endif
#ifdef pp_MEM2
  if(this_ptr->marker==markerByte){
    this_ptr=(MMdata *)((char *)pv-infoblocksize);

    prev_ptr=this_ptr->prev;
    next_ptr=this_ptr->next;

    prev_ptr->next=next_ptr;
    next_ptr->prev=prev_ptr;
  }
#endif
  free((char *)pv-infoblocksize);
}

/* ------------------ ResizeMemory ------------------------ */

mallocflag _ResizeMemory(void **ppv, size_t sizeNew){
  bbyte **ppb = (bbyte **)ppv;
  bbyte *pbNew;
  int infoblocksize;
#ifdef pp_MEM2
  MMdata *this_ptr, *prev_ptr, *next_ptr;
#endif
#ifdef _DEBUG
  char *c;
  size_t sizeOld;
#endif

#ifdef pp_MEM2
  infoblocksize=(sizeof(MMdata)+3)/4;
  infoblocksize*=4;
#else
  infoblocksize=0;
#endif
  ASSERT(ppb != NULL && sizeNew != 0);
#ifdef _DEBUG
  {
    CheckMemory;
    sizeOld = sizeofBlock(*ppb);
    if(sizeNew<sizeOld){
      memset((*ppb)+sizeNew,memGarbage,sizeOld-sizeNew);
    }
    else if (sizeNew > sizeOld){
      void *pbForceNew;
      if(_NewMemory((void **)&pbForceNew, sizeNew)){
        memcpy(pbForceNew, *ppb, sizeOld);
        FreeMemory(*ppb);
        *ppb = pbForceNew;
      }
    }
  }
#endif

  pbNew = realloc(*ppb, infoblocksize+sizeNew+sizeofDebugByte);
#ifdef pp_MEM2
  if(pbNew!=*ppb){
    this_ptr=(MMdata *)(*ppb-infoblocksize);
    prev_ptr=this_ptr->prev;
    next_ptr=this_ptr->next;
    this_ptr=(MMdata *)pbNew;
    prev_ptr->next=this_ptr;
    next_ptr->prev=this_ptr;
    this_ptr->next=next_ptr;
    this_ptr->prev=prev_ptr;
    this_ptr->marker=markerByte;
  }
#endif
  if(pbNew != NULL){
#ifdef _DEBUG
    {
      if(sizeofDebugByte!=0){
        c = pbNew + sizeNew;
        *c=(char)debugByte;
      }
      UpdateBlockInfo(*ppb, pbNew, sizeNew);
      if(sizeNew>sizeOld){
        memset(pbNew+sizeOld,memGarbage,sizeNew-sizeOld);
      }
    }
#endif
    *ppb = pbNew+infoblocksize;
  }
  return (pbNew != NULL);
}

#ifdef _DEBUG
/* ------------------ pointer comparison defines ------------------------ */

#define fPtrLess(pLeft, pRight)   ((pLeft) <  (pRight))
#define fPtrGrtr(pLeft, pRight)   ((pLeft) >  (pRight))
#define fPtrEqual(pLeft, pRight)  ((pLeft) == (pRight))
#define fPtrLessEq(pLeft, pRight) ((pLeft) <= (pRight))
#define fPtrGrtrEq(pLeft, pRight) ((pLeft) >= (pRight))

static blockinfo *GetBlockInfo(bbyte *pb);
mallocflag __NewMemory(void **ppv, size_t size, char *varname, char *file, int linenumber){
  void **ppb=(void **)ppv;
  blockinfo *pbi;
  int return_code;
  char *varname2;
  char *file2;
  char ampersand='&';
#ifdef WIN32
  char dirsep='\\';
#else
  char dirsep='/';
#endif

  return_code=_NewMemory(ppb,size);
  pbi=GetBlockInfo((bbyte *)*ppb);
  pbi->linenumber=linenumber;

  file2=strrchr(file,dirsep);
  if(file2==NULL){
    file2=file;
  }
  else{
    file2++;
  }
  if(strlen(file2)<256){
    strcpy(pbi->filename,file2);
  }
  else{
    strncpy(pbi->filename,file2,255);
    strcat(pbi->filename,(char)0);
  }

  varname2 = strchr(varname,ampersand);
  if(varname2==NULL){
    varname2=varname;
  }
  else{
    varname2++;
  }
  if(strlen(varname2)<256){
    strcpy(pbi->varname,varname2);
  }
  else{
    strncpy(pbi->varname,varname2,255);
    strcat(pbi->varname,(char)0);
  }
  return return_code;
}

/* ------------------ ResizeMemory ------------------------ */

mallocflag __ResizeMemory(void **ppv, size_t size, char *varname, char *file, int linenumber){
  void **ppb=(void **)ppv;
  blockinfo *pbi;
  int return_code;

  return_code=_ResizeMemory(ppb,size);
  pbi=GetBlockInfo((bbyte *)*ppb);
  pbi->linenumber=linenumber;
  if(strlen(file)<256){
    strcpy(pbi->filename,file);
  }
  else{
    strncpy(pbi->filename,file,255);
    strcat(pbi->filename,(char)0);
  }
  if(strlen(varname)<256){
    strcpy(pbi->varname,varname);
  }
  else{
    strncpy(pbi->varname,varname,255);
    strcat(pbi->varname,(char)0);
  }
  return return_code;
}

static blockinfo *pbiHead = NULL;

/* ------------------ GetBlockInfo ------------------------ */

static blockinfo *GetBlockInfo(bbyte *pb){
  blockinfo *pbi;
  for (pbi = pbiHead; pbi != NULL; pbi = pbi->pbiNext)
  {
    bbyte *pbStart = pbi->pb;
    bbyte *pbEnd   = pbi->pb + pbi->size - 1;

    if(fPtrGrtrEq(pb, pbStart) && fPtrLessEq(pb, pbEnd))
      break;
  }
  ASSERT(pbi != NULL);
  return (pbi);
}

/* ------------------ GetMemoryInfo ------------------------ */

int _GGetMemoryInfo(void){
  blockinfo *pbi;
  int n=0,size=0;

  for (pbi = pbiHead; pbi != NULL; pbi = pbi->pbiNext)
  {
    n++;
    size += pbi->size;
  }
  return n;
}

/* ------------------ PrintMemoryInfo ------------------------ */

void _PrintMemoryInfo(void){
  blockinfo *pbi;
  int n=0,size=0;

  for (pbi = pbiHead; pbi != NULL; pbi = pbi->pbiNext)
  {
    n++;
    size += pbi->size;
  }
  printf("nblocks=%i sizeblocks=%i\n",n,size);
}


/* ------------------ PrintMemoryInfo ------------------------ */

void _PrintAllMemoryInfo(void){
  blockinfo *pbi;
  int n=0,size=0;

  printf("\n\n");
  printf("********************************************\n");
  printf("********************************************\n");
  printf("********************************************\n");
  for (pbi = pbiHead; pbi != NULL; pbi = pbi->pbiNext)
  {
    n++;
    size += pbi->size;
    printf("%s allocated in %s at line %i\n",pbi->varname,pbi->filename,pbi->linenumber);
  }
  printf("nblocks=%i sizeblocks=%i\n",n,size);
}

/* ------------------ GetBlockInfo_nofail ------------------------ */

static blockinfo *GetBlockInfo_nofail(bbyte *pb){
  blockinfo *pbi;
  for (pbi = pbiHead; pbi != NULL; pbi = pbi->pbiNext)
  {
    bbyte *pbStart = pbi->pb;
    bbyte *pbEnd   = pbi->pb + pbi->size - 1;

    if(fPtrGrtrEq(pb, pbStart) && fPtrLessEq(pb, pbEnd))
      break;
  }
  return (pbi);
}

/* ------------------ _CheckMemoryOn ------------------------ */

void _CheckMemoryOn(void){
  checkmemoryflag=1;
}

/* ------------------ _CheckMemoryOff ------------------------ */

void _CheckMemoryOff(void){
  checkmemoryflag=0;
}

/* ------------------ _CheckMemory ------------------------ */

void _CheckMemory(void){
  blockinfo *pbi;
  if(checkmemoryflag==0)return;
  for (pbi = pbiHead; pbi != NULL; pbi = pbi->pbiNext)
  {
  if(sizeofDebugByte!=0)ASSERT((char)*(pbi->pb+pbi->size)==(char)debugByte);
  }
  return;
}

/* ------------------ CreateBlockInfo ------------------------ */

mallocflag CreateBlockInfo(bbyte *pbNew, size_t sizeNew){
  blockinfo *pbi;

  ASSERT(pbNew != NULL && sizeNew != 0);

  pbi = (blockinfo *)malloc(sizeof(blockinfo));
  if( pbi != NULL){
    pbi->pb = pbNew;
    pbi->size = sizeNew;
    pbi->pbiNext = pbiHead;
    pbiHead = pbi;
  }
  return (mallocflag)(pbi != NULL);
}

/* ------------------ FreeBlockIfno ------------------------ */

void FreeBlockInfo(bbyte *pbToFree){
  blockinfo *pbi, *pbiPrev;

  pbiPrev = NULL;
  for (pbi = pbiHead; pbi != NULL; pbi = pbi->pbiNext){
    if(fPtrEqual(pbi->pb, pbToFree)){
      if(pbiPrev == NULL){
        pbiHead = pbi->pbiNext;
      }
      else{
        pbiPrev->pbiNext = pbi->pbiNext;
      }
      break;
    }
    pbiPrev = pbi;
  }
  ASSERT(pbi != NULL);
  if(sizeofDebugByte!=0)ASSERT((char)*(pbi->pb+pbi->size)==(char)debugByte);
  free(pbi);
}

/* ------------------ UpdateBlockInfo ------------------------ */

void UpdateBlockInfo(bbyte *pbOld, bbyte *pbNew, size_t sizeNew){
  blockinfo *pbi;

  ASSERT(pbNew != NULL && sizeNew != 0);

  pbi = GetBlockInfo(pbOld);
  ASSERT(pbOld == pbi->pb);

  pbi->pb = pbNew;
  pbi->size = sizeNew;
}

/* ------------------ sizeofBlock ------------------------ */

size_t sizeofBlock(bbyte *pb){
  blockinfo *pbi;

  pbi = GetBlockInfo(pb);
  ASSERT(pb==pbi->pb);
  if(sizeofDebugByte!=0)ASSERT((char)*(pbi->pb+pbi->size)==(char)debugByte);
  return(pbi->size);
}

/* ------------------ ValidPointer ------------------------ */

mallocflag ValidPointer(void *pv, size_t size){
  blockinfo *pbi;
  bbyte *pb = (bbyte *)pv;

  ASSERT(pv != NULL && size != 0);

  pbi = GetBlockInfo(pb);
  ASSERT(pb==pbi->pb);

  ASSERT(fPtrLessEq(pb+size,pbi->pb + pbi->size));

  if(sizeofDebugByte!=0)ASSERT((char)*(pbi->pb+pbi->size)==(char)debugByte);
  return(1);
}

/* ------------------ strcpy ------------------------ */

char *_strcpy(char *s1, const char *s2){
  blockinfo *pbi;
  int offset;
  CheckMemory;
  pbi = GetBlockInfo_nofail(s1);
  if(pbi!=NULL){
    offset = s1 - pbi->pb;
    ASSERT(pbi->size - offset >= strlen(s2)+1);
  }

  return strcpy(s1,s2);
}

/* ------------------ strcat ------------------------ */

char *_strcat(char *s1, const char *s2){
  blockinfo *pbi;
  int offset;
  CheckMemory;
  pbi = GetBlockInfo_nofail(s1);
  if(pbi!=NULL){
    offset = s1 - pbi->pb;
    ASSERT(pbi->size - offset >= strlen(s1)+strlen(s2)+1);
  }

  return strcat(s1,s2);
}
#endif
