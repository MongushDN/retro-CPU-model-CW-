#ifndef TEST_CPU_H
#define TEST_CPU_H

#include "CPU.h"
#include <string>
#include <vector>

int runAllTests();
bool runSingleTest(const std::string& testName_,
	CPU& cpu_,
	const std::vector<uint8_t>& program_,
	uint16_t startAddress_,
	int steps_,
	uint8_t expectedA_,
	uint16_t expectedPC_,
	uint8_t expectedSP_,
	bool expectedZ_, bool expectedC_, bool expectedS_, bool expectedP_,
	const std::vector<std::pair<uint16_t, uint8_t>>& memoryChecks_ = {});

#endif // !TEST_CPU_H