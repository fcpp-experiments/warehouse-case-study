/*
 * Copyright (c) 2018, University of Trento, Italy
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
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``as-is'' AND
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
/**
 * \file
 *      Platform Dependent DW1000 Driver Source File
 *
 */

/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "nrf.h"
#include "nrfx_spi.h"
#include "nrf_drv_spi.h"

#include "nrfx_gpiote.h"
#include "nrf_delay.h"
#include "app_util_platform.h"
#include "app_error.h"
/*---------------------------------------------------------------------------*/

#include "sys/clock.h"
/*---------------------------------------------------------------------------*/
#include "leds.h"
/*---------------------------------------------------------------------------*/
// #include "deca_device_api.h"
#include "deca_types.h"
// #include "deca_regs.h"
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
#include "mem-arch.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"


#define MEM_SCK_PIN 4
#define MEM_MOSI_PIN 6
#define MEM_MISO_PIN 7
/*---------------------------------------------------------------------------*/
#define NRFX_SPI_DEFAULT_CONFIG_2M			     \
  {                                                          \
   .sck_pin      = MEM_SCK_PIN,			     \
   .mosi_pin     = MEM_MOSI_PIN,			     \
   .miso_pin     = MEM_MISO_PIN,			     \
   .ss_pin       = SPI_CS_MEM,				     \
   .irq_priority = APP_IRQ_PRIORITY_LOW,			     \
   .orc          = 0xFF,				     \
   .frequency    = NRF_SPI_FREQ_2M,			     \
   .mode         = NRF_SPI_MODE_0,			     \
   .bit_order    = NRF_SPI_BIT_ORDER_MSB_FIRST,		     \
  }

/*---------------------------------------------------------------------------*/
static const nrfx_spi_t spi = NRFX_SPI_INSTANCE(SPI_MEM_INSTANCE);  /* SPI instance. */
/*---------------------------------------------------------------------------*/

void
memory_spi_init(void)
{
  nrfx_spi_config_t spi_config = NRFX_SPI_DEFAULT_CONFIG_2M;
  APP_ERROR_CHECK(nrfx_spi_init(&spi, &spi_config, NULL, NULL));
  nrf_delay_ms(2);
}


int
// memory_spi_read(uint8 *tx_buf, uint16 tx_len, uint8 *rx_buf, uint16 rx_len)
memory_spi_transfer(uint8 *tx_buf, uint16 tx_len, uint8 *rx_buf, uint16 rx_len)
{

  nrfx_spi_xfer_desc_t const spi_xfer_desc =
    {
     .p_tx_buffer = tx_buf,
     .tx_length   = tx_len,
     .p_rx_buffer = rx_buf,
     .rx_length   = rx_len,
    };
  nrfx_spi_xfer(&spi, &spi_xfer_desc, 0);
   
  return 0;
}


/*---------------------------------------------------------------------------*/
int
memory_spi_write(uint16 hdrlen, const uint8 *hdrbuf, uint32 len, const uint8 *buf)
{
  uint8_t total_len = hdrlen + len;

  /* Temporal buffer for the SPI write/read operation */
  uint8_t hbuf[total_len];

  /* Initialize header buffer */
  memcpy(hbuf, hdrbuf, hdrlen);
  memcpy(hbuf + hdrlen, buf, len);

  /* Write header */
  //nrf_drv_spi_transfer(&spi, hbuf, total_len, NULL, 0);
  nrfx_spi_xfer_desc_t const spi_xfer_desc =
    {
     .p_tx_buffer = hbuf,
     .tx_length   = total_len,
     .p_rx_buffer = NULL,
     .rx_length   = 0,
    };
  nrfx_spi_xfer(&spi, &spi_xfer_desc, 0);

  // /* Re-enable DW1000 EXT Interrupt state */
  // dw1000_enable_interrupt(irqn_status);

  return 0;
}