#include "stdafx.h"

#include "SHA.h"

ShaSegment::ShaSegment(DWORD addr) : Segment(addr, 0x1c) {

}
DWORD ShaSegment::get_u32(DWORD addr) {
	return Segment::get_u32(addr);
}
void ShaSegment::set_u32(DWORD addr, DWORD val) {
	Segment::set_u32(addr, val);
}
WORD ShaSegment::get_u16(DWORD addr) {
	throw "ShaSegment doesn't support getting/setting u16s.";
}
void ShaSegment::set_u16(DWORD addr, WORD val) {
	throw "ShaSegment doesn't support getting/setting u16s.";
}
BYTE ShaSegment::get_u8(DWORD addr) {
	throw "ShaSegment doesn't support getting/setting u8s.";
}
void ShaSegment::set_u8(DWORD addr, BYTE val) {
	throw "ShaSegment doesn't support getting/setting u8s.";
}