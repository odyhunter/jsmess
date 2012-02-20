/*
 * apollo.h - APOLLO DN3500/DN3000 driver includes
 *
 *  Created on: May 12, 2010
 *      Author: Hans Ostermeyer
 *
 *  Released for general non-commercial use under the MAME license
 *  Visit http://mamedev.org for licensing and usage restrictions.
 *
 */

#pragma once

#ifndef APOLLO_H_
#define APOLLO_H_

#include "emu.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

#define LOG(x)  { logerror x; logerror ("\n"); apollo_check_log(); }
#define LOG1(x) { if (VERBOSE > 0) LOG(x) }
#define LOG2(x) { if (VERBOSE > 1) LOG(x) }
#define DLOG(x) { logerror ("%s - %s: ", apollo_cpu_context(device->machine().device(MAINCPU)), device->tag()); LOG(x) }
#define DLOG1(x) { if (VERBOSE > 0) DLOG(x) }
#define DLOG2(x) { if (VERBOSE > 1) DLOG(x) }
#define MLOG(x)  { logerror ("%s: ", apollo_cpu_context(machine.device(MAINCPU))); LOG(x) }
#define MLOG1(x) { if (VERBOSE > 0) MLOG(x) }
#define MLOG2(x) { if (VERBOSE > 1) MLOG(x) }
#define SLOG(x)  { logerror ("%s: ", apollo_cpu_context(&space->device())); LOG(x) }
#define SLOG1(x) { if (VERBOSE > 0) SLOG(x) }
#define SLOG2(x) { if (VERBOSE > 1) SLOG(x) }

#define  MAINCPU "maincpu"

/*----------- debug/apollo.c -----------*/

int apollo_debug_instruction_hook(device_t *device, offs_t curpc);

/*----------- drivers/apollo.c -----------*/

// return the current CPU context for log file entries
const char *apollo_cpu_context(device_t *cpu);

// enable/disable the FPU
void apollo_set_cpu_has_fpu(device_t *device, int onoff);

// check for excessive logging
void apollo_check_log();

// return 1 if node is DN3000 or DSP3000, 0 otherwise
int apollo_is_dn3000(void);

// return 1 if node is DSP3000 or DSP3500, 0 otherwise
int apollo_is_dsp3x00(void);

// get the ram configuration byte
UINT8 apollo_get_ram_config_byte(void);

//apollo_get_node_id - get the node id
UINT32 apollo_get_node_id(void);

 // should be called by the CPU core before executing each instruction
int apollo_instruction_hook(device_t *device, offs_t curpc);

void apollo_set_cache_status_register(UINT8 mask, UINT8 data);

void apollo_terminal_write(UINT8 data);

/*----------- machine/apollo.c -----------*/

#define APOLLO_CONF_TAG "conf"
#define APOLLO_FDC_TAG  "fdc"
#define APOLLO_DMA1_TAG "dma8237_1"
#define APOLLO_DMA2_TAG "dma8237_2"
#define APOLLO_KBD_TAG  "kbd"
#define APOLLO_PIC1_TAG "pic8259_master"
#define APOLLO_PIC2_TAG "pic8259_slave"
#define APOLLO_PTM_TAG  "ptm"
#define APOLLO_RTC_TAG  "rtc"
#define APOLLO_SIO_TAG  "sio"
#define APOLLO_SIO2_TAG "sio2"
#define APOLLO_ETH_TAG  "3c505"
#define APOLLO_WDC_TAG  "omti8621"
#define APOLLO_CTAPE_TAG "ctape"

class apollo_state : public driver_device
{
public:
//	apollo_state(running_machine &machine, const driver_device_config_base &config)
//		: driver_device(machine, config) { }

	apollo_state(const machine_config &mconfig, device_type type, const char *tag)
			: driver_device(mconfig, type, tag) { }

	device_t *dma8237_1;
	device_t *dma8237_2;
	device_t *pic8259_master;
	device_t *pic8259_slave;
};

MACHINE_CONFIG_EXTERN( apollo );

DRIVER_INIT( apollo );
MACHINE_START( apollo );
MACHINE_RESET( apollo );

/*----------- machine/apollo_config.c -----------*/

// configuration bit definitions

#define APOLLO_CONF_SERVICE_MODE 0x001
#define APOLLO_CONF_GERMAN_KBD   0x002
#define APOLLO_CONF_DATE_1990    0x004
#define APOLLO_CONF_NODE_ID      0x008
#define APOLLO_CONF_IDLE_SLEEP   0x010
#define APOLLO_CONF_TRAP_TRACE   0x020
#define APOLLO_CONF_FPU_TRACE    0x040
#define APOLLO_CONF_DISK_TRACE   0x080
#define APOLLO_CONF_NET_TRACE    0x100

// check configuration setting
int apollo_config(int mask);

INPUT_PORTS_EXTERN(apollo_config);

/*----------- machine/apollo_csr.c -----------*/

#define APOLLO_CSR_SR_SERVICE            0x0001
#define APOLLO_CSR_SR_ATBUS_IO_TIMEOUT   0x0002
#define APOLLO_CSR_SR_FP_TRAP            0x0004
#define APOLLO_CSR_SR_INTERRUPT_PENDING  0x0008 // DN3000 only
#define APOLLO_CSR_SR_PARITY_BYTE_MASK   0x00f0
#define APOLLO_CSR_SR_CPU_TIMEOUT        0x0100
#define APOLLO_CSR_SR_ATBUS_MEM_TIMEOUT  0x2000
#define APOLLO_CSR_SR_BIT15              0x8000
#define APOLLO_CSR_SR_CLEAR_ALL          0x3ffe

