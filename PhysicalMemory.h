#pragma once

#include "stdafx.h"

class PhysicalMemory {
	struct Segment {
		DWORD addr;
		DWORD size;
		std::vector<BYTE> buffer;
	};
	std::vector<struct Segment> segments;
public:
	void add_segment(DWORD addr, DWORD size);
	BYTE* find_address(DWORD addr);
	BYTE& operator[](DWORD addr);
	DWORD get_u32(DWORD addr);
	void set_u32(DWORD addr, DWORD val);
	WORD get_u16(DWORD addr);
	void set_u16(DWORD addr, WORD val);
	void clear_LT_TIMER();
	void increment_LT_TIMER();
};
