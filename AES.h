#pragma once

#include "PhysicalMemory.h"

class AesSegment : public Segment {
	enum AesRegisters {
		AES_CTRL = 0x0D020000,
		AES_SRC = 0x0D020004,
		AES_DEST = 0x0D020008,
		AES_KEY = 0x0D02000C,
		AES_IV = 0x0D020010,
	};

	DWORD key[4];
	DWORD iv[4];

	void set_AES_CTRL(DWORD val);
	void set_AES_KEY(DWORD val);
	void set_AES_IV(DWORD val);
public:
	AesSegment(DWORD addr);
	virtual DWORD get_u32(DWORD addr);
	virtual void set_u32(DWORD addr, DWORD val);
	virtual WORD get_u16(DWORD addr);
	virtual void set_u16(DWORD addr, WORD val);
	virtual BYTE get_u8(DWORD addr);
	virtual void set_u8(DWORD addr, BYTE val);
};