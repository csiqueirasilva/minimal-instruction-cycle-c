#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define BASE_CHAR_CODE 48

#define WRITE_REG_A 1
#define WRITE_REG_B 2
#define READ_REG_A 3
#define READ_REG_B 4
#define READ_FLAGS 5
#define WRITE_OP 6
#define PRINT_OP 7
#define RUN_ALU 8
#define OP_QUIT 9

#define REG_SIZE 6 // bits

#define MEM_SIZE 1<<REG_SIZE // number of memory cells, will use 2^REG_SIZE

#define MAX_NAME_SIZE 8

#define OP_ALU_ADD 1
#define OP_ALU_ADD_NAME "ADD (A = A + B)"
#define OP_ALU_SUB 2
#define OP_ALU_SUB_NAME "SUB (A = A - B)"
#define OP_ALU_MULT 3
#define OP_ALU_MULT_NAME "MULT (A = A * B)"
#define OP_ALU_DIV 4
#define OP_ALU_DIV_NAME "DIV (A = A / B)"

#define OP_ALU_AND 5
#define OP_ALU_AND_NAME "AND (A = A & B)"
#define OP_ALU_OR 6
#define OP_ALU_OR_NAME "OR(A = A | B)"
#define OP_ALU_NOT 7
#define OP_ALU_NOT_NAME "NOT, COMPL1 (A = !A)"
#define OP_ALU_XOR 8
#define OP_ALU_XOR_NAME "XOR(A = A XOR B)"

#define OP_MEM_SW 9
#define OP_MEM_SW_NAME "SW (*ADDR = REG)"
#define OP_MEM_LW 10
#define OP_MEM_LW_NAME "LW (REG = *ADDR)"
#define OP_MEM_JMP 11
#define OP_MEM_JMP_NAME "JMP (PC = OP)"
#define OP_MEM_JZ 12
#define OP_MEM_JZ_NAME "JZ (PC = OP IF flagZero)"
#define OP_MEM_JE 13
#define OP_MEM_JE_NAME "JE (PC = OP1 IF A = *OP2)"
#define OP_MEM_JNE 14
#define OP_MEM_JNE_NAME "JNE (PC = OP1 IF A != *OP2)"
#define OP_MEM_HALT 15
#define OP_MEM_HALT_NAME "HALT"
#define OP_MEM_NOP 0
#define OP_MEM_NOP_NAME "NO OPERATION"

struct reg {
#define REGBITSIZE REG_SIZE + 1
	char bits[REGBITSIZE];
	int val;

#define REGNAMESIZE MAX_NAME_SIZE + 1
	char name[REGNAMESIZE];
};

static struct reg * A = NULL,
*B = NULL,
*C = NULL,
*D = NULL,
*E = NULL,
*F = NULL,
*IR = NULL, // RI, instruction register
*PC = NULL; // PC, program counter 

static int flagCarry = 0;
static int flagZero = 0;
static int flagOverflow = 0;
static int flagDivisionZero = 0;

static struct reg * PROG_memory[MEM_SIZE];

/* alguns protótipos, para funções definidas antes dessas */

void OP_add(struct reg * r1, struct reg * r2);
void initFlags(void);
void runALU(void);
void updateValFromBits(struct reg * r);
void updateBitsFromVal(struct reg * r);
int fromBitsToVal(char *);
int addBit(struct reg * r1, struct reg * r2, int i);
struct reg * createRegister(char * name);
struct reg * cloneRegister(struct reg *);

/* fim de protótipos */

/* funcao de UI */

void clearScreen() {
#ifdef __linux__
	system("clear");
#elif _WIN32
	system("cls");
#endif
}

void pauseInput() {
#ifdef __linux__
	int enter = 0;
	printf("\n\nPressione ENTER para continuar...\n");
	while (enter != '\r' && enter != '\n') {
		enter = getchar();
	}
#elif _WIN32
	system("pause");
#endif
}

/* fim de funcao de UI */

/* ops */

void OP_and(struct reg * r1, struct reg * r2) {
	int i;
	for (i = 0; i < REG_SIZE; i++) {
		r1->bits[i] = r1->bits[i] == r2->bits[i] && r1->bits[i] == '1' ? '1' : '0';
	}
}

void OP_or(struct reg * r1, struct reg * r2) {
	int i;
	for (i = 0; i < REG_SIZE; i++) {
		r1->bits[i] = r2->bits[i] == '1' || r1->bits[i] == '1' ? '1' : '0';
	}
}

