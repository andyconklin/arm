#pragma once

#include "stdafx.h"
#include "PhysicalMemory.h"

class Processor {
	DWORD r[16];
	DWORD cp15 = 0xDEADC0DE;
	DWORD apsr = 0;
	DWORD epsr = 0;
	PhysicalMemory *mem;
	/*
	struct Thumb {
		std::pair<std::function<BOOL(DWORD)>, std::function<BOOL(DWORD)> > filters[]{
			{
				[](DWORD instr) { return (instr & 0xE000) == 0 && (instr & 0x1800) != 0x1800; },
				shift_by_immediate
			},
			{
				[](DWORD instr) { return (instr & 0xFC00) == 0x1800; },
				add_subtract_register
			},
			{
				[](DWORD instr) { return (instr & 0xFC00) == 0x1C00; },
				add_subtract_immediate
			},
			{
				[](DWORD instr) { return (instr & 0xE000) == 0x2000; },
				add_subtract_compare_move_immediate
			},
			{
				[](DWORD instr) { return (instr & 0xFC00) == 0x4000; },
				data_processing_register
			},
			{
				[](DWORD instr) { return (instr & 0xFC00) == 0x4400 && (instr & 0x0300) != 0x0300; },
				special_data_processing
			},
			{
				[](DWORD instr) { return (instr & 0xFF00) == 0x4700; },
				branch_exchange_instruction_set
			},
			{
				[](DWORD instr) { return (instr & 0xF800) == 0x4800; },
				load_from_literal_pool
			},
			{
				[](DWORD instr) { return (instr & 0xF000) == 0x5000; },
				load_store_register_offset
			},
			{
				[](DWORD instr) { return (instr & 0xE000) == 0x6000; },
				load_store_word_byte_immediate_offset
			},
			{
				[](DWORD instr) { return (instr & 0xF000) == 0x8000; },
				load_store_halfword_immediate_offset
			},
			{
				[](DWORD instr) { return (instr & 0xF000) == 0x9000; },
				load_store_to_from_stack
			},
			{
				[](DWORD instr) { return (instr & 0xF000) == 0xA000; },
				add_to_sp_or_pc
			},
			{
				[](DWORD instr) { return (instr & 0xF000) == 0xB000; },
				miscellaneous
			},
			{
				[](DWORD instr) { return (instr & 0xF000) == 0xC000; },
				load_store_multiple
			},
			{
				[](DWORD instr) { return (instr & 0xF000) == 0xD000 && (instr & 0x0E00) != 0x0E00; },
				conditional_branch
			},
			{
				[](DWORD instr) { return (instr & 0xF000) == 0xD000 && (instr & 0x0F00) != 0x0E00; },
				undefined_instruction
			},
			{
				[](DWORD instr) { return (instr & 0xF000) == 0xD000 && (instr & 0x0F00) != 0x0F00; },
				software_interrupt
			},
			{
				[](DWORD instr) { return (instr & 0xF800) == 0xE000; },
				unconditional_branch
			},
			{
				[](DWORD instr) { return (instr & 0xF800) == 0xE800 && !(instr & 1); },
				blx_suffix
			},
			{
				[](DWORD instr) { return (instr & 0xF800) == 0xE800 && (instr & 1); },
				undefined_instruction
			},
			{
				[](DWORD instr) { return (instr & 0xF800) == 0xF000; },
				bl_blx_prefix
			},
			{
				[](DWORD instr) { return (instr & 0xF800) == 0xF800; },
				bl_suffix
			},
		};
	};*/

	DWORD addressing_mode_1(DWORD instr);
	DWORD addressing_mode_2(DWORD instr);
	void set_c_flag(BOOL val);
	void set_n_flag(BOOL val);
	void set_z_flag(BOOL val);
	BOOL get_z_flag();
	BOOL get_c_flag();
	void set_t_bit(BOOL val);
	BOOL ConditionPassed(DWORD cond);
	int arm_step();
	int thumb_step();

public:
	Processor(PhysicalMemory *mem);
	int step();
	void display_info();
};

