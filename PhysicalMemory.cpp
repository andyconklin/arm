#include "stdafx.h"
#include "PhysicalMemory.h"

void PhysicalMemory::add_segment(DWORD addr, DWORD size) {
	segments.push_back({ addr, size, std::vector<BYTE>(size, 0xA5) });
}
BYTE* PhysicalMemory::find_address(DWORD addr) {
	for (DWORD i = 0; i < segments.size(); i++) {
		if (segments[i].addr <= addr && addr <= segments[i].addr + segments[i].size)
			return (segments[i].buffer.data() + (addr - segments[i].addr));
	}
	return NULL;
}
BYTE& PhysicalMemory::operator[](DWORD addr) {
	BYTE* p = find_address(addr);
	if (p) return *p;
	throw "NOT FOUND";
}
DWORD PhysicalMemory::get_u32(DWORD addr) {
	BYTE *p = &(*this)[addr];
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}
void PhysicalMemory::set_u32(DWORD addr, DWORD val) {
	BYTE *p = &(*this)[addr];
	p[0] = (val & 0xFF000000) >> 24;
	p[1] = (val & 0x00FF0000) >> 16;
	p[2] = (val & 0x0000FF00) >> 8;
	p[3] = (val & 0x000000FF);
}
WORD PhysicalMemory::get_u16(DWORD addr) {
	BYTE *p = &(*this)[addr];
	return (p[0] << 8) | p[1];
}
void PhysicalMemory::set_u16(DWORD addr, WORD val) {
	BYTE *p = &(*this)[addr];
	p[0] = (val & 0xFF00) >> 8;
	p[1] = (val & 0x00FF);
}
void PhysicalMemory::clear_LT_TIMER() {
	BYTE* p = find_address(0x0D800010);
	p[0] = p[1] = p[2] = p[3] = 0;
}
void PhysicalMemory::increment_LT_TIMER() {
	BYTE* p = find_address(0x0D800010);
	if (p) {
		DWORD val = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
		val++;
		p[0] = (val & 0xFF000000) >> 24;
		p[1] = (val & 0x00FF0000) >> 16;
		p[2] = (val & 0x0000FF00) >> 8;
		p[3] = (val & 0x000000FF);
		return;
	}
	throw "CAN'T INCREMENT LT_TIMER";
}