void OP_xor(struct reg * r1, struct reg * r2) {
	int i;
	for (i = 0; i < REG_SIZE; i++) {
		r1->bits[i] = r1->bits[i] != r2->bits[i] ? '1' : '0';
	}
}

void OP_compl1(struct reg * r) {
	int i;
	for (i = 0; i < REG_SIZE; i++) {
		r->bits[i] = r->bits[i] == '1' ? '0' : '1';
	}
}

void OP_compl2(struct reg * r) {
	struct reg * compl2 = createRegister("compl2");
	compl2->val = 1;
	updateBitsFromVal(compl2);
	OP_compl1(r);
	OP_add(r, compl2);
	free(compl2);
}

void OP_add(struct reg * r1, struct reg * r2) {
	int i;
	flagCarry = 0;
	flagZero = 1;
	for (i = REG_SIZE - 1; i >= 0; i--) {
		flagZero = !addBit(r1, r2, i) && flagZero;
	}
}

void OP_sub(struct reg * r1, struct reg * r2) {
	struct reg * aux = cloneRegister(r2);
	OP_compl2(aux);
	OP_add(r1, aux);
	free(aux);
}

void OP_div(struct reg *r1, struct reg * r2) {
	int base = abs(r1->val);
	int times = abs(r2->val);
	int sign = (r1->val * r2->val) >= 0 ? 1 : -1;
	struct reg * aux;

	if (r2->val != 0) {
		aux = createRegister("AUX");
		r1->val = 0;
		aux->val = 1;
		updateBitsFromVal(r1);
		updateBitsFromVal(aux);
		flagZero = 1;

		while (base >= times) {
			OP_add(r1, aux);
			base -= times;
		}

		if (sign < 0) {
			OP_compl2(r1);
		}

		free(aux);
	}
}

void OP_mult(struct reg * r1, struct reg * r2) {
	int base = abs(r1->val);
	int times = abs(r2->val);
	int i;
	int sign = (r1->val * r2->val) >= 0 ? 1 : -1;
	struct reg * aux = createRegister("AUX");

	r1->val = 0;
	aux->val = base;
	updateBitsFromVal(r1);
	updateBitsFromVal(aux);
	flagZero = 1;

	for (i = 1; i <= times; i++) {
		OP_add(r1, aux);
	}

	if (sign < 0) {
		OP_compl2(r1);
	}

	free(aux);
}

/* fim de ops */

int addBit(struct reg * r1, struct reg * r2, int i) {
	int sum = (r1->bits[i] - BASE_CHAR_CODE) + (r2->bits[i] - BASE_CHAR_CODE) + flagCarry;
	if (sum > 1) {
		flagCarry = 1;
	}
	else {
		flagCarry = 0;
	}
	r1->bits[i] = (sum % 2) + BASE_CHAR_CODE;
	return r1->bits[i] - BASE_CHAR_CODE;
}

void checkOverflow(int operation) {
	int max = (int)pow(2, REG_SIZE - 1) - 1;
	int min = -(max + 1);

	switch (operation) {
	case OP_ALU_ADD:
		if ((A->val + B->val > max) || (A->val + B->val < min)) {
			flagOverflow = 1;
		}
		break;
	case OP_ALU_MULT:
		if ((A->val * B->val > max) || (A->val * B->val < min)) {
			flagOverflow = 1;
		}
		break;
	case OP_ALU_SUB:
		if ((A->val - B->val > max) || (A->val - B->val < min)) {
			flagOverflow = 1;
		}
		break;
	case OP_ALU_DIV:
		if (B->val == 0) {
			flagDivisionZero = 1;
		}
		else if ((A->val / B->val > max) || (A->val / B->val < min)) {
			flagOverflow = 1;
		}
		break;
	}

}

void runALU(void) {
	int success = 1;

	initFlags();

	checkOverflow(IR->val);

	switch (IR->val) {
	case OP_ALU_ADD:
		OP_add(A, B);
		break;
	case OP_ALU_SUB:
		OP_sub(A, B);
		break;
	case OP_ALU_MULT:
		OP_mult(A, B);
		break;
	case OP_ALU_DIV:
		OP_div(A, B);
		break;
	case OP_ALU_AND:
		OP_and(A, B);
		break;
	case OP_ALU_OR:
		OP_or(A, B);
		break;
	case OP_ALU_NOT:
		OP_compl1(A);
		break;
	case OP_ALU_XOR:
		OP_xor(A, B);
	}

	updateValFromBits(A);
}

