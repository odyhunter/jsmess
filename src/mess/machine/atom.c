/***********************************************************************

	atom.c

	Functions to emulate general aspects of the machine (RAM, ROM,
	interrupts, I/O ports)

	Many thanks to Kees van Oss for:
	1.	Tape input/output circuit diagram. It describes in great detail how the 2.4khz
		tone, 2.4khz tone enable, tape output and tape input are connected.
	2.	The DOS rom for the Atom so I could complete the floppy disc emulation.
	3.	Details of the eprom expansion board for the Atom.
	4.	His demo programs which I used to test the driver.

***********************************************************************/


#include "driver.h"
#include "deprecat.h"
#include "machine/8255ppi.h"
#include "video/m6847.h"
#include "machine/i8271.h"
#include "machine/6522via.h"
#include "devices/basicdsk.h"
#include "devices/flopdrv.h"
#include "devices/cassette.h"
#include "devices/printer.h"
#include "devices/snapquik.h"
#include "includes/atom.h"
#include "sound/speaker.h"


UINT8 atom_8255_porta;
static UINT8 atom_8255_portb;
UINT8 atom_8255_portc;

/* printer data written */
static char atom_printer_data = 0x07f;

/* I am not sure if this is correct, the atom appears to have a 2.4Khz timer used for reading tapes?? */
static int	timer_state = 0;

static void atom_via_irq_func(running_machine *machine, int state)
{
	if (state)
	{
		cpu_set_input_line(machine->cpu[0], 0, HOLD_LINE);
	}
	else
	{
		cpu_set_input_line(machine->cpu[0], 0, CLEAR_LINE);
	}
}

static const device_config *cassette_device_image(running_machine *machine)
{
	return device_list_find_by_tag( machine->config->devicelist, CASSETTE, "cassette" );
}

static const device_config *printer_image(running_machine *machine)
{
	return device_list_find_by_tag(machine->config->devicelist, PRINTER, "printer");
}

/* printer status */
static  READ8_HANDLER(atom_via_in_a_func)
{
	unsigned char data;

	data = atom_printer_data;

	if (!printer_is_ready(printer_image(space->machine)))
	{
		/* offline */
		data |=0x080;
	}

	return data;
}

/* printer data */
static WRITE8_HANDLER(atom_via_out_a_func)
{
	/* data is written to port, this causes a pulse on ca2 (printer /strobe input),
	and data is written */
	/* atom has a 7-bit printer port */
	atom_printer_data = data & 0x07f;
}

static unsigned char previous_ca2_data = 0;

/* one of these is pulsed! */
static WRITE8_HANDLER(atom_via_out_ca2_func)
{
	/* change in state of ca2 output? */
	if (((previous_ca2_data^data)&0x01)!=0)
	{
		/* only look for one transition state */
		if (data & 0x01)
		{
			/* output data to printer */
			printer_output(printer_image(space->machine), atom_printer_data);
		}
	}

	previous_ca2_data = data;
}

static const struct via6522_interface atom_6522_interface=
{
	atom_via_in_a_func,		/* printer status */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	atom_via_out_a_func,	/* printer data */
	NULL,
	NULL,
	NULL,
	atom_via_out_ca2_func,	/* printer strobe */
	NULL,
	atom_via_irq_func,
};



const ppi8255_interface atom_8255_int =
{
	atom_8255_porta_r,
	atom_8255_portb_r,
	atom_8255_portc_r,
	atom_8255_porta_w,
	atom_8255_portb_w,
	atom_8255_portc_w,
};

static int previous_i8271_int_state = 0;

static void atom_8271_interrupt_callback(int state)
{
	/* I'm assuming that the nmi is edge triggered */
	/* a interrupt from the fdc will cause a change in line state, and
	the nmi will be triggered, but when the state changes because the int
	is cleared this will not cause another nmi */
	/* I'll emulate it like this to be sure */

	if (state!=previous_i8271_int_state)
	{
		if (state)
		{
			/* I'll pulse it because if I used hold-line I'm not sure
			it would clear - to be checked */
			cpu_set_input_line(Machine->cpu[0], INPUT_LINE_NMI, PULSE_LINE);
		}
	}

	previous_i8271_int_state = state;
}

static const struct i8271_interface atom_8271_interface=
{
	atom_8271_interrupt_callback,
	NULL
};

