#include "CPU.h"
#include <iostream>
#include <iomanip>

CPU::CPU() : memory(65536, 0), running(false),
A(0), PC(0), SP(StackStart),
flagZ(false), flagC(false), flagP(false), flagS(false)
{
    for (int i = 0; i < 4;++i) { R[i] = 0; }
}

void CPU::reset() {
    A = 0; PC = 0; SP = StackStart;
    flagC = flagP = flagS = flagZ = false;
    running = true;
    for (int i = 0;i < 4;++i) { R[i] = 0; }
}

void CPU::loadProgram(const std::vector<uint8_t>& program,
    uint16_t startAddress) {
    for (size_t i = 0;i < program.size();++i) {
        memory[startAddress + i] = program[i];
    }
    PC = startAddress;
    running = true;
}
void CPU::loadProgram(const uint8_t* program, size_t size,
    uint16_t startAddress) {
    for (size_t i = 0;i < size;++i) {
        memory[startAddress + i] = program[i];
    }
    PC = startAddress;
    running = true;
}

//стек.
void CPU::push(uint8_t value) { memory[SP--] = value; }
uint8_t CPU::pop() { return memory[++SP]; }
void CPU::push16(uint16_t val) {
    push(static_cast<uint8_t>((val >> 8) & 0xFF)); //старший байт.
    push(static_cast<uint8_t>(val & 0xFF));      //младший байт.
}
uint16_t CPU::pop16() {
    uint8_t low = pop();
    uint8_t high = pop();
    return (static_cast<uint16_t>(high) << 8) | low;
}
//флаги.
uint8_t CPU::calculateParity(uint8_t value) {
    uint8_t count = 0;
    for (int i = 0; i < 8;++i) {
        if (value & (1 << i)) ++count;
    }
    return (count % 2 == 0) ? 1 : 0; //четная четность.
}

void CPU::updateFlags(uint8_t result, bool carryOut) {
    flagC = carryOut;
    flagZ = (result == 0);
    flagP = (calculateParity(result) == 1);
    flagS = (result & 0x80) != 0; //старший бит = знак.
}

//alu.
uint8_t CPU::aluADD(uint8_t a, uint8_t b) {
    uint16_t Res = static_cast<uint16_t>(a) + b;
    uint8_t result = static_cast<uint8_t>(Res & 0xFF);
    updateFlags(result, Res > 0xFF);
    return result;
}
uint8_t CPU::aluSUB(uint8_t a, uint8_t b) {
    uint16_t Res = static_cast<uint16_t>(a) - b;
    uint8_t result = static_cast<uint8_t>(Res & 0xFF);
    updateFlags(result, a < b);
    return result;
}
uint8_t CPU::aluAND(uint8_t a, uint8_t b) {
    uint8_t result = a & b;
    updateFlags(result, false);
    return result;
}
uint8_t CPU::aluOR(uint8_t a, uint8_t b) {
    uint8_t result = a | b;
    updateFlags(result, false);
    return result;
}
uint8_t CPU::aluXOR(uint8_t a, uint8_t b) {
    uint8_t result = a ^ b;
    updateFlags(result, false);
    return result;
}
uint8_t CPU::aluINC(uint8_t a) {
    return aluADD(a, 1);
}
uint8_t CPU::aluDEC(uint8_t b) {
    return aluSUB(b, 1);
}
void CPU::aluCMP(uint8_t a, uint8_t b) {
    uint8_t result = a - b;
    flagZ = (a == b);   //А не изменится.
    flagC = (a < b);
    flagS = (result & 0x80) != 0;
    flagP = (calculateParity(result) == 1);
}



