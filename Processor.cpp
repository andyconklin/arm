#include "stdafx.h"

#include "Processor.h"

namespace {
	DWORD inline rotated_immediate(DWORD instr) {
		DWORD imm = instr & 0x000000FF;
		DWORD rot = ((instr & 0x00000F00) >> 8) * 2;
		DWORD ans = (imm >> rot) | (imm << ((~rot + 1) & 0x1F));
		return ans;
	}

	DWORD inline popcnt(DWORD i) {
		i = i - ((i >> 1) & 0x55555555);
		i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
		return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
	}
};

DWORD Processor::addressing_mode_1(DWORD instr) {
	DWORD I = (instr & 0x02000000) >> 25;
	DWORD S = (instr & 0x00100000) >> 20;
	DWORD Rn = (instr & 0x000F0000) >> 16;
	DWORD Rd = (instr & 0x0000F000) >> 12;
	DWORD shifter_operand;
	if (I) {
		shifter_operand = rotated_immediate(instr);
	}
	else {
		if ((instr & 0x90) == 0x10) {
			/* register shifts */
			DWORD shift = (instr & 0x60) >> 5;
			DWORD Rm = instr & 0xF;
			DWORD Rs = (instr & 0xF00) >> 8;
			if (Rm == 15 || Rm == 15 || Rd == 15 || Rn == 15) {
				std::cout << "UNPREDICTABLE results." << std::endl;
				return 0xDEADC0DE;
			}
			switch (shift) {
			case 0:
				/* LSL */
				shifter_operand = r[Rm] << r[Rs];
				break;
			case 1:
				/* LSR */
				shifter_operand = r[Rm] >> r[Rs];
				break;
			case 2:
				/* ASR */
				shifter_operand = r[Rm] >> r[Rs];
				if (r[Rm] & 0x80000000) {
					shifter_operand |= (~(1 << (32 - r[Rs])) + 1);
				}
				break;
			case 3:
				/* rotate right by register */
				std::cout << "Eff that!" << std::endl;
				return 0xDEADC0DE;
			}
		}
		else if ((instr & 0x10) == 0) {
			/* immediate shifts */
			DWORD Rm = (instr & 0xF);
			DWORD shift = (instr & 0x60) >> 5;
			DWORD shift_imm = (instr & 0xF80) >> 7;
			switch (shift) {
			case 0:
				shifter_operand = r[Rm] << shift_imm;
				break;
			case 1:
				shifter_operand = (shift_imm) ? r[Rm] >> shift_imm : 0;
				break;
			case 2:
				if (!shift_imm)
					shifter_operand = (r[Rm] & 0x80000000) ? 0xFFFFFFFF : 0;
				else {
					shifter_operand = r[Rm] >> shift_imm;
					if (r[Rm] & 0x80000000) {
						shifter_operand |= (~(1 << (32 - shift_imm)) + 1);
					}
				}
			case 3:
				std::cout << "What even is ROR/RRX?" << std::endl;
				return 0xDEADC0DE;
			}
		}
		else {
			std::cout << "This is actually a load/store instruction. FJFJFJF" << std::endl;
			return 0xDEADC0DE;
		}
	}
	return shifter_operand;
}
DWORD Processor::addressing_mode_2(DWORD instr) {
	DWORD I = (instr & 0x02000000) >> 25;
	DWORD U = (instr & 0x00800000) >> 23;
	DWORD Rn = (instr & 0x000F0000) >> 16;
	DWORD calculated_address;
	/* Addressing mode 2 */
	if (I) {
		if (instr & 0x10) {
			std::cout << "not a load or store" << std::endl;
			goto err;
		}
		/* Scaled register offset */
		DWORD shift_imm = (instr & 0x00000F80) >> 7;
		DWORD shift = (instr & 0x60) >> 5;
		DWORD Rm = (instr & 0xF);
		if (Rm == 15) {
			std::cout << "UNPREDICTABLE." << std::endl;
			goto err;
		}
		DWORD index;
		switch (shift) {
		case 0:
			/* LSL */
			index = r[Rm] << shift_imm;
			break;
		case 1:
			/* LSR */
			index = r[Rm] >> shift_imm;
			break;
		case 2:
			/* ASR */
			if (!shift_imm)
				index = (r[Rm] & 0x80000000) ? 0xFFFFFFFF : 0;
			else {
				index = r[Rm] >> shift_imm;
				if (r[Rm] & 0x80000000) {
					index |= (~(1 << (32 - shift_imm)) + 1);
				}
			}
			break;
		case 3:
			/* ROR or RRX */
			std::cout << "Implement this later!" << std::endl;
			goto err;
		}
		calculated_address = U ? r[Rn] + index : r[Rn] - index;
		if (Rn == 15) calculated_address += 8;
	}
	else {
		/* Immediate offset/index */
		calculated_address = U ? r[Rn] + (instr & 0xFFF) : r[Rn] - (instr & 0xFFF);
		if (Rn == 15) calculated_address += 8;
	}
	return calculated_address;
err:
	return 0xDEADC0DE;
}
Processor::Processor(PhysicalMemory *mem) : mem(mem) {
	r[15] = 0x0D4100A0; 
}
void Processor::set_c_flag(BOOL val) {
	if (val) {
		apsr |= 0x20000000;
	}
	else {
		apsr &= ~0x20000000;
	}
}
void Processor::set_n_flag(BOOL val) {
	if (val) {
		apsr |= 0x80000000;
	}
	else {
		apsr &= ~0x80000000;
	}
}
void Processor::set_z_flag(BOOL val) {
	if (val) {
		apsr |= 0x40000000;
	}
	else {
		apsr &= ~0x40000000;
	}
}
BOOL Processor::get_z_flag() {
	return apsr & 0x40000000;
}
BOOL Processor::get_c_flag() {
	return apsr & 0x20000000;
}
void Processor::set_t_bit(BOOL val) {
	if (val) {
		epsr |= 0x01000000;
	}
	else {
		epsr &= ~0x01000000;
	}
}
BOOL Processor::ConditionPassed(DWORD cond) {
	if (cond == 0) // EQ
		return get_z_flag();
	else if (cond == 1) // NE
		return !get_z_flag();
	else if (cond == 0x2) // CS/HS
		return get_c_flag();
	else if (cond == 0x3) // CC/LO
		return !get_c_flag();
	else if (cond == 0x8) // HI
		return (get_c_flag() && !get_z_flag());
	else if (cond == 0x9) // LS
		return (!get_c_flag() || get_z_flag());
	else if (cond == 0xE) // AL
		return true;
	else {
		std::cout << "Unimplemented condition!" << std::endl;
		return false;
	}
}
int Processor::arm_step() {
	DWORD instr = mem->get_u32(r[15]);
	if (instr >> 28 == 0x2) {
		if (!(apsr & 0x20000000)) {
			goto skip;
		}
	}
	else if (instr >> 28 != 0xE) {
		std::cout << "CONDITONAL INSTRUCTION AHOY" << std::endl;
		goto skip;
	}
	if ((instr & 0x0FF00000) == 0x03A00000) {
		/* MOV<cond><S> Rd, # */
		r[(instr & 0x0000F000) >> 12] = rotated_immediate(instr);
	}
	else if ((instr & 0x0F000010) == 0x0E000010) {
		/* MCR or MRC */
		DWORD op1 = (instr & 0x00E00000) >> 21;
		DWORD is_MRC = (instr & 0x00100000) >> 20; // 0 = MCR, 1 = MRC
		DWORD CRn = (instr & 0x000F0000) >> 16;
		DWORD Rd = (instr & 0x0000F000) >> 12;
		DWORD cp_num = (instr & 0x00000F00) >> 8;
		DWORD op2 = (instr & 0x000000E0) >> 5;
		DWORD CRm = (instr & 0x0000000F);
		if (cp_num == 15) {
			if (CRn == 1) {
				if (CRm == 0 && op1 == 0 && op2 == 0) {
					if (!is_MRC) {
						cp15 = r[Rd];
					}
					else {
						r[Rd] = cp15;
					}
				}
			}
			else if (CRn == 7) {
				if (CRm == 5) {
					if (op1 == 0 && op2 == 0 && !is_MRC) {
						std::cout << "MCR: Invalidating Icache." << std::endl;
					}
					else {
						std::cout << "MRC/MCR: OMMR" << std::endl;
					}
				}
				else if (CRm == 6) {
					if (op1 == 0 && op2 == 0 && !is_MRC) {
						std::cout << "MCR: Invalidating Dcache." << std::endl;
					}
					else {
						std::cout << "MRC/MCR: IOWJEFIO" << std::endl;
					}
				}
				else {
					std::cout << "MRC/MCR: OOMMMGG" << std::endl;
				}
			}
			else {
				std::cout << "MRC/MCR: Can't do this." << std::endl;
			}
		}
		else {
			std::cout << "MRC/MCR: Unimplemented CP" << cp_num << " operation." << std::endl;
		}
	}
	else if ((instr & 0x0FE00000) == 0x03800000) {
		DWORD Rn = (instr & 0x000F0000) >> 16;
		DWORD Rd = (instr & 0x0000F000) >> 12;
		DWORD immediate = rotated_immediate(instr);
		r[Rd] = r[Rn] | immediate;
	}
	else if ((instr & 0x0C000000) == 0x04000000) {
		/* Load and Store Word or Unsigned Byte - Immediate Offset */
		DWORD I = (instr & 0x02000000) >> 25;
		DWORD P = (instr & 0x01000000) >> 24;
		DWORD U = (instr & 0x00800000) >> 23;
		DWORD B = (instr & 0x00400000) >> 22;
		DWORD W = (instr & 0x00200000) >> 21;
		DWORD L = (instr & 0x00100000) >> 20;
		DWORD Rn = (instr & 0x000F0000) >> 16;
		DWORD Rd = (instr & 0x0000F000) >> 12;
		DWORD calculated_address = addressing_mode_2(instr);

		DWORD actual_address;
		if (P == 0) {
			actual_address = r[Rn];
			if (Rn == 15) actual_address += 8;
			r[Rn] = calculated_address;
		}
		else {
			actual_address = calculated_address;
			if (W) r[Rn] = calculated_address;
		}

		if (L) {
			if (B) {
				r[Rd] = (DWORD)(*mem)[actual_address];
			}
			else {
				r[Rd] = mem->get_u32(actual_address);
			}
			if (Rd == 15) {
				r[Rd] -= 4;
				if (r[Rd] & 1) {
					r[Rd] &= ~1;
					set_t_bit(true);
				}
			}
		}
		else {
			if (B) {
				(*mem)[actual_address] = r[Rd] & 0xFF;
			}
			else {
				mem->set_u32(actual_address, r[Rd]);
			}
		}
	}
	else if ((instr & 0x0DE00000) == 0x00800000) {
		/* ADD */
		DWORD S = (instr & 0x00100000) >> 20;
		DWORD Rn = (instr & 0x000F0000) >> 16;
		DWORD Rd = (instr & 0x0000F000) >> 12;
		DWORD shifter_operand = addressing_mode_1(instr);
		r[Rd] = r[Rn] + shifter_operand;
	}
	else if ((instr & 0x0E000000) == 0x0A000000) {
		if (instr & 0x01000000)
			r[14] = r[15] + 4;
		DWORD imm = instr & 0x00FFFFFF;
		if (imm & 0x00800000)
			imm |= 0x3F000000;
		imm = imm << 2;
		r[15] = r[15] + imm + 4;
	}
	else if ((instr & 0x0FFFFFF0) == 0x012FFF10) {
		DWORD Rm = instr & 0xF;
		r[15] = r[Rm] - 4;
		if (Rm == 15) r[15] += 8;
		std::cout << "Branch and exchange!" << std::endl;
	}
	else if ((instr & 0x0DF00000) == 0x01500000) {
		DWORD I = (instr & 0x02000000) >> 25;
		DWORD Rn = (instr & 0x000F0000) >> 16;
		if (!I) {
			if ((instr & 0x70) == 0) {
				DWORD Rm = (instr & 0xF);
				if (instr & 0xFF0) {
					std::cout << "Unimplemented CMP" << std::endl;
					goto skip;
				}
				DWORD shifter_operand = r[Rm];
				DWORD alu_out = r[Rn] - shifter_operand;
				set_n_flag(alu_out & 0x80000000);
				set_z_flag(!alu_out);
				set_c_flag(r[Rn] >= shifter_operand);
				// std::cout << "TODO: Handle overflow" << std::endl; // V flag
			}
			else {
				std::cout << "No!" << std::endl;
			}
		}
		else {
			std::cout << "I has to be 0 for CMP for now..." << std::endl;
		}
	}
	else {
		std::cout << "WHAT IS THIS INSTRUCTION???" << std::endl;
	}
skip:
	r[15] += 4;
	return 0;
}
int Processor::thumb_step() {
	WORD instr = mem->get_u16(r[15]);
	if ((instr & 0xF000) == 0x9000) {
		/* Load/store to/from stack */
		DWORD L = (instr & 0x0800) >> 11;
		DWORD Rd = (instr & 0x0700) >> 8;
		DWORD immed_8 = (instr & 0x00FF);
		if (L) {
			DWORD address = r[13] + (immed_8 * 4);
			r[Rd] = mem->get_u32(address);
			if (Rd == 15) r[Rd] -= 2; // can't even be this tbh
		}
		else {
			DWORD address = r[13] + (immed_8 * 4);
			DWORD d = (Rd == 15) ? r[Rd] + 4 : r[Rd]; // can't even be this tbh
			mem->set_u32(address, d);
		}
	}
	else if ((instr & 0xF000) == 0xB000) {
		/* Miscellaneous instruction */
		if ((instr & 0x0600) == 0x0400) {
			/* Push/pop register list */
			DWORD L = (instr & 0x0800) >> 11;
			DWORD R = (instr & 0x0100) >> 8;
			DWORD register_list = (instr & 0x00FF);
			if (L == 0) { /* PUSH */
				DWORD start_address = r[13] - 4 * (R + popcnt(register_list));
				DWORD end_address = r[13] - 4;
				DWORD address = start_address;
				for (DWORD i = 0; i < 8; i++) {
					if (register_list & (1 << i)) {
						std::cout << "Setting mem[" << address << "] = " << r[i] << std::endl;
						mem->set_u32(address, r[i]);
						address += 4;
					}
				}
				if (R) {
					std::cout << "Setting mem[" << address << "] = " << r[14] << std::endl;
					mem->set_u32(address, r[14]);
					address += 4;
				}
				if (end_address != address - 4) {
					std::cout << "WHATHT HTH" << std::endl;
					goto skip;
				}
				r[13] = r[13] - 4 * (R + popcnt(register_list));
			}
			else { /* POP */
				DWORD start_address = r[13];
				DWORD end_address = r[13] + 4 * (R + popcnt(register_list));
				DWORD address = start_address;

				for (DWORD i = 0; i < 8; i++) {
					if (register_list & (1 << i)) {
						std::cout << "Setting r[" << i << "] = mem[" << address << "] = ";
						r[i] = mem->get_u32(address);
						std::cout << r[i] << std::endl;
						address += 4;
					}
				}

				if (R) {
					DWORD value = mem->get_u32(address);
					r[15] = value & 0xFFFFFFFE;
					set_t_bit(value & 1);
					address += 4;
					r[15] -= 2;
				}

				if (end_address != address) {
					std::cout << "ASRMFJgH LFLFWf" << std::endl;
					goto skip;
				}

				r[13] = end_address;
			}
		}
		else if ((instr & 0xFF00) == 0xB000) {
			/* Adjust stack pointer */
			DWORD immed_7 = instr & 0x007F;
			if (instr & 0x0080) { /* SUB (4) */
				r[13] -= (immed_7 << 2);
			}
			else { /* ADD (7) */
				r[13] += (immed_7 << 2);
			}
		}
		else {
			std::cout << "Maldita sea" << std::endl;
			goto skip;
		}
	}
	else if ((instr & 0xFC00) == 0x4400 && (instr & 0x0300) != 0x0300) {
		/* Special data processing: MOV (3) | ADD | CMP | CPY */
		DWORD opcode = (instr & 0x0300) >> 8;
		DWORD H1 = (instr & 0x0080) >> 7;
		DWORD H2 = (instr & 0x0040) >> 6;
		DWORD Rm = (instr & 0x0038) >> 3;
		DWORD Rdn = (instr & 0x0007);
		if (opcode == 2) { /* MOV (3) */
			r[Rdn | (H1 << 3)] = r[Rm | (H2 << 3)];
		}
		else {
			std::cout << "Special opcode not supported" << std::endl;
			goto skip;
		}
	}
	else if ((instr & 0xFF00) == 0x4700) {
		/* Branch/exchange instruction set */
		DWORD L = (instr & 0x0080) >> 7;
		DWORD Rm = (instr & 0x0078) >> 3;
		DWORD m = (Rm == 15) ? r[Rm] + 4 : r[Rm];
		set_t_bit(m & 1);
		r[15] = m & ~1;
		r[15] -= 2;
	}
	else if ((instr & 0xF800) == 0x4800) {
		/* Load from literal pool: LDR(3) */
		DWORD Rd = (instr & 0x0700) >> 8;
		DWORD immed_8 = (instr & 0x00FF);
		DWORD address = ((r[15] + 4) & 0xFFFFFFFC) + (immed_8 * 4);
		r[Rd] = mem->get_u32(address);
		if (Rd == 15) r[Rd] -= 2;
	}
	else if ((instr & 0xE000) == 0x6000) {
		/* Load/store word/byte immediate offset */
		if ((instr & 0xF800) == 0x6800) { /* LDR (1) */
			DWORD immed_5 = (instr & 0x07C0) >> 6;
			DWORD Rn = (instr & 0x0038) >> 3;
			DWORD Rd = (instr & 0x0007);
			DWORD address = r[Rn] + (immed_5 * 4);
			if (Rn == 15) address += 4;
			r[Rd] = mem->get_u32(address);
			if (Rd == 15) r[Rd] -= 2;
		}
		else if ((instr & 0xF800) == 0x6000) { /* STR (1) */
			DWORD immed_5 = (instr & 0x07C0) >> 6;
			DWORD Rn = (instr & 0x0038) >> 3;
			DWORD Rd = (instr & 0x0007);
			DWORD address = r[Rn] + (immed_5 * 4);
			if (Rn == 15) address += 4;
			DWORD data_to_store = r[Rd];
			if (Rd == 15) data_to_store += 4;
			mem->set_u32(address, data_to_store);
		}
		else {
			std::cout << "This load or store instruction is not yet implemented." << std::endl;
			goto skip;
		}
	}
	else if ((instr & 0xE000) == 0x0000 && (instr & 0x1800) != 0x1800) {
		/* Shift by immediate */
		if ((instr & 0x1800) == 0x0000) { /* LSL (1) */
			DWORD immed_5 = (instr & 0x07C0) >> 6;
			DWORD Rm = (instr & 0x0038) >> 3;
			DWORD Rd = (instr & 0x0007);
			if (immed_5) {
				set_c_flag(r[Rm] & (1 << (32 - immed_5)));
				DWORD m = r[Rm];
				if (Rm == 15) m += 4;
				r[Rd] = m << immed_5;
			}
			else {
				r[Rd] = r[Rm];
				if (Rm == 15) r[Rd] += 4;
			}
			set_n_flag(r[Rd] & 0x80000000);
			set_z_flag(!r[Rd]);
			if (Rd == 15) r[Rd] -= 2;
		}
		else if ((instr & 0x1800) == 0x0800) { /* LSR (1) */
			DWORD immed_5 = (instr & 0x07C0) >> 6;
			DWORD Rm = (instr & 0x0038) >> 3;
			DWORD Rd = (instr & 0x0007);
			DWORD m = (Rm == 15) ? r[Rm] + 4 : r[Rm];
			if (immed_5) {
				set_c_flag(m & (1 << (immed_5 - 1)));
				r[Rd] = m >> immed_5;
			}
			else {
				set_c_flag(m & 0x80000000);
				r[Rd] = 0;
			}
			set_n_flag(r[Rd] & 0x80000000);
			set_z_flag(!r[Rd]);
			if (Rd == 15) r[Rd] -= 2;
		}
		else {
			std::cout << "Unimplemented shift by immediate instruction." << std::endl;
			goto skip;
		}
	}
	else if ((instr & 0xFC00) == 0x1800) {
		// It's not possible for PC to be referenced here!
		// So no Rd == 15? etc. needed!!!
		DWORD Rm = (instr & 0x01C0) >> 6;
		DWORD Rn = (instr & 0x38) >> 3;
		DWORD Rd = (instr & 0x7);
		/* Add/subtract register */
		if ((instr & 0x0200) == 0) { /* ADD (3) */
			r[Rd] = r[Rn] + r[Rm];
		}
		else { /* SUB (3) */
			r[Rd] = r[Rn] - r[Rm];
		}
		set_n_flag(r[Rd] & 0x80000000);
		set_z_flag(!r[Rd]);
		// TODO set C and V flags!!!
	}
	else if ((instr & 0xFC00) == 0x1C00) {
		/* Add/subtract immediate */
		if ((instr & 0x0200) == 0) { /* ADD (1) */
			DWORD immed_3 = (instr & 0x01C0) >> 6;
			DWORD Rn = (instr & 0x0038) >> 3;
			DWORD Rd = (instr & 0x0007);
			DWORD n = (Rn == 15) ? r[Rn] + 4 : r[Rn];
			r[Rd] = n + immed_3;
			set_n_flag(r[Rd] & 0x80000000);
			set_z_flag(!r[Rd]);
			// TODO figure out C and V flags... CarryFrom and OverflowFrom
			if (Rd == 15) r[Rd] -= 2;
		}
		else { /* SUB (1) */
			DWORD immed_3 = (instr & 0x01C0) >> 6;
			DWORD Rn = (instr & 0x0038) >> 3;
			DWORD Rd = (instr & 0x0007);
			DWORD n = (Rn == 15) ? r[Rn] + 4 : r[Rn];
			r[Rd] = n - immed_3;
			set_n_flag(r[Rd] & 0x80000000);
			set_z_flag(!r[Rd]);
			// TODO figure out C and V flags... CarryFrom and OverflowFrom
			if (Rd == 15) r[Rd] -= 2;
		}
	}
	else if ((instr & 0xFC00) == 0x4000) {
		/* Data processing register */
		DWORD op_5 = (instr & 0x03C0) >> 6;
		if (op_5 == 0) { /* AND */
			DWORD Rm = (instr & 0x0038) >> 3;
			DWORD Rd = instr & 0x0007;
			DWORD m = (Rm == 15) ? r[Rm] + 4 : r[Rm];
			DWORD d = (Rd == 15) ? r[Rd] + 4 : r[Rd];
			r[Rd] = d & m;
			set_n_flag(r[Rd] & 0x80000000);
			set_z_flag(!r[Rd]);
			if (Rd == 15) r[Rd] -= 2;
		}
		else if (op_5 == 0xC) { /* ORR */
			DWORD Rm = (instr & 0x0038) >> 3;
			DWORD Rd = instr & 0x0007;
			DWORD m = (Rm == 15) ? r[Rm] + 4 : r[Rm];
			DWORD d = (Rd == 15) ? r[Rd] + 4 : r[Rd];
			r[Rd] = d | m;
			set_n_flag(r[Rd] & 0x80000000);
			set_z_flag(!r[Rd]);
			if (Rd == 15) r[Rd] -= 2;
		}
		else if (op_5 == 0xD) { /* MUL */
			DWORD Rm = (instr & 0x0038) >> 3;
			DWORD Rd = (instr & 0x0007);
			DWORD m = (Rm == 15) ? r[Rm] + 4 : r[Rm];
			DWORD d = (Rd == 15) ? r[Rd] + 4 : r[Rd];
			r[Rd] = d * m;
			set_n_flag(r[Rd] & 0x80000000);
			set_z_flag(!r[Rd]);
			if (Rd == 15) r[Rd] -= 2;
		}
		else if (op_5 == 0xA) { /* CMP */
			DWORD Rm = (instr & 0x0038) >> 3;
			DWORD Rn = (instr & 0x0007);
			DWORD alu_out = r[Rn] - r[Rm];
			set_n_flag(alu_out & 0x80000000);
			set_z_flag(!alu_out);
			set_c_flag(r[Rn] >= r[Rm]);
			// TODO set V flag!!! OVERFLOW???
		}
		else {
			std::cout << "Unimplemented data processing register instruction." << std::endl;
			goto skip;
		}
	}
	else if ((instr & 0xF800) == 0xF000) {
		/* BL/BLX prefix */
		DWORD offset_11 = instr & 0x07FF;
		if (offset_11 & 0x0400) {
			offset_11 |= (~0x0400 + 1);
		}
		r[14] = r[15] + 4 + (offset_11 << 12);
	}
	else if ((instr & 0xF800) == 0xF800) {
		/* BL suffix */
		DWORD offset_11 = instr & 0x07FF;
		DWORD address_of_next_instruction = r[15] + 2;
		r[15] = r[14] + (offset_11 << 1) - 2;
		r[14] = address_of_next_instruction | 1;
	}
	else if ((instr & 0xE000) == 0x2000) {
		/* Add/subtract/compare/move immediate */
		DWORD opcode = (instr & 0x1800) >> 11;
		if (opcode == 0) {
			/* MOV (1) */
			DWORD Rd = (instr & 0x0700) >> 8;
			DWORD immed_8 = (instr & 0x00FF);
			r[Rd] = immed_8;
			set_n_flag(FALSE);
			set_z_flag(!r[Rd]);
			if (Rd == 15) r[Rd] -= 2;
		}
		else if (opcode == 1) {
			/* CMP (1) */
			DWORD Rn = (instr & 0x0700) >> 8;
			DWORD immed_8 = (instr & 0x00FF);
			DWORD n = (Rn == 15) ? r[Rn] + 4 : r[Rn];
			DWORD alu_out = n - immed_8;
			set_n_flag(alu_out & 0x80000000);
			set_z_flag(!alu_out);
			set_c_flag(n >= immed_8);
			// TODO set V flag = OverflowFrom(Rn - immed_8)
		}
		else if (opcode == 2) {
			/* ADD (2) */
			DWORD Rd = (instr & 0x0700) >> 8;
			DWORD immed_8 = (instr & 0x00FF);
			r[Rd] = r[Rd] + immed_8;
			set_n_flag(r[Rd] & 0x80000000);
			set_z_flag(!r[Rd]);
			set_c_flag(r[Rd] >= immed_8);
			// SET V FLAG TODO
		}
		else if (opcode == 3) {
			/* SUB (2) */
			DWORD Rd = (instr & 0x0700) >> 8;
			DWORD immed_8 = (instr & 0x00FF);
			r[Rd] = r[Rd] - immed_8;
			set_n_flag(r[Rd] & 0x80000000);
			set_z_flag(!r[Rd]);
			set_c_flag(r[Rd] >= immed_8);
			// SET V FLAG TODO
		}
		else {
			std::cout << "This add/subtract/compare/move immediate instruction is not yet implemented." << std::endl;
			goto skip;
		}
	}
	else if ((instr & 0xF000) == 0xD000 && (instr & 0x0F00) != 0x0E00 && (instr & 0x0F00) != 0x0F00) {
		/* Conditional branch: B(1) */
		DWORD cond = (instr & 0x0F00) >> 8;
		if (ConditionPassed(cond)) {
			DWORD immed_8 = (instr & 0x00FF);
			if (immed_8 & 0x80) {
				immed_8 |= 0xFFFFFF80;
			}
			r[15] = r[15] + 4 + (immed_8 << 1);
			r[15] -= 2;
		}
	}
	else if ((instr & 0xF800) == 0xE000) {
		/* Unconditional branch */
		DWORD signed_immed_11 = instr & 0x07FF;
		if (signed_immed_11 & 0x0400) {
			signed_immed_11 |= 0xFFFFFC00;
		}
		r[15] = r[15] + 4 + (signed_immed_11 << 1);
		r[15] -= 2;
	}
	else {
		std::cout << "Unrecognized encoding" << std::endl;
		goto skip;
	}
skip:
	r[15] += 2;
	return 0;
}
int Processor::step() {
	mem->increment_LT_TIMER();
	if (epsr & 0x01000000) return thumb_step();
	else return arm_step();
}
void Processor::display_info() {
	for (int i = 0; i < 16; i++) {
		std::cout << "r" << i << ": " << std::hex << r[i] << std::endl;
	}
	std::cout << "apsr: " << apsr << std::endl;
	std::cout << "epsr: " << epsr << std::endl << std::dec;
}
