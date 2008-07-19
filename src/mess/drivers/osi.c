/*

	TODO:

	- fix UK101 keyboard
	- fix UK101 video to 64x16
	- accurate video timing
	- proper mirroring
	- floppy disk support
	- cegmon/wemon bioses

*/

#include "driver.h"
#include "image.h"
#include "machine/6850acia.h"
#include "sound/discrete.h"

/* Video */

static tilemap *bg_tilemap;

static WRITE8_HANDLER( sb2m600_videoram_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset);
}

static TILE_GET_INFO(get_bg_tile_info)
{
	UINT8 code = videoram[tile_index];

	SET_TILE_INFO(0, code, 0, 0);
}

static VIDEO_START( sb2m600 )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 8, 8, 32, 32);
}

static VIDEO_START( uk101 )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 8, 16, 32, 32);
}

static VIDEO_UPDATE( sb2m600 )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);

	return 0;
}

/* Sound */

static const discrete_dac_r1_ladder sb2m600_dac =
{
	4,			// size of ladder
	{180, 180, 180, 180, 0, 0, 0, 0},	// R68, R69, R70, R71
	5,			// 5V
	RES_K(1),	// R67
	0,			// no rGnd
	0			// no cFilter
};

static DISCRETE_SOUND_START(sb2m600_discrete_interface)
	DISCRETE_INPUT_DATA(NODE_01)
	DISCRETE_DAC_R1(NODE_02, 1, NODE_01, DEFAULT_TTL_V_LOGIC_1, &sb2m600_dac)
	DISCRETE_CRFILTER(NODE_03, 1, NODE_02, 1.0/(1.0/RES_K(1)+1.0/180+1.0/180+1.0/180+1.0/180), CAP_U(0.1))
	DISCRETE_OUTPUT(NODE_03, 100)
	DISCRETE_GAIN(NODE_04, NODE_03, 32767.0/5)
	DISCRETE_OUTPUT(NODE_04, 100)
DISCRETE_SOUND_END

/* Keyboard */

static UINT8 key_row_latch;

static READ8_HANDLER( osi_keyboard_r )
{
	UINT8 key_column_data = 0xff;

	if (!(key_row_latch & 0x01)) key_column_data &= input_port_read(machine, "ROW0");
	if (!(key_row_latch & 0x02)) key_column_data &= input_port_read(machine, "ROW1");
	if (!(key_row_latch & 0x04)) key_column_data &= input_port_read(machine, "ROW2");
	if (!(key_row_latch & 0x08)) key_column_data &= input_port_read(machine, "ROW3");
	if (!(key_row_latch & 0x10)) key_column_data &= input_port_read(machine, "ROW4");
	if (!(key_row_latch & 0x20)) key_column_data &= input_port_read(machine, "ROW5");
	if (!(key_row_latch & 0x40)) key_column_data &= input_port_read(machine, "ROW6");
	if (!(key_row_latch & 0x80)) key_column_data &= input_port_read(machine, "ROW7");

	return key_column_data;
}

static WRITE8_HANDLER ( sb2m600b_keyboard_w )
{
	key_row_latch = data;
	discrete_sound_w(machine, NODE_01, (data >> 2) & 0x0f);
}

static WRITE8_HANDLER ( uk101_keyboard_w )
{
	key_row_latch = data;
}

/* Disk Drive */

static READ8_HANDLER( osi470_floppy_status_r )
{
	/*
		C000 FLOPIN			FLOPPY DISK STATUS PORT

		BIT	FUNCTION
		0	DRIVE 0 READY (0 IF READY)
		1	TRACK 0 (0 IF AT TRACK 0)
		2	FAULT (0 IF FAULT)
		3
		4	DRIVE 1 READY (0 IF READY)
		5	WRITE PROTECT (0 IF WRITE PROTECT)
		6	DRIVE SELECT (1 = A OR C, 0 = B OR D)
		7	INDEX (0 IF AT INDEX HOLE)
	*/

	return 0x00;
}

