/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
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
 * This file is part of the Contiki operating system.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */


#include "cfs/cfs.h"

#define XMEM_ERASE_UNIT_SIZE (32 * 1024L)

#define FILESIZE (32 * 1024L)

#ifdef CFS_XMEM_CONF_OFFSET
#define CFS_XMEM_OFFSET CFS_XMEM_CONF_OFFSET
#else
#define CFS_XMEM_OFFSET XMEM_ERASE_UNIT_SIZE*1
#endif

/* Note the CFS_XMEM_CONF_SIZE must be a tuple of XMEM_ERASE_UNIT_SIZE */
#ifdef CFS_XMEM_CONF_SIZE
#define CFS_XMEM_SIZE CFS_XMEM_CONF_SIZE
#else
#define CFS_XMEM_SIZE XMEM_ERASE_UNIT_SIZE
#endif

/*---------------------------------------------------------------------------*/
int cfs_open(const char *n, int f);
/*---------------------------------------------------------------------------*/
void cfs_close(int f);
/*---------------------------------------------------------------------------*/
int cfs_read(int f, void *buf, unsigned int len);
/*---------------------------------------------------------------------------*/
int cfs_write(int f, const void *buf, unsigned int len);
/*---------------------------------------------------------------------------*/
cfs_offset_t cfs_seek(int f, cfs_offset_t o, int w);
/*---------------------------------------------------------------------------*/
int cfs_remove(const char *name);
/*---------------------------------------------------------------------------*/
int cfs_opendir(struct cfs_dir *p, const char *n);
/*---------------------------------------------------------------------------*/
int cfs_readdir(struct cfs_dir *p, struct cfs_dirent *e);
/*---------------------------------------------------------------------------*/
void cfs_closedir(struct cfs_dir *p);
/*---------------------------------------------------------------------------*/
