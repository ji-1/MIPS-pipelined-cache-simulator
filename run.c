/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   SCE212 Ajou University                                    */
/*   run.c                                                     */
/*   Adapted from CS311@KAIST                                  */
/*                                                             */
/***************************************************************/

#include <stdio.h>

#include "util.h"
#include "run.h"

/***************************************************************/
/*                                                             */
/* Procedure: get_inst_info                                    */
/*                                                             */
/* Purpose: Read insturction information                       */
/*                                                             */
/***************************************************************/
instruction* get_inst_info(uint32_t pc) {
    return &INST_INFO[(pc - MEM_TEXT_START) >> 2];
}

/***************************************************************/
/*                                                             */
/* Procedure: IFetch_Stage                                     */
/*                                                             */
/* Purpose: Instruction fetch                                  */
/*                                                             */
/***************************************************************/
void IFetch_Stage() {

    if (CURRENT_STATE.PIPE_STALL[MEM_STAGE]==TRUE) {
	return;
    }
    if (FETCH_BIT == FALSE/* || CURRENT_STATE.PIPE_STALL[IF_STAGE] == TRUE*/) {
	//FOR DEBUG

	CURRENT_STATE.PIPE[IF_STAGE] = 0;
	CURRENT_STATE.IF_ID_INST = 0;
	return;
    }

    if (CURRENT_STATE.PIPE_STALL[IF_STAGE] == FALSE) {
	CURRENT_STATE.PIPE[IF_STAGE] = CURRENT_STATE.PC;
	CURRENT_STATE.IF_PC = CURRENT_STATE.PC + BYTES_PER_WORD;
    } else {
	// IF stall
	CURRENT_STATE.IF_PC = CURRENT_STATE.PC;
    }

    if (CURRENT_STATE.PIPE_STALL[ID_STAGE] == FALSE) {
	CURRENT_STATE.IF_ID_INST = CURRENT_STATE.PIPE[IF_STAGE];
	CURRENT_STATE.IF_ID_NPC = CURRENT_STATE.PIPE[IF_STAGE] + BYTES_PER_WORD;
	if (CURRENT_STATE.PIPE_STALL[IF_STAGE] == TRUE) {
	    /** Resuming from a pipeline stall */
	    CURRENT_STATE.IF_PC = CURRENT_STATE.PC + BYTES_PER_WORD;
	    CURRENT_STATE.PIPE_STALL[IF_STAGE] = FALSE;
	}
    } else { /** ID Stage is stalled */
	CURRENT_STATE.PIPE_STALL[IF_STAGE] = TRUE;
	CURRENT_STATE.IF_PC = CURRENT_STATE.PC;
    }
}

