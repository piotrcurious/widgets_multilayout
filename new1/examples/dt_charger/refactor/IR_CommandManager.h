#ifndef IR_COMMAND_MANAGER_H
#define IR_COMMAND_MANAGER_H

#include <Arduino.h>
#include <functional>
#include <vector>

namespace IRCodes {
    constexpr uint32_t UP = 0xFF629D;
    constexpr uint32_t DOWN = 0xFFA857;
    constexpr uint32_t LEFT = 0xFF22DD;
    constexpr uint32_t RIGHT = 0xFFC23D;
    constexpr uint32_t OK = 0xFF02FD;
    constexpr uint32_t RED = 0xF720DF;
    constexpr uint32_t GREEN = 0xA720DF;
    constexpr uint32_t BLUE = 0x6720DF;
}

using CommandHandler = std::function<void()>;

struct CommandMapping {
    uint32_t code;
    CommandHandler handler;
    const char* description;
};

class IRCommandManager {
private:
    std::vector<CommandMapping> commandMappings;
    uint32_t lastCode = 0;
    unsigned long lastCommandTime = 0;
    static constexpr unsigned long REPEAT_DELAY = 250;

public:
    void addCommand(uint32_t code, CommandHandler handler, const char* description) {
        commandMappings.push_back({code, handler, description});
    }

    void clearCommands() {
        commandMappings.clear();
    }

    bool handleCommand(uint32_t code, bool repeat = false) {
        unsigned long currentTime = millis();

        if (code == 0xFFFFFFFF) {
            if (lastCode != 0 && currentTime - lastCommandTime >= REPEAT_DELAY) {
                code = lastCode;
                repeat = true;
            } else {
                return false;
            }
        }

        for (const auto& mapping : commandMappings) {
            if (mapping.code == code) {
                if (mapping.handler) mapping.handler();
                lastCode = code;
                lastCommandTime = currentTime;
                return true;
            }
        }

        return false;
    }

    void printCommands() {
        Serial.println("Available IR Commands:");
        for (const auto& mapping : commandMappings) {
            Serial.printf("Code: 0x%06X - %s\n", mapping.code, mapping.description);
        }
    }
};

#endif // IR_COMMAND_MANAGER_H