static WRITE8_HANDLER( osi470_floppy_control_w )
{
	/*
		C002 FLOPOT			FLOPPY DISK CONTROL PORT

		BIT	FUNCTION
		0	WRITE ENABLE (0 ALLOWS WRITING)
		1	ERASE ENABLE (0 ALLOWS ERASING)
			ERASE ENABLE IS ON 200us AFTER WRITE IS ON
			ERASE ENABLE IS OFF 530us AFTER WRITE IS OFF
		2	STEP BIT : INDICATES DIRECTION OF STEP (WAIT 10us FIRST)
			0 INDICATES STEP TOWARD 76
			1 INDICATES STEP TOWARD 0
		3	STEP (TRANSITION FROM 1 TO 0)
			MUST HOLD AT LEAST 10us, MIN 8us BETWEEN
		4	FAULT RESET (0 RESETS)
		5	SIDE SELECT (1 = A OR B, 0 = C OR D)
		6	LOW CURRENT (0 FOR TRKS 43-76, 1 FOR TRKS 0-42)
		7	HEAD LOAD (0 TO LOAD : MUST WAIT 40ms AFTER)

		C010 ACIA			DISK CONTROLLER ACIA STATUS PORT
		C011 ACIAIO			DISK CONTROLLER ACIA I/O PORT
	*/
}

/* Memory Maps */