/***************************************************************/
/*                                                             */
/* Procedure: IDecode_Stage                                    */
/*                                                             */
/* Purpose: Instruction decode                                 */
/*                                                             */
/***************************************************************/
void IDecode_Stage() {
    instruction *inst;

    if (CURRENT_STATE.PIPE_STALL[MEM_STAGE]==TRUE) {
	return;
    }

    if (!(FETCH_BIT == FALSE && CURRENT_STATE.PIPE_STALL[ID_STAGE] == TRUE)) {
	CURRENT_STATE.PIPE[ID_STAGE] = CURRENT_STATE.IF_ID_INST;
    }

    if (CURRENT_STATE.PIPE[ID_STAGE] == 0) {
	return;
    }

    inst = get_inst_info(CURRENT_STATE.PIPE[ID_STAGE]);
    CURRENT_STATE.PIPE_STALL[ID_STAGE] = FALSE;

    switch (OPCODE(inst)) {
	case 0x9:		//ADDIU
	case 0xc:		//ANDI
	case 0xd:		//ORI
	case 0xb:		//SLTIU
	    if (CURRENT_STATE.REGS_LOCK[RS(inst)]) {
		CURRENT_STATE.PIPE_STALL[ID_STAGE] = TRUE;
		return;
	    }
	    break;
	case 0xf:		//LUI
	    break;		//No dependency
	case 0x23:		//LW
	    if (CURRENT_STATE.REGS_LOCK[BASE(inst)]) {
		CURRENT_STATE.PIPE_STALL[ID_STAGE] = TRUE;
		return;
	    }
	    break;
	case 0x2b:		//SW
	    if (CURRENT_STATE.REGS_LOCK[BASE(inst)] || CURRENT_STATE.REGS_LOCK[RT(inst)]) {
		CURRENT_STATE.PIPE_STALL[ID_STAGE] = TRUE;
		return;
	    }
	    break;
	case 0x4:		//BEQ
	case 0x5:		//BNE
	    if (CURRENT_STATE.REGS_LOCK[RS(inst)] || CURRENT_STATE.REGS_LOCK[RT(inst)]) {
		CURRENT_STATE.PIPE_STALL[ID_STAGE] = TRUE;
		return;
	    }
	    break;
	case 0x0:
	    switch (FUNC(inst)) {
		case 0x21:	//ADDU
		case 0x24:	//AND
		case 0x27:	//NOR
		case 0x25:	//OR
		case 0x2B:	//SLTU
		case 0x23:	//SUBU
		    if (CURRENT_STATE.REGS_LOCK[RS(inst)] || CURRENT_STATE.REGS_LOCK[RT(inst)]) {
			CURRENT_STATE.PIPE_STALL[ID_STAGE] = TRUE;
			return;
		    }
		    break;
		case 0x0:	//SLL
		case 0x2:	//SRL
		    if (CURRENT_STATE.REGS_LOCK[RT(inst)]) {
			CURRENT_STATE.PIPE_STALL[ID_STAGE] = TRUE;
			return;
		    }
		    break;
		case 0x8:	//JR
		    if (CURRENT_STATE.REGS_LOCK[RS(inst)]) {
			CURRENT_STATE.PIPE_STALL[ID_STAGE] = TRUE;
			return;
		    }
		    break;
		default:
		    printf("Unknown function code type: 0x%x\n", FUNC(inst));
		    break;
	    }
	    break;
	case 0x2:		//J
	case 0x3:		//JAL
	    break;		//No dependency
	default:
	    printf("Unknown instruction type: 0x%x\n", OPCODE(inst));
	    break;
    }

    if (CURRENT_STATE.PIPE_STALL[ID_STAGE] == FALSE) {
	CURRENT_STATE.ID_EX_DEST = MIPS_REGS;

	switch (OPCODE(inst)) {
	    case 0x9:		//ADDIU
	    case 0xc:		//ANDI
	    case 0xd:		//ORI
	    case 0xb:		//SLTIU
		CURRENT_STATE.REGS_LOCK[RT(inst)]++;			//Lock dest register
		CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.REGS[RS(inst)];
		CURRENT_STATE.ID_EX_IMM = IMM(inst);
		CURRENT_STATE.ID_EX_DEST = RT(inst);
		break;
	    case 0xf:		//LUI
		CURRENT_STATE.REGS_LOCK[RT(inst)]++;			//Lock dest register
		CURRENT_STATE.ID_EX_IMM = IMM(inst);
		CURRENT_STATE.ID_EX_DEST = RT(inst);
		break;
	    case 0x23:		//LW
		CURRENT_STATE.REGS_LOCK[RT(inst)]++;			//Lock dest register
		CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.REGS[BASE(inst)];
		CURRENT_STATE.ID_EX_IMM = IOFFSET(inst);
		CURRENT_STATE.ID_EX_DEST = RT(inst);
	    case 0x2b:		//SW
		CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.REGS[BASE(inst)];
		CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.REGS[RT(inst)];
		CURRENT_STATE.ID_EX_IMM = IOFFSET(inst);
		break;
	    case 0x4:		//BEQ
	    case 0x5:		//BNE
		CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.REGS[RS(inst)];
		CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.REGS[RT(inst)];
		CURRENT_STATE.ID_EX_IMM = IDISP(inst);
		break;
	    case 0x0:
		switch (FUNC (inst)) {
		    case 0x21:	//ADDU
		    case 0x24:	//AND
		    case 0x27:	//NOR
		    case 0x25:	//OR
		    case 0x2B:	//SLTU
		    case 0x23:	//SUBU
			CURRENT_STATE.REGS_LOCK[RD(inst)]++;	//Lock dest register
			CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.REGS[RS(inst)];
			CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.REGS[RT(inst)];
			CURRENT_STATE.ID_EX_DEST = RD(inst);
			break;
		    case 0x0:	//SLL
		    case 0x2:	//SRL
			CURRENT_STATE.REGS_LOCK[RD(inst)]++;	//Lock dest register
			CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.REGS[RT(inst)];
			CURRENT_STATE.ID_EX_IMM = (short)SHAMT(inst);
			CURRENT_STATE.ID_EX_DEST = RD(inst);
			break;
		    case 0x8:	//JR
			CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.REGS[RS(inst)];
			CURRENT_STATE.JUMP_PC = CURRENT_STATE.ID_EX_REG1;
			break;
		    default:
			printf("Unknown function code type: 0x%x\n", FUNC(inst));
			break;
		}
		break;
	    case 0x2:		//J
		CURRENT_STATE.ID_EX_REG1 = (uint32_t)TARGET(inst) << 2;
		CURRENT_STATE.JUMP_PC = CURRENT_STATE.ID_EX_REG1;
		break;
	    case 0x3:		//JAL
		CURRENT_STATE.REGS_LOCK[31]++;			//Lock $31 register
		CURRENT_STATE.ID_EX_REG1 = (uint32_t)TARGET(inst) << 2;
		CURRENT_STATE.JUMP_PC = CURRENT_STATE.ID_EX_REG1;
		CURRENT_STATE.ID_EX_DEST = 31;
		break;
	    default:
		printf("Unknown instruction type: 0x%x\n", OPCODE(inst));
		break;
	}

	CURRENT_STATE.ID_EX_NPC = CURRENT_STATE.IF_ID_NPC;
    }
}

