#pragma once

#include "stdafx.h"
#include "PhysicalMemory.h"

class Processor {
	DWORD r[16];
	DWORD cp15 = 0xDEADC0DE;
	DWORD cpsr = 0;
	DWORD spsr = 0;
	PhysicalMemory *mem;

	BOOL InAPrivilegedMode();
	BOOL CurrentModeHasSPSR();

	/********************** THUMB INSTRUCTION SET **********************/
	BOOL shift_by_immediate(DWORD);
	BOOL add_subtract_register(DWORD);
	BOOL add_subtract_immediate(DWORD);
	BOOL add_subtract_compare_move_immediate(DWORD);
	BOOL data_processing_register(DWORD);
	BOOL special_data_processing(DWORD);
	BOOL branch_exchange_instruction_set(DWORD);
	BOOL load_from_literal_pool(DWORD);
	BOOL load_store_register_offset(DWORD);
	BOOL load_store_word_byte_immediate_offset(DWORD);
	BOOL load_store_halfword_immediate_offset(DWORD);
	BOOL load_store_to_from_stack(DWORD);
	BOOL add_to_sp_or_pc(DWORD);
	BOOL miscellaneous(DWORD);
	BOOL load_store_multiple(DWORD);
	BOOL conditional_branch(DWORD);
	BOOL undefined_instruction(DWORD);
	BOOL software_interrupt(DWORD);
	BOOL unconditional_branch(DWORD);
	BOOL blx_suffix(DWORD);
	BOOL bl_blx_prefix(DWORD);
	BOOL bl_suffix(DWORD);

	/********************** ARM INSTRUCTION SET **********************/
	BOOL arm_data_processing(DWORD);
	BOOL arm_miscellaneous(DWORD);
	BOOL arm_multiplies_extra_load_stores(DWORD);
	BOOL arm_undefined_instruction(DWORD);
	BOOL arm_move_to_status_register(DWORD);
	BOOL arm_load_store(DWORD);
	BOOL arm_media_instructions(DWORD);
	BOOL arm_architecturally_undefined(DWORD);
	BOOL arm_load_store_multiple(DWORD);
	BOOL arm_branch_and_branch_with_link(DWORD);
	BOOL arm_coprocessor_load_store_and_double_register_transfers(DWORD);
	BOOL arm_coprocessor_data_processing(DWORD);
	BOOL arm_coprocessor_register_transfers(DWORD);
	BOOL arm_software_interrupt(DWORD);
	BOOL arm_unconditional_instructions(DWORD);

	typedef BOOL(Processor::*ProcHandleFn)(DWORD);
	
#define COND1(x) (((x) & 0xF0000000) != 0xF0000000)

