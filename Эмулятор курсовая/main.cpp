#include <SFML/Graphics.hpp>
#include "CPU.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <optional>

std::string hex8(uint8_t val) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)val;
    return oss.str();
}
std::string hex16(uint16_t val) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::setw(4) << std::setfill('0') << val;
    return oss.str();
}


// Функция получения дизассемблированного листинга 

std::vector<std::pair<uint16_t, std::string>> getDisassemblyAround(const CPU& cpu, uint16_t pc, int count = 20) {
    std::vector<std::pair<uint16_t, std::string>> result;
    uint16_t addr = (pc > count * 2) ? pc - count * 2 : 0;
    for (int i = 0; i < count && addr < 65535; ++i) {
        uint8_t op = cpu.readMemory(addr);
        uint8_t operand = cpu.readMemory(addr + 1);
        int length = 1;
        switch (op) {
        case HLT: case RET: length = 1; break;
        default: length = 2; break;
        }
        std::ostringstream oss;
        auto old_buf = std::cout.rdbuf(oss.rdbuf());
        cpu.printInstruction(op, operand);
        std::cout.rdbuf(old_buf);
        std::string mnemonic = oss.str();
        result.push_back({ addr, mnemonic });
        addr += length;
        if (result.size() >= count) break;
    }
    return result;
}