/***************************************************************/
/*                                                             */
/* Procedure: Execute_Stage                                    */
/*                                                             */
/* Purpose: Instruction execution                              */
/*                                                             */
/***************************************************************/
void Execute_Stage() {
    instruction *inst;

    if (CURRENT_STATE.PIPE_STALL[MEM_STAGE]==TRUE) {
	return;
    }


    if (CURRENT_STATE.PIPE_STALL[ID_STAGE] == FALSE) {
	CURRENT_STATE.PIPE[EX_STAGE] = CURRENT_STATE.PIPE[ID_STAGE];
    } else {
	CURRENT_STATE.PIPE[EX_STAGE] = 0;
    }

    if (CURRENT_STATE.PIPE[EX_STAGE] == 0) {
	CURRENT_STATE.EX_MEM_DEST = 0;
	CURRENT_STATE.EX_MEM_ALU_OUT = 0;
	return;
    }

    inst = get_inst_info(CURRENT_STATE.PIPE[EX_STAGE]);

    //Forwarding and unlock registers
    if (FORWARDING_BIT == TRUE) {
	CURRENT_STATE.EX_MEM_FORWARD_REG = CURRENT_STATE.EX_MEM_DEST;
	CURRENT_STATE.EX_MEM_FORWARD_VALUE = CURRENT_STATE.EX_MEM_ALU_OUT;

	switch (OPCODE(inst)) {
	    case 0x9:		//ADDIU
	    case 0xc:		//ANDI
	    case 0xd:		//ORI
	    case 0xb:		//SLTIU
		CURRENT_STATE.REGS_LOCK[RT(inst)] = FALSE;			//Unlock dest register
		if (RS(inst) == CURRENT_STATE.MEM_WB_FORWARD_REG) {
		    CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.MEM_WB_FORWARD_VALUE;
		}
		if (RS(inst) == CURRENT_STATE.EX_MEM_FORWARD_REG) {
		    CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.EX_MEM_FORWARD_VALUE;
		}
		break;
	    case 0xf:		//LUI
		CURRENT_STATE.REGS_LOCK[RT(inst)] = FALSE;			//Unlock dest register
		break;
	    case 0x23:		//LW
		if (BASE(inst) == CURRENT_STATE.MEM_WB_FORWARD_REG) {
		    CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.MEM_WB_FORWARD_VALUE;
		}
		if (BASE(inst) == CURRENT_STATE.EX_MEM_FORWARD_REG) {
		    CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.EX_MEM_FORWARD_VALUE;
		}
	    case 0x2b:		//SW
		if (BASE(inst) == CURRENT_STATE.MEM_WB_FORWARD_REG) {
		    CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.MEM_WB_FORWARD_VALUE;
		}
		if (BASE(inst) == CURRENT_STATE.EX_MEM_FORWARD_REG) {
		    CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.EX_MEM_FORWARD_VALUE;
		}
		if (RT(inst) == CURRENT_STATE.MEM_WB_FORWARD_REG) {
		    CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.MEM_WB_FORWARD_VALUE;
		}
		if (RT(inst) == CURRENT_STATE.EX_MEM_FORWARD_REG) {
		    CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.EX_MEM_FORWARD_VALUE;
		}
		break;
	    case 0x4:		//BEQ
	    case 0x5:		//BNE
		if (RS(inst) == CURRENT_STATE.MEM_WB_FORWARD_REG) {
		    CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.MEM_WB_FORWARD_VALUE;
		}
		if (RS(inst) == CURRENT_STATE.EX_MEM_FORWARD_REG) {
		    CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.EX_MEM_FORWARD_VALUE;
		}
		if (RT(inst) == CURRENT_STATE.MEM_WB_FORWARD_REG) {
		    CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.MEM_WB_FORWARD_VALUE;
		}
		if (RT(inst) == CURRENT_STATE.EX_MEM_FORWARD_REG) {
		    CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.EX_MEM_FORWARD_VALUE;
		}
		break;
	    case 0x0:
		switch (FUNC(inst)) {
		    case 0x21:	//ADDU
		    case 0x24:	//AND
		    case 0x27:	//NOR
		    case 0x25:	//OR
		    case 0x2B:	//SLTU
		    case 0x23:	//SUBU
			CURRENT_STATE.REGS_LOCK[RD(inst)] = FALSE;	//Unlock dest register
			//for debug

			if (RS(inst) == CURRENT_STATE.MEM_WB_FORWARD_REG) {
			    CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.MEM_WB_FORWARD_VALUE;
			}
			//for debug

			if (RS(inst) == CURRENT_STATE.EX_MEM_FORWARD_REG) {
			    CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.EX_MEM_FORWARD_VALUE;
			}
			//for debug
			//printf("RS: %d\n", CURRENT_STATE.ID_EX_REG1);

			if (RT(inst) == CURRENT_STATE.MEM_WB_FORWARD_REG) {
			    CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.MEM_WB_FORWARD_VALUE;
			}
			//for debug
			//printf("RS: %d\n", CURRENT_STATE.ID_EX_REG2);

			if (RT(inst) == CURRENT_STATE.EX_MEM_FORWARD_REG) {
			    CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.EX_MEM_FORWARD_VALUE;
			}
			//for debug
			//printf("RS: %d\n", CURRENT_STATE.ID_EX_REG2);

			break;
		    case 0x0:	//SLL
		    case 0x2:	//SRL
			CURRENT_STATE.REGS_LOCK[RD(inst)] = FALSE;	//Unlock dest register
			if (RT(inst) == CURRENT_STATE.MEM_WB_FORWARD_REG) {
			    CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.MEM_WB_FORWARD_VALUE;
			}
			if (RT(inst) == CURRENT_STATE.EX_MEM_FORWARD_REG) {
			    CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.EX_MEM_FORWARD_VALUE;
			}
			break;
		    case 0x8:	//JR
			if (RS(inst) == CURRENT_STATE.MEM_WB_FORWARD_REG) {
			    CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.MEM_WB_FORWARD_VALUE;
			}
			if (RS(inst) == CURRENT_STATE.EX_MEM_FORWARD_REG) {
			    CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.EX_MEM_FORWARD_VALUE;
			}
			break;
		    default:
			printf("Unknown function code type: 0x%x\n", FUNC(inst));
			break;
		}
		break;
	    case 0x2:		//J
		break;		//Nothing to do
	    case 0x3:		//JAL
		CURRENT_STATE.REGS_LOCK[31] = FALSE;			//Unlock $31 register
		break;
	    default:
		printf("Unknown instruction type: 0x%x\n", OPCODE(inst));
		break;
	}
    }

    switch (OPCODE(inst)) {
	case 0x9:		//ADDIU
	    CURRENT_STATE.EX_MEM_ALU_OUT = CURRENT_STATE.ID_EX_REG1 + CURRENT_STATE.ID_EX_IMM;
	    break;
	case 0xc:		//ANDI
	    CURRENT_STATE.EX_MEM_ALU_OUT = CURRENT_STATE.ID_EX_REG1 & (0xffff & CURRENT_STATE.ID_EX_IMM);
	    break;
	case 0xf:		//LUI
	    CURRENT_STATE.EX_MEM_ALU_OUT = (CURRENT_STATE.ID_EX_IMM << 16) & 0xffff0000;
	    break;
	case 0xd:		//ORI
	    CURRENT_STATE.EX_MEM_ALU_OUT = CURRENT_STATE.ID_EX_REG1 | (0xffff & CURRENT_STATE.ID_EX_IMM);
	    break;
	case 0xb:		//SLTIU
	    {
		int x = (short)CURRENT_STATE.ID_EX_IMM;

		if ((uint32_t) CURRENT_STATE.ID_EX_REG1 < (uint32_t) x) {
		    CURRENT_STATE.EX_MEM_ALU_OUT = 1;
		} else {
		    CURRENT_STATE.EX_MEM_ALU_OUT = 0;
		}
		break;
	    }
	case 0x23:		//LW
	    CURRENT_STATE.EX_MEM_ALU_OUT = CURRENT_STATE.ID_EX_REG1 + CURRENT_STATE.ID_EX_IMM;
	    break;
	case 0x2b:		//SW
	    CURRENT_STATE.EX_MEM_ALU_OUT = CURRENT_STATE.ID_EX_REG1 + CURRENT_STATE.ID_EX_IMM;
	    CURRENT_STATE.EX_MEM_W_VALUE = CURRENT_STATE.ID_EX_REG2; //?
	    break;
	case 0x4:		//BEQ
	    //FOR DEBUG
	    //printf("%d %d\n", CURRENT_STATE.ID_EX_REG1, CURRENT_STATE.ID_EX_REG2);

	    if (CURRENT_STATE.ID_EX_REG1 == CURRENT_STATE.ID_EX_REG2) {
		CURRENT_STATE.EX_MEM_BR_TAKE = TRUE;
		CURRENT_STATE.EX_MEM_BR_TARGET = CURRENT_STATE.ID_EX_NPC + CURRENT_STATE.ID_EX_IMM;
		//                /** Miss prediction, should have taken! */
		//                CURRENT_STATE.BRANCH_PC = CURRENT_STATE.ID_EX_NPC + CURRENT_STATE.ID_EX_IMM;
		//                Flush_By_Branch_EX();
	    } else {
		CURRENT_STATE.EX_MEM_BR_TAKE = FALSE;
	    }
	    break;
	case 0x5:		//BNE
	    if (CURRENT_STATE.ID_EX_REG1 != CURRENT_STATE.ID_EX_REG2) {
		CURRENT_STATE.EX_MEM_BR_TAKE = TRUE;
		CURRENT_STATE.EX_MEM_BR_TARGET = CURRENT_STATE.ID_EX_NPC + CURRENT_STATE.ID_EX_IMM;
		//                /** Miss prediction, should have taken! */
		//                CURRENT_STATE.BRANCH_PC = CURRENT_STATE.ID_EX_NPC + CURRENT_STATE.ID_EX_IMM;
		//                Flush_By_Branch_EX();
	    } else {
		CURRENT_STATE.EX_MEM_BR_TAKE = FALSE;
	    }
	    break;
	case 0x0:
	    switch (FUNC(inst)) {
		case 0x21:	//ADDU
		    CURRENT_STATE.EX_MEM_ALU_OUT = CURRENT_STATE.ID_EX_REG1 + CURRENT_STATE.ID_EX_REG2;
		    break;
		case 0x24:	//AND
		    CURRENT_STATE.EX_MEM_ALU_OUT = CURRENT_STATE.ID_EX_REG1 & CURRENT_STATE.ID_EX_REG2;
		    break;
		case 0x27:	//NOR
		    CURRENT_STATE.EX_MEM_ALU_OUT = ~ (CURRENT_STATE.ID_EX_REG1 | CURRENT_STATE.ID_EX_REG2);
		    //for debug
		    //printf("reg1: %d, reg2: %d\n", CURRENT_STATE.ID_EX_REG1, CURRENT_STATE.ID_EX_REG2);
		    break;
		case 0x25:	//OR
		    CURRENT_STATE.EX_MEM_ALU_OUT = CURRENT_STATE.ID_EX_REG1 | CURRENT_STATE.ID_EX_REG2;
		    break;
		case 0x2B:	//SLTU
		    if (CURRENT_STATE.ID_EX_REG1 < CURRENT_STATE.ID_EX_REG2) {
			CURRENT_STATE.EX_MEM_ALU_OUT = 1;
		    } else {
			CURRENT_STATE.EX_MEM_ALU_OUT = 0;
		    }
		    break;
		case 0x0:	//SLL
		    {
			int shamt = CURRENT_STATE.ID_EX_IMM;
			if (shamt >= 0 && shamt < 32) {
			    CURRENT_STATE.EX_MEM_ALU_OUT = CURRENT_STATE.ID_EX_REG2 << shamt;
			} else {
			    CURRENT_STATE.EX_MEM_ALU_OUT = CURRENT_STATE.ID_EX_REG2;
			}
			break;
		    }
		case 0x2:	//SRL
		    {
			int shamt = CURRENT_STATE.ID_EX_IMM;
			uint32_t val = CURRENT_STATE.ID_EX_REG2;
			if (shamt >= 0 && shamt < 32) {
			    CURRENT_STATE.EX_MEM_ALU_OUT = val >> shamt;
			} else {
			    CURRENT_STATE.EX_MEM_ALU_OUT = val;
			}
			break;
		    }
		case 0x23:	//SUBU
		    CURRENT_STATE.EX_MEM_ALU_OUT = CURRENT_STATE.ID_EX_REG1 - CURRENT_STATE.ID_EX_REG2;
		    break;

		case 0x8:	//JR
		    Flush_By_Jump();
		    CURRENT_STATE.PC = CURRENT_STATE.ID_EX_REG1;
		    break;
		default:
		    printf("Unknown function code type: 0x%x\n", FUNC(inst));
		    break;
	    }
	    break;
	case 0x2:		//J
	    Flush_By_Jump();
	    //CURRENT_STATE.PC = CURRENT_STATE.ID_EX_REG1;
	    break;
	case 0x3:		//JAL
	    Flush_By_Jump();
	    //CURRENT_STATE.PC = CURRENT_STATE.ID_EX_REG1;
	    CURRENT_STATE.EX_MEM_ALU_OUT = CURRENT_STATE.ID_EX_NPC + BYTES_PER_WORD;
	    break;
	default:
	    printf("Unknown instruction type: 0x%x\n", OPCODE(inst));
	    break;
    }
    CURRENT_STATE.EX_MEM_NPC = CURRENT_STATE.ID_EX_NPC;
    CURRENT_STATE.EX_MEM_DEST = CURRENT_STATE.ID_EX_DEST;

}

