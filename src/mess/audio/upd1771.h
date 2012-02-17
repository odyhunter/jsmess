/**********************************************************************

    NEC uPD1771

**********************************************************************/

#ifndef __UPD1771_H__
#define __UPD1771_H__

#include "devcb.h"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _upd1771_interface upd1771_interface;
struct _upd1771_interface
{
	devcb_write_line	ack_callback;
};


/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_SOUND_DEVICE(UPD1771C, upd1771c);


/***************************************************************************
    PROTOTYPES
***************************************************************************/

WRITE8_DEVICE_HANDLER( upd1771_w );
WRITE_LINE_DEVICE_HANDLER( upd1771_pcm_w );

#endif /* __UPD1771_H__ */