	std::pair<std::function<BOOL(DWORD)>, ProcHandleFn> filters[23] {
		{
			[](DWORD instr) { return (instr & 0xE000) == 0 && (instr & 0x1800) != 0x1800; },
			&Processor::shift_by_immediate
		},
		{
			[](DWORD instr) { return (instr & 0xFC00) == 0x1800; },
			&Processor::add_subtract_register
		},
		{
			[](DWORD instr) { return (instr & 0xFC00) == 0x1C00; },
			&Processor::add_subtract_immediate
		},
		{
			[](DWORD instr) { return (instr & 0xE000) == 0x2000; },
			&Processor::add_subtract_compare_move_immediate
		},
		{
			[](DWORD instr) { return (instr & 0xFC00) == 0x4000; },
			&Processor::data_processing_register
		},
		{
			[](DWORD instr) { return (instr & 0xFC00) == 0x4400 && (instr & 0x0300) != 0x0300; },
			&Processor::special_data_processing
		},
		{
			[](DWORD instr) { return (instr & 0xFF00) == 0x4700; },
			&Processor::branch_exchange_instruction_set
		},
		{
			[](DWORD instr) { return (instr & 0xF800) == 0x4800; },
			&Processor::load_from_literal_pool
		},
		{
			[](DWORD instr) { return (instr & 0xF000) == 0x5000; },
			&Processor::load_store_register_offset
		},
		{
			[](DWORD instr) { return (instr & 0xE000) == 0x6000; },
			&Processor::load_store_word_byte_immediate_offset
		},
		{
			[](DWORD instr) { return (instr & 0xF000) == 0x8000; },
			&Processor::load_store_halfword_immediate_offset
		},
		{
			[](DWORD instr) { return (instr & 0xF000) == 0x9000; },
			&Processor::load_store_to_from_stack
		},
		{
			[](DWORD instr) { return (instr & 0xF000) == 0xA000; },
			&Processor::add_to_sp_or_pc
		},
		{
			[](DWORD instr) { return (instr & 0xF000) == 0xB000; },
			&Processor::miscellaneous
		},
		{
			[](DWORD instr) { return (instr & 0xF000) == 0xC000; },
			&Processor::load_store_multiple
		},
		{
			[](DWORD instr) { return (instr & 0xF000) == 0xD000 && (instr & 0x0E00) != 0x0E00; },
			&Processor::conditional_branch
		},
		{
			[](DWORD instr) { return (instr & 0xF000) == 0xD000 && (instr & 0x0F00) == 0x0E00; },
			&Processor::undefined_instruction
		},
		{
			[](DWORD instr) { return (instr & 0xF000) == 0xD000 && (instr & 0x0F00) == 0x0F00; },
			&Processor::software_interrupt
		},
		{
			[](DWORD instr) { return (instr & 0xF800) == 0xE000; },
			&Processor::unconditional_branch
		},
		{
			[](DWORD instr) { return (instr & 0xF800) == 0xE800 && !(instr & 1); },
			&Processor::blx_suffix
		},
		{
			[](DWORD instr) { return (instr & 0xF800) == 0xE800 && (instr & 1); },
			&Processor::undefined_instruction
		},
		{
			[](DWORD instr) { return (instr & 0xF800) == 0xF000; },
			&Processor::bl_blx_prefix
		},
		{
			[](DWORD instr) { return (instr & 0xF800) == 0xF800; },
			&Processor::bl_suffix
		},
	};
	std::pair<std::function<BOOL(DWORD)>, ProcHandleFn> arm_filters[19] {
		{
			[](DWORD instr) { return COND1(instr) && !(instr & 0x0E000010); },
			&Processor::arm_data_processing
		},
		{
			[](DWORD instr) { return COND1(instr) && (instr & 0x0F900010) == 0x01000000; },
			&Processor::arm_miscellaneous
		},
		{
			[](DWORD instr) { return COND1(instr) && (instr & 0x0E000090) == 0x00000010 && (instr & 0x01900000) != 0x01000000; },
			&Processor::arm_data_processing
		},
		{
			[](DWORD instr) { return COND1(instr) && (instr & 0x0F900090) == 0x01000010; },
			&Processor::arm_miscellaneous
		},
		{
			[](DWORD instr) { return COND1(instr) && (instr & 0x0E900090) == 0x00000090; },
			&Processor::arm_multiplies_extra_load_stores
		},
		{
			[](DWORD instr) { return COND1(instr) && (instr & 0x0E000000) == 0x02000000 && (instr & 0x01900000) != 0x01000000; },
			&Processor::arm_data_processing
		},
		{
			[](DWORD instr) { return COND1(instr) && (instr & 0x0FB00000) == 0x03000000; },
			&Processor::arm_undefined_instruction
		},
		{
			[](DWORD instr) { return COND1(instr) && (instr & 0x0FB00000) == 0x03200000; },
			&Processor::arm_move_to_status_register
		},
		{
			[](DWORD instr) { return COND1(instr) && (instr & 0x0E000000) == 0x04000000; },
			&Processor::arm_load_store
		},
		{
			[](DWORD instr) { return COND1(instr) && (instr & 0x0E000010) == 0x06000000; },
			&Processor::arm_load_store
		},
		{
			[](DWORD instr) { return COND1(instr) && (instr & 0x0E000010) == 0x06000010; },
			&Processor::arm_media_instructions
		},
		{
			[](DWORD instr) { return COND1(instr) && (instr & 0x0FF000F0) == 0x07F000F0; },
			&Processor::arm_architecturally_undefined
		},
		{
			[](DWORD instr) { return COND1(instr) && (instr & 0x0E000000) == 0x08000000; },
			&Processor::arm_load_store_multiple
		},
		{
			[](DWORD instr) { return COND1(instr) && (instr & 0x0E000000) == 0x0A000000; },
			&Processor::arm_branch_and_branch_with_link
		},
		{
			[](DWORD instr) { return COND1(instr) && (instr & 0x0E000000) == 0x0C000000; },
			&Processor::arm_coprocessor_load_store_and_double_register_transfers
		},
		{
			[](DWORD instr) { return COND1(instr) && (instr & 0x0F000010) == 0x0E000000; },
			&Processor::arm_coprocessor_data_processing
		},
		{
			[](DWORD instr) { return COND1(instr) && (instr & 0x0F000010) == 0x0E000010; },
			&Processor::arm_coprocessor_register_transfers
		},
		{
			[](DWORD instr) { return COND1(instr) && (instr & 0x0F000000) == 0x0F000000; },
			&Processor::arm_software_interrupt
		},
		{
			[](DWORD instr) { return COND1(instr) && (instr & 0xF0000000) == 0xF0000000; },
			&Processor::arm_unconditional_instructions
		},
	};

	DWORD addressing_mode_1(DWORD instr);
	DWORD addressing_mode_2(DWORD instr);
	std::pair<DWORD, DWORD> addressing_mode_4(DWORD instr);
	void set_c_flag(BOOL val);
	void set_n_flag(BOOL val);
	void set_z_flag(BOOL val);
	void set_v_flag(BOOL val);
	BOOL get_z_flag();
	BOOL get_c_flag();
	BOOL get_n_flag();
	BOOL get_v_flag();
	void set_t_bit(BOOL val);
	BOOL ConditionPassed(DWORD cond);
	int arm_step();
	int thumb_step();

public:
	Processor(PhysicalMemory *mem, DWORD start_addr);
	int step();
	void display_info();
	void continue_until(DWORD addr);
};