/***************************************************************/
/*                                                             */
/* Procedure: Memory_Stage                                     */
/*                                                             */
/* Purpose: Memory related execution                           */
/*                                                             */
/***************************************************************/
void Memory_Stage() {
    instruction *inst;

    if (CURRENT_STATE.PIPE_STALL[MEM_STAGE]== TRUE) {
	return;
    }
    if (FORWARDING_BIT == TRUE) {
	CURRENT_STATE.MEM_WB_FORWARD_REG = CURRENT_STATE.MEM_WB_DEST;
	CURRENT_STATE.MEM_WB_FORWARD_VALUE = CURRENT_STATE.MEM_WB_ALU_OUT;

	//for debug
	//        printf("Mem Forward/ REG: %d, VALUE: %x\n", CURRENT_STATE.MEM_WB_FORWARD_REG, CURRENT_STATE.MEM_WB_FORWARD_VALUE);
    }

    CURRENT_STATE.PIPE[MEM_STAGE] = CURRENT_STATE.PIPE[EX_STAGE];
    if (CURRENT_STATE.PIPE[MEM_STAGE] == 0) {
	CURRENT_STATE.MEM_WB_DEST = 0;
	CURRENT_STATE.MEM_WB_ALU_OUT = 0;
	return;
    }

    inst = get_inst_info(CURRENT_STATE.PIPE[MEM_STAGE]);

    CURRENT_STATE.MEM_WB_NPC = CURRENT_STATE.EX_MEM_NPC;
    switch (OPCODE(inst)) {
	case 0x9:		//ADDIU
	case 0xc:		//ANDI
	case 0xf:		//LUI
	case 0xd:		//ORI
	case 0xb:		//SLTIU
	    CURRENT_STATE.MEM_WB_ALU_OUT = CURRENT_STATE.EX_MEM_ALU_OUT;
	    break;
	case 0x23:		//LW
	    if (FORWARDING_BIT == TRUE) {
		CURRENT_STATE.REGS_LOCK[RT(inst)] = FALSE;			//Unlock dest register
	    }
	    CURRENT_STATE.MEM_WB_MEM_OUT = cache_read_32(CURRENT_STATE.EX_MEM_ALU_OUT) & 0xffffffff;

	    if (CURRENT_STATE.STALL_FOR_DCACHE == 1) {
		CURRENT_STATE.MEM_STALL_DEST = RT(inst);
		break;
	    }
	    CURRENT_STATE.MEM_WB_ALU_OUT = CURRENT_STATE.MEM_WB_MEM_OUT;
	    break;
	case 0x2b:		//SW
	    cache_write_32(CURRENT_STATE.EX_MEM_ALU_OUT, CURRENT_STATE.EX_MEM_W_VALUE);
	    if (CURRENT_STATE.STALL_FOR_DCACHE==2) {
		CURRENT_STATE.MEM_STALL_PC = CURRENT_STATE.EX_MEM_ALU_OUT;
	    }
	    break;
	case 0x4:		//BEQ
	case 0x5:		//BNE
	    /* We have already decided to branch in the EX stage */
	    if (CURRENT_STATE.EX_MEM_BR_TAKE) {
		CURRENT_STATE.BRANCH_PC = CURRENT_STATE.EX_MEM_BR_TARGET;
		Flush_By_Branch_MEM();
	    }
	    break;
	case 0x0:
	    switch (FUNC(inst)) {
		case 0x21:	//ADDU
		case 0x24:	//AND
		case 0x27:	//NOR
		case 0x25:	//OR
		case 0x2B:	//SLTU
		case 0x0:	//SLL
		case 0x2:	//SRL
		case 0x23:	//SUBU
		    CURRENT_STATE.MEM_WB_ALU_OUT = CURRENT_STATE.EX_MEM_ALU_OUT;
		    break;
		case 0x8:	//JR
		    break;	//Done by prev stage
		default:
		    printf("Unknown function code type: %x\n", FUNC(inst));
		    break;
	    }
	case 0x2:		//J
	    break;		//Done by prev stage
	case 0x3:		//JAL
	    CURRENT_STATE.MEM_WB_ALU_OUT = CURRENT_STATE.EX_MEM_ALU_OUT;
	    break;
	default:
	    printf("Unknown instruction type: %x\n", OPCODE(inst));
	    break;
    }
    CURRENT_STATE.MEM_WB_DEST = CURRENT_STATE.EX_MEM_DEST;
}

