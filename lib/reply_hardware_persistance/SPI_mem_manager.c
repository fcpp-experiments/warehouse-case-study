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
 * SPI_mem_manager.c
 * 
 * \brief
 * SPI Memory Manager module Source File
 *
 * \ingroup SPIMemManager
 * 
 * \authors
 * Gabriele Tarantino, <ga.tarantino@reply.it>
 * \authors
 * Andres Munoz Herrera, <a.munozherrera@reply.it>
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "SPI_mem_manager.h"
#include "mem-arch.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "nrf_delay.h"
#include "sys/log.h"

#define LOG_MODULE "SPI-MEM"
#ifdef SPI_MEM_MANAGER_LOG_LEVEL_CONF
#define LOG_LEVEL SPI_MEM_MANAGER_LOG_LEVEL_CONF
#else
#define LOG_LEVEL LOG_LEVEL_ERR
#endif

//#define DEBUG_MODE 1

void SPI_chip_erase()
{

    #ifdef POWER_DOWN_ON
        SPI_release_power_down();
    #endif

    #ifdef DEBUG_MODE
        LOG_INFO("*******Erasing chip**********\n\r");
    #endif

    // Write Enable cmd
    SPI_write_en();

    // Chip Erase erase
    uint8_t command = CHIP_ERASE; 
    memory_spi_transfer(&command, CMD_SIZE, NULL, 0);
    
    while(SPI_busy() == true)
    {
        
    }

    #ifdef POWER_DOWN_ON
        SPI_power_down();
    #endif
}

void SPI_sector_erase(uint8_t *addr)  // erase 4-KBytes
{
    #ifdef POWER_DOWN_ON
        SPI_release_power_down();
    #endif

    #ifdef DEBUG_MODE
        LOG_INFO("*******Erasing sector**********\n\r");
    #endif

    uint32_t tx_len = CMD_SIZE + ADDR_SIZE;
    uint8_t tx_buf[tx_len];
    memset(tx_buf, 0, tx_len);

    tx_buf[0] = SEC_ERASE;
    memcpy(&tx_buf[CMD_SIZE], addr, ADDR_SIZE);

    // #ifdef DEBUG_MODE 
    //     LOG_INFO("SPI_MEM", "TX buffer is: ");
    //     for (int j = 0; j < tx_len; j++)
    //     {
    //         printf("%X ", tx_buf[j]);
    //     }
    // #endif

    memory_spi_transfer(tx_buf, tx_len, NULL, 0);

    #ifdef POWER_DOWN_ON
        SPI_power_down();
    #endif

}

void SPI_read_data(uint8_t *addr, uint32_t num_bytes, uint8_t *rx_buf)
{

    #ifdef POWER_DOWN_ON    
        SPI_release_power_down();
    #endif

    #ifdef DEBUG_MODE
        // NRF_LOG_DEBUG("*******Reading data**********");
        LOG_INFO("*******Reading data**********\n\r");
    #endif

    uint32_t tx_len = CMD_SIZE + ADDR_SIZE;
    uint32_t rx_len = tx_len + num_bytes;
    uint8_t rx_tmp[rx_len];
    memset(rx_tmp, 0, sizeof(rx_tmp));
    
    uint8_t tx_buf[tx_len];
    memset(tx_buf, 0, tx_len);

    tx_buf[0] = READ_DATA; // Read data cmd
    memcpy(&tx_buf[CMD_SIZE], addr, ADDR_SIZE);
    
    memory_spi_transfer(tx_buf, tx_len, rx_tmp, rx_len);

    memcpy(rx_buf, &rx_tmp[tx_len], num_bytes);

    #ifdef POWER_DOWN_ON
        SPI_power_down();
    #endif
}