#define REG_A 0
#define REG_B 1
#define REG_C 2
#define REG_D 3
#define REG_E 4
#define REG_F 5

struct reg * getRegByCode(int regCode) {

	struct reg * ret = NULL;

	switch (regCode) {
	case REG_A:
		ret = A;
		break;
	case REG_B:
		ret = B;
		break;
	case REG_C:
		ret = C;
		break;
	case REG_D:
		ret = D;
		break;
	case REG_E:
		ret = E;
		break;
	case REG_F:
		ret = F;
	}

	return ret;
}

void runControlUnit(void) {
	int instVal = fromBitsToVal(IR->bits);

	switch (instVal) {
	case OP_MEM_LW:
	{
		int reg = PROG_memory[PC->val++]->val;
		int addr = fromBitsToVal(PROG_memory[PC->val++]->bits);
		struct reg * r = getRegByCode(reg);
		strcpy(r->bits, PROG_memory[addr]->bits);
		updateValFromBits(r);
		updateBitsFromVal(PC);
	}
	break;
	case OP_MEM_SW:
	{
		int addr = fromBitsToVal(PROG_memory[PC->val++]->bits);
		int reg = PROG_memory[PC->val++]->val;
		struct reg * r = getRegByCode(reg);
		strcpy(PROG_memory[addr]->bits, r->bits);
		updateValFromBits(PROG_memory[addr]);
		updateBitsFromVal(PC);
	}
	break;
	case OP_MEM_JMP:
	{
		int addr = fromBitsToVal(PROG_memory[PC->val++]->bits);
		PC->val = addr;
		updateBitsFromVal(PC);
	}
	break;
	case OP_MEM_JZ:
	{
		int addr = fromBitsToVal(PROG_memory[PC->val++]->bits);
		if (flagZero) {
			PC->val = addr;
			updateBitsFromVal(PC);
		}
	}
	break;
	case OP_MEM_JE:
	{
		int addr = fromBitsToVal(PROG_memory[PC->val++]->bits);
		int cmp = fromBitsToVal(PROG_memory[PC->val++]->bits);
		if (strcmp(PROG_memory[cmp]->bits, A->bits) == 0) {
			PC->val = addr;
			updateBitsFromVal(PC);
		}
	}
	break;
	case OP_MEM_JNE:
	{
		int addr = fromBitsToVal(PROG_memory[PC->val++]->bits);
		int cmp = fromBitsToVal(PROG_memory[PC->val++]->bits);
		if (strcmp(PROG_memory[cmp]->bits, A->bits) != 0) {
			PC->val = addr;
			updateBitsFromVal(PC);
		}
	}
	break;
	}
}

void initFlags(void) {
	flagOverflow = 0;
	flagZero = 0;
	flagCarry = 0;
	flagDivisionZero = 0;
}

void updateValFromBits(struct reg * r) {
	int sign = r->bits[0] == '1' ? -1 : 1;
	r->val = 0;

	if (sign < 0) {
		OP_compl2(r);
	}

	r->val = fromBitsToVal(r->bits);

	if (sign < 0) {
		r->val *= sign;
		OP_compl2(r);
	}
}

void updateBitsFromVal(struct reg * r) {
	int i;
	int buffer = abs(r->val);
	int sign = r->val < 0 ? -1 : 1;

	for (i = 0; i < REG_SIZE; i++) {
		int bit = (int)pow(2, REG_SIZE - 1 - i);
		if (buffer >= bit) {
			buffer -= bit;
			r->bits[i] = '1';
		}
		else {
			r->bits[i] = '0';
		}
	}

	if (sign < 0) {
		OP_compl2(r);
	}
}

struct reg * cloneRegister(struct reg * r) {
	struct reg * c = createRegister("TEMP");
	strcpy(c->bits, r->bits);
	c->val = r->val;
	return c;
}

struct reg * createRegister(char * name) {
	struct reg * ret = (struct reg *) malloc(sizeof(struct reg));
	if (ret == NULL) {
		printf("falha ao alocar memoria. saindo da aplicacao\n");
		pauseInput();
		exit(-1);
	}
	else {
		int i;
		for (i = 0; i < REG_SIZE; i++) {
			ret->bits[i] = '0';
		}
		ret->bits[REG_SIZE] = 0;
		strcpy(ret->name, name);
		ret->val = 0;
	}

	return ret;
}