int main() {
    sf::RenderWindow window(sf::VideoMode({ 1500, 950 }), "Intel 8080 Emulator");
    window.setFramerateLimit(30);

    sf::Font font;
    if (!font.openFromFile("consola.ttf")) {
        if (!font.openFromFile("C:/Windows/Fonts/consola.ttf")) {
            std::cerr << "Font error\n";
            return -1;
        }
    }

    CPU cpu;
    
    std::vector<uint8_t> demoProgram = {
    LDI, 0x12,                
    CALL, 0x10, 0x10,         
    HLT
    };
    cpu.loadProgram(demoProgram, 0x1000);

    // Записываем подпрограмму в память по адресу 0x1010
    cpu.writeMemory(0x1010, MOV_R_A);   // опкод MOV R0, A
    cpu.writeMemory(0x1011, 0x00);      // номер регистра R0
    cpu.writeMemory(0x1012, RET);       // возврат

  
    bool runningContinuous = false;

    while (window.isOpen()) {
        //Обработка событий 
        std::optional<sf::Event> eventOpt = window.pollEvent();
        while (eventOpt.has_value()) {
            if (auto closed = eventOpt->getIf<sf::Event::Closed>())
                window.close();
            if (auto key = eventOpt->getIf<sf::Event::KeyPressed>()) {
                switch (key->scancode) {
                case sf::Keyboard::Scancode::Space:
                    if (!runningContinuous && cpu.isRunning()) cpu.step();
                    break;
                case sf::Keyboard::Scancode::R:
                    if (cpu.isRunning()) runningContinuous = true;
                    break;
                case sf::Keyboard::Scancode::P:
                    runningContinuous = false;
                    break;
                case sf::Keyboard::Scancode::C:
                    cpu.reset();
                    cpu.loadProgram(demoProgram, 0x1000);
                    runningContinuous = false;
                    break;
                case sf::Keyboard::Scancode::L:
                    cpu.loadProgram(demoProgram, 0x1000);
                    runningContinuous = false;
                    break;
                default: break;
                }
            }
            eventOpt = window.pollEvent();
        }

        // Непрерывное выполнение
        if (runningContinuous && cpu.isRunning()) {
            for (int i = 0; i < 1000; ++i) {
                cpu.step();
                if (!cpu.isRunning()) { runningContinuous = false; break; }
            }
        }

        // Отрисовка
        window.clear(sf::Color(20, 20, 40));

        // Панель регистров и флагов 
        sf::RectangleShape regPanel({ 800, 130 });
        regPanel.setFillColor(sf::Color(0, 0, 0, 180));
        regPanel.setOutlineColor(sf::Color(100, 150, 200));
        regPanel.setOutlineThickness(1);
        regPanel.setPosition({ 10, 10 });
        window.draw(regPanel);

        sf::Text title(font, "REGISTERS and FLAGS", 18);
        title.setFillColor(sf::Color::Cyan);
        title.setPosition({ 20, 15 });
        window.draw(title);

        std::string regStr = "A = " + hex8(cpu.getA()) +
            "    R0 = " + hex8(cpu.getR(0)) +
            "    R1 = " + hex8(cpu.getR(1)) +
            "    R2 = " + hex8(cpu.getR(2)) +
            "    R3 = " + hex8(cpu.getR(3)) +
            "    PC = " + hex16(cpu.getPC()) +
            "    SP = " + hex8(cpu.getSP());
        sf::Text regText(font, regStr, 16);
        regText.setFillColor(sf::Color::White);
        regText.setPosition({ 20, 45 });
        window.draw(regText);

        auto drawFlag = [&](const std::string& name, bool val, float x, float y) {
            sf::Text t(font, name + " = " + (val ? "1" : "0"), 14);
            t.setFillColor(val ? sf::Color::Green : sf::Color::Red);
            t.setPosition({ x,y });
            window.draw(t);
            };
        drawFlag("Z", cpu.getFlagZ(), 20, 80);
        drawFlag("C", cpu.getflagC(), 100, 80);
        drawFlag("S", cpu.getflagS(), 180, 80);
        drawFlag("P", cpu.getflagP(), 260, 80);

        // Ассемблерный листинг 
        auto listing = getDisassemblyAround(cpu, cpu.getPC(), 20);
        sf::RectangleShape listingPanel({ 450, 550 });
        listingPanel.setFillColor(sf::Color(0, 0, 0, 180));
        listingPanel.setOutlineColor(sf::Color(100, 150, 200));
        listingPanel.setOutlineThickness(1);
        listingPanel.setPosition({ 10, 160 });
        window.draw(listingPanel);

        sf::Text listingTitle(font, "DISASSEMBLY", 18);
        listingTitle.setFillColor(sf::Color::Cyan);
        listingTitle.setPosition({ 20, 165 });
        window.draw(listingTitle);

        float listingY = 195;
        for (size_t i = 0; i < listing.size(); ++i) {
            uint16_t addr = listing[i].first;
            std::string line = hex16(addr) + ": " + listing[i].second;
            sf::Text lineText(font, line, 14);
            if (addr == cpu.getPC()) {
                lineText.setFillColor(sf::Color::Yellow);
                lineText.setStyle(sf::Text::Bold);
            }
            else {
                lineText.setFillColor(sf::Color::White);
            }
            lineText.setPosition({ 20, listingY + i * 20 });
            window.draw(lineText);
        }

        // Дамп памяти 
        sf::RectangleShape memPanel({ 600, 550 });
        memPanel.setFillColor(sf::Color(0, 0, 0, 180));
        memPanel.setOutlineColor(sf::Color(100, 150, 200));
        memPanel.setOutlineThickness(1);
        memPanel.setPosition({ 470, 160 });
        window.draw(memPanel);

        sf::Text memTitle(font, "MEMORY", 18);
        memTitle.setFillColor(sf::Color::Cyan);
        memTitle.setPosition({ 480, 165 });
        window.draw(memTitle);

        uint16_t base = cpu.getPC() - 32;
        if (base > 0xFFFF - 64) base = 0;
        float memY = 195;
        for (int row = 0; row < 8; ++row) {
            std::string line = hex16(base + row * 8) + ": ";
            for (int col = 0; col < 8; ++col) {
                uint16_t addr = base + row * 8 + col;
                line += hex8(cpu.readMemory(addr)) + " ";
            }
            sf::Text memLine(font, line, 14);
            if (base + row * 8 <= cpu.getPC() && cpu.getPC() < base + (row + 1) * 8)
                memLine.setFillColor(sf::Color::Yellow);
            else
                memLine.setFillColor(sf::Color::White);
            memLine.setPosition({ 480, memY + row * 20 });
            window.draw(memLine);
        }

        // Панель стека 
        sf::RectangleShape stackPanel({ 320, 300 });
        stackPanel.setFillColor(sf::Color(0, 0, 0, 180));
        stackPanel.setOutlineColor(sf::Color(100, 150, 200));
        stackPanel.setOutlineThickness(1);
        stackPanel.setPosition({ 1080, 160 });
        window.draw(stackPanel);

        sf::Text stackTitle(font, "STACK", 16);
        stackTitle.setFillColor(sf::Color::Cyan);
        stackTitle.setPosition({ 1090, 165 });
        window.draw(stackTitle);

       
        uint16_t stackStart = cpu.getSP() + 1;
        std::vector<uint8_t> stackBytes;
        for (int i = 0; i < 16; ++i) {
            uint16_t addr = stackStart + i;
            if (addr < 65536)
                stackBytes.push_back(cpu.readMemory(addr));
            else
                stackBytes.push_back(0);
        }
        float stackY = 195;
        
        for (int row = 0; row < 4; ++row) {
            std::string line;
            for (int col = 0; col < 4; ++col) {
                int idx = row * 4 + col;
                if (idx < (int)stackBytes.size())
                    line += hex8(stackBytes[idx]) + " ";
                else
                    line += "      ";
            }
            sf::Text byteLine(font, line, 14);
            byteLine.setFillColor(sf::Color::White);
            byteLine.setPosition({ 1090, stackY + row * 20 });
            window.draw(byteLine);
        }
        
        sf::Text addrTitle(font, "As return addresses:", 14);
        addrTitle.setFillColor(sf::Color(200, 200, 100));
        addrTitle.setPosition({ 1090, stackY + 90 });
        window.draw(addrTitle);
        for (int i = 0; i < 8; ++i) {
            int low = stackBytes[i * 2];
            int high = stackBytes[i * 2 + 1];
            uint16_t word = (high << 8) | low;
            std::string wstr = hex16(word);
            sf::Text addrText(font, wstr, 14);
            addrText.setFillColor(sf::Color(100, 200, 100)); 
            addrText.setPosition(sf::Vector2f(1090 + (i % 4) * 70, stackY + 110 + (i / 4) * 20));
            window.draw(addrText);
        }

        // Статус выполнения
        sf::Text status(font, "", 20);
        status.setStyle(sf::Text::Bold);
        if (!cpu.isRunning()) {
            status.setString("HALTED");
            status.setFillColor(sf::Color::Red);
        }
        else if (runningContinuous) {
            status.setString("RUNNING");
            status.setFillColor(sf::Color::Green);
        }
        else {
            status.setString("PAUSED");
            status.setFillColor(sf::Color::Yellow);
        }
        status.setPosition({ 20, 730 });
        window.draw(status);

        // Подсказки 
        sf::RectangleShape ctrlPanel({ 1050, 100 });
        ctrlPanel.setFillColor(sf::Color(0, 0, 0, 180));
        ctrlPanel.setOutlineColor(sf::Color(100, 150, 200));
        ctrlPanel.setOutlineThickness(1);
        ctrlPanel.setPosition({ 10, 770 });
        window.draw(ctrlPanel);

        sf::Text ctrlTitle(font, "CONTROLS", 16);
        ctrlTitle.setFillColor(sf::Color::Cyan);
        ctrlTitle.setPosition({ 20, 775 });
        window.draw(ctrlTitle);

        sf::Text help(font, "[Space] Step   [R] Run   [P] Pause   [C] Reset   [L] Reload", 14);
        help.setFillColor(sf::Color(210, 210, 210));
        help.setPosition({ 20, 805 });
        window.draw(help);

        window.display();
    }
    return 0;
}