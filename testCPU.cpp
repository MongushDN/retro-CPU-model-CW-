#include "testCPU.h"
#include <iostream>
#include <iomanip>
#include <cassert>

#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"

#define ASSERT_EQ(actual, expected, field) \
if ((actual) != (expected)) { \
std::cerr << RED <<"[FAIL]"<<field<<": expected "<<(int)(expected)\
<<", got "<<(int)(actual)<<RESET<<std::endl;\
return false;\
}

#define ASSERT_FLAG(actual, expected, name)\
if((actual)!=(expected)){\
std::cerr<<RED<<"[FAIL] Flag "<<name<<": expected "<<(expected)\
<<", got "<<(actual)<<RESET<<std::endl;\
return false;\
}

bool runSingleTest(const std::string& testName_,
	CPU& cpu_,
	const std::vector<uint8_t>& program_,
	uint16_t startAddress_,
	int steps_,
	uint8_t expectedA_,
	uint16_t expectedPC_,
	uint8_t expectedSP_,
	bool expectedZ_, bool expectedC_, bool expectedS_, bool expectedP_,
	const std::vector<std::pair<uint16_t, uint8_t>>& memoryChecks_)
{
	std::cout << YELLOW << "[TEST]" << testName_ << RESET << std::endl;

	cpu_.reset();
	cpu_.loadProgram(program_, startAddress_);

	for (int i = 0; i < steps_; i++) {
		if (!cpu_.isRunning())
			break;
		cpu_.step();
	}

	bool ok = true;

	ASSERT_EQ(cpu_.getA(), expectedA_, "A");
	ASSERT_EQ(cpu_.getPC(), expectedPC_, "PC");
	ASSERT_EQ(cpu_.getSP(), expectedSP_, "SP");
	ASSERT_FLAG(cpu_.getFlagZ(), expectedZ_, "Z");
	ASSERT_FLAG(cpu_.getflagC(), expectedC_, "C");
	ASSERT_FLAG(cpu_.getflagS(), expectedS_, "S");
	ASSERT_FLAG(cpu_.getflagP(), expectedP_, "P");

	for (const auto& [addr, value] : memoryChecks_) {
		uint8_t memVal = cpu_.readMemory(addr);

		if (memVal != value) {
			std::cerr << RED << "[FAIL] Memory[0x" << std::hex << addr << std::dec
				<< "]: expected 0x" << std::hex << (int)value
				<< ", got 0x" << (int)memVal << RESET << std::endl;

			ok = false;
		}
	}

	if (ok) {
		std::cout << GREEN << "[PASS] " << testName_ << RESET << std::endl;
	}
	else {
		std::cout << RED << "[FAIL] " << testName_ << RESET << std::endl;

		cpu_.printState();
	}
	std::cout << std::endl;
	return ok;
}

bool test_LDI_basic() {
	CPU cpu;
	std::vector<uint8_t> prog = {
		0x01, 0x42
	};

	return runSingleTest("LDI basic", cpu, prog, 0x200, 1,
		0x42, 0x202, 0xFF,
		false, false, false, true, {});
}

bool test_MOV() {
	CPU cpu;
	std::vector<uint8_t> prog = {
		0x01,0x42, // LDI #0x42
		0x02,0x02, // MOV R2, A (R2=0x42)
		0x01,0x00, // LDI #0x00
		0x03,0x02 // MOV A, R2 (A=0x42)
	};

	return runSingleTest("MOV", cpu, prog, 0x00, 4,
		0x42, 0x08, 0xFF,
		false, false, false, true, {});
}

bool test_memory_STA_LDA() {
	CPU cpu;
	std::vector<uint8_t>prog = {
		0x01,0x42, // LDI #0x42
		0x04,0x34, // STA 0x34
		0x01,0x00, // LDI #0x00
		0x05,0x34 // LDA 0x34
	};
	std::vector<std::pair<uint16_t, uint8_t>>memChecks = { {0x34,0x42} };

	return runSingleTest("STA_LDA", cpu, prog, 0x100, 4,
		0x42, 0x108, 0xFF,
		false, false, false, true, memChecks);
}

