#pragma once

#include "stdafx.h"

/* Segment base class. Can be extended! */
class Segment {
public:
	DWORD addr;
	DWORD size;
	std::vector<BYTE> buffer;
	std::map<DWORD, BYTE> sparse;
	Segment(DWORD addr, DWORD size);
	Segment(DWORD addr, DWORD size, DWORD filesize);
	virtual DWORD get_u32(DWORD addr);
	virtual void set_u32(DWORD addr, DWORD val);
	virtual WORD get_u16(DWORD addr);
	virtual void set_u16(DWORD addr, WORD val);
	virtual BYTE get_u8(DWORD addr);
	virtual void set_u8(DWORD addr, BYTE val);
};

class PhysicalMemory {
	std::vector<Segment *> segments;
	std::vector<std::pair<DWORD, DWORD> > watchlist;
public:
	~PhysicalMemory();
	void watch(DWORD addr, DWORD size);
	void add_segment(DWORD addr, DWORD size);
	void add_segment(DWORD addr, DWORD size, DWORD file_size);
	void add_segment(Segment *s);
	Segment& find_segment(DWORD addr);
	BYTE* get_buffer(DWORD addr);
	DWORD get_u32(DWORD addr);
	void set_u32(DWORD addr, DWORD val);
	WORD get_u16(DWORD addr);
	void set_u16(DWORD addr, WORD val);
	BYTE get_u8(DWORD addr);
	void set_u8(DWORD addr, BYTE val);
	void clear_LT_TIMER();
	void increment_LT_TIMER();
};