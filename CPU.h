#ifndef CPU_H
#define CPU_H

#include <vector>
#include <cstdint> //uint всякие.

// коды операций. 
// Работа с данными:
#define LDI     0x01     // LDI imm: загрузить константу в А.
#define MOV_R_A 0x02     // MOV R, A: скопировать А в регистр(R) 0-3
#define MOV_A_R 0x03     // MOV A, R: скоп регистр в А.
#define STA     0x04     //STA addr: сохранение А в памяти.
#define LDA     0x05     //LDA addr: взять А из памяти.

// Арифметические и логические операции. наконец-то.
#define ADD_R 0x10   //ADD R(регистр): A=A+R.
#define SUB_R 0x11  //SUB R: A-=R.
#define INC_R 0x12   //INC R: ++R. 
#define DEC_R 0x13   //DEC R: --R. (инкремент декремент регистра)
#define AND_R 0x14   //AND R: A=A&R
#define OR_R  0x15   //OR R: A=A||R.
#define XOR_R 0x16   //XOR R: A=A^R.
#define CMP_R 0x17  //CMP R: сравнение А и R. (тут установить флаги)

//Управление потоком/ noooou/(
#define JMP     0x20     //JMP addr: безусловный перех.(!)
#define JZ      0x21     //JZ addr: переход при Z=1
#define JNZ     0x22    //JNZ addr: переход при Z=0
#define JC      0X23      //JC addr: переход при С=1
#define JNC     0x24    //JNC addr: переход при С=0.
#define CALL    0x25    //CALL addr: вызов подпрограммы.
#define RET     0x26    //RET: возврат из подпрограммы.
#define HLT     0xFF    //HLT: остановка процессор. :О


class CPU{
private:
    std::vector<uint8_t> memory; //64кб ОЗУ. 
    bool running; //true - CPU работает, иначе false.

    uint8_t A;   // аккумулятор(8 бит).
    uint8_t R[4]; //регистры общего назначения R0, R1, R2, R3 (0-3)
    uint16_t PC;     //Program Counter - счётчик команд.(16 бит, адресуеь до 64Кб).
    uint8_t SP; //Stack Pointer указатель в стеке (в памяти 0x00-0xFF)

    static constexpr uint8_t StackStart = 0xFF; //идём снизу вверх.

    bool flagZ;     //Zero, когда результат равен 0.
    bool flagC;     //Carry, был перенос.
    bool flagS;     //Sign, результат отриц.<0
    bool flagP;     //Parity, чётное кол-во 1 в результате.

    uint8_t aluADD(uint8_t a, uint8_t b); //+
    uint8_t aluSUB(uint8_t a, uint8_t b); //-
    uint8_t aluAND(uint8_t a, uint8_t b); //&&
    uint8_t aluOR(uint8_t a, uint8_t b);  //||
    uint8_t aluXOR(uint8_t a, uint8_t b); //^
    uint8_t aluINC(uint8_t a); //++a;
    uint8_t aluDEC(uint8_t b); //--b;
    void aluCMP(uint8_t a, uint8_t b); //a==b.

    void push(uint8_t val);
    uint8_t pop();
    void push16(uint16_t val); //16 бит значение для PC.
    uint16_t pop16();


    void updateFlags(uint8_t result, bool carryOut=false);
    uint8_t calculateParity(uint8_t val);
public:
    CPU();
    void reset();
    bool isRunning() const{return running;}
//памятью.
    void loadProgram(const std::vector<uint8_t>& program, uint16_t startAddress);
    void loadProgram(const uint8_t* program, size_t size, uint16_t startAddress);
    uint8_t readMemory(uint16_t addr)const {return memory[addr];}
    void writeMemory(uint16_t addr, uint8_t value) {memory[addr]=value; }

    void step(); //для одной инструкции.
    void run(); //делать до HLT.
    void printState() const;
    void printInstruction(uint8_t OPcode, uint8_t operand) const;

    uint8_t getA() const{return A;}
    uint16_t getPC() const{ return PC;}
    uint8_t getSP() const{ return SP;}
    bool getFlagZ() const{ return flagZ;}
    bool getflagC() const{ return flagC;}
    bool getflagP() const{ return flagP;}
    bool getflagS() const{ return flagS;}
    uint8_t getR(int index) const {return (index>=0 && index<4)?R[index]:0;}
};

#endif