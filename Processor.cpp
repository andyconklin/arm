#include "stdafx.h"

#include "Processor.h"

namespace {
	BOOL shifter_carry_out = 0;

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
	BOOL inline BorrowFrom(DWORD a, DWORD b) {
		return (a < b);
	}
	BOOL inline OverflowFrom(DWORD a, DWORD b, BOOL add) {
		if (add)
			return ((a & 0x80000000) == (b & 0x80000000)) &&
			(((a + b) & 0x80000000) != (a & 0x80000000));
		else
			return ((a & 0x80000000) != (b & 0x80000000)) && 
			(((a - b) & 0x80000000) != (a & 0x80000000));
	}
	BOOL inline CarryFrom(DWORD a, DWORD b) {
		unsigned __int64 x = a;
		unsigned __int64 y = b;
		unsigned __int64 z = x + y;
		return (z >> 32);
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
		if (!((instr >> 8) & 0xF))
			shifter_carry_out = get_c_flag();
		else
			shifter_carry_out = (shifter_operand & 0x80000000) ? 1 : 0;
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
				if (!(r[Rs] & 0xFF)) {
					shifter_operand = r[Rm];
					shifter_carry_out = get_c_flag();
				}
				else if ((r[Rs] & 0xFF) < 32) {
					shifter_operand = r[Rm] << (r[Rs] & 0xFF);
					shifter_carry_out = r[Rm] >> (32 - (r[Rs] & 0xFF));
				}
				else if ((r[Rs] & 0xFF) == 32) {
					shifter_operand = 0;
					shifter_carry_out = r[Rm] & 0x1;
				}
				else { /* Rs[7:0] > 32 */
					shifter_operand = 0;
					shifter_carry_out = 0;
				}
				break;
			case 1:
				/* LSR */
				if ((r[Rs] & 0xFF) == 0) {
					shifter_operand = r[Rm];
					shifter_carry_out = get_c_flag();
				}
				else if ((r[Rs] & 0xFF) < 32) {
					shifter_operand = r[Rm] >> (r[Rs] & 0xFF);
					shifter_carry_out = r[Rm] >> ((r[Rs] & 0xFF) - 1);
				}
				else if ((r[Rs] & 0xFF) == 32) {
					shifter_operand = 0;
					shifter_carry_out = r[Rm] & 0x80000000;
				}
				else /* Rs[7:0] > 32 */ {
					shifter_operand = 0;
					shifter_carry_out = 0;
				}
				break;
			case 2:
				/* ASR */
				if ((r[Rs] & 0xFF) == 0) {
					shifter_operand = r[Rm];
					shifter_carry_out = get_c_flag();
				}
				else if ((r[Rs] & 0xFF) < 32) {
					shifter_operand = r[Rm] >> (r[Rs] & 0xFF);
					if (r[Rm] & 0x80000000)
						shifter_operand |= (~(1 << (32 - (r[Rs] & 0xFF))) + 1);
					shifter_carry_out = r[Rm] >> ((r[Rs] & 0xFF) - 1);
				}
				else { /* Rs[7:0] >= 32 */
					if (!(r[Rm] & 0x80000000)) {
						shifter_operand = 0;
						shifter_carry_out = r[Rm] & 0x80000000;
					}
					else { /* Rm[31] == 1 */
						shifter_operand = 0xFFFFFFFF;
						shifter_carry_out = r[Rm] & 0x80000000;
					}
				}
				break;
			case 3:
				/* rotate right by register */
				throw "Eff that!";
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
				shifter_carry_out = (!shift_imm) ? get_c_flag() : ((r[Rm] >> (32 - shift_imm)) & 0x1);
				break;
			case 1:
				shifter_operand = (shift_imm) ? r[Rm] >> shift_imm : 0;
				shifter_carry_out = (!shift_imm) ? r[Rm] & 0x80000000 : ((r[Rm] >> (shift_imm - 1)) & 0x1);
				break;
			case 2:
				if (!shift_imm) {
					shifter_operand = (r[Rm] & 0x80000000) ? 0xFFFFFFFF : 0;
					shifter_carry_out = r[Rm] & 0x80000000;
				}
				else {
					shifter_operand = r[Rm] >> shift_imm;
					if (r[Rm] & 0x80000000) {
						shifter_operand |= (~(1 << (32 - shift_imm)) + 1);
					}
					shifter_carry_out = (r[Rm] >> (shift_imm - 1)) & 0x1;
				}
				break;
			case 3:
				throw "What even is ROR/RRX?";
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
std::pair<DWORD,DWORD> Processor::addressing_mode_4(DWORD instr) {
	DWORD cond = (instr >> 28) & 0xF;
	DWORD P = (instr >> 24) & 0x1;
	DWORD U = (instr >> 23) & 0x1;
	DWORD S = (instr >> 22) & 0x1;
	DWORD W = (instr >> 21) & 0x1;
	DWORD L = (instr >> 20) & 0x1;
	DWORD Rn = (instr >> 16) & 0xF;
	DWORD register_list = (instr & 0xFFFF);
	DWORD start_address;
	DWORD end_address;
	if (P) {
		if (U) { /* increment before */
			start_address = r[Rn] + ((Rn == 15) ? 12 : 4);
			end_address = r[Rn] + ((Rn == 15) ? 8 : 0) + (popcnt(register_list) * 4);
			if (ConditionPassed(cond) && W) {
				r[Rn] = r[Rn] + ((Rn == 15) ? 4 : 0) + (popcnt(register_list) * 4);
			}
		}
		else { /* decrement before */
			start_address = r[Rn] + ((Rn == 15) ? 8 : 0) - (popcnt(register_list) * 4);
			end_address = r[Rn] + ((Rn == 15) ? 8 : 0) - 4;
			if (ConditionPassed(cond) && W) {
				r[Rn] = r[Rn] + ((Rn == 15) ? 4 : 0) - (popcnt(register_list) * 4);
			}
		}
	}
	else { 
		if (U) { /* increment after */
			start_address = r[Rn] + ((Rn == 15) ? 8 : 0);
			end_address = r[Rn] + ((Rn == 15) ? 8 : 0) + (popcnt(register_list) * 4) - 4;
			if (ConditionPassed(cond) && W) {
				r[Rn] = r[Rn] + ((Rn == 15) ? 4 : 0) + (popcnt(register_list) * 4);
			}
		}
		else { /* decrement after */
			start_address = r[Rn] + ((Rn == 15) ? 8 : 0) - (popcnt(register_list) * 4) + 4;
			end_address = r[Rn] + ((Rn == 15) ? 8 : 0);
			if (ConditionPassed(cond) && W) {
				r[Rn] = r[Rn] + ((Rn == 15) ? 4 : 0) - (popcnt(register_list) * 4);
			}
		}
	}
	return { start_address, end_address };
}
Processor::Processor(PhysicalMemory *mem, DWORD start_addr) : mem(mem) {
	r[15] = start_addr; 
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
BOOL Processor::get_n_flag() {
	return apsr &= 0x80000000;
}
void Processor::set_v_flag(BOOL val) {
	if (val) {
		apsr |= 0x10000000;
	}
	else {
		apsr &= ~0x10000000;
	}
}
BOOL Processor::get_v_flag() {
	return apsr & 0x10000000;
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
	else if (cond == 0x4) // MI
		return get_n_flag();
	else if (cond == 0x5) // PL
		return !get_n_flag();
	else if (cond == 0x6) // VS
		return get_v_flag();
	else if (cond == 0x7) // VC
		return !get_v_flag();
	else if (cond == 0x8) // HI
		return (get_c_flag() && !get_z_flag());
	else if (cond == 0x9) // LS
		return (!get_c_flag() || get_z_flag());
	else if (cond == 0xA) // GE
		return (get_n_flag() == get_v_flag());
	else if (cond == 0xB) // LT
		return (get_n_flag() != get_v_flag());
	else if (cond == 0xC) // GT
		return (!get_z_flag() && (get_n_flag() == get_v_flag()));
	else if (cond == 0xD) // LE
		return (get_z_flag() || (get_n_flag() != get_v_flag()));
	else if (cond == 0xE) // AL
		return true;
	else {
		throw "Unimplemented condition!";
		return false;
	}
}
int Processor::arm_step() {
	/* Delegate the instruction to whichever encoding it belongs to */
	DWORD instr = mem->get_u32(r[15]);
	DWORD f = 0xFAFA;
	for (DWORD i = 0; i < 19; i++) {
		if (arm_filters[i].first(instr)) {
			if (f == 0xFAFA) f = i;
			else {
				std::cout << "Old f: " << f << " and new f: " << i << std::endl;
				throw "Ambiguous thumb instruction reached!";
			}
		}
	}
	if (f == 0xFAFA)
		throw "Unrecognized thumb instruction!";
	BOOL ret = (this->*(this->arm_filters[f].second))(instr);
	if (!ret)
		throw "An instruction function returned false.";
	r[15] += 4;
	return ret;
}
int Processor::thumb_step() {
	/* Delegate the instruction to whichever encoding it belongs to */
	WORD instr = mem->get_u16(r[15]);
	DWORD f = 0xFAFA;
	for (DWORD i = 0; i < 23; i++) {
		if (filters[i].first(instr)) {
			if (f == 0xFAFA) f = i;
			else throw "Ambiguous thumb instruction reached!";
		}
	}
	if (f == 0xFAFA)
		throw "Unrecognized thumb instruction!";
	BOOL ret = (this->*(this->filters[f].second))(instr);
	if (!ret)
		throw "An instruction function returned false.";
    	r[15] += 2;
	return ret;
}
BOOL Processor::shift_by_immediate(DWORD instr) { 
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
		/* V flag unaffected */
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
		/* V flag unaffected */
		if (Rd == 15) r[Rd] -= 2;
	}
	else {
		std::cout << "Unimplemented shift by immediate instruction." << std::endl;
		return false;
	}
	return true;
}
BOOL Processor::add_subtract_register(DWORD instr) { 
	// It's not possible for PC to be referenced here!
	// So no Rd == 15? etc. needed!!!
	DWORD Rm = (instr & 0x01C0) >> 6;
	DWORD Rn = (instr & 0x38) >> 3;
	DWORD Rd = (instr & 0x7);
	/* Add/subtract register */
	if ((instr & 0x0200) == 0) { /* ADD (3) */
		r[Rd] = r[Rn] + r[Rm];
		set_c_flag(CarryFrom(r[Rn], r[Rm]));
		set_v_flag(OverflowFrom(r[Rn], r[Rm], true));
	}
	else { /* SUB (3) */
		r[Rd] = r[Rn] - r[Rm];
		set_c_flag(!BorrowFrom(r[Rn], r[Rm]));
		set_v_flag(OverflowFrom(r[Rn], r[Rm], false));
	}
	set_n_flag(r[Rd] & 0x80000000);
	set_z_flag(!r[Rd]);
	return true;
}
BOOL Processor::add_subtract_immediate(DWORD instr) { 
	if ((instr & 0x0200) == 0) { /* ADD (1) */
		DWORD immed_3 = (instr & 0x01C0) >> 6;
		DWORD Rn = (instr & 0x0038) >> 3;
		DWORD Rd = (instr & 0x0007);
		DWORD n = (Rn == 15) ? r[Rn] + 4 : r[Rn];
		r[Rd] = n + immed_3;
		set_n_flag(r[Rd] & 0x80000000);
		set_z_flag(!r[Rd]);
		set_c_flag(CarryFrom(r[Rn], immed_3));
		set_v_flag(OverflowFrom(r[Rn], immed_3, true));
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
		set_c_flag(!BorrowFrom(r[Rn], immed_3));
		set_v_flag(OverflowFrom(r[Rn], immed_3, false));
		if (Rd == 15) r[Rd] -= 2;
	}
	return true;
}
BOOL Processor::add_subtract_compare_move_immediate(DWORD instr) { 
	DWORD opcode = (instr & 0x1800) >> 11;
	if (opcode == 0) {
		/* MOV (1) */
		DWORD Rd = (instr & 0x0700) >> 8;
		DWORD immed_8 = (instr & 0x00FF);
		r[Rd] = immed_8;
		set_n_flag(FALSE);
		set_z_flag(!r[Rd]);
		/* C and V unaffected. */
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
		set_c_flag(!BorrowFrom(n, immed_8));
		set_v_flag(OverflowFrom(n, immed_8, false));
	}
	else if (opcode == 2) {
		/* ADD (2) */
		DWORD Rd = (instr & 0x0700) >> 8;
		DWORD immed_8 = (instr & 0x00FF);
		r[Rd] = r[Rd] + immed_8;
		set_n_flag(r[Rd] & 0x80000000);
		set_z_flag(!r[Rd]);
		set_c_flag(CarryFrom(r[Rd], immed_8));
		set_v_flag(OverflowFrom(r[Rd], immed_8, true));
	}
	else if (opcode == 3) {
		/* SUB (2) */
		DWORD Rd = (instr & 0x0700) >> 8;
		DWORD immed_8 = (instr & 0x00FF);
		r[Rd] = r[Rd] - immed_8;
		set_n_flag(r[Rd] & 0x80000000);
		set_z_flag(!r[Rd]);
		set_c_flag(!BorrowFrom(r[Rd], immed_8));
		set_v_flag(OverflowFrom(r[Rd], immed_8, false));
	}
	else {
		std::cout << "This add/subtract/compare/move immediate instruction is not yet implemented." << std::endl;
		return false;
	}
	return true;
}
BOOL Processor::data_processing_register(DWORD instr) { 
	DWORD op_5 = (instr & 0x03C0) >> 6;
	if (op_5 == 0) { /* AND */
		DWORD Rm = (instr & 0x0038) >> 3;
		DWORD Rd = instr & 0x0007;
		DWORD m = (Rm == 15) ? r[Rm] + 4 : r[Rm];
		DWORD d = (Rd == 15) ? r[Rd] + 4 : r[Rd];
		r[Rd] = d & m;
		set_n_flag(r[Rd] & 0x80000000);
		set_z_flag(!r[Rd]);
		/* C and V unaffected. */
		if (Rd == 15) r[Rd] -= 2;
	}
	else if (op_5 == 0x1) { /* EOR */
		DWORD Rm = (instr & 0x0038) >> 3;
		DWORD Rd = instr & 0x0007;
		DWORD m = (Rm == 15) ? r[Rm] + 4 : r[Rm];
		DWORD d = (Rd == 15) ? r[Rd] + 4 : r[Rd];
		r[Rd] = d ^ m;
		set_n_flag(r[Rd] & 0x80000000);
		set_z_flag(!r[Rd]);
		/* C and V unaffected. */
		if (Rd == 15) r[Rd] -= 2;
	}
	else if (op_5 == 0x2) { /* LSL (2) */
		DWORD Rs = (instr & 0x0038) >> 3;
		DWORD Rd = instr & 0x0007;
		if (!(r[Rs] & 0xFF)) {
			/* C and Rd unaffected */
		}
		else if ((r[Rs] & 0xFF) < 32) {
			set_c_flag((r[Rd] >> (32 - (r[Rs] & 0xFF))) & 0x1);
			r[Rd] = r[Rd] << (r[Rs] & 0xFF);
		}
		else if ((r[Rs] & 0xFF) == 32) {
			set_c_flag(r[Rd] & 0x1);
			r[Rd] = 0;
		}
		else {
			set_c_flag(false);
			r[Rd] = 0;
		}
		set_n_flag(r[Rd] & 0x80000000);
		set_z_flag(!r[Rd]);
		/* V flag unaffected */
	}
	else if (op_5 == 0x3) { /* LSR (2) */
		DWORD Rs = (instr & 0x0038) >> 3;
		DWORD Rd = instr & 0x0007;
		if (!(r[Rs] & 0xFF)) {
			/* C and Rd unaffected */
		}
		else if ((r[Rs] & 0xFF) < 32) {
			set_c_flag((r[Rd] >> ((r[Rs] & 0xFF) - 1)) & 0x1);
			r[Rd] = r[Rd] >> (r[Rs] & 0xFF);
		}
		else if ((r[Rs] & 0xFF) == 32) {
			set_c_flag(r[Rd] & 0x80000000);
			r[Rd] = 0;
		}
		else {
			set_c_flag(false);
			r[Rd] = 0;
		}
		set_n_flag(r[Rd] & 0x80000000);
		set_z_flag(!r[Rd]);
		/* V flag unaffected */
	}
	else if (op_5 == 0x8) { /* TST */
		DWORD Rm = (instr & 0x0038) >> 3;
		DWORD Rn = instr & 0x0007;
		DWORD m = (Rm == 15) ? r[Rm] + 4 : r[Rm];
		DWORD n = (Rn == 15) ? r[Rn] + 4 : r[Rn];
		DWORD alu_out = n & m;
		set_n_flag(alu_out & 0x80000000);
		set_z_flag(!alu_out);
		/* C and V unaffected. */
	}
	else if (op_5 == 0xA) { /* CMP */
		DWORD Rm = (instr & 0x0038) >> 3;
		DWORD Rn = (instr & 0x0007);
		DWORD alu_out = r[Rn] - r[Rm];
		set_n_flag(alu_out & 0x80000000);
		set_z_flag(!alu_out);
		set_c_flag(!BorrowFrom(r[Rn], r[Rm]));
		set_v_flag(OverflowFrom(r[Rn], r[Rm], false));
	}
	else if (op_5 == 0xC) { /* ORR */
		DWORD Rm = (instr & 0x0038) >> 3;
		DWORD Rd = instr & 0x0007;
		DWORD m = (Rm == 15) ? r[Rm] + 4 : r[Rm];
		DWORD d = (Rd == 15) ? r[Rd] + 4 : r[Rd];
		r[Rd] = d | m;
		set_n_flag(r[Rd] & 0x80000000);
		set_z_flag(!r[Rd]);
		/* C and V unaffected. */
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
		/* C and V unaffected. */
		if (Rd == 15) r[Rd] -= 2;
	}
	else if (op_5 == 0xE) { /* BIC */
		DWORD Rm = (instr & 0x0038) >> 3;
		DWORD Rd = (instr & 0x0007);
		r[Rd] = r[Rd] & ~(r[Rm]);
		set_n_flag(r[Rd] & 0x80000000);
		set_z_flag(!r[Rd]);
		/* C and V unaffected. */
	}
	else if (op_5 == 0xF) { /* MVN */
		DWORD Rm = (instr & 0x0038) >> 3;
		DWORD Rd = (instr & 0x0007);
		r[Rd] = ~r[Rm];
		set_n_flag(r[Rd] & 0x80000000);
		set_z_flag(!r[Rd]);
		/* C and V unaffected. */
	}
	else {
		std::cout << "Unimplemented data processing register instruction." << std::endl;
		return false;
	}
	return true;
}
BOOL Processor::special_data_processing(DWORD instr) { 
	/* Special data processing: MOV (3) | ADD | CMP | CPY */
	DWORD opcode = (instr & 0x0300) >> 8;
	DWORD H1 = (instr & 0x0080) >> 7;
	DWORD H2 = (instr & 0x0040) >> 6;
	DWORD Rm = (instr & 0x0038) >> 3;
	DWORD Rdn = (instr & 0x0007);

	DWORD HRm = Rm | (H2 << 3);
	DWORD HRdn = Rdn | (H1 << 3);

	if (opcode == 0) { /* ADD (4) */
		r[HRdn] = r[HRdn] + ((HRdn == 15) ? 4 : 0) + r[HRm] + ((HRm == 15) ? 4 : 0);
		if (HRdn == 15) r[HRdn] -= 2;
	}
	else if (opcode == 2) { /* MOV (3) */
		r[HRdn] = r[HRm] + ((HRm == 15) ? 4 : 0);
		if (HRdn == 15) r[HRdn] -= 2;
	}
	else {
		std::cout << "Special opcode not supported" << std::endl;
		return false;
	}
	return true;
}
BOOL Processor::branch_exchange_instruction_set(DWORD instr) {
	DWORD L = (instr & 0x0080) >> 7;
	DWORD Rm = (instr & 0x0078) >> 3;
	DWORD m = (Rm == 15) ? r[Rm] + 4 : r[Rm];
	set_t_bit(m & 1);
	r[15] = m & ~1;
	r[15] -= 2;
	return true;
}
BOOL Processor::load_from_literal_pool(DWORD instr) { 
	/* Load from literal pool: LDR(3) */
	DWORD Rd = (instr & 0x0700) >> 8;
	DWORD immed_8 = (instr & 0x00FF);
	DWORD address = ((r[15] + 4) & 0xFFFFFFFC) + (immed_8 * 4);
	r[Rd] = mem->get_u32(address);
	if (Rd == 15) r[Rd] -= 2;
	return true;
}
BOOL Processor::load_store_register_offset(DWORD instr) { 
	DWORD Rm = (instr >> 6) & 0x7;
	DWORD Rn = (instr >> 3) & 0x7;
	DWORD Rd = (instr) & 0x7;
	DWORD address, data;

	if ((instr >> 11) & 0x1) { /* LDR (2) */
		//MemoryAccess(B - bit, E - bit)
		address = r[Rn] + r[Rm];
		//if (CP15_reg1_Ubit == 0)
		//	if address[1:0] == 0b00 then
		//		data = Memory[address, 4]
		//	else
		//		data = UNPREDICTABLE
		//else /* CP15_reg1_Ubit == 1 */
		data = mem->get_u32(address);
		r[Rd] = data;
	}
	else { /* STR (2) */
		//MemoryAccess(B - bit, E - bit)
		//processor_id = ExecutingProcessor()
		address = r[Rn] + r[Rm];
		//if (CP15_reg1_Ubit == 0)
		//	if address[1:0] == 0b00 then
		//		Memory[address, 4] = Rd
		//	else
		//		Memory[address, 4] = UNPREDICTABLE
		//else /* CP15_reg1_Ubit == 1 */
		mem->set_u32(address, r[Rd]);
		//	if Shared(address) then /* from ARMv6 */
		//		physical_address = TLB(address)
		//		ClearExclusiveByAddress(physical_address, 4)
	}
	return true;
}
BOOL Processor::load_store_word_byte_immediate_offset(DWORD instr) { 
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
	else if ((instr & 0xF800) == 0x7800) { /* LDRB (1) */
		DWORD immed_5 = (instr & 0x07C0) >> 6;
		DWORD Rn = (instr & 0x0038) >> 3;
		DWORD Rd = (instr & 0x0007);
		DWORD address = r[Rn] + immed_5;
		r[Rd] = mem->get_u8(address);
	}
	else if ((instr & 0xF800) == 0x7000) { /* STRB (1) */
		DWORD immed_5 = (instr & 0x07C0) >> 6;
		DWORD Rn = (instr & 0x0038) >> 3;
		DWORD Rd = (instr & 0x0007);
		//MemoryAccess(B - bit, E - bit)
		//processor_id = ExecutingProcessor()
		DWORD address = r[Rn] + immed_5;
		mem->set_u8(address, r[Rd] & 0xFF);
		//if Shared(address) then /* from ARMv6 */
		//	physical_address = TLB(address)
		//	ClearExclusiveByAddress(physical_address, 1)
	}
	else {
		std::cout << "This load or store instruction is not yet implemented." << std::endl;
		return false;
	}
	return true;
}
BOOL Processor::load_store_halfword_immediate_offset(DWORD instr) { 
	DWORD immed_5 = (instr >> 6) & 0x1F;
	DWORD Rn = (instr >> 3) & 0x7;
	DWORD Rd = (instr & 0x7);

	if ((instr >> 11) & 0x1) { /* LDRH (1) */
		// MemoryAccess(B - bit, E - bit)
		DWORD address = r[Rn] + (immed_5 * 2);
		//if (CP15_reg1_Ubit == 0)
		//	if address[0] == 0b0 then
		//		data = Memory[address, 2]
		//	else
		//		data = UNPREDICTABLE
		//else /* CP15_reg1_Ubit == 1 */
		DWORD data = mem->get_u16(address);
		r[Rd] = data & 0xFFFF;
	}
	else { /* STRH (1) */
		//MemoryAccess(B - bit, E - bit)
		//processor_id = ExecutingProcessor()
		DWORD address = r[Rn] + (immed_5 * 2);
		//if (CP15_reg1_Ubit == 0)
		//	if address[0] == 0b0 then
		//		Memory[address, 2] = Rd[15:0]
		//	else
		//		Memory[address, 2] = UNPREDICTABLE
		//else /* CP15_reg1_Ubit == 1 */
		mem->set_u16(address, r[Rd] & 0xFFFF);
		//	if Shared(address) then /* from ARMv6 */
		//		physical_address = TLB(address)
		//		ClearExclusiveByAddress(physical_address, 2)
	}
	return true;
}
BOOL Processor::load_store_to_from_stack(DWORD instr) { 
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
	return true; 
}
BOOL Processor::add_to_sp_or_pc(DWORD) { return false; }
BOOL Processor::miscellaneous(DWORD instr) {
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
					mem->set_u32(address, r[i]);
					address += 4;
				}
			}
			if (R) {
				mem->set_u32(address, r[14]);
				address += 4;
			}
			if (end_address != address - 4) {
				std::cout << "WHATHT HTH" << std::endl;
				return false;
			}
			r[13] = r[13] - 4 * (R + popcnt(register_list));
		}
		else { /* POP */
			DWORD start_address = r[13];
			DWORD end_address = r[13] + 4 * (R + popcnt(register_list));
			DWORD address = start_address;

			for (DWORD i = 0; i < 8; i++) {
				if (register_list & (1 << i)) {
					r[i] = mem->get_u32(address);
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
				return false;
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
		return false;
	}
	return true;
}
BOOL Processor::load_store_multiple(DWORD instr) { 
	DWORD Rn = (instr >> 8) & 0x7;
	DWORD register_list = (instr & 0xFF);

	if ((instr >> 11) & 0x1) { /* LDMIA */
		//MemoryAccess(B - bit, E - bit)
		DWORD start_address = r[Rn];
		DWORD end_address = r[Rn] + (popcnt(register_list) * 4) - 4;
		DWORD address = start_address;
		for (int i = 0; i < 8; i++) {
			if ((register_list >> i) & 0x1) {
				r[i] = mem->get_u32(address);
				address += 4;
			}
		}
		if (end_address != address - 4)
			throw "a fit";
		r[Rn] = r[Rn] + (popcnt(register_list) * 4);
	}
	else { /* STMIA */
		// MemoryAccess(B - bit, E - bit)
		// processor_id = ExecutingProcessor()
		DWORD start_address = r[Rn];
		DWORD end_address = r[Rn] + (popcnt(register_list) * 4) - 4;
		DWORD address = start_address;
		for (int i = 0; i < 8; i++) {
			if ((register_list >> i) & 0x1) {
				mem->set_u32(address, r[i]);
				//if Shared(address then /* from ARMv6 */
				//	physical_address = TLB(address
				//		ClearExclusiveByAddress(physical_address, 4)
				address += 4;
			}
		}
		if (end_address != address - 4)
			throw "a fit";
		r[Rn] = r[Rn] + (popcnt(register_list) * 4);
	}
	return true;
}
BOOL Processor::conditional_branch(DWORD instr) { 
	DWORD cond = (instr & 0x0F00) >> 8;
	if (ConditionPassed(cond)) {
		DWORD immed_8 = (instr & 0x00FF);
		if (immed_8 & 0x80) {
			immed_8 |= 0xFFFFFF80;
		}
		r[15] = r[15] + 4 + (immed_8 << 1);
		r[15] -= 2;
	}
	return true;
}
BOOL Processor::undefined_instruction(DWORD) { return false; }
BOOL Processor::software_interrupt(DWORD) { return false; }
BOOL Processor::unconditional_branch(DWORD instr) { 
	DWORD signed_immed_11 = instr & 0x07FF;
	if (signed_immed_11 & 0x0400) {
		signed_immed_11 |= 0xFFFFFC00;
	}
	r[15] = r[15] + 4 + (signed_immed_11 << 1);
	r[15] -= 2;
	return true;
}
BOOL Processor::blx_suffix(DWORD) { return false; }
BOOL Processor::bl_blx_prefix(DWORD instr) { 
	DWORD offset_11 = instr & 0x07FF;
	if (offset_11 & 0x0400) {
		offset_11 |= (~0x0400 + 1);
	}
	r[14] = r[15] + 4 + (offset_11 << 12);
	return true;
}
BOOL Processor::bl_suffix(DWORD instr) { 
	DWORD offset_11 = instr & 0x07FF;
	DWORD address_of_next_instruction = r[15] + 2;
	r[15] = r[14] + (offset_11 << 1) - 2;
	r[14] = address_of_next_instruction | 1;
	return true;
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

void Processor::continue_until(DWORD addr) {
	while (r[15] != addr) step();
}

BOOL Processor::arm_data_processing(DWORD instr) { 
	DWORD cond = (instr >> 28) & 0xF;
	DWORD I = (instr >> 25) & 0x1;
	DWORD opcode = (instr >> 21) & 0xF;
	DWORD S = (instr >> 20) & 0x1;
	DWORD Rn = (instr >> 16) & 0xF;
	DWORD Rd = (instr >> 12) & 0xF;
	DWORD shifter_operand = addressing_mode_1(instr);
	DWORD alu_out = 0;

	switch (opcode) {
	case 0x0:
		if (ConditionPassed(cond)) {
			r[Rd] = ((Rn == 15) ? r[Rn] + 8 : r[Rn]) & shifter_operand;
			if (S && Rd == 15) {
				/* if CurrentModeHasSPSR() then
				     CPSR = SPSR
				   else UNPREDICTABLE */
			}
			else if (S) {
				set_n_flag(r[Rd] & 0x80000000);
				set_z_flag(!r[Rd]);
				set_c_flag(shifter_carry_out);
				/* V Flag = unaffected */
			}
		}
		break;
	case 0x2:
		if (ConditionPassed(cond)) {
			r[Rd] = ((Rn == 15) ? r[Rn] + 8 : r[Rn]) - shifter_operand;
			if (S && Rd == 15) {
				/* if CurrentModeHasSPSR() then
				     CPSR = SPSR
				   else UNPREDICTABLE */
			}
			else if (S) {
				set_n_flag(r[Rd] & 0x80000000);
				set_z_flag(!r[Rd]);
				set_c_flag(!BorrowFrom((Rn == 15) ? r[Rn] + 8 : r[Rn], shifter_operand));
				set_v_flag(OverflowFrom((Rn == 15) ? r[Rn] + 8 : r[Rn], shifter_operand, false));
			}
		}
		break;
	case 0x4:
		if (ConditionPassed(cond)) {
			r[Rd] = ((Rn == 15) ? r[Rn] + 8 : r[Rn]) + shifter_operand;
			if (S && Rd == 15) {
				/* if CurrentModeHasSPSR() then
				     CPSR = SPSR
				   else UNPREDICTABLE */
			}
			else if (S) {
				set_n_flag(r[Rd] & 0x80000000);
				set_z_flag(!r[Rd]);
				set_c_flag(CarryFrom((Rn == 15) ? r[Rn] + 8 : r[Rn], shifter_operand));
				set_v_flag(OverflowFrom((Rn == 15) ? r[Rn] + 8 : r[Rn], shifter_operand, true));
			}
		}
		break;
	case 0x8:
		if (ConditionPassed(cond)) {
			alu_out = ((Rn == 15) ? r[Rn] + 8 : r[Rn]) & shifter_operand;
			set_n_flag(alu_out & 0x80000000);
			set_z_flag(!alu_out);
			set_c_flag(shifter_carry_out);
			/* V flag unaffected. */
		}
		break;
	case 0xA: /* CMP */
		alu_out = (Rn == 15) ? r[Rn] + 8 - shifter_operand : r[Rn] - shifter_operand;
		set_n_flag(alu_out & 0x80000000);
		set_z_flag(!alu_out);
		set_c_flag(!BorrowFrom((Rn == 15) ? r[Rn] + 8 : r[Rn], shifter_operand));
		set_v_flag(OverflowFrom((Rn == 15) ? r[Rn] + 8 : r[Rn], shifter_operand, false));
		break;
	case 0xC: /* ORR */
		if (ConditionPassed(cond)) {
			if (Rn == 15) r[Rd] = (r[Rn] + 8) | shifter_operand;
			else r[Rd] = r[Rn] | shifter_operand;
			if (S && Rd == 15) {
				/* if CurrentModeHasSPSR() then
					CPSR = SPSR
				else UNPREDICTABLE */
			}
			else if (S) {
				set_n_flag(r[Rd] & 0x80000000);
				set_z_flag(!r[Rd]);
				set_c_flag(shifter_carry_out);
				/* V Flag = unaffected */
			}
		}
		break;
	case 0xD: /* MOV */
		if (ConditionPassed(cond)) {
			r[Rd] = shifter_operand;
			if (S && Rd == 15) {
				/* if CurrentModeHasSPSR() then
				     CPSR = SPSR
				   else UNPREDICTABLE */
			}
			else if (S) {
				set_n_flag(r[Rd] & 0x80000000);
				set_z_flag(!r[Rd]);
				set_c_flag(shifter_carry_out);
				/* V Flag = unaffected */
			}
		}
		break;
	case 0xE: /* BIC */
		if (ConditionPassed(cond)) {
			r[Rd] = ((Rn == 15) ? r[Rn] + 8 : r[Rn]) & ~shifter_operand;
			if (S && Rd == 15) {
				/* if CurrentModeHasSPSR() then
					CPSR = SPSR
				else UNPREDICTABLE */
			}
			else if (S) {
				set_n_flag(r[Rd] & 0x80000000);
				set_z_flag(!r[Rd]);
				set_c_flag(shifter_carry_out);
				/* V Flag = unaffected */
			}
		}
		break;
	default:
		throw "Unimplemented ARM instruction: data processing immediate shift";
	}

	if (Rd == 15 && ConditionPassed(cond))
		Rd -= 4; /* Compensate for PC advancement */
	return true;
}
BOOL Processor::arm_miscellaneous(DWORD instr) { 
	if ((instr & 0x0FF000F0) == 0x01200010) { /* BX */
		DWORD cond = (instr >> 28) & 0xF;
		DWORD Rm = (instr & 0xF);
		if (ConditionPassed(cond)) {
			set_t_bit(r[Rm] & 0x1);
			r[15] = (((Rm == 15) ? r[Rm] + 8 : r[Rm]) & 0xFFFFFFFE) - 4;
		}
	}
	else return false;
	return true;
}
BOOL Processor::arm_multiplies_extra_load_stores(DWORD) { return false; }
BOOL Processor::arm_undefined_instruction(DWORD) { return false; }
BOOL Processor::arm_move_immediate_to_status_register(DWORD) { return false; }
BOOL Processor::arm_load_store(DWORD instr) { 
	DWORD cond = (instr >> 28) & 0xF;
	DWORD I = (instr >> 25) & 0x1;
	DWORD P = (instr >> 24) & 0x1;
	DWORD U = (instr >> 23) & 0x1;
	DWORD B = (instr >> 22) & 0x1;
	DWORD W = (instr >> 21) & 0x1;
	DWORD L = (instr >> 20) & 0x1;
	DWORD Rn = (instr >> 16) & 0xF;
	DWORD Rd = (instr >> 12) & 0xF;
	DWORD immediate = (instr & 0xFFF);
	DWORD address = addressing_mode_2(instr);

	if (ConditionPassed(cond)) {
		if (P == 0) {
			/* post-indexed addressing */
			if (L) {
				if (B) r[Rd] = mem->get_u8((Rn == 15) ? r[Rn] + 8 : r[Rn]);
				else r[Rd] = mem->get_u32((Rn == 15) ? r[Rn] + 8 : r[Rn]);
				if (Rd == 15) {
					set_t_bit(r[Rd] & 0x1);
					r[Rd] &= ~0x1;
				}
			}
			else {
				if (B) mem->set_u8((Rn == 15) ? r[Rn] + 8 : r[Rn], (Rd == 15) ? r[Rd] + 8 : r[Rd]);
				else mem->set_u32((Rn == 15) ? r[Rn] + 8 : r[Rn], (Rd == 15) ? r[Rd] + 8 : r[Rd]);
			}
			r[Rn] = (Rn == 15) ? address - 4 : address;
		}
		else {
			/* offset or pre-indexed addressing */
			if (L) {
				if (B) r[Rd] = mem->get_u8(address);
				else r[Rd] = mem->get_u32(address);
				if (Rd == 15) {
					set_t_bit(r[Rd] & 0x1);
					r[Rd] &= ~0x1;
				}
			}
			else {
				if (B) mem->set_u8(address, (Rd == 15) ? r[Rd] + 8 : r[Rd]);
				else mem->set_u32(address, (Rd == 15) ? r[Rd] + 8 : r[Rd]);
			}
			if (W)
				r[Rn] = (Rn == 15) ? address - 4 : address;
		}

		if (Rd == 15)
			r[Rd] -= 4;
	}
	return true;
}
BOOL Processor::arm_media_instructions(DWORD) { return false; }
BOOL Processor::arm_architecturally_undefined(DWORD) { return false; }
BOOL Processor::arm_load_store_multiple(DWORD instr) { 
	DWORD cond = (instr >> 28) & 0xF;
	DWORD P = (instr >> 24) & 0x1;
	DWORD U = (instr >> 23) & 0x1;
	DWORD S = (instr >> 22) & 0x1;
	DWORD W = (instr >> 21) & 0x1;
	DWORD L = (instr >> 20) & 0x1;
	DWORD Rn = (instr >> 16) & 0xF;
	DWORD register_list = (instr & 0xFFFF);
	std::pair<DWORD, DWORD> start_end_addresses = addressing_mode_4(instr);

	if (S == 0 && L == 0) {
		/* STM (1) */
		//MemoryAccess(B - bit, E - bit)
		//processor_id = ExecutingProcessor()
		if (ConditionPassed(cond)) {
			DWORD address = start_end_addresses.first;
			for (int i = 0; i < 16; i++) {
				if ((register_list >> i) & 0x1) {
					mem->set_u32(address, r[i] + ((i == 15) ? 8 : 0));
					address += 4;
					//if Shared(address) then /* from ARMv6 */
					//	physical_address = TLB(address)
					//	ClearExclusiveByAddress(physical_address, processor_id, 4)
						/* See Summary of operation on page A2-49 */
				}
			}
			if (start_end_addresses.second != address - 4)
				throw "a fit";
		}
	}
	else if (S == 0 && L == 1) {
		/* LDM (1) */
		//MemoryAccess(B-bit, E-bit)
		if (ConditionPassed(cond)) {
			DWORD address = start_end_addresses.first;
			for (int i = 0; i < 15; i++) {
				if ((register_list >> i) & 0x1) {
					r[i] = mem->get_u32(address);
					address += 4;
				}
			}
			if ((register_list >> 15) & 0x1) {
				DWORD value = mem->get_u32(address);
				//if (architecture version 5 or above) then
				r[15] = (value & 0xFFFFFFFE) - 4;
				set_t_bit(value & 0x1);
				//else
				//	pc = value AND 0xFFFFFFFC
				address += 4;
			}
			if (start_end_addresses.second != address - 4)
				throw "a fit";
		}
	}
	else return false;
	return true;
}
BOOL Processor::arm_branch_and_branch_with_link(DWORD instr) { 
	DWORD cond = (instr >> 28) & 0xF;
	DWORD L = (instr >> 24) & 0x1;
	DWORD signed_immed_24 = (instr & 0xFFFFFF);
	if (signed_immed_24 & 0x800000)
		signed_immed_24 |= (~(0x800000) + 1);

	if (ConditionPassed(cond)) {
		if (L) {
			r[14] = r[15] + 4;
		}
		r[15] = r[15] + 4 + (signed_immed_24 << 2);
	}
	return true;
}
BOOL Processor::arm_coprocessor_load_store_and_double_register_transfers(DWORD) { return false; }
BOOL Processor::arm_coprocessor_data_processing(DWORD) { return false; }
BOOL Processor::arm_coprocessor_register_transfers(DWORD instr) { 
	DWORD cond = (instr >> 28) & 0xF;
	DWORD opcode_1 = (instr >> 21) & 0x7;
	DWORD L = (instr >> 20) & 0x1;
	DWORD CRn = (instr >> 16) & 0xF;
	DWORD Rd = (instr >> 12) & 0xF;
	DWORD cp_num = (instr >> 8) & 0xF;
	DWORD opcode_2 = (instr >> 5) & 0x7;
	DWORD CRm = instr & 0xF;

	if (L) {
		if (cp_num == 15 && opcode_1 == 0 && opcode_2 == 0 && CRn == 1 && CRm == 0) {
			std::cout << "TODO: read from control register (c1)." << std::endl;
		}
		else return false;
	}
	else {
		if (cp_num == 15 && opcode_1 == 0 && opcode_2 == 0 && CRn == 7 && CRm == 5) {
			std::cout << "Invalidate entire instruction cache." << std::endl;
		}
		else if (cp_num == 15 && opcode_1 == 0 && opcode_2 == 0 && CRn == 7 && CRm == 6) {
			std::cout << "Invalidate entire data cache." << std::endl;
		} else if (cp_num == 15 && opcode_1 == 0 && opcode_2 == 0 && CRn == 1 && CRm == 0) {
			std::cout << "TODO: write to control register (c1)." << std::endl;
		}
		else return false;
	}
	return true;
}
BOOL Processor::arm_software_interrupt(DWORD) { return false; }
BOOL Processor::arm_unconditional_instructions(DWORD) { return false; }