#define APOLLO_CSR_CR_INTERRUPT_ENABLE   0x0001
#define APOLLO_CSR_CR_RESET_DEVICES	      0x0002
#define APOLLO_CSR_CR_FPU_TRAP_ENABLE    0x0004
#define APOLLO_CSR_CR_FORCE_BAD_PARITY   0x0008
#define APOLLO_CSR_CR_PARITY_BYTE_MASK   0x00f0

UINT16 apollo_csr_get_control_register(void);
UINT16 apollo_csr_get_status_register(void);
void apollo_csr_set_status_register(UINT16 mask, UINT16 data);

WRITE16_HANDLER( apollo_csr_status_register_w );
READ16_HANDLER( apollo_csr_status_register_r );
WRITE16_HANDLER( apollo_csr_control_register_w );
READ16_HANDLER( apollo_csr_control_register_r );

/*----------- machine/apollo_dma.c -----------*/

WRITE8_HANDLER(apollo_dma_1_w );
READ8_HANDLER( apollo_dma_1_r );

WRITE8_HANDLER(apollo_dma_2_w );
READ8_HANDLER( apollo_dma_2_r );

WRITE8_HANDLER(apollo_dma_page_register_w );
READ8_HANDLER( apollo_dma_page_register_r );

WRITE16_HANDLER(apollo_address_translation_map_w );
READ16_HANDLER( apollo_address_translation_map_r );

/*----------- machine/apollo_pic.c -----------*/

WRITE8_DEVICE_HANDLER(apollo_pic8259_master_w ) ;
READ8_DEVICE_HANDLER( apollo_pic8259_master_r );

WRITE8_DEVICE_HANDLER(apollo_pic8259_slave_w );
READ8_DEVICE_HANDLER( apollo_pic8259_slave_r );

IRQ_CALLBACK(apollo_pic_acknowledge);

/*----------- machine/apollo_ptm.c -----------*/

WRITE8_DEVICE_HANDLER( apollo_ptm_w );
READ8_DEVICE_HANDLER( apollo_ptm_r );

/*----------- machine/apollo_rtc.c -----------*/

WRITE8_HANDLER( apollo_rtc_w );
READ8_HANDLER( apollo_rtc_r );

/*----------- machine/apollo_sio.c -----------*/

void apollo_sio_rx_data( device_t* device, int ch, UINT8 data );

READ8_DEVICE_HANDLER(apollo_sio_r);
WRITE8_DEVICE_HANDLER(apollo_sio_w);

/*----------- machine/apollo_sio2.c -----------*/

READ8_DEVICE_HANDLER(apollo_sio2_r);
WRITE8_DEVICE_HANDLER(apollo_sio2_w);

/*----------- machine/apollo_fdc.c -----------*/

READ8_HANDLER(apollo_fdc_r);
WRITE8_HANDLER(apollo_fdc_w);

/*----------- machine/apollo_eth.c -----------*/

// ethernet transmitter

int apollo_eth_transmit(device_t* device,
		const UINT8 data_buffer[], const int data_length);

int apollo_eth_setfilter(device_t *device, int node_id);
// ethernet receiver callback

typedef int (*apollo_eth_receive)(device_t *, const UINT8 *, int);

void apollo_eth_init(device_t *device, apollo_eth_receive rx_data);

/*----------- machine/apollo_net.c -----------*/

// netserver receiver

int apollo_netserver_receive(device_t* device,
		const UINT8 rx_data_buffer[], const int rx_data_length);

// transmitter callback

typedef int (*apollo_netserver_transmit)(device_t *, const UINT8 *, int);

void apollo_netserver_init(const char *root_path,  apollo_netserver_transmit tx_data);

/*----------- video/apollo.c -----------*/

#define APOLLO_SCREEN_TAG "apollo_screen"

DECLARE_LEGACY_DEVICE(APOLLO_MONO19I, apollo_mono19i);

#define MCFG_APOLLO_MONO19I_ADD(_tag) \
	MCFG_FRAGMENT_ADD(apollo_mono19i) \
	MCFG_DEVICE_ADD(_tag, APOLLO_MONO19I, 0)

DEVICE_GET_INFO( apollo_mono19i );

MACHINE_CONFIG_EXTERN( apollo_mono19i );

DECLARE_LEGACY_DEVICE(APOLLO_MONO15I, apollo_mono15i);

#define MCFG_APOLLO_MONO15I_ADD( _tag) \
	MCFG_FRAGMENT_ADD(apollo_mono15i) \
	MCFG_DEVICE_ADD(_tag, APOLLO_MONO15I, 0)

DEVICE_GET_INFO( apollo_mono15i );

MACHINE_CONFIG_EXTERN( apollo_mono15i );

READ16_DEVICE_HANDLER( apollo_mcr_r ) ;
WRITE16_DEVICE_HANDLER(apollo_mcr_w );

READ16_DEVICE_HANDLER( apollo_mgm_r );
WRITE16_DEVICE_HANDLER( apollo_mgm_w );

#endif /* APOLLO_H_ */