void CPU::step() {
    if (!running) return;
    uint8_t OPcode = memory[PC++]; //код операции.
    uint8_t operand = memory[PC++]; //2 байт инструкции.
    std::cout << "\n[PC=" << std::setw(4) << (PC - 2) << "] "; //вывод выполняемой ф-ции.
    printInstruction(OPcode, operand);
    switch ((OPcode))
    {
    case LDI: //загрузка константы в А.
        A = operand;
        updateFlags(A, false);
        std::cout << " ->A = " << (int)A;
        break;
    case MOV_R_A: //ну кароч в хедере в самом начале это всё есть.
        if (operand < 4) {
            R[operand] = A;
            updateFlags(R[operand], false);
            std::cout << " ->R " << (int)operand << " = " << (int)A;
        }
        else { std::cout << " -> Error: wrong register\n"; }
        break;
    case MOV_A_R:
        if (operand < 4) {
            A = R[operand];
            updateFlags(A, false);
            std::cout << " -> A = R " << (int)operand << " = " << (int)A;
        }
        else { std::cout << " -> Error: wrong register\n"; }
        break;
    case STA:
        memory[operand] = A;
        std::cout << " ->M[" << std::setw(4) << (int)operand << "] = " << (int)A;
        break;
    case LDA:
        A = memory[operand];
        updateFlags(A, false);
        std::cout << " ->A = M[" << std::setw(4) << (int)operand << "] = " << (int)A;
        break;

    case ADD_R:
        if (operand < 4) {
            A = aluADD(A, R[operand]);
            std::cout << " -> A = " << (int)A << " (Z = "
                << flagZ << ", C = " << flagC << ", S = " << flagS
                << ", P = " << flagP << ")";
        }
        else {
            std::cout << " -> Error: wrong register\n";
        }
        break;
    case SUB_R:
        if (operand < 4) {
            A = aluSUB(A, R[operand]);
            std::cout << " -> A = " << (int)A << " (Z = "
                << flagZ << ", C = " << flagC << ", S = " << flagS
                << ", P = " << flagP << ")";
        }
        else {
            std::cout << " -> Error: wrong register\n";
        }
        break;
    case INC_R:
        if (operand < 4) {
            R[operand] = aluINC(R[operand]);
            A = R[operand];
            std::cout << " -> R = " << (int)operand << " (Z = "
                << flagZ << ", C = " << flagC << ", S = " << flagS
                << ", P = " << flagP << ")";
        }
        else {
            std::cout << " -> Error: wrong register\n";
        }
        break;
    case DEC_R:
        if (operand < 4) {
            R[operand] = aluDEC(R[operand]);
            A = R[operand];
            std::cout << " -> R = " << (int)operand << " (Z = "
                << flagZ << ", C = " << flagC << ", S = " << flagS
                << ", P = " << flagP << ")";
        }
        else {
            std::cout << " -> Error: wrong register\n";
        }
        break;
    case AND_R:
        if (operand < 4) {
            A = aluAND(A, R[operand]);
            std::cout << " -> A = " << (int)A << " (Z = "
                << flagZ << ", C = " << flagC << ", S = " << flagS
                << ", P = " << flagP << ")";
        }
        else {
            std::cout << " -> Error: wrong register\n";
        }
        break;
    case OR_R:
        if (operand < 4) {
            A = aluOR(A, R[operand]);
            std::cout << " -> A = " << (int)A << " (Z = "
                << flagZ << ", C = " << flagC << ", S = " << flagS
                << ", P = " << flagP << ")";
        }
        else {
            std::cout << " -> Error: wrong register\n";
        }
        break;
    case XOR_R:
        if (operand < 4) {
            A = aluXOR(A, R[operand]);
            std::cout << " -> A = " << (int)A << " (Z = "
                << flagZ << ", C = " << flagC << ", S = " << flagS
                << ", P = " << flagP << ")";
        }
        else {
            std::cout << " -> Error: wrong register\n";
        }
        break;
    case CMP_R:
        if (operand < 4) {
            aluCMP(A, R[operand]);
            std::cout << " -> A = " << (int)A << " (Z = "
                << flagZ << ", C = " << flagC << ", S = " << flagS
                << ", P = " << flagP << ")";
        }
        else {
            std::cout << " -> Error: wrong register\n";
        }
        break;

    case JMP:
        PC = operand;
        std::cout << " -> SHIFT to adress " << std::setw(4) << (int)operand;
        break;
    case JZ:
        if (flagZ) {
            PC = operand;
            std::cout << " -> SHIFT (Z=1) to adress " << std::setw(4) << (int)operand;
        }
        else { std::cout << " -> NO SHIFT (Z=0)"; }
        break;
    case JNZ:
        if (!flagZ) {
            PC = operand;
            std::cout << " -> SHIFT (Z=0) to adress " << std::setw(4) << (int)operand;
        }
        else { std::cout << " -> NO SHIFT (Z=1)"; }
        break;
    case JC:
        if (flagC) {
            PC = operand;
            std::cout << " -> SHIFT (C=1) to adress " << std::setw(4) << (int)operand;
        }
        else {
            std::cout << " -> NO SHIFT (C=0)";
        }
        break;
    case JNC:
        if (!flagC) {
            PC = operand;
            std::cout << " -> SHIFT (C=0) to adress " << std::setw(4) << (int)operand;
        }
        else {
            std::cout << " -> NO SHIFT (C=1)";
        }
        break;
    case CALL:
        push16(PC);
        PC = operand;
        std::cout << " -> CALL sub program at adress " << std::setw(4) << (int)operand;
        break;
    case RET:
        PC = pop16();
        std::cout << " ->BACK to adress " << std::setw(4) << PC;
        break;
    case HLT:
        running = false;
        std::cout << " -> CPU is stopped (HLT).";
        break;
    default:
        std::cout << " -> UNKNOWN instruction: 0x" << std::hex << (int)OPcode << std::dec;
        running = false;
        break;
    }
    std::cout << std::endl;
}