/***************************************************************/
/*                                                             */
/* Procedure: WriteBack_Stage                                  */
/*                                                             */
/* Purpose: Write back related execution                       */
/*                                                             */
/***************************************************************/
void WriteBack_Stage() {
    instruction *inst;

    if (CURRENT_STATE.PIPE_STALL[MEM_STAGE]==FALSE) {
	CURRENT_STATE.PIPE[WB_STAGE] = CURRENT_STATE.PIPE[MEM_STAGE];
    } else {
	CURRENT_STATE.PIPE[WB_STAGE]=0;
    }
    if (CURRENT_STATE.PIPE[WB_STAGE] == 0) {
	return;
    }

    INSTRUCTION_COUNT++;
    inst = get_inst_info(CURRENT_STATE.PIPE[WB_STAGE]);
    switch (OPCODE(inst)) {
	case 0x9:		//ADDIU
	case 0xc:		//ANDI
	case 0xf:		//LUI
	case 0xd:		//ORI
	case 0xb:		//SLTIU
	    CURRENT_STATE.REGS[RT(inst)] = CURRENT_STATE.MEM_WB_ALU_OUT;
	    if (!FORWARDING_BIT) {
		CURRENT_STATE.REGS_LOCK[RT(inst)]--;
	    }
	    break;
	case 0x23:		//LW
	    CURRENT_STATE.REGS[RT(inst)] = CURRENT_STATE.MEM_WB_MEM_OUT;
	    if (!FORWARDING_BIT) {
		CURRENT_STATE.REGS_LOCK[RT(inst)]--;
	    }
	    break;
	case 0x2b:		//SW
	    break;		//Nothing to do
	case 0x4:		//BEQ
	case 0x5:		//BNE
	    /** We have already taken the branch */
	    break;
	case 0x0:
	    switch (FUNC(inst)) {
		case 0x21:	//ADDU
		case 0x24:	//AND
		case 0x27:	//NOR
		case 0x25:	//OR
		case 0x2B:	//SLTU
		case 0x0:	//SLL
		case 0x2:	//SRL
		case 0x23:	//SUBU
		    CURRENT_STATE.REGS[RD(inst)] = CURRENT_STATE.MEM_WB_ALU_OUT;
		    if (!FORWARDING_BIT) {
			CURRENT_STATE.REGS_LOCK[RD(inst)]--;
		    }
		    break;
		case 0x8:	//JR
		    break;	//Done by prev stage
		default:
		    printf("Unknown function code type: %x\n", FUNC(inst));
		    break;
	    }
	    break;
	case 0x2:		//J
	    break;		//Done by prev stage
	case 0x3:		//JAL
	    CURRENT_STATE.REGS[31] = CURRENT_STATE.MEM_WB_ALU_OUT;
	    if (!FORWARDING_BIT) {
		CURRENT_STATE.REGS_LOCK[31]--;
	    }
	    break;
	default:
	    printf("Unknown instruction type: %x\n", OPCODE(inst));
	    break;
    }
}