bool test_ADD_R_with_carry() {
	CPU cpu;
	std::vector<uint8_t> prog = {
		0x01, 0x01, // LDI #1
		0X02, 0x00, // MOV R0, A (R0=1)
		0x01, 0xFF, // LDI #0xFF
		0x10, 0x00 // ADD R0 -> A=0xFF+1=0x00, C=1, Z=1, S=0, P=1
	};

	return runSingleTest("ADD_R with carry",
		cpu, prog, 0x100, 4,
		0x00, //expected A
		0x108, //expected PC (after 4 instruction)
		0xFF, //expected SP(don't change)
		true, true, false, true, //Z=1, C=1, S=0, P=1
		{});
}

bool test_SUB_R_with_borrow() {
	CPU cpu;
	std::vector<uint8_t>prog = {
		0x01,0x01,
		0x02,0x00,
		0x01,0x00, // LDI #0
		0x11,0x00 // SUB R0 -> A=0-1=0xFF, C=1, Z=0, S=1, P=1
	};

	return runSingleTest("SUB_R with borrow",
		cpu, prog, 0x100, 4,
		0xFF, 0x108, 0xFF,
		false, true, true, true, {});
}

bool test_INC_R() {
	CPU cpu;
	std::vector<uint8_t>prog = {
		0x01,0xFF, // LDI #0xFF
		0x02,0x00, // MOV R0, A
		0x12,0x00 // INC R0
	};

	return runSingleTest("INC_R overflow", cpu, prog, 0x00, 3,
		0x00, 0x06, 0xFF,
		true, true, false, true, {});
}

bool test_DEC_R() {
	CPU cpu;
	std::vector<uint8_t>prog = {
		0x01,0x00, // LDI #0x00
		0x02,0x00, // MOV R0, A
		0x13,0x00 // DEC R0
	};

	return runSingleTest("DEC_R underflow", cpu, prog, 0x00, 3,
		0xFF, 0x06, 0xFF,
		false, true, true, true, {});
}

bool test_AND_R() {
	CPU cpu;
	std::vector<uint8_t> prog = {
		0x01, 0xCC, // LDI #0xCC
		0x02, 0x00, // MOV R0, A
		0x01, 0xAA, // LDI #0xAA
		0x14, 0x00 // AND R0 -> 0xCC & 0xAA = 0x88
	};

	return runSingleTest("AND_R", cpu, prog, 0x100, 4,
		0x88, 0x108, 0xFF,
		false, false, true, true, {});
}

bool test_AND_zero_flag() {
	CPU cpu;
	std::vector<uint8_t> prog = {
		0x01, 0x0F, // LDI #0x0F
		0x02, 0x00, // MOV A R0
		0x01, 0xF0, // LDI #0xF0
		0x14, 0x00 //AND R0 -> 0xF0 & 0xF0 = 0x00
	};
	return runSingleTest("AND zero flag", cpu, prog, 0x100, 4,
		0x00, 0x108, 0xFF,
		true, false, false, true, {});
}

bool test_OR_R() {
	CPU cpu;
	std::vector<uint8_t>prog = {
		0x01, 0x88, // LDI #0x88
		0x02,0x00, // MOV R0, A
		0x01,0x44, // LDI #0xAA
		0x15,0x00 // OR R0 -> 0x88 | 0xCC = 0xCC
	};

	return runSingleTest("OR_R", cpu, prog, 0x100, 4,
		0xCC, 0x108, 0xFF,
		false, false, true, true, {});
}

bool test_XOR_R() {
	CPU cpu;
	std::vector<uint8_t>prog = {
		0x01,0xCC, // LDI #0xCC
		0x02,0x00, // MOV R0, A
		0x01,0xCC, //LDI #0xCC
		0x16,0x00 // XOR R0 -> 0xCC ^ 0xCC = 0x00
	};

	return runSingleTest("XOR_R", cpu, prog, 0x100, 4,
		0x00, 0x108, 0xFF,
		true, false, false, true, {});
}

bool test_CMP_equal() {
	CPU cpu;
	std::vector<uint8_t> prog = {
		0x01, 0x55,
		0x02, 0x00,
		0x17, 0x00
	};

	return runSingleTest("CMP equal", cpu, prog, 0x00, 3,
		0x55, 0x06, 0xFF,
		true, false, false, true, {});
}

bool test_CMP_less() {
	CPU cpu;
	std::vector<uint8_t> prog = {
		0x01, 0x01,
		0x02, 0x00,
		0x01, 0x02,
		0x17, 0x00
	};

	return runSingleTest("CMP greater", cpu, prog, 0x00, 4,
		0x02, 0x08, 0xFF,
		false, false, false, false, {});
}

