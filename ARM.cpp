// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "PhysicalMemory.h"
#include "Processor.h"
#include "AES.h"
#include "SHA.h"

int main()
{
	PhysicalMemory mem;

	ELFIO::elfio reader;
	reader.load("C:\\Users\\Andy\\Desktop\\fw.img.dec");

	// Print ELF file segments info
	ELFIO::Elf_Half seg_num = reader.segments.size();
	for (int i = seg_num-1; i >= 0; --i) {
		const ELFIO::segment* pseg = reader.segments[i];
		if (pseg->get_type() != PT_LOAD) continue;
		if (i == 2) continue;
		mem.add_segment(pseg->get_physical_address(), pseg->get_memory_size());
		memcpy(mem.get_buffer(pseg->get_physical_address()), pseg->get_data(), pseg->get_file_size());
	}

	/* Patch the function spDrvMemFgConfigureClient to return immediately 
	   because I can't figure out what it does and it's probably not relevant 
	   to my interests anyway. */
	mem.set_u32(0x08130D2C, 0xE3A00000);
	mem.set_u32(0x08130D30, 0xE12FFF1E);

	mem.add_segment(0x0D010000, 0x34); // NAND
	mem.add_segment(0x0D800000, 0x0D806880 - 0x0D800000); // GPIO
	mem.add_segment(0x0D8B0000, 0x0D8B46A0 - 0x0D8B0000); // DRAMctrl

	/*mem.add_segment(new AesSegment(0x0D020000));
	mem.add_segment(new ShaSegment(0x0D030000)); */

	mem.clear_LT_TIMER();

	Processor cpu(&mem, 0xFFFF0060);

	/* Set the LT_DEBUG register */
	mem.set_u32(0x0D8005A4, 0x80000000);

	cpu.continue_until(0xFFFFE860);

	/* Continue until Start_Kernel() */
	cpu.continue_until(0x08121B18);

	cpu.continue_until(0x08125310); // after the two KERNEL_MEMSETs

	/* Start watching after the memsets */
	//mem.watch(0x081A8000, 0x4000); // translation table
	//mem.watch(0x08156C00, 0x10000); // page table

	while (true) {
		//cpu.display_info();
		//system("pause");
		cpu.step();
	}
    return 0;
}

