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
 * SPI_mem_manager.h
 * 
 * \brief
 * SPI Memory Manager module Header File
 * 
 * Flash memory used: W25Q80DV/DL.
 *
 * \ingroup SPIMemManager
 * 
 * \authors
 * Gabriele Tarantino, <ga.tarantino@reply.it>
 * \authors
 * Andres Munoz Herrera, <a.munozherrera@reply.it>
 */

#ifndef SPI_MEM_MANAGER_H
#define SPI_MEM_MANAGER_H

#include "mem-arch.h"
#include "contiki.h" 


/**
 * \def CMD_SIZE
 * \brief Size (in bytes) of CMDs to control SPI flash memory.
 */
#define CMD_SIZE 1

/**
 * \def ADDR_SIZE
 * \brief Size (in bytes) of the address for SPI flash memory operations.
 */
#define ADDR_SIZE 3

// COMMANDS FOR THE FLASH MEMORY W25Q80DV/DL

/**
 * \def WRITE_EN
 * \brief Write Enable CMD.
 */
#define WRITE_EN        0x06

/**
 * \def CHIP_ERASE
 * \brief Chip Erase CMD.
 */
#define CHIP_ERASE      0xC7

/**
 * \def SEC_ERASE
 * \brief 4 KBytes Sector Erase CMD.
 */
#define SEC_ERASE       0x20

/**
 * \def BLOCK32_ERASE
 * \brief 32 KBytes Block Erase CMD.
 */
#define BLOCK32_ERASE   0x52

/**
 * \def READ_DATA
 * \brief Read Data CMD.
 */
#define READ_DATA       0x03

/**
 * \def PAGE_PRG
 * \brief Page Program CMD.
 */
#define PAGE_PRG        0x02

/**
 * \def POWER_DOWN
 * \brief Power Down CMD.
 */
#define POWER_DOWN      0xB9

/**
 * \def REL_POWER_DOWN
 * \brief Relase Power Down CMD.
 */
#define REL_POWER_DOWN  0xAB

/**
 * \def READ_SR
 * \brief Read Status Register CMD.
 */
#define READ_SR         0x05

// #define POWER_DOWN_ON 
// #define DEBUG_MODE

/**
 * \brief Write a page in SPI flash memory.
 * \param[in] addr          Address in bytes.
 * \param[in] num_bytes     Number of bytes to write.
 * \param[in] tx_buf        buffer containing data to write.
 * 
 * \note A Write Enable operation has to be done every time a write/erase operation want to be executed. 
 */
void SPI_page_program(uint8_t *addr, uint32_t num_bytes, uint8_t *tx_buf);

/**
 * \brief Write a page in SPI flash memory (input address as unsigned long).
 * \param[in] lu_addr       Address.
 * \param[in] num_bytes     Number of bytes to write.
 * \param[in] tx_buf        buffer containing data to write.
 * 
 * \note A Write Enable operation has to be done every time a write/erase operation want to be executed.  
 */
void SPI_page_program_int_off(unsigned long lu_addr, uint32_t num_bytes, uint8_t *tx_buf);

/**
 * \brief Chip Erase operation of the SPI flash memory.
 * Delete all data stored in the SPI flash memory. 
 * 
 * \note A Write Enable operation has to be done every time a write/erase operation want to be executed. 
 */
void SPI_chip_erase();

/**
 * \brief Read data from SPI flash memory.
 * \param[in] addr          Address in bytes.
 * \param[in] num_bytes     Number of bytes to write.
 * \param[out] rx_buf        buffer containing read.
 * 
 * \note A Write Enable operation has to be done every time a write/erase operation want to be executed.  
 */
void SPI_read_data(uint8_t *addr, uint32_t num_bytes, uint8_t *rx_buf);

/**
 * \brief Read data from SPI flash memory (input address as unsigned long).
 * \param[in] lu_addr       Address.
 * \param[in] num_bytes     Number of bytes to write.
 * \param[out] rx_buf       buffer containing read.
 */
void SPI_read_data_int_off(unsigned long lu_addr, uint32_t num_bytes, uint8_t *rx_buf);

/**
 * \brief Write Enable operation on SPI flash memory.
 * \note A Write Enable operation has to be done every time a write/erase operation want to be executed.  
 */
void SPI_write_en();

/**
 * \brief 4 KBytes Sector Erase operation on SPI flash memory.
 * \param[in]   addr    Sector to Erase Address.
 * \note A Write Enable operation has to be done every time a write/erase operation want to be executed.  
 */
void SPI_sector_erase(uint8_t *addr);

/**
 * \brief 32 KBytes Block Erase operation on SPI flash memory.
 * \param[in]   addr    Block to Erase Address.
 * \note A Write Enable operation has to be done every time a write/erase operation want to be executed.  
 */
void SPI_block32_erase(uint8_t *addr);

/**
 * \brief SPI pheriperal initialization.
 */
void SPI_init();

/**
 * \brief Set SPI flash memory in power down mode.
 * \note    Not enabled in current fw version.
 */
void SPI_power_down();

/**
 * \brief Release SPI flash memory from power down mode.
 * \note    Not enabled in current fw version.
 */
void SPI_release_power_down();

/**
 * \brief Check SPI flash memory Status Register to know if memory is busy.
 * \return    Memory busy.
 */
bool SPI_busy();

/**
 * \brief Read SPI flash memory Status Register.
 * \return    Status Register content.
 */
unsigned char SPI_read_SR();

#endif