bool test_JMP() {
	CPU cpu;
	std::vector<uint8_t> prog = {
		0x20, 0x06,
		0x01, 0xAA,
		0xFF, 0x00,
		0x01, 0x55,
		0xFF, 0x00
	};

	return runSingleTest("JMP", cpu, prog, 0x00, 2,
		0x55, 0x08, 0xFF,
		false, false, false, true, {});
}

bool test_JZ_jump() {
	CPU cpu;
	std::vector<uint8_t>prog = {
		0x01,0x00, // LDI #0x00
		0x21,0x08, // JZ 0x10
		0x01,0xFF, // LDI #0xFF
		0xFF,0x00, // HLT
		0x01,0x55,
		0xFF,0x00// LDI #0x55
	};

	return runSingleTest("JZ jump", cpu, prog, 0x00, 3,
		0x55, 0x0A, 0xFF,
		false, false, false, true, {});
}

bool test_JNZ() {
	CPU cpu;
	std::vector<uint8_t> prog = {
		0x01,0x01,
		0x22,0x08,
		0x01,0xAA,
		0xFF,0x00,
		0x01,0x55,
		0xFF,0x00
	};

	return runSingleTest("JNZ", cpu, prog, 0x00, 3,
		0x55, 0x0A, 0xFF,
		false, false, false, true, {});
}

bool test_stack_CALL_RET() {
	CPU cpu;
	std::vector<uint8_t>prog = {
		0x01,0xAA, // LDI #0xAA
		0x25,0x06, // CALL 0x06
		0xFF,0x00, // HLT
		0x01,0xBB, // LDI #0xBB
		0x26, 0x00 // RET
	};

	return runSingleTest("CALL_RET", cpu, prog, 0x00, 5,
		0xBB, 0x06, 0xFF,
		false, false, true, true, {});
}

bool test_Parity_flag() {
	CPU cpu;
	std::vector<uint8_t>prog = {
		0x01,0x03
	};

	return runSingleTest("Parity even", cpu, prog, 0x00, 1,
		0x03, 0x02, 0xFF,
		false, false, false, true, {});
}

bool test_nested_CALL() {
	CPU cpu;
	std::vector<uint8_t>prog = {
		0x01,0xAA,
		0x25,0x08,
		0xFF,0x00,
		0x01,0xBB,
		0x25,0x0C,
		0x26,0x00,
		0x01,0xCC,
		0x26,0x00
	};

	return runSingleTest("nested CALL", cpu, prog, 0x00, 8,
		0xCC, 0x06, 0xFF,
		false, false, true, true, {});
}

bool test_HLT() {
	CPU cpu;
	std::vector<uint8_t>prog = {
		0xFF,0x00
	};
	cpu.loadProgram(prog, 0x00);
	cpu.step();
	bool ok = !cpu.isRunning();
	std::cout << (ok ? "[PASS] HLT\n" : "[FAIL] HLT\n");

	return ok;
}

bool test_reset() {
	CPU cpu;
	cpu.reset();
	bool ok = (cpu.getA() == 0 && cpu.getPC() == 0 && cpu.getSP() == 0xFF
		&& !cpu.getFlagZ() && !cpu.getflagC() && !cpu.getflagS() && !cpu.getflagP());

	std::cout << (ok ? "[PASS] reset\n" : "[FAIL] reset\n");

	return ok;
}

int runAllTests() {
	int failed = 0;

	if (!test_LDI_basic()) failed++;
	if (!test_MOV()) failed++;
	if (!test_memory_STA_LDA()) failed++;

	if (!test_ADD_R_with_carry()) failed++;
	if (!test_SUB_R_with_borrow()) failed++;
	if (!test_INC_R()) failed++;
	if (!test_DEC_R())failed++;
	if (!test_AND_R()) failed++;
	if (!test_AND_zero_flag()) failed++;
	if (!test_OR_R()) failed++;
	if (!test_XOR_R()) failed++;
	if (!test_CMP_equal()) failed++;
	if (!test_CMP_less()) failed++;

	if (!test_JMP()) failed++;
	if (!test_JZ_jump()) failed++;
	if (!test_JNZ())failed++;
	if (!test_stack_CALL_RET()) failed++;

	if (!test_Parity_flag())failed++;
	if (!test_nested_CALL())failed++;
	if (!test_HLT())failed++;
	if (!test_reset())failed++;


	std::cout << "\n=== Testing is completed. Failed tests: " << failed << " ===\n";
	return failed;
}