void SPI_read_data_int_off(unsigned long lu_addr, uint32_t num_bytes, uint8_t *rx_buf)
{

    uint8_t addr[3];
    memset(addr, 0 , sizeof(addr));

    memcpy(addr, &lu_addr, 3);
    uint8_t temp;
    temp = addr[2];
    addr[2] = addr[0];
    addr[0] = temp;

    // LOG_INFO("SPI_MEM", "Addr write: %02x %02x %02x\n", addr[2], addr[1], addr[0]);

    #ifdef POWER_DOWN_ON    
        SPI_release_power_down();
    #endif

    #ifdef DEBUG_MODE
        // NRF_LOG_DEBUG("*******Reading data**********");
        LOG_INFO("*******Reading data**********\n\r");
    #endif

    uint32_t tx_len = CMD_SIZE + ADDR_SIZE;
    uint32_t rx_len = tx_len + num_bytes;
    uint8_t rx_tmp[rx_len];
    memset(rx_tmp, 0, rx_len);
    
    uint8_t tx_buf[tx_len];
    memset(tx_buf, 0, tx_len);

    tx_buf[0] = READ_DATA; // Read data cmd
    memcpy(&tx_buf[CMD_SIZE], addr, ADDR_SIZE);
    
    memory_spi_transfer(tx_buf, tx_len, rx_tmp, rx_len);

    LOG_INFO("tx_buf of read len %lu: ", tx_len);
    for (int i = 0; i< tx_len; i++)
    {
        printf("%02X ", tx_buf[i]);
    }
    printf("\n\r");

    LOG_INFO("rx_tmp of read len %lu: ", rx_len);
    for (int i = 0; i< rx_len; i++)
    {
        printf("%02X ", rx_tmp[i]);
    }
    printf("\n\r");

    memcpy(rx_buf, &rx_tmp[tx_len], num_bytes);

    // LOG_INFO("SPI_MEM", "rx_buf of read len %d: ", num_bytes);
    // for (int i = 0; i< num_bytes; i++)
    // {
    //     printf("%02X ", rx_buf[i]);
    // }
    // printf("\n\r");

    

    #ifdef POWER_DOWN_ON
        SPI_power_down();
    #endif
}

void SPI_page_program(uint8_t *addr, uint32_t num_bytes, uint8_t *tx_buf)

{
    #ifdef POWER_DOWN_ON
        SPI_release_power_down();
    #endif

    // Write Enable cmd

    #ifdef DEBUG_MODE
        // NRF_LOG_DEBUG("*******Programming page**********");
        LOG_INFO("*******Programming page**********\n\r");
    #endif


    uint32_t tx_len = num_bytes + ADDR_SIZE + CMD_SIZE;

    uint8_t tx_buffer[tx_len];
    memset(tx_buffer, 0, tx_len); 

    tx_buffer[0] = PAGE_PRG; 
    memcpy(&tx_buffer[CMD_SIZE], addr, ADDR_SIZE);
    
    memcpy(&tx_buffer[ADDR_SIZE + CMD_SIZE], tx_buf, num_bytes);

    #ifdef DEBUG_MODE
        LOG_INFO("Program page TX buffer is: ");
        for (int j = 0; j < tx_len; j++)
        {
            printf("%X ", tx_buffer[j]);
        }
    #endif

    memory_spi_transfer(tx_buffer, tx_len, NULL, 0);

    // int t0 = clock_time();

    while(SPI_busy() == true)
    {
        
    }

    #ifdef POWER_DOWN_ON
        SPI_power_down();
    #endif

}

void SPI_page_program_int_off(unsigned long lu_addr, uint32_t num_bytes, uint8_t *tx_buf)
{

    uint8_t addr[3];
    memset(addr, 0, sizeof(addr));

    memcpy(addr, &lu_addr, 3);
    uint8_t temp;
    temp = addr[2];
    addr[2] = addr[0];
    addr[0] = temp;

    // LOG_INFO("SPI_MEM", "Addr write: %02x %02x %02x\n", addr[2], addr[1], addr[0]);

    #ifdef POWER_DOWN_ON
        SPI_release_power_down();
    #endif

    // Write Enable cmd

    #ifdef DEBUG_MODE
        // NRF_LOG_DEBUG("*******Programming page**********");
        LOG_INFO("*******Programming page**********\n\r");
    #endif


    uint32_t tx_len = num_bytes + ADDR_SIZE + CMD_SIZE;

    uint8_t tx_buffer[tx_len];
    memset(tx_buffer, 0, tx_len);

    tx_buffer[0] = PAGE_PRG; 
    memcpy(&tx_buffer[CMD_SIZE], addr, ADDR_SIZE);
    
    memcpy(&tx_buffer[ADDR_SIZE + CMD_SIZE], tx_buf, num_bytes);

    #ifdef DEBUG_MODE
        LOG_INFO("Program page TX buffer is: ");
        for (int j = 0; j < tx_len; j++)
        {
            printf("%X ", tx_buffer[j]);
        }
    #endif

    LOG_INFO("tx_buf of len %lu: ", tx_len);
    for (int i = 0; i< tx_len; i++)
    {
        printf("%02X ", tx_buffer[i]);
    }
    printf("\n\r");


    memory_spi_transfer(tx_buffer, tx_len, NULL, 0);

    // int t0 = clock_time();

    while(SPI_busy() == true)
    {
        
    }

    #ifdef POWER_DOWN_ON
        SPI_power_down();
    #endif

}