/***************************************************************/
/*                                                             */
/* Procedure: Choose_PC                                        */
/*                                                             */
/* Purpose: Choose corret PC among 3 candidates                */
/*                                                             */
/***************************************************************/
void Choose_PC() {
    if(CURRENT_STATE.IF_PC)	CURRENT_STATE.PC = CURRENT_STATE.IF_PC;
    if(CURRENT_STATE.JUMP_PC)	CURRENT_STATE.PC = CURRENT_STATE.JUMP_PC;
    if(CURRENT_STATE.BRANCH_PC)	CURRENT_STATE.PC = CURRENT_STATE.BRANCH_PC;

    CURRENT_STATE.IF_PC = 0;
    CURRENT_STATE.JUMP_PC = 0;
    CURRENT_STATE.BRANCH_PC = 0;

    //Check instruction fetch boundary
    if(CURRENT_STATE.PC < MEM_REGIONS[0].start || CURRENT_STATE.PC >= (MEM_REGIONS[0].start + (NUM_INST * 4)))
	FETCH_BIT = FALSE;
}

/***************************************************************/
/*                                                             */
/* Procedure: Flush_By_Jump                                    */
/*                                                             */
/* Purpose: Flush IF, ID stage                                 */
/*                                                             */
/***************************************************************/
void Flush_By_Jump() {
    CURRENT_STATE.PIPE[IF_STAGE] = 0;
    CURRENT_STATE.IF_ID_INST = 0;
    CURRENT_STATE.PIPE[ID_STAGE] = 0;
    CURRENT_STATE.PIPE_STALL[ID_STAGE] = FALSE;
}

