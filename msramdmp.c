/*
 * msramdmp - McGrew Security Ram Dumper 
 * Version 0.5.1
 *
 * Robert Wesley McGrew - wesley@mcgrewsecurity.com
 * 
 * Homepage, docs, etc: http://mcgrewsecurity.com/projects/msramdmp/
 *
 * Portions of the code are adapted from H. Peter Anvin's com32 code
 * in SYSLINUX.  I especially got a good start on the extended int 13h
 * writes from the code in the chainloader module.
 * 
 * As such, this code is also free software under the GNU General Public
 * License, Version 2.  I would appreciate an email if you do something 
 * cool with it, or if you have any questions.
 *
 */
#include <com32.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <console.h>

#define SECTOR 512		/* bytes/sector */
static inline void memset(void *buf, int ch, unsigned int len)
{
  asm volatile("cld; rep; stosb"
               : "+D" (buf), "+c" (len) : "a" (ch) : "memory");
}

int int13_retry(const com32sys_t *inreg, com32sys_t *outreg)
{
  int retry = 6;		/* Number of retries */
  com32sys_t tmpregs;

  if ( !outreg ) outreg = &tmpregs;

  while ( retry-- ) {
    __com32.cs_intcall(0x13, inreg, outreg);
    if ( !(outreg->eflags.l & EFLAGS_CF) )
      return 0;			/* CF=0, OK */
  }

  return -1;			/* Error */
}

/*
 * Get a disk block; buf is REQUIRED TO BE IN LOW MEMORY.
 */
struct ebios_dapa {
  uint16_t len;
  uint16_t count;
  uint16_t off;
  uint16_t seg;
  uint64_t lba;
} *dapa;

int read_sector(void *buf, unsigned int lba, uint8_t drive, uint16_t num)
{
  com32sys_t inreg;

  memset(&inreg, 0, sizeof inreg);

  dapa->len = sizeof(*dapa);
  dapa->count = num;
  dapa->off = OFFS(buf);
  dapa->seg = SEG(buf);
  dapa->lba = lba;

  inreg.esi.w[0] = OFFS(dapa);
  inreg.ds       = SEG(dapa);
  inreg.edx.b[0] = drive; 
  inreg.eax.b[1] = 0x42;	/* Extended read */

  return int13_retry(&inreg, NULL);
}

int write_sector(void *buf, unsigned int lba, uint8_t drive, uint16_t num)
{
  com32sys_t inreg;

  memset(&inreg, 0, sizeof inreg);

  dapa->len = sizeof(*dapa);
  dapa->count = num;
  dapa->off = OFFS(buf);
  dapa->seg = SEG(buf);
  dapa->lba = lba;

  inreg.esi.w[0] = OFFS(dapa);
  inreg.ds       = SEG(dapa);
  inreg.edx.b[0] = drive;
  inreg.eax.b[1] = 0x43; /* Extended write */

  return int13_retry(&inreg, NULL);
}

int __start(void)
{
  char* buf = NULL;
  char* buf2 = NULL;
  dapa = (struct ebios_dapa *)__com32.cs_bounce;
  buf = (char *)__com32.cs_bounce + SECTOR;
  buf2 = (char *)__com32.cs_bounce + (SECTOR * 2);
  int drive = 0;
  int start_addr = 0;
  int part_size = 0;

  uint8_t i = 0;
  uint32_t j = 0;
  int found = 0;
  int cur_sector = 0;
  uint32_t ptr;
  uint32_t memtop;

  com32sys_t inreg, outreg;
  
  printf("\n\n-----------------------------------------------\n");
  printf("msramdmp - McGrew Security Ram Dumper - v 0.5.1\n");
  printf("http://mcgrewsecurity.com/projects/msramdmp/\n");
  printf("Robert Wesley McGrew: wesley@mcgrewsecurity.com\n");
  printf("-----------------------------------------------\n\n");
  
  for(i=0x80;i<=0x81;i++)
  {
    if(read_sector(buf,0,i,1))
    {
      continue;
    }
    for(j=0;j<4;j++)
    {
      if(buf[(j*0x10)+0x1c2]==(char)0x40)
      {
        printf("Found msramdmp partition at disk 0x%x : partition %d\n",i,j+1);
        printf("Partition isn't marked as used.  Using it.\n");
        drive = i;
        start_addr = *((uint32_t*)(buf+0x1be +(j*(0x10)+8)));
        part_size = *((uint32_t*)(buf+0x1be +(j*(0x10)+12)));
        found = 1;
        break;
      }
    }
    if(found)
    {
      break;
    }
  }

  if(!found)
  {
    printf("No unused msramdmp partitions found, quitting.\n");
    return 1;
  }

  buf[(j*0x10)+0x1c2] = 0x41;
  
  if(write_sector(buf,0,drive,1))
  {
    printf("Could not mark msramdmp partition as used.  Something's wrong.\n");
    return 1;
  }
  else
  {
    printf("Marked partition as used.\n");
  }

  memset(&inreg,0,sizeof(inreg));
  memset(&outreg,0,sizeof(outreg));

  inreg.eax.w[0] = 0xe801;
  
  __com32.cs_intcall(0x15,&inreg,&outreg);

  memtop = outreg.eax.w[0]*1024 + outreg.ebx.w[0]*65536 + 2162688;

  printf("Writing section from 0x00000000 to 0x0009FFFF\n");
  ptr = 0x00000000;
  cur_sector = start_addr;  
  while(ptr < 0x0009FFFF)
  {
    for(j=0;(j<8192)&&(j<(0x0009FFFF-ptr));j++)
    {
      buf[j] = *((char*)(ptr+j));
    }
    while(j<8192)
    {
      buf[j] = (char)0x00;
      j++;
    }
    if(write_sector(buf,cur_sector,drive,16))
    {
      printf("Could not write to disk.  Quitting.\n");
      return 1;
    }
    cur_sector+=16;
    if(cur_sector >= (start_addr+part_size))
    {
      printf("Out of space on the msramdmp partition.  Quitting.\n");
      return 1;
    }
    ptr += 8192;
  }
  
  printf("Writing section from 0x00100000 to 0x%x\n",memtop);
  ptr = 0x00100000;
  while(ptr < memtop)
  {
    for(j=0;(j<8192)&&(j<(memtop-ptr));j++)
    {
      buf[j] = *((char*)(ptr+j));
    }
    while(j<8192)
    {
      buf[j] = (char)0x00;
      j++;
    }
    if(write_sector(buf,cur_sector,drive,16))
    {
      printf("Could not write to disk.  Quitting.\n");
      return 1;
    }
    cur_sector+=16;
    if(cur_sector >= (start_addr+part_size))
    {
      printf("Out of space on the msramdmp partition.  Quitting.\n");
      return 1;
    }
    ptr += 8192;
  }

  printf("Done! You can turn off the machine and remove your drive.\n");
 
  return 0;
}