void freeAllData(void) {
	int i;

	free(A);
	free(B);
	free(C);
	free(D);
	free(E);
	free(F);

	free(IR);
	free(PC);

	for (i = 0; i < MEM_SIZE; i++) {
		free(PROG_memory[i]);
	}
}

void readReg(struct reg *r) {
	int maxInput = ((int)pow(2, REG_SIZE) - 1) / 2;
	int minInput = -(maxInput + 1);
	char buffer[REGBITSIZE];
	int i = 0;
	int strSize = 0;
	int zeroSize = 0;
	printf("\nDigite um valor numerico binario, em representacao de complemento a 2, para o registrador %s (MAX : %d, MIN: %d, Tamanho: %d bits) => ", r->name, maxInput, minInput, REG_SIZE);
	scanf("%s", buffer);

	strSize = strlen(buffer);
	zeroSize = REG_SIZE - strSize;

	for (i = 0; i < zeroSize; i++) {
		r->bits[i] = '0';
	}

	strcpy(&r->bits[i], buffer);

	updateValFromBits(r);
}

void printReg(struct reg *r) {
	printf("Valor do registrador %s: %d (0b%s)\n\n", r->name, r->val, r->bits);
	pauseInput();
}

void printFlags(void) {
	printf("Carry: %d\nOverflow: %d\nZero: %d\nDivisao por Zero: %d\n\n", flagCarry, flagOverflow, flagZero, flagDivisionZero);
	pauseInput();
}

void printOps(void) {
	printf("\n\
ARITMETICA\n\
0001. ADD (A = A + B)\n\
0010. SUB (A = A - B)\n\
0011. MULT (A = A * B)\n\
0100. DIV (A = A / B)\n\
\n\
LOGICA\n\
0101. AND (A = A & B)\n\
0110. OR (A = A | B)\n\
0111. NOT, COMPL1 (A = !A)\n\
1000. XOR (A = A XOR B)\n\
\n\
CONTROLE\n\
1001. SW (*ADDR = REG) \n\
1010. LW (REG = *ADDR) \n\
1011. JMP (PC = OP) \n\
1100. JZ (PC = OP IF flagZero) \n\
1101. JE (PC = OP1 IF A = OP2)\n\
1110. JNE (PC = OP1 IF A != OP2)\n\
1111. HALT\n\
0000. NO OPERATION\n\
");

}

char * getInstName(int val) {
	char * ret = OP_MEM_NOP_NAME;

	switch (val) {
	case OP_ALU_ADD:
		ret = OP_ALU_ADD_NAME;
		break;
	case OP_ALU_SUB:
		ret = OP_ALU_SUB_NAME;
		break;
	case OP_ALU_MULT:
		ret = OP_ALU_MULT_NAME;
		break;
	case OP_ALU_DIV:
		ret = OP_ALU_DIV_NAME;
		break;
	case OP_ALU_AND:
		ret = OP_ALU_AND_NAME;
		break;
	case OP_ALU_OR:
		ret = OP_ALU_OR_NAME;
		break;
	case OP_ALU_NOT:
		ret = OP_ALU_NOT_NAME;
		break;
	case OP_ALU_XOR:
		ret = OP_ALU_XOR_NAME;
		break;
	case OP_MEM_SW:
		ret = OP_MEM_SW_NAME;
		break;
	case OP_MEM_LW:
		ret = OP_MEM_LW_NAME;
		break;
	case OP_MEM_HALT:
		ret = OP_MEM_HALT_NAME;
		break;
	case OP_MEM_JE:
		ret = OP_MEM_JE_NAME;
		break;
	case OP_MEM_JZ:
		ret = OP_MEM_JZ_NAME;
		break;
	case OP_MEM_JNE:
		ret = OP_MEM_JNE_NAME;
		break;
	case OP_MEM_JMP:
		ret = OP_MEM_JMP_NAME;
	}

	return ret;
}

void operacao(int op) {
	switch (op) {
	case WRITE_REG_A:
		readReg(A);
		break;
	case WRITE_REG_B:
		readReg(B);
		break;
	case READ_REG_A:
		printReg(A);
		break;
	case READ_REG_B:
		printReg(B);
		break;
	case READ_FLAGS:
		printFlags();
		break;
	case WRITE_OP:
		printOps();
		readReg(IR);
		break;
	case PRINT_OP:
		printOps();
		printReg(IR);
		break;
	case RUN_ALU:
		runALU();
		break;
	case OP_QUIT:
		break;
	default:
		printf("\nOPCAO INVALIDA\n");
		pauseInput();
	}
}

