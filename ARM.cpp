// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "PhysicalMemory.h"
#include "Processor.h"

int ReadFileIntoMemory(PhysicalMemory *mem, LPCWSTR path_to_file, DWORD load_address) {
	HANDLE hFile;
	DWORD file_size;
	DWORD bytes_read;

	hFile = CreateFileW(path_to_file,
		GENERIC_READ, 7, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		std::cerr << "Failed to open boot0.bin" << std::endl;
		return 1;
	}

	file_size = GetFileSize(hFile, NULL);

	mem->add_segment(load_address, file_size);

	ReadFile(hFile, &(*mem)[load_address], file_size, &bytes_read, NULL);

	if (bytes_read != file_size) {
		std::cerr << "Not all bytes could be read from the file" << std::endl;
		return 2;
	}

	CloseHandle(hFile);
	return 0;
}

int main()
{
	PhysicalMemory mem;
	ReadFileIntoMemory(&mem, L"C:\\Users\\Andy\\Desktop\\jajaj.bin", 0x0D4100A0); /* boot0 */
	mem.add_segment(0x0D800000, 0x6880); /* GPIO */
	mem.clear_LT_TIMER();

	Processor cpu(&mem);
	while (true) {
		cpu.step();
	}
    return 0;
}