static ADDRESS_MAP_START( sb2m600_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_RAM
	AM_RANGE(0x1000, 0x1fff) AM_RAM
	AM_RANGE(0xa000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xc000) AM_READ(osi470_floppy_status_r)
	AM_RANGE(0xc002, 0xc002) AM_WRITE(osi470_floppy_control_w)
	AM_RANGE(0xc010, 0xc010) AM_READWRITE(acia6850_1_stat_r, acia6850_1_ctrl_w)
	AM_RANGE(0xc011, 0xc011) AM_READWRITE(acia6850_1_data_r, acia6850_1_data_w)
	AM_RANGE(0xd000, 0xd3ff) AM_RAM AM_WRITE(sb2m600_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xdf00, 0xdf00) AM_READWRITE(osi_keyboard_r, sb2m600b_keyboard_w)
	AM_RANGE(0xf000, 0xf000) AM_READWRITE(acia6850_0_stat_r, acia6850_0_ctrl_w)
	AM_RANGE(0xf001, 0xf001) AM_READWRITE(acia6850_0_data_r, acia6850_0_data_w)
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( uk101_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_RAM
	AM_RANGE(0xa000, 0xbfff) AM_ROM
	AM_RANGE(0xd000, 0xd3ff) AM_RAM AM_WRITE(sb2m600_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xdf00, 0xdf00) AM_MIRROR(0x03ff) AM_READWRITE(osi_keyboard_r, uk101_keyboard_w)
	AM_RANGE(0xf000, 0xf000) AM_MIRROR(0x00fe) AM_READWRITE(acia6850_0_stat_r, acia6850_0_ctrl_w)
	AM_RANGE(0xf001, 0xf001) AM_MIRROR(0x00fe) AM_READWRITE(acia6850_0_data_r, acia6850_0_data_w)
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( sb2m600 )
	PORT_START_TAG("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK)) PORT_TOGGLE
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RIGHT SHIFT") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LEFT SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_TAB) PORT_CHAR(27)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("REPEAT") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\')

	PORT_START_TAG("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')

	PORT_START_TAG("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) PORT_CHAR('X')

	PORT_START_TAG("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) PORT_CHAR('S')

	PORT_START_TAG("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) PORT_CHAR('W')

	PORT_START_TAG("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LINE FEED") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(10)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')

	PORT_START_TAG("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RUB OUT") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')

	PORT_START_TAG("ROW7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
INPUT_PORTS_END

static INPUT_PORTS_START( uk101 )
	PORT_INCLUDE(sb2m600)

	PORT_MODIFY("ROW0")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(27)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('~')

	PORT_MODIFY("ROW5")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\')
INPUT_PORTS_END

/* Graphics Layouts */

static const gfx_layout sb2m600_charlayout =
{
	8, 8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8 * 8
};

static const gfx_layout uk101_charlayout =
{
	8, 16,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 0*8, 1*8, 1*8, 2*8, 2*8, 3*8, 3*8,
	  4*8, 4*8, 5*8, 5*8, 6*8, 6*8, 7*8, 7*8 },
	8 * 8
};

/* Graphics Decode Information */

static GFXDECODE_START( sb2m600 )
	GFXDECODE_ENTRY( REGION_GFX1, 0x0000, sb2m600_charlayout, 0, 1 )
GFXDECODE_END

static GFXDECODE_START( uk101 )
	GFXDECODE_ENTRY( REGION_GFX1, 0x0000, uk101_charlayout, 0, 1 )
GFXDECODE_END

/* Machine Start */

static UINT8 rx, tx;

static const struct acia6850_interface sb2m600_acia_intf =
{
	500000, //
	500000, //
	&rx,
	&tx,
	NULL,
	NULL,
	NULL,
	NULL
};

static const struct acia6850_interface osi470_acia_intf =
{
	500000, //
	500000, //
	&rx,
	&tx,
	NULL,
	NULL,
	NULL,
	NULL
};

static const struct acia6850_interface uk101_acia_intf =
{
	500000, //
	500000, //
	&rx,
	&tx,
	NULL,
	NULL,
	NULL,
	NULL
};

static MACHINE_START( sb2m600 )
{
	// TODO: save states
	acia6850_config(0, &sb2m600_acia_intf);
	acia6850_config(1, &osi470_acia_intf);
}

static MACHINE_START( uk101 )
{
	// TODO: save states
	acia6850_config(0, &uk101_acia_intf);
}

/* Machine Drivers */

#define X1 3932160

static MACHINE_DRIVER_START( sb2m600 )
	// basic machine hardware
	MDRV_CPU_ADD_TAG("main", M6502, X1/4) // .98304 MHz
	MDRV_CPU_PROGRAM_MAP(sb2m600_mem, 0)
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(X1/256/256) // 60 Hz

	MDRV_MACHINE_START(sb2m600)

    // video hardware
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(5*8, 29*8-1, 0, 28*8-1)
	MDRV_GFXDECODE(sb2m600)
	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(sb2m600)
	MDRV_VIDEO_UPDATE(sb2m600)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG_DISCRETE(sb2m600_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

#define UK101_X1 XTAL_8MHz

static MACHINE_DRIVER_START( uk101 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6502, UK101_X1/8) /* 1 MHz */
	MDRV_CPU_PROGRAM_MAP(uk101_mem, 0)

	MDRV_MACHINE_START(uk101)

    /* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*16)
	MDRV_SCREEN_VISIBLE_AREA(5*8, 29*8-1, 2*32, 29*16-1)
	MDRV_GFXDECODE(uk101)
	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(uk101)
	MDRV_VIDEO_UPDATE(sb2m600)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( sb2m600b )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "basus01.u9",  0xa000, 0x0800, CRC(f4f5dec0) SHA1(b41bf24b4470b6e969d32fe48d604637276f846e) )
	ROM_LOAD( "basuk02.u10", 0xa800, 0x0800, CRC(0039ef6a) SHA1(1397f0dc170c16c8e0c7d02e63099e986e86385b) )
	ROM_LOAD( "basus03.u11", 0xb000, 0x0800, CRC(ca25f8c1) SHA1(f5e8ee93a5e0656657d0cc60ef44e8a24b8b0a80) )
	ROM_LOAD( "basus04.u12", 0xb800, 0x0800, CRC(8ee6030e) SHA1(71f210163e4268cba2dd78a97c4d8f5dcebf980e) )
	ROM_LOAD( "monde01.u13", 0xf800, 0x0800, CRC(95a44d2e) SHA1(4a0241c4015b94c436d0f0f58b3dd9d5207cd847) )

	ROM_REGION( 0x800, REGION_GFX1,0)
	ROM_LOAD( "chgsup2.u41", 0x0000, 0x0800, CRC(735f5e0a) SHA1(87c6271497c5b00a974d905766e91bb965180594) )
ROM_END

ROM_START( uk101 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "basuk01.ic9",   0xa000, 0x0800, CRC(9d3caa92) SHA1(b2c3d1af0c4f3cead1dbd44aaf5a11680880f772) )
	ROM_LOAD( "basuk02.ic10",  0xa800, 0x0800, CRC(0039ef6a) SHA1(1397f0dc170c16c8e0c7d02e63099e986e86385b) )
	ROM_LOAD( "basuk03.ic11",  0xb000, 0x0800, CRC(0d011242) SHA1(54bd33522a5d1991086eeeff3a4f73c026be45b6) )
	ROM_LOAD( "basuk04.ic12",  0xb800, 0x0800, CRC(667223e8) SHA1(dca78be4b98317413376d69119942d692e39575a) )
	ROM_LOAD( "monuk02.ic13",  0xf800, 0x0800, CRC(04ac5822) SHA1(2bbbcd0ca18103fd68afcf64a7483653b925d83e) )

	ROM_REGION( 0x800, REGION_GFX1, 0 )
	ROM_LOAD( "chguk101.ic41", 0x0000, 0x0800, CRC(fce2c84a) SHA1(baa66a7a48e4d62282671ef53abfaf450b888b70) )
ROM_END

/* System Configuration */

static UINT8 *sb2m600_tape_image;
static int sb2m600_tape_size;
static int sb2m600_tape_index;

static DEVICE_IMAGE_LOAD( sb2m600_cassette )
{
	sb2m600_tape_image = (UINT8 *)image_malloc(image, sb2m600_tape_size);
	sb2m600_tape_size = image_length(image);

	if (!sb2m600_tape_image || (image_fread(image, sb2m600_tape_image, sb2m600_tape_size) != sb2m600_tape_size))
	{
		return INIT_FAIL;
	}

	sb2m600_tape_index = 0;

	return INIT_PASS;
}

static DEVICE_IMAGE_UNLOAD( sb2m600_cassette )
{
	sb2m600_tape_image = NULL;
	sb2m600_tape_size = 0;
	sb2m600_tape_index = 0;
}

static void sb2m600_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_TYPE:							info->i = IO_CASSETTE; break;
		case MESS_DEVINFO_INT_READABLE:						info->i = 1; break;
		case MESS_DEVINFO_INT_WRITEABLE:						info->i = 0; break;
		case MESS_DEVINFO_INT_CREATABLE:						info->i = 0; break;
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(sb2m600_cassette); break;
		case MESS_DEVINFO_PTR_UNLOAD:						info->unload = DEVICE_IMAGE_UNLOAD_NAME(sb2m600_cassette); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "bas"); break;
	}
}

static SYSTEM_CONFIG_START(sb2m600)
	CONFIG_DEVICE(sb2m600_cassette_getinfo)
	CONFIG_RAM_DEFAULT(4 * 1024)
	CONFIG_RAM(8 * 1024)
SYSTEM_CONFIG_END

/* System Drivers */

//    YEAR  NAME      PARENT    COMPAT MACHINE INPUT INIT     CONFIG   COMPANY            FULLNAME
COMP( 1978, sb2m600b, 0,        0, sb2m600, sb2m600, 0, 	sb2m600, "Ohio Scientific", "Superboard II Model 600 (Rev. B)" , 0)
COMP( 1979,	uk101,    sb2m600b, 0, uk101,   uk101,   0, 	sb2m600, "Compukit",        "UK101" , GAME_NOT_WORKING)