int menu(void) {

	int op = 0;

	clearScreen();
	printf(
		"Menu Principal da ULA\n\
\n\
\t1. Definir registrador A\n\
\t2. Definir registrador B\n\
\t3. Ler registrador A (Acc)\n\
\t4. Ler registrador B\n\
\t5. Ler registrador de flags\n\
\t6. Definir operacao\n\
\t7. Ler operacao (OPCODE)\n\
\t8. Executar ULA\n\
\t9. Sair\n\
\n\
Escolha a opcao => ");

	scanf("%d", &op);

	return op;
}

int fromBitsToVal(char * bits) {
	int ret = 0, i;
	for (i = REG_SIZE - 1; i >= 0; i--) {
		ret += (int)pow(2, REG_SIZE - 1 - i) * (bits[i] - BASE_CHAR_CODE);
	}
	return ret;
}

int menuInstructionCycle() {
	int i = 0;
	struct reg * temp = createRegister("temp");
	struct reg * inst = NULL;

	clearScreen();

	printf("CICLO DE INSTRUCAO\n");
	printf("CELULA: %d bits ENDERECOS: %d\n", REG_SIZE, MEM_SIZE);
	printf("A: 0b%s (%d) B: 0b%s (%d)\n", A->bits, A->val, B->bits, B->val);
	printf("C: 0b%s (%d) D: 0b%s (%d)\n", C->bits, C->val, D->bits, D->val);
	printf("E: 0b%s (%d) F: 0b%s (%d)\n", E->bits, E->val, F->bits, F->val);
	printf("FLAGS\n");
	printf("ZERO: %d CARRY: %d OVERFLOW: %d DIVISIONBYZERO: %d\n", flagZero, flagCarry, flagOverflow, flagDivisionZero);

	inst = PROG_memory[PC->val++];
	updateBitsFromVal(PC);

	IR->val = inst->val;
	updateBitsFromVal(IR);

	printf("PC: 0b%s (%d) RI: 0b%s (%d)\n\n", PC->bits, PC->val, IR->bits, IR->val);

	printf("Instrucao 0b%s (%s) sendo executava\n\n", inst->bits, getInstName(inst->val));

	printf("Dump de Memoria (ENDERECO seguido de VALOR):\n\n");

	for (; i < MEM_SIZE; i++) {
		temp->val = i;
		updateBitsFromVal(temp);
		printf("E%s\tV%s\t", temp->bits, PROG_memory[i]->bits);
		if ((i + 1) % 4 == 0) {
			printf("\n");
		}
	}

	if (IR->val != 0 && fromBitsToVal(IR->bits) <= 8) {
		runALU();
	}
	else {
		runControlUnit();
	}

	free(temp);

	pauseInput();

	return inst->val != OP_MEM_HALT && PC->val < MEM_SIZE;
}

void initProgMemory() {
	int i, nInst = 0;
	char buf[255];
	FILE * fp = fopen("inst.txt", "rt");

	for (i = 0; i < MEM_SIZE; i++) {
		sprintf(buf, "c%d", i);
		PROG_memory[i] = createRegister(buf);
	}

	while (!feof(fp)) {
		char inst[REG_SIZE + 1], addr[REG_SIZE + 1];
		int intAddr;
		fscanf(fp, "%s %s%*[^\n]\n", addr, inst);
		if (inst[REG_SIZE] == 0 && addr[REG_SIZE] == 0) {
			intAddr = fromBitsToVal(addr);
			strcpy(PROG_memory[intAddr]->bits, inst);
			PROG_memory[intAddr]->val = fromBitsToVal(PROG_memory[intAddr]->bits);
			nInst++;
		}
	}

	printf("Total de %d celulas validas lidas\n", nInst);

	pauseInput();
	clearScreen();

	fclose(fp);

}

void main(void) {
	int op = 1;

	A = createRegister("A");
	B = createRegister("B");
	C = createRegister("C");
	D = createRegister("D");
	E = createRegister("E");
	F = createRegister("F");

	PC = createRegister("PC");
	IR = createRegister("IR");

	initProgMemory();

	while (op) {
		op = menuInstructionCycle();
	}

	freeAllData();
}

/*void main(void) {

	int op = -1;

	A = createRegister("A");
	B = createRegister("B");
	IR = createRegister("OPCODE");

	while (op != OP_QUIT) {
		op = menu();
		operacao(op);
	}

	freeAllData();
}*/