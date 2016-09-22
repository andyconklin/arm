#include "stdafx.h"

#include "GPIO.h"

GpioSegment::GpioSegment(DWORD addr) : Segment(addr, 0x6880) { }
DWORD GpioSegment::get_u32(DWORD addr) {
	if (addr != LT_TIMER)
		std::cout << "Reading from GPIO: 0x" << std::hex << addr << std::dec << std::endl;
	return Segment::get_u32(addr);
}
void GpioSegment::set_u32(DWORD addr, DWORD val) {
	if (addr != LT_TIMER)
		std::cout << "Writing 0x" << std::hex << val << " to GPIO address 0x" << addr << std::dec << std::endl;
	switch (addr) {
	default:
		Segment::set_u32(addr, val);
		break;
	}
}
WORD GpioSegment::get_u16(DWORD addr) {
	throw "GpioSegment doesn't support getting/setting u16s.";
}
void GpioSegment::set_u16(DWORD addr, WORD val) {
	throw "GpioSegment doesn't support getting/setting u16s.";
}
BYTE GpioSegment::get_u8(DWORD addr) {
	throw "GpioSegment doesn't support getting/setting u8s.";
}
void GpioSegment::set_u8(DWORD addr, BYTE val) {
	throw "GpioSegment doesn't support getting/setting u8s.";
}