void CPU::run() {
    while (running) {
        step();
        if (!running) break;
    }
}

void CPU::printState() const { //регистры, флаги, счётчики.
    std::cout << "--- The state of CPU. ---\n";
    std::cout << "| A = " << std::setw(3) << (int)A << "   |";
    std::cout << "    R0 = " << std::setw(3) << (int)R[0];
    std::cout << ", R1 = " << std::setw(3) << (int)R[1];
    std::cout << ", R2 = " << std::setw(3) << (int)R[2];
    std::cout << ", R3 = " << std::setw(3) << (int)R[3] << "     |\n";

    std::cout << "| PC = " << std::setw(4) << PC
        << "   |   SP = " << std::setw(3) << (int)SP
        << " (stack)  |\n";
    std::cout << "| Flags: Z = " << flagZ << ", C = " << flagC
        << ", S = " << flagS << ", P = " << flagP << "  |\n";
    std::cout << "--------------------------\n";
}
void CPU::printInstruction(uint8_t code, uint8_t operand) const {
    switch (code)
    {
    case LDI:       std::cout << "LDI #" << (int)operand; break;
    case MOV_R_A:   std::cout << "MOV R" << (int)operand << ", A"; break;
    case MOV_A_R:   std::cout << "MOV A, R" << (int)operand; break;
    case STA:       std::cout << "STA " << std::setw(4) << (int)operand; break;
    case LDA:       std::cout << "LDA " << std::setw(4) << (int)operand; break;
    case ADD_R:     std::cout << "ADD R" << (int)operand; break;
    case SUB_R:     std::cout << "SUB R" << (int)operand; break;
    case INC_R:     std::cout << "INC R" << (int)operand; break;
    case DEC_R:     std::cout << "DEC R" << (int)operand; break;
    case AND_R:     std::cout << "AND R" << (int)operand; break;
    case OR_R:      std::cout << "OR R" << (int)operand;  break;
    case XOR_R:     std::cout << "XOR R" << (int)operand; break;
    case CMP_R:     std::cout << "CMP R" << (int)operand; break;
    case JMP:       std::cout << "JMP " << std::setw(4) << (int)operand; break;
    case JZ:        std::cout << "JZ " << std::setw(4) << (int)operand;  break;
    case JNZ:       std::cout << "JNZ " << std::setw(4) << (int)operand; break;
    case JC:        std::cout << "JC " << std::setw(4) << (int)operand;  break;
    case JNC:       std::cout << "JNC " << std::setw(4) << (int)operand; break;
    case CALL:      std::cout << "CALL " << std::setw(4) << (int)operand; break;
    case RET:       std::cout << "RET (operand is ignored)"; break;
    case HLT:       std::cout << "HLT"; break;
    default:        std::cout << "UNKNOWN operand"; break;
    }
}