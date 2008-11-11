/* ASG 971222 -- rewrote this interface */
#ifndef __NECINTRF_H__
#define __NECINTRF_H__

#include "cpuintrf.h"

enum
{
	NEC_PC=0,
	NEC_IP, NEC_AW, NEC_CW, NEC_DW, NEC_BW, NEC_SP, NEC_BP, NEC_IX, NEC_IY,
	NEC_FLAGS, NEC_ES, NEC_CS, NEC_SS, NEC_DS,
	NEC_VECTOR, NEC_PENDING
};

/* Public functions */
extern CPU_GET_INFO( v30mz );

#endif /* __NECINTRF_H__ */
