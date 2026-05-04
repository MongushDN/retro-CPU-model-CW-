#include "CPU.h"
#include <iostream>
#include <iomanip>

CPU::CPU() : A(0), PC(0), SP(StackStart), running(false),
    flagZ(false), flagC(false), flagP(false), flagS(false) {
        
        for(int i=0; i<65536;++i){
            memory[i]=0;
        }
        for(int i=0; i<4;++i){
            R[i]=0;
        }
}

void CPU::reset(){
    A=0; PC=0; SP=StackStart;
    flagC=false; flagP=false; flagS=false; flagZ=false;
    running=true;
    for(int i=0;i<4;++i){ R[i]=0;}
}

void CPU::loadProgram(const std::vector<uint8_t>& program,
                                            uint16_t startAddress){
    for(size_t i=0;i<program.size();++i){
        memory[startAddress+i]=program[i];
    }
    PC=startAddress;
    running =true;
}
void CPU::loadProgram(const uint8_t* program, size_t size,
                                     uint16_t startAddress){
    for(size_t i=0;i<size;++i){
        memory[startAddress+i]=program[i];
    }
    PC=startAddress;
    running=true;
}


void CPU::push(uint8_t value){ memory[SP--]=value; }
uint8_t CPU::pop(){ return memory[++SP]; }
uint8_t CPU::push16(uint16_t val){ 
    push((val>>8)&0xFF); //старший байт.
    push(val&0xFF);      //младший байт.
}
uint16_t CPU::pop16(){
    uint8_t low=pop();
    uint8_t high=pop();
    return (high<<8)|low;
}

uint8_t CPU::calculateParity(uint8_t value){
    uint8_t count=0;
    for(int i=0; i<8;++i){
        if(value&(1<<i)) ++count;
    }
    return (count%2==0)?1:0; //четная четность.
}

void CPU::updateFlags(uint8_t result, bool carryOut){
    flagC= carryOut;
    if (result==0) {flagZ=true;}
    else{ flagZ=false;}
    flagP=(calculateParity(result)==1);
    flagS=((result&0x80)!=0); //старший бит = знак.
}

uint8_t CPU::aluADD(uint8_t a, uint8_t b){
    uint16_t result=a+b;
    updateFlags((uint8_t)(result&0xFF), (result>0xFF));
    return (uint8_t)(result & 0xFF);
}
uint8_t CPU::aluSUB(uint8_t a, uint8_t b){
    uint16_t result = a-b;
    updateFlags((uint8_t)(result&0xFF),(a<b));
    return (uint8_t)(result&0xFF);
}
uint8_t CPU::aluAND(uint8_t a, uint8_t b){
    uint8_t result=a&b;
    updateFlags(result,false);
    return result;
}
uint8_t CPU::aluOR(uint8_t a, uint8_t b){
    uint8_t result=a|b;
    updateFlags(result,false);
    return result;
}
uint8_t CPU::aluXOR(uint8_t a, uint8_t b){
    uint8_t result=a^b;
    updateFlags(result, false);
    return result;
}
uint8_t CPU::aluINC(uint8_t a){
    return aluADD(a,1);
}
uint8_t CPU::aluDEC(uint8_t b){
    return aluSUB(b, 1);
}
void CPU::aluCMP(uint8_t a, uint8_t b){
    uint16_t result=a-b;
    flagZ=(a==b);   //А не изменится.
    flagC=(a<b);
    flagS=((result&0xFF)&0x80!=0);
    flagP=(calculateParity((uint8_t)(result&0xFF))==1);
}

