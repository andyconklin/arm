#include "stdafx.h"
#include "PhysicalMemory.h"

PhysicalMemory::~PhysicalMemory() {
	for (int i = 0; i < segments.size(); i++)
		delete segments[i];
}

Segment::Segment(DWORD addr, DWORD size) : addr(addr), size(size), buffer(size) {}
Segment::Segment(DWORD addr, DWORD size, DWORD filesize) : addr(addr), size(size), buffer(filesize) {}

DWORD Segment::get_u32(DWORD addr) {
	DWORD actual_addr = addr - this->addr;
	if (addr < this->addr || addr >= (this->addr + size))
		throw "Out of bounds!";
	if (actual_addr < buffer.size()) {
		BYTE *p = buffer.data() + actual_addr;
		return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	}
	else {
		DWORD result = 0;
		for (int i = 0; i < 4; i++) {
			auto x = sparse.find(addr + i);
			if (x == sparse.end()) {
				sparse.insert({ addr + i, 0 });
				result |= 0 << ( 24- (8 * i));
			}
			else {
				result |= x->second << (24-(8 * i));
			}
		}
		return result;
	}
}
void Segment::set_u32(DWORD addr, DWORD val) {
	DWORD actual_addr = addr - this->addr;
	if (addr < this->addr || addr >= (this->addr + size))
		throw "Out of bounds!";
	if (actual_addr < buffer.size()) {
		BYTE *p = buffer.data() + actual_addr;
		p[0] = (val & 0xFF000000) >> 24;
		p[1] = (val & 0x00FF0000) >> 16;
		p[2] = (val & 0x0000FF00) >> 8;
		p[3] = (val & 0x000000FF);
	}
	else {
		for (int i = 0; i < 4; i++) {
			auto x = sparse.find(addr + i);
			if (x == sparse.end()) {
				sparse.insert({ addr + i, (val >> (24 - (8 * i))) & 0xFF });
			}
			else {
				x->second = (val >> (24 - (8 * i))) & 0xFF;
			}
		}
	}
}
WORD Segment::get_u16(DWORD addr) {
	DWORD actual_addr = addr - this->addr;
	if (addr < this->addr || addr >= (this->addr + size))
		throw "Out of bounds!";
	if (actual_addr < buffer.size()) {
		BYTE *p = buffer.data() + actual_addr;
		return (p[0] << 8) | p[1];
	}
	else {
		WORD result = 0;
		for (int i = 0; i < 2; i++) {
			auto x = sparse.find(addr + i);
			if (x == sparse.end()) {
				sparse.insert({ addr + i, 0 });
				result |= 0 << (24 - (8 * i));
			}
			else {
				result |= x->second << (24 - (8 * i));
			}
		}
		return result;
	}
}
void Segment::set_u16(DWORD addr, WORD val) {
	DWORD actual_addr = addr - this->addr;
	if (addr < this->addr || addr >= (this->addr + size))
		throw "Out of bounds!";
	if (actual_addr < buffer.size()) {
		BYTE *p = buffer.data() + actual_addr;
		p[0] = (val & 0x0000FF00) >> 8;
		p[1] = (val & 0x000000FF);
	}
	else {
		for (int i = 0; i < 2; i++) {
			auto x = sparse.find(addr + i);
			if (x == sparse.end()) {
				sparse.insert({ addr + i, (val >> (24 - (8 * i))) & 0xFF });
			}
			else {
				x->second = (val >> (24 - (8 * i))) & 0xFF;
			}
		}
	}
}
BYTE Segment::get_u8(DWORD addr) {
	DWORD actual_addr = addr - this->addr;
	if (addr < this->addr || addr >= (this->addr + size))
		throw "Out of bounds!";
	if (actual_addr < buffer.size()) {
		BYTE *p = buffer.data() + actual_addr;
		return p[0];
	}
	else {
		auto x = sparse.find(addr);
		if (x == sparse.end()) {
			sparse.insert({ addr,0 });
			return 0;
		}
		else {
			return x->second;
		}
	}
}
void Segment::set_u8(DWORD addr, BYTE val) {
	DWORD actual_addr = addr - this->addr;
	if (addr < this->addr || addr >= (this->addr + size))
		throw "Out of bounds!";
	if (actual_addr < buffer.size()) {
		BYTE *p = buffer.data() + actual_addr;
		p[0] = (val & 0x000000FF);
	}
	else {
		auto x = sparse.find(addr);
		if (x == sparse.end()) {
			sparse.insert({ addr, val });
		}
		else {
			x->second = val;
		}
	}
}

void PhysicalMemory::add_segment(DWORD addr, DWORD size) {
	segments.push_back(new Segment(addr, size));
}
void PhysicalMemory::add_segment(DWORD addr, DWORD size, DWORD file_size) {
	segments.push_back(new Segment(addr, size, file_size));
}
Segment& PhysicalMemory::find_segment(DWORD addr) {
	for (DWORD i = 0; i < segments.size(); i++) {
		if (segments[i]->addr <= addr && addr < segments[i]->addr + segments[i]->size)
			return *segments[i];
	}
	throw "SEGMENT NOT FOUND";
}
DWORD PhysicalMemory::get_u32(DWORD addr) {
	return find_segment(addr).get_u32(addr);
}
void PhysicalMemory::set_u32(DWORD addr, DWORD val) {
	return find_segment(addr).set_u32(addr, val);
}
WORD PhysicalMemory::get_u16(DWORD addr) {
	return find_segment(addr).get_u16(addr);
}
void PhysicalMemory::set_u16(DWORD addr, WORD val) {
	return find_segment(addr).set_u16(addr, val);
}
BYTE PhysicalMemory::get_u8(DWORD addr) {
	return find_segment(addr).get_u8(addr);
}
void PhysicalMemory::set_u8(DWORD addr, BYTE val) {
	return find_segment(addr).set_u8(addr, val);
}
void PhysicalMemory::add_segment(Segment *s) {
	segments.push_back(s);
}
void PhysicalMemory::clear_LT_TIMER() {
	find_segment(0x0D800010).set_u32(0x0D800010, 0);
}
void PhysicalMemory::increment_LT_TIMER() {
	Segment &x = find_segment(0x0D800010);
	x.set_u32(0x0D800010, x.get_u32(0x0D800010) + 1);
}

BYTE* PhysicalMemory::get_buffer(DWORD addr) {
	Segment &x = find_segment(addr);
	return x.buffer.data();
}