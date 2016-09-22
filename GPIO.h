#pragma once

#include "PhysicalMemory.h"

class GpioSegment : public Segment {
	enum {
		LT_TIMER = 0x0D800010,
	};
public:
	GpioSegment(DWORD addr);
	virtual DWORD get_u32(DWORD addr);
	virtual void set_u32(DWORD addr, DWORD val);
	virtual WORD get_u16(DWORD addr);
	virtual void set_u16(DWORD addr, WORD val);
	virtual BYTE get_u8(DWORD addr);
	virtual void set_u8(DWORD addr, BYTE val);
};