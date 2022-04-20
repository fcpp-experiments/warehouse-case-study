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
 * flash_mem_manager.h
 * 
 * \brief
 * Flash Memory Manager module Header File
 *
 * \ingroup FlashMemManager
 * 
 * \authors
 * Gabriele Tarantino, <ga.tarantino@reply.it>
 * \authors
 * Andres Munoz Herrera, <a.munozherrera@reply.it>
 */

#ifndef FLASHMEM_MANAGER_H
#define FLASHMEM_MANAGER_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    APPEND_MODE,    
    WRITE_MODE,     
    READ_MODE       
} mem_mode;

void init_flash_mem_manager(void);

/**
 * \retval      0         Contact successfulty read from mem.     
 * \retval      -1        Failed to read next contact ( \p p_cnt forced to NULL ).
 */
int read_round(char* into, int max_len);

void write_round(const char* data, int len);

/**
 * \brief       Open file stored in flash memory.
 * \param[in]   p_f_name            File name buffer.
 * \param[in]   f_name_size         File name size.
 * \param[in]   mode                Open mode.
 * \retval      n                   File descriptor.   
 * \retval      -1                  Failed to open file in the mode specified.
 */
int open_file(const char* p_f_name, int f_name_size, mem_mode mode);

/**
 * \brief       Close a file.
 * \param[in]   fd                  File descriptor.
 */
void close_file(int fd);

void close_file_forever(int fd);

#ifdef __cplusplus
}
#endif

#endif