/*
                                                                  +--------+
PC0,  Tape Output ------------------------------------------------|
                                                                  |
                                                  +---------|     | NAND   |------CAS OUT
PC1, Enable 2400 Hz ------------------------------|         |     |        |
                                  +------+        | NAND    |--B--|        |
PC4, 2400 Hz ---------------+-----|  INV | ---A---|         |     +--------+
                            |     +------+        +---------+
                            |
                            2400 Hz


PC4      PC1 PC0     :   A     B  CAS OUT
  0      0      0    :    1     1        1
  0      0      1    :    1     1        0
  0      1      0    :    1     0        1
  0      1      1    :    1     0        1
  1      0      0    :    0     1        1
  1      0      1    :    0     1        0
  1      1      0    :    0     1        1
  1      1      1    :    0     1        0


Here is a detail from the Atom circuit diagram about Tape Input:

PC5, Cassette
Input ----------------------------------------------------------------CAS IN
*/
static TIMER_CALLBACK(atom_timer_callback)
{
	/* change timer state */
	timer_state^=1;

	/* the 2.4khz signal is notted (A), and nand'ed with the 2.4kz enable, resulting
	in B. The final cassette output is the result of tape output nand'ed with B */


	{
		unsigned char A;
		unsigned char B;
		unsigned char result;

		/* 2.4khz signal - notted */
		A = (~timer_state);
		/* 2.4khz signal notted, and anded with 2.4khz enable */
		B = (~(A & (atom_8255_portc>>1))) & 0x01;

		result = (~(B & atom_8255_portc)) & 0x01;

		/* tape output */
		cassette_output(cassette_device_image(machine), (result & 0x01) ? -1.0 : +1.0);
	}
}


static DIRECT_UPDATE_HANDLER(atom_opbase_handler)
{
	/* clear op base override */
	memory_set_direct_update_handler(space, NULL);

	/* this is temporary */
	/* Kees van Oss mentions that address 8-b are used for the random number
	generator. I don't know if this is hardware, or random data because the
	ram chips are not cleared at start-up. So at this time, these numbers
	are poked into the memory to simulate it. When I have more details I will fix it */
	memory_region(space->machine, "main")[0x08] = mame_rand(space->machine) & 0x0ff;
	memory_region(space->machine, "main")[0x09] = mame_rand(space->machine) & 0x0ff;
	memory_region(space->machine, "main")[0x0a] = mame_rand(space->machine) & 0x0ff;
	memory_region(space->machine, "main")[0x0b] = mame_rand(space->machine) & 0x0ff;

	return cpu_get_pc( space->cpu ) & 0x0ffff;
}

MACHINE_RESET( atom )
{
	atom_8255_porta = 0xff;
	atom_8255_portb = 0xff;
	atom_8255_portc = 0xff;

	i8271_init(&atom_8271_interface);

	via_config(0, &atom_6522_interface);
	via_set_clock(0,1000000);
	via_reset();

	timer_state = 0;
	timer_pulse(ATTOTIME_IN_HZ(2400*2), NULL, 0, atom_timer_callback);

	memory_set_direct_update_handler(cpu_get_address_space( machine->cpu[0], ADDRESS_SPACE_PROGRAM ),atom_opbase_handler);
}

struct atm
{
	UINT8	atm_name[16];
	UINT8	atm_start_high;
	UINT8	atm_start_low;
	UINT8	atm_exec_high;
	UINT8	atm_exec_low;
	UINT8	atm_size_high;
	UINT8	atm_size_low;
};

/* this only works if loaded using file-manager. This should work
for binary files, but will not work with basic files. This also does not support
.tap files which contain multiple .atm files joined together! */
QUICKLOAD_LOAD(atom)
{
	unsigned char *quickload_data;
	int i;
	unsigned char *data;
	struct atm *atm_header;
	unsigned long addr;
	unsigned long exec;
	unsigned long size;

	quickload_data = malloc(quickload_size);
	if (!quickload_data)
		return INIT_FAIL;

	if (image_fread(image, quickload_data, quickload_size) != quickload_size)
	{
		free(quickload_data);
		return INIT_FAIL;
	}

	atm_header = (struct atm *)quickload_data;

	/* calculate data address */
	data = quickload_data + sizeof(struct atm);

	/* get start address */
	addr = (
			(atm_header->atm_start_low & 0x0ff) |
			((atm_header->atm_start_high & 0x0ff)<<8)
			);

	/* get size */
	size = (
			(atm_header->atm_size_low & 0x0ff) |
			((atm_header->atm_size_high & 0x0ff)<<8)
			);

	/* get execute address */
	exec = (
		(atm_header->atm_exec_low & 0x0ff)	|
		((atm_header->atm_exec_high & 0x0ff)<<8)
		);

	/* copy data into memory */
	for (i=size-1; i>=0; i--)
	{
		memory_write_byte( cpu_get_address_space( Machine->cpu[0], ADDRESS_SPACE_PROGRAM ), addr, data[0]);
		addr++;
		data++;
	}


	/* set new pc address */
	cpu_set_reg( Machine->cpu[0], REG_PC, exec);

	/* free the data */
	free(quickload_data);
	return INIT_PASS;
}


/* load floppy */
DEVICE_IMAGE_LOAD( atom_floppy )
{
	if (device_load_basicdsk_floppy(image)==INIT_PASS)
	{
		/* sector id's 0-9 */
		/* drive, tracks, heads, sectors per track, sector length, dir_sector, dir_length, first sector id */
		basicdsk_set_geometry(image, 80, 1, 10, 256, 0, 0, FALSE);

		return INIT_PASS;
	}

	return INIT_PASS;
}


READ8_DEVICE_HANDLER (atom_8255_porta_r )
{
	/* logerror("8255: Read port a\n"); */
	return (atom_8255_porta);
}

READ8_DEVICE_HANDLER ( atom_8255_portb_r )
{
	int row;
	static const char *keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3", "KEY4", "KEY5", 
										"KEY6", "KEY7", "KEY8", "KEY9", "KEY10", "KEY11" };
	
	row = atom_8255_porta & 0x0f;
	/* logerror("8255: Read port b: %02X %02X\n", input_port_read(machine, port), 
									input_port_read(machine, "KEY10") & 0xc0); */
	return ((input_port_read(device->machine, keynames[row]) & 0x3f) | (input_port_read(device->machine, "KEY10") & 0xc0));
}

READ8_DEVICE_HANDLER ( atom_8255_portc_r )
{
	atom_8255_portc &= 0x0f;

	/* cassette input */
	if (cassette_input(cassette_device_image(device->machine)) > 0.0)
	{
		atom_8255_portc |= (1<<5);
	}

	/* 2.4khz input */
	if (timer_state)
	{
		atom_8255_portc |= (1<<4);
	}

	atom_8255_portc |= (m6847_get_field_sync() ? 0x00 : 0x80);
	atom_8255_portc |= (input_port_read(device->machine, "KEY11") & 0x40);
	/* logerror("8255: Read port c (%02X)\n",atom_8255.atom_8255_portc); */
	return (atom_8255_portc);
}

/* Atom 6847 modes:

0000xxxx	Text
0001xxxx	64x64	4
0011xxxx	128x64	2
0101xxxx	128x64	4
0111xxxx	128x96	2
1001xxxx	128x96	4
1011xxxx	128x192	2
1101xxxx	128x192	4
1111xxxx	256x192	2

*/

WRITE8_DEVICE_HANDLER ( atom_8255_porta_w )
{
	atom_8255_porta = data;
}

WRITE8_DEVICE_HANDLER ( atom_8255_portb_w )
{
	atom_8255_portb = data;
}

WRITE8_DEVICE_HANDLER (atom_8255_portc_w)
{
	atom_8255_portc = data;
	speaker_level_w(0, (data & 0x04) >> 2);
}


/* KT- I've assumed that the atom 8271 is linked in exactly the same way as on the bbc */
READ8_HANDLER(atom_8271_r)
{
	switch (offset)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			/* 8271 registers */
			return i8271_r(space, offset);
		case 4:
			return i8271_data_r(space, offset);
		default:
			break;
	}

	return 0x0ff;
}

WRITE8_HANDLER(atom_8271_w)
{
	switch (offset)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			/* 8271 registers */
			i8271_w(space, offset, data);
			return;
		case 4:
			i8271_data_w(space, offset, data);
			return;
		default:
			break;
	}
}


/*********************************************/
/* emulates a 16-slot eprom box for the Atom */
static unsigned char selected_eprom = 0;

static void atom_eprom_box_refresh(running_machine *machine)
{
    unsigned char *eprom_data;

	/* get address of eprom data */
	eprom_data = memory_region(machine, "main") + 0x010000 + (selected_eprom<<12);
	/* set bank address */
	memory_set_bankptr(machine, 1, eprom_data);
}

void atom_eprom_box_init(running_machine *machine)
{
	/* set initial eprom */
	selected_eprom = 0;
	/* set memory handler */
	/* init */
	atom_eprom_box_refresh(machine);
}

/* write to eprom box, changes eprom selected */
WRITE8_HANDLER(atom_eprom_box_w)
{
	selected_eprom = data & 0x0f;

	atom_eprom_box_refresh(space->machine);
}

/* read from eprom box register, can this be done in the real hardware */
READ8_HANDLER(atom_eprom_box_r)
{
	return selected_eprom;
}

MACHINE_RESET( atomeb )
{
	MACHINE_RESET_CALL(atom);
	atom_eprom_box_init(machine);
}