/***************************************************************/
/*                                                             */
/* Procedure: Flush_By_Branch_EX                               */
/*                                                             */
/* Purpose: Flush IF, ID stage                                 */
/*          and stall IF, ID stage if prediction bit is unset  */
/*                                                             */
/***************************************************************/
void Flush_By_Branch_EX() {
    CURRENT_STATE.PIPE[IF_STAGE] = 0;
    CURRENT_STATE.IF_ID_INST = 0;
    /** Stall the IF stage so that it does not issue the instruction in this cycle
     * Because the IF stage will be executed after this Flush_By_Branch_EX(), and so it will read a instruction.
     * Stall this to the next cycle */
    CURRENT_STATE.PIPE_STALL[IF_STAGE] = TRUE;

    CURRENT_STATE.PIPE[ID_STAGE] = 0;
}

/***************************************************************/
/*                                                             */
/* Procedure: Flush_By_Branch_MEM                              */
/*                                                             */
/* Purpose: Flush IF, ID, and EX stage                         */
/*      and stall IF, ID, EX stage if prediction bit is unset  */
/*                                                             */
/***************************************************************/
void Flush_By_Branch_MEM() {
    CURRENT_STATE.PIPE[IF_STAGE] = 0;
    CURRENT_STATE.IF_ID_INST = 0;
    CURRENT_STATE.PIPE[ID_STAGE] = 0;

    /** Stall the IF stage so that it does not issue the instruction in this cycle
     * Because the IF stage will be executed after this Flush_By_Branch_EX(), and so it will read a instruction.
     * Stall IF to the next cycle */

    int i;
    for(i = 0; i < MIPS_REGS; i++)
	CURRENT_STATE.REGS_LOCK[i] = FALSE;

    CURRENT_STATE.PIPE_STALL[IF_STAGE] = TRUE;
    CURRENT_STATE.PIPE_STALL[ID_STAGE] = FALSE;

    CURRENT_STATE.PIPE[EX_STAGE] = 0;
}

/***************************************************************/
/*                                                             */
/* Procedure: Flush_By_Branch                                  */
/*                                                             */
/* Purpose: Flush IF, ID, EX, MEM stage                        */
/*                                                             */
/***************************************************************/
void Flush_By_Branch() {
    int i;	//for loop

    for(i = 0; i < MIPS_REGS; i++)
	CURRENT_STATE.REGS_LOCK[i] = FALSE;

    CURRENT_STATE.PIPE[IF_STAGE] = 0;
    CURRENT_STATE.IF_ID_INST = 0;
    CURRENT_STATE.PIPE_STALL[IF_STAGE] = FALSE;
    CURRENT_STATE.PIPE[ID_STAGE] = 0;
    CURRENT_STATE.PIPE_STALL[ID_STAGE] = FALSE;
    CURRENT_STATE.PIPE[EX_STAGE] = 0;
    CURRENT_STATE.PIPE[MEM_STAGE] = 0;
}

void Stall_By_Cache_Miss(int* penalty) {

    if (CURRENT_STATE.STALL_FOR_DCACHE==1 || CURRENT_STATE.STALL_FOR_DCACHE==2) {
	if (!((*penalty)--)) {
	    CURRENT_STATE.PIPE_STALL[MEM_STAGE] = FALSE;
	    // CURRENT_STATE.MEM_WB_MEM_OUT = (cache_miss_mem_read_32() & 0xfffffff);
	    *penalty=30;
	    if (CURRENT_STATE.STALL_FOR_DCACHE==1)  {
		CURRENT_STATE.MEM_WB_MEM_OUT = (cache_miss_mem_read_32() & 0xffffffff);
		CURRENT_STATE.MEM_WB_ALU_OUT = CURRENT_STATE.MEM_WB_MEM_OUT;
	    }
	    if (CURRENT_STATE.STALL_FOR_DCACHE==2){ 
		cache_miss_mem_write_32(CURRENT_STATE.MEM_STALL_PC, CURRENT_STATE.MEM_STALL_W_VALUE) ;
	    }

	    CURRENT_STATE.STALL_FOR_DCACHE = FALSE;
	    return;
	}
	CURRENT_STATE.PIPE_STALL[MEM_STAGE]=TRUE;
    }
}

/***************************************************************/
/*                                                             */
/* Procedure: process_instruction                              */
/*                                                             */
/* Purpose: Process one instrction                             */
/*                                                             */
/***************************************************************/
void process_instruction() {
    WriteBack_Stage();
    Memory_Stage();
    Execute_Stage();
    IDecode_Stage();
    IFetch_Stage();

    timer++;
    Stall_By_Cache_Miss(&miss_penalty);
    Choose_PC();

    if (FETCH_BIT == FALSE &&
	    CURRENT_STATE.PIPE[IF_STAGE] == 0 &&
	    CURRENT_STATE.PIPE[ID_STAGE] == 0 &&
	    CURRENT_STATE.PIPE[EX_STAGE] == 0 &&
	    CURRENT_STATE.PIPE[MEM_STAGE] == 0) {
	RUN_BIT = FALSE;
	cache_flush();
    }
}
