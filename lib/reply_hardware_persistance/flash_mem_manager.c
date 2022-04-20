/*
 * Copyright (c) 2020, Concept Reply
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are not permitted without an explicit written authorization 
 * by Reply S.p.A..
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
 * This file is part of the Covid Tool Firmware.
 *
 */
/**
 * \file
 * flash_mem_manager.c
 * 
 * \brief
 * Flash Memory Manager module Source File
 *
 * \ingroup FlashMemManager
 * 
 * \authors
 * Gabriele Tarantino, <ga.tarantino@reply.it>
 * \authors
 * Andres Munoz Herrera, <a.munozherrera@reply.it>
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "flash_mem_manager.h"
#include "cfs-xmem.h"
#include "xmem.h"
#include "SPI_mem_manager.h" // JUST FOR DEBUG

#include "sys/log.h"

#define LOG_MODULE "FLASH-MEM"
#ifdef FLASH_MEM_LOG_LEVEL_CONF
#define LOG_LEVEL FLASH_MEM_LOG_LEVEL_CONF
#else
#define LOG_LEVEL LOG_LEVEL_ERR
#endif

static int file_closed_forever = 0;

void init_flash_mem_manager(void) {
    xmem_init();
}


void write_round(const char* data, int len)
{
    short s = len;
    char size[2];
    size[0] = (s & 0xFF00) >> 8;
    size[1] = s;
    LOG_DBG("write_round, size %d[%d,%d]\n", len, size[0], size[1]);
    if (cfs_write(1, size, 2) < 2 || cfs_write(1, data, len) < len)
    {
        LOG_ERR("round data writing failed\n");
    }
    else
    {
        LOG_DBG("round data dumped\n");
    }
}

int read_round(char* into, int max_len)
{
    char len_bytes[2];
    if (cfs_read(1, len_bytes, 2) == 2)
    {
        short len = (len_bytes[0] << 8) + len_bytes[1];
        LOG_DBG("read_round, size %d[%d,%d]\n", len, len_bytes[0], len_bytes[1]);
        if (len > max_len) {
            len = max_len;
        }
        if (len > 0 && cfs_read(1, into, len)) {
            LOG_DBG("round data read\n");
            return len;
        }
        else
        {
            LOG_DBG("Error reading round data\n");
            return -1;
        }
    } 
    else
    {
        LOG_DBG("Error reading round data length\n");
        return -1;
    }
}
/************************************************************************/
int open_file(const char *p_f_name, int f_name_size, mem_mode mode)
{
    if (file_closed_forever) {
        return -1;
    }
    int fd;
    char fn[f_name_size];
    memset(fn, 0, f_name_size);
    snprintf(fn, f_name_size, "%s", p_f_name);

    char fn_d[f_name_size + 1];
    memset(fn_d, 0, f_name_size + 1);
    snprintf(fn_d, f_name_size + 1, "%s", p_f_name);

    LOG_INFO("Opening file %s in mode %d.\n", p_f_name, mode);

    switch (mode)
    {
    case READ_MODE:
        fd = cfs_open(fn, CFS_READ);
        if (fd < 0)
        {
            LOG_ERR("could not open file for reading, aborting\n");
            fd = -1;
        }
        break;

    case WRITE_MODE:
        fd = cfs_open(fn, CFS_WRITE);
        if (fd < 0)
        {
            LOG_ERR("could not open file for writing, aborting\n");
            fd = -1;
        }
        break;

    case APPEND_MODE:
        fd = cfs_open(fn, CFS_APPEND);
        if (fd < 0)
        {
            LOG_ERR("could not open file for writing, aborting\n");
            fd = -1;
        }
        break;

    default:
        LOG_ERR("Open mode not accepted (0 for reaad, 1 for write), aborting\n");
        fd = -1;
        break;
    }

    cfs_seek(fd, 0, CFS_SEEK_SET);

    return fd;
}
/************************************************************************/

/************************************************************************/
void close_file(int fd)
{
    cfs_close(fd);
}
/************************************************************************/

void close_file_forever(int fd)
{
    close_file(fd);
    file_closed_forever = 1;
}
