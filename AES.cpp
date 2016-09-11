#include "stdafx.h"

#include "AES.h"

AesSegment::AesSegment(DWORD addr) : Segment(addr, 0x14) {
	for (int i = 0; i < 4; i++) {
		key[i] = 0;
		iv[i] = 0;
	}
}
DWORD AesSegment::get_u32(DWORD addr) {
	return Segment::get_u32(addr);
}
void AesSegment::set_AES_CTRL(DWORD val) {
	if (val & 0x80000000)
		std::cout << "Initiate AES command." << std::endl;
	else
		std::cout << "Reset AES engine." << std::endl;
	if (val & 0x40000000)
		std::cout << "IRQ generation enabled." << std::endl;
	else
		std::cout << "IRQ generation disabled." << std::endl;
	if (val & 0x20000000)
		std::cout << "Just set the ERR bit." << std::endl;
	else
		std::cout << "Just cleared the ERR bit." << std::endl;
	if (val & 0x10000000)
		std::cout << "Cryptography enabled." << std::endl;
	else
		std::cout << "Cryptography disabled." << std::endl;
	if (val & 0x08000000)
		std::cout << "Just entered decryption mode." << std::endl;
	else
		std::cout << "Just entered encryption mode." << std::endl;
	if (val & 0x00001000)
		std::cout << "IV set: Continue CBC mode." << std::endl;
	else
		std::cout << "IV clear: Use the supplied IV." << std::endl;
	std::cout << "Number of 16-byte blocks to process, minus one: " << (val & 0xFFF) << std::endl;
}
void AesSegment::set_AES_KEY(DWORD val) {
	for (int i = 0; i < 3; i++)
		key[i] = key[i + 1];
	key[3] = val;
}
void AesSegment::set_AES_IV(DWORD val) {
	for (int i = 0; i < 3; i++)
		iv[i] = iv[i + 1];
	iv[3] = val;
}
void AesSegment::set_u32(DWORD addr, DWORD val) {
	switch (addr) {
	case AES_CTRL:
		set_AES_CTRL(val);
		break;
	case AES_KEY:
		set_AES_KEY(val);
		break;
	case AES_IV:
		set_AES_IV(val);
		break;
	default:
		Segment::set_u32(addr, val);
		break;
	}
}
WORD AesSegment::get_u16(DWORD addr) {
	throw "AesSegment doesn't support getting/setting u16s.";
}
void AesSegment::set_u16(DWORD addr, WORD val) {
	throw "AesSegment doesn't support getting/setting u16s.";
}
BYTE AesSegment::get_u8(DWORD addr) {
	throw "AesSegment doesn't support getting/setting u8s.";
}
void AesSegment::set_u8(DWORD addr, BYTE val) {
	throw "AesSegment doesn't support getting/setting u8s.";
}