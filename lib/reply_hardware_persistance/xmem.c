/*
 * Copyright (c) 2006, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <string.h>

#include "contiki.h"
#include "dev/spi.h"
#include "dev/xmem.h"
#include "dev/watchdog.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "SPI_mem_manager.h"
#include "sys/log.h"

#define LOG_MODULE "XMEM"
#ifdef XMEM_LOG_LEVEL_CONF
#define LOG_LEVEL XMEM_LOG_LEVEL_CONF
#else
#define LOG_LEVEL LOG_LEVEL_ERR
#endif


#define XMEM_ERASE_UNIT_SIZE (32 * 1024L)


#if 1
//#define PRINTF(...) NRF_LOG_DEBUG(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif

// #define  SPI_FLASH_INS_WREN        0x06
// #define  SPI_FLASH_INS_WRDI        0x04
// #define  SPI_FLASH_INS_RDSR        0x05
// #define  SPI_FLASH_INS_WRSR        0x01
// #define  SPI_FLASH_INS_READ        0x03
// #define  SPI_FLASH_INS_FAST_READ   0x0b
// #define  SPI_FLASH_INS_PP          0x02
// #define  SPI_FLASH_INS_SE          0xd8
// #define  SPI_FLASH_INS_BE          0xc7
// #define  SPI_FLASH_INS_DP          0xb9
// #define  SPI_FLASH_INS_RES         0xab
/*---------------------------------------------------------------------------*/
static void
write_enable(void)
{
  
  SPI_write_en();

}
/*---------------------------------------------------------------------------*/
static unsigned
read_status_register(void)
{

  // NRF_LOG_DEBUG("Read SR");
  return SPI_read_SR();

}
/*---------------------------------------------------------------------------*/
/*
 * Wait for a write/erase operation to finish.
 */
static unsigned
wait_ready(void)
{
  // NRF_LOG_DEBUG("Wait ready");
  unsigned u;
  do {
    u = read_status_register();
    watchdog_periodic();
  } while(u & 0x01);		/* WIP=1, write in progress */
  return u;
}
/*---------------------------------------------------------------------------*/
/*
 * Erase 64k bytes of data. It takes about 1s before WIP goes low!
 */
static void
erase_sector(unsigned long offset)
{
  // ERASE SECTOR
  /*
  wait_ready();

  write_enable();

  uint8_t addr[3];
  addr[2] = offset >> 0; // LSB
  addr[1] = offset >> 8;
  addr[0] = offset >> 16;

  // for (int j = 0; j < 3; j++)
  // {
  //   printf("%X ", addr[j]);
  // }
  // printf("\n\r");

  SPI_sector_erase(addr);
  */

  // BLOCK ERASE
  
  wait_ready();

  write_enable();

  uint8_t addr[3];
  addr[2] = offset >> 0; // LSB
  addr[1] = offset >> 8;
  addr[0] = offset >> 16;

  LOG_INFO("Erasing sector at addr %X %X %X", addr[2], addr[1], addr[0]);

  SPI_block32_erase(addr);
  
}
/*---------------------------------------------------------------------------*/
/*
 * Initialize external flash *and* SPI bus!
 */
void
xmem_init(void)
{

  LOG_INFO("xmem init");
  SPI_init();

}
/*---------------------------------------------------------------------------*/
int
xmem_pread(void *_p, int size, unsigned long offset)
{

  unsigned char *p = _p;
  wait_ready();

  uint8_t addr[3];
  addr[2] = offset >> 0; // LSB
  addr[1] = offset >> 8;
  addr[0] = offset >> 16;

  SPI_read_data( addr, size, (uint8_t*) p );

  //ENERGEST_OFF(ENERGEST_TYPE_FLASH_READ);

  return size;
}
/*---------------------------------------------------------------------------*/
static const unsigned char *
program_page(unsigned long offset, const unsigned char *p, int nbytes)
{

  wait_ready();
  write_enable();

  uint8_t addr[3];
  addr[2] = offset >> 0; // LSB
  addr[1] = offset >> 8;
  addr[0] = offset >> 16;
  SPI_page_program(addr, (uint32_t)nbytes, (uint8_t*)p );

  return p;
}
/*---------------------------------------------------------------------------*/
int
xmem_pwrite(const void *_buf, int size, unsigned long addr)
{

  const unsigned char *p = _buf;
  const unsigned long end = addr + size;
  unsigned long i, next_page;

  //ENERGEST_ON(ENERGEST_TYPE_FLASH_WRITE);
  
  for(i = addr; i < end;) {
    next_page = (i | 0xff) + 1;
    if(next_page > end) {
      next_page = end;
    }
    p = program_page(i, p, next_page - i);
    i = next_page;
  }

  //ENERGEST_OFF(ENERGEST_TYPE_FLASH_WRITE);

  return size;
}
/*---------------------------------------------------------------------------*/
int
xmem_erase(long size, unsigned long addr)
{
  printf("xmem erase \n");
  // NRF_LOG_DEBUG("xmem erase");
  unsigned long end = addr + size;

  if(size % XMEM_ERASE_UNIT_SIZE != 0) {
    LOG_ERR("xmem_erase: bad size\n");
    return -1;
  }

  if(addr % XMEM_ERASE_UNIT_SIZE != 0) {
    LOG_ERR("xmem_erase: bad offset\n");
    return -1;
  }

  for (; addr < end; addr += XMEM_ERASE_UNIT_SIZE) {
    erase_sector(addr);
  }

  return size;
}
/*---------------------------------------------------------------------------*/