void SPI_init()
{
    memory_spi_init();

    // SPI_chip_erase();
    #ifdef POWER_DOWN_ON
        SPI_power_down();
    #endif

}

void SPI_power_down()
{
    #ifdef DEBUG_MODE
        // NRF_LOG_DEBUG("*******Power down**********");
        LOG_INFO("*******Power down**********\n\r");
    #endif
    uint8_t command = POWER_DOWN;
    memory_spi_transfer(&command, CMD_SIZE, NULL, 0);
    // nrf_delay_ms(3);

}

void SPI_release_power_down()
{
    #ifdef DEBUG_MODE
        // NRF_LOG_DEBUG("*******Release power down**********");
        LOG_INFO("*******Release power down**********\n\r");
    #endif
    uint8_t command = REL_POWER_DOWN;
    memory_spi_transfer(&command, CMD_SIZE, NULL, 0);
    // nrf_delay_ms(3);

}

void SPI_write_en()
{
    // Write Enable cmd
    uint8_t command = WRITE_EN;
    memory_spi_transfer(&command, CMD_SIZE, NULL, 0);

    // nrf_delay_ms(3);

}

bool SPI_busy()
{
    #ifdef DEBUG_MODE
        // NRF_LOG_DEBUG("*******Write enable**********");
        // printf("*******Read SR**********\n\r");
    #endif

    // uint8_t command = READ_SR;
    // uint8_t rx_tmp[2];
    // memset(rx_tmp, 0, sizeof(rx_tmp));

    // memory_spi_transfer(&command, CMD_SIZE, rx_tmp, 2);

    unsigned char SR = SPI_read_SR();
    #ifdef DEBUG_MODE
        // NRF_LOG_DEBUG("*******Write enable**********");
        // printf("*******SR: %02x**********\n\r", rx_tmp[1]);
    #endif

    uint8_t check_BUSY = 0x01;
    check_BUSY = check_BUSY & SR;

    if (check_BUSY == 0x01)
    {
        return true;
    }
    else
    {
        return false;
    }

}

unsigned char SPI_read_SR()
{
    uint8_t command = READ_SR;
    uint8_t rx_tmp[2];
    memset(rx_tmp, 0, sizeof(rx_tmp));
    memory_spi_transfer(&command, CMD_SIZE, rx_tmp, 2);

    return (unsigned char) rx_tmp[1];
    
}

void SPI_block32_erase(uint8_t *addr)
{
    #ifdef POWER_DOWN_ON
        SPI_release_power_down();
    #endif

    LOG_INFO("*******Erasing sector at addr %02X %02X %02X**********\n\r", addr[2], addr[1], addr[0]);

    uint32_t tx_len = CMD_SIZE + ADDR_SIZE;
    uint8_t tx_buf[tx_len];
    memset(tx_buf, 0, tx_len);

    tx_buf[0] = BLOCK32_ERASE;
    memcpy(&tx_buf[CMD_SIZE], addr, ADDR_SIZE);

    // #ifdef DEBUG_MODE 
    //     printf("TX buffer is: ");
    //     for (int j = 0; j < tx_len; j++)
    //     {
    //         printf("%X ", tx_buf[j]);
    //     }
    //     printf("\n\r");
    // #endif

    memory_spi_transfer(tx_buf, tx_len, NULL, 0);

    while(SPI_busy() == true)
    {
        
    }

    #ifdef POWER_DOWN_ON
        SPI_power_down();
    #endif

}