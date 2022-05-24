#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>

#define NUM_OF_INT_REGISTER 32
// The number of floating-point registers (F0, F2, F4, ... , F30) is actually 16
// But in order to call it without conversions (F[0], ... , F[30]), we set it to 30
#define NUM_OF_FP_REGISTER 31
#define SIZE_OF_MEMORY 8

#define NUM_OF_ADDER_RS 3
#define NUM_OF_MULTIPLIER_RS 2
#define NUM_OF_LOAD_BUFFER 2
#define NUM_OF_STORE_BUFFER 2

#define CYCLE_OF_LOAD 2
#define CYCLE_OF_STORE 1
#define CYCLE_OF_ADD 2
#define CYCLE_OF_SUB 2
#define CYCLE_OF_MUL 10
#define CYCLE_OF_DIV 40


class Tomasulo {
public:
    int R[NUM_OF_INT_REGISTER]{};
    double F[NUM_OF_FP_REGISTER]{};
    double MEM[SIZE_OF_MEMORY]{};

    Tomasulo() {
        // Initialize the value of registers and memory
        for (int &i: R) i = 0;
        for (double &i: F) i = 1.0;
        for (double &i: MEM) i = 1.0;

        // Set reservation station ID
        for (int i = 0; i < NUM_OF_ADDER_RS; i++) {
            RS[i].id.type = ReservationStationID::ADD;
            RS[i].id.index = i;
        }
        for (int i = NUM_OF_ADDER_RS; i < NUM_OF_ADDER_RS + NUM_OF_MULTIPLIER_RS; i++) {
            RS[i].id.type = ReservationStationID::MULT;
            RS[i].id.index = i - NUM_OF_ADDER_RS;
        }
        for (int i = NUM_OF_ADDER_RS + NUM_OF_MULTIPLIER_RS;
             i < NUM_OF_ADDER_RS + NUM_OF_MULTIPLIER_RS + NUM_OF_LOAD_BUFFER; i++) {
            RS[i].id.type = ReservationStationID::LOAD;
            RS[i].id.index = i - (NUM_OF_ADDER_RS + NUM_OF_MULTIPLIER_RS);
        }
        for (int i = NUM_OF_ADDER_RS + NUM_OF_MULTIPLIER_RS + NUM_OF_LOAD_BUFFER;
             i < NUM_OF_ADDER_RS + NUM_OF_MULTIPLIER_RS + NUM_OF_LOAD_BUFFER + NUM_OF_STORE_BUFFER; i++) {
            RS[i].id.type = ReservationStationID::STORE;
            RS[i].id.index = i - (NUM_OF_ADDER_RS + NUM_OF_MULTIPLIER_RS + NUM_OF_LOAD_BUFFER);
        }
    }

    // Run one clock cycle
    void runNextCycle() {
        clockCycle++;
        writeResult();
        execute();
        issue();
    }

    int getCurrentClockCycle() const {
        return clockCycle;
    }

    // Return if NOT all the result from instructions are written (writeResult > 0)
    bool hasRemainingInstruction() {
        return !std::all_of(instructions.begin(), instructions.end(),
                            [](const Instruction &i) { return i.writeResult > 0; });
    }

    void printCurrentInstructionStatus() {
        std::cout << "+-----------------------------------------+" << std::endl;
        std::cout << std::left << "| " << std::setw(22) << "Instructions"
                  << std::setw(6) << "Issue"
                  << std::setw(6) << "ExecC"
                  << std::setw(6) << "Write" << "|" << std::endl;
        std::cout << "+-----------------------------------------+" << std::endl;
        for (auto &i: instructions) {
            std::cout << "| " << std::setw(7) << i.operation;
            if (i.operation == "L.D" || i.operation == "S.D") {
                std::cout << std::setw(5) << "F" + std::to_string(i.rt) + ",";
                std::cout << std::setw(10) << std::to_string(i.imm) + "(R" + std::to_string(i.rs) + ")";
            } else {
                std::cout << std::setw(5) << "F" + std::to_string(i.rd) + ",";
                std::cout << std::setw(5) << "F" + std::to_string(i.rs) + ",";
                std::cout << std::setw(5) << "F" + std::to_string(i.rt);
            }
            std::cout << std::setw(6) << (i.issue > 0 ? std::to_string(i.issue) : "");
            std::cout << std::setw(6) << (i.execComp > 0 ? std::to_string(i.execComp) : "");
            std::cout << std::setw(6) << (i.writeResult > 0 ? std::to_string(i.writeResult) : "") << "|" << std::endl;
        }
        std::cout << "+-----------------------------------------+" << std::endl;
    }

    void printReservationStations() {
        std::cout << "+--------+--------+--------+--------+--------+--------+--------+--------+--------+" << std::endl;
        std::cout << std::left << std::setw(9) << "| Name";
        std::cout << std::setw(9) << "| Busy";
        std::cout << std::setw(9) << "| Op";
        std::cout << std::setw(9) << "| Vj";
        std::cout << std::setw(9) << "| Vk";
        std::cout << std::setw(9) << "| Qj";
        std::cout << std::setw(9) << "| Qk";
        std::cout << std::setw(9) << "| A";
        std::cout << std::setw(9) << "| Time   |" << std::endl;
        std::cout << "+--------+--------+--------+--------+--------+--------+--------+--------+--------+" << std::endl;
        for (auto &i: RS) {
            std::cout << "| " << std::left << std::setw(7) << i.id.toString();
            std::cout << "| " << std::setw(7) << (i.busy ? "Yes" : "No");
            if (i.busy) {
                std::cout << "| " << std::setw(7) << instructions[i.instructionIndex].operation;
                i.Qj.empty() ? std::cout << "| " << std::setw(7) << i.Vj : std::cout << "| " << std::setw(7) << "";
                i.Qk.empty() ? std::cout << "| " << std::setw(7) << i.Vk : std::cout << "| " << std::setw(7) << "";
                std::cout << "| " << std::setw(7) << i.Qj.toString();
                std::cout << "| " << std::setw(7) << i.Qk.toString();
                (i.id.type == ReservationStationID::LOAD || i.id.type == ReservationStationID::STORE) ?
                std::cout << "| " << std::setw(7) << i.addr : std::cout << "| " << std::setw(7) << "";
                std::cout << "| " << std::setw(7) << i.cyclesRemaining << "|" << std::endl;
            } else {
                std::cout << "|        |        |        |        |        |        |        |" << std::endl;
            }
            std::cout << "+--------+--------+--------+--------+--------+--------+--------+--------+--------+"
                      << std::endl;
        }
    }

    void printFloatingPointRegisters() {
        for (int i = 0; i < NUM_OF_FP_REGISTER; i += 2) {
            std::cout << "+--------";
        }
        std::cout << "+" << std::endl;
        for (int i = 0; i < NUM_OF_FP_REGISTER; i += 2) {
            std::cout << "| " << std::setw(7) << "F" + std::to_string(i);
        }
        std::cout << "|" << std::endl;
        for (int i = 0; i < NUM_OF_FP_REGISTER; i += 2) {
            std::cout << "+--------";
        }
        std::cout << "+" << std::endl;
        for (int i = 0; i < NUM_OF_FP_REGISTER; i += 2) {
            std::cout << "| " << std::setw(7) << registerStat[i].Qi.toString();
        }
        std::cout << "|" << std::endl;
        for (int i = 0; i < NUM_OF_FP_REGISTER; i += 2) {
            std::cout << "+--------";
        }
        std::cout << "+" << std::endl;
    }

    void writeCurrentCycleOutputToFile(const std::string &filepath) {
        std::fstream file;

        if (clockCycle == 1) {
            file.open(filepath, std::ios::out | std::ios::trunc);
        } else {
            file.open(filepath, std::ios::out | std::ios::app);
        }

        if (!file.is_open()) {
            std::cerr << "Failed to write output to file!" << std::endl;
            exit(1);
        }

        std::streambuf *coutbuf = std::cout.rdbuf();
        std::cout.rdbuf(file.rdbuf());

        std::cout << "Clock Cycle: " << getCurrentClockCycle() << std::endl;
        printCurrentInstructionStatus();
        printReservationStations();
        printFloatingPointRegisters();
        std::cout << std::endl;

        std::cout.rdbuf(coutbuf); // Reset to standard output
        file.close();
    }

    void loadInstructionsFromFile(const std::string &filepath) {
        std::fstream file;
        file.open(filepath, std::ios::in);
        if (!file.is_open()) {
            std::cerr << "Failed to load the file!" << std::endl;
            exit(1);
        }

        // Read all the instructions from the text file
        while (!file.eof()) {
            std::string input;
            std::getline(file, input);

            // Split command, rs, rt, rd registers (or immediate value)
            std::vector<std::string> tokens;
            size_t beg, pos = 0;
            while ((beg = input.find_first_not_of(" ,()", pos)) != std::string::npos) {
                pos = input.find_first_of(" ,()", beg + 1);
                tokens.push_back(input.substr(beg, pos - beg));
            }

            // Assign results from above to the instruction struct
            Instruction instruction;
            instruction.operation = tokens[0];
            if (instruction.operation == "L.D" || instruction.operation == "S.D") {
                instruction.rs = std::stoi(tokens[3].substr(1));
                instruction.rt = std::stoi(tokens[1].substr(1));
                instruction.imm = std::stoi(tokens[2]);
            } else {
                instruction.rd = std::stoi(tokens[1].substr(1));
                instruction.rs = std::stoi(tokens[2].substr(1));
                instruction.rt = std::stoi(tokens[3].substr(1));
            }
            instructions.push_back(instruction);
        }
        file.close();
    }

private:
    struct Instruction {
        std::string operation;
        int rd = 0;
        int rs = 0;
        int rt = 0;
        int imm = 0;

        int issue = 0;
        int execComp = 0;
        int writeResult = 0;
    };

    struct ReservationStationID {
        enum Type {
            NONE,
            ADD,
            MULT,
            LOAD,
            STORE,
        } type;
        int index;

        explicit ReservationStationID(Type type = NONE, int index = 0) {
            this->type = type;
            this->index = index;
        };

        // Return if the ID is NONE
        bool empty() const {
            return this->type == NONE;
        }

        // Return if two IDs are equal
        bool equals(ReservationStationID id) const {
            return this->type == id.type && this->index == id.index;
        }

        // Set the ID of an RS to NONE
        void clear() {
            this->type = NONE;
        }

        // Return a string representing the ID (type+index)
        std::string toString() const {
            switch (type) {
                case ADD:
                    return "Add" + std::to_string(index);
                case MULT:
                    return "Mult" + std::to_string(index);
                case LOAD:
                    return "Load" + std::to_string(index);
                case STORE:
                    return "Store" + std::to_string(index);
                default:
                    return "";
            }
        }
    };

    struct ReservationStation {
        ReservationStationID id;
        double Vj = 0.0;
        double Vk = 0.0;
        ReservationStationID Qj;
        ReservationStationID Qk;
        int addr = 0;
        bool busy = false;
        int instructionIndex = 0;
        int cyclesRemaining = 0;
        // This value is updated when an RS finished its job (writeResult)
        // And other RSs which need its result must wait util next cycle (lastUsedCycle != clockCycle)
        int lastUsedCycle = 0;
    } RS[NUM_OF_ADDER_RS + NUM_OF_MULTIPLIER_RS + NUM_OF_LOAD_BUFFER + NUM_OF_STORE_BUFFER];

    struct RegisterResultStatus {
        ReservationStationID Qi;
    } registerStat[NUM_OF_FP_REGISTER];

    std::vector<Instruction> instructions;
    int clockCycle = 0; // Counter of clock cycles
    int currentInstructionIndex = 0; // Index to the instruction which is going to be issued

    void issue() {
        if (currentInstructionIndex >= instructions.size()) return;
        Instruction *instruction = &instructions[currentInstructionIndex];
        if (instruction->operation == "ADD.D" || instruction->operation == "SUB.D") {
            ReservationStationID rsId = findEmptyAdderRS();
            // If no empty RS at the moment, wait until there's one
            if (rsId.empty()) return;
            int r = getReservationStationIndexByID(rsId);
            RS[r].instructionIndex = currentInstructionIndex;
            RS[r].cyclesRemaining = instruction->operation == "ADD.D" ? CYCLE_OF_ADD : CYCLE_OF_SUB;
            RS[r].busy = true;

            if (!registerStat[instruction->rs].Qi.empty()) {
                RS[r].Qj = registerStat[instruction->rs].Qi;
            } else {
                RS[r].Vj = F[instruction->rs];
                RS[r].Qj.clear();
            }

            if (!registerStat[instruction->rt].Qi.empty()) {
                RS[r].Qk = registerStat[instruction->rt].Qi;
            } else {
                RS[r].Vk = F[instruction->rt];
                RS[r].Qk.clear();
            }

            registerStat[instruction->rd].Qi = rsId;
        } else if (instruction->operation == "MUL.D" || instruction->operation == "DIV.D") {
            ReservationStationID rsId = findEmptyMultiplierRS();
            // If no empty RS at the moment, wait until there's one
            if (rsId.empty()) return;
            int r = getReservationStationIndexByID(rsId);
            RS[r].instructionIndex = currentInstructionIndex;
            RS[r].cyclesRemaining = instruction->operation == "MUL.D" ? CYCLE_OF_MUL : CYCLE_OF_DIV;
            RS[r].busy = true;

            if (!registerStat[instruction->rs].Qi.empty()) {
                RS[r].Qj = registerStat[instruction->rs].Qi;
            } else {
                RS[r].Vj = F[instruction->rs];
                RS[r].Qj.clear();
            }

            if (!registerStat[instruction->rt].Qi.empty()) {
                RS[r].Qk = registerStat[instruction->rt].Qi;
            } else {
                RS[r].Vk = F[instruction->rt];
                RS[r].Qk.clear();
            }

            registerStat[instruction->rd].Qi = rsId;
        } else if (instruction->operation == "L.D") {
            ReservationStationID rsId = findEmptyLoadBufferRS();
            // If no empty RS at the moment, wait until there's one
            if (rsId.empty()) return;
            int r = getReservationStationIndexByID(rsId);
            RS[r].instructionIndex = currentInstructionIndex;
            RS[r].cyclesRemaining = CYCLE_OF_LOAD;
            RS[r].busy = true;
            RS[r].addr = instruction->imm;

            // We assume the value must be in the register
            RS[r].Vj = R[instruction->rs];
            RS[r].Qj.clear();

            registerStat[instruction->rt].Qi = rsId;
        } else if (instruction->operation == "S.D") {
            ReservationStationID rsId = findEmptyStoreBufferRS();
            // If no empty RS at the moment, wait until there's one
            if (rsId.empty()) return;
            int r = getReservationStationIndexByID(rsId);
            RS[r].instructionIndex = currentInstructionIndex;
            RS[r].cyclesRemaining = CYCLE_OF_STORE;
            RS[r].busy = true;
            RS[r].addr = instruction->imm;

            // We assume the value must in the register
            RS[r].Vj = R[instruction->rs];
            RS[r].Qj.clear();

            if (!registerStat[instruction->rt].Qi.empty()) {
                RS[r].Qk = registerStat[instruction->rt].Qi;
            } else {
                RS[r].Vk = F[instruction->rt];
                RS[r].Qk.clear();
            }
        }

        // Record the clock cycle of ISSUE stage of the instruction
        instruction->issue = clockCycle;
        currentInstructionIndex++;
    }

    void execute() {
        for (auto &r: RS) {
            if (r.id.type == ReservationStationID::LOAD || r.id.type == ReservationStationID::STORE) {
                // For Load and Store operations, execute when RS[r].Oj = 0
                if (r.busy && r.Qj.empty() && r.cyclesRemaining > 0) {
                    if (r.lastUsedCycle == clockCycle) {
                        continue;
                    }
                    if (r.cyclesRemaining == 1) {
                        // Add offset to address
                        r.addr += (int) r.Vj;
                        // Record the clock cycle when EXECUTE stage of the instruction is complete
                        instructions[r.instructionIndex].execComp = clockCycle;
                    }
                    r.cyclesRemaining--;
                }
            } else {
                // For other operations, execute when RS[r].Qj = 0 and RS[r].Qk = 0
                if (r.busy && r.Qj.empty() && r.Qk.empty() && r.cyclesRemaining > 0) {
                    if (r.lastUsedCycle == clockCycle) {
                        continue;
                    }
                    if (r.cyclesRemaining == 1) {
                        // Record the clock cycle when EXECUTE stage of the instruction is complete
                        instructions[r.instructionIndex].execComp = clockCycle;
                    }
                    r.cyclesRemaining--;
                }
            }
        }
    }

    void writeResult() {
        for (auto &r: RS) {
            // When the execution of an instruction is complete
            if (r.busy && r.cyclesRemaining == 0) {
                // Store operations need to wait until RS[r].Qk = 0
                // And r.lastUsedCycle cannot be the current cycle because if an RS which the store buffer is waiting for
                // finished writeResult stage, the store buffer must wait util the next cycle to write that result in memory
                if (r.id.type == ReservationStationID::STORE && (!r.Qk.empty() || r.lastUsedCycle == clockCycle)) {
                    continue;
                }

                // Record the clock cycle of WRITE-RESULT stage of the instruction
                instructions[r.instructionIndex].writeResult = clockCycle;
                r.busy = false;

                // It's updated because if an instruction wants to use this RS must wait one cycle
                // And will prevent problem is ISSUE stage
                r.lastUsedCycle = clockCycle;

                // Calculate the result
                double result;
                if (instructions[r.instructionIndex].operation == "ADD.D") {
                    result = r.Vj + r.Vk;
                } else if (instructions[r.instructionIndex].operation == "SUB.D") {
                    result = r.Vj - r.Vk;
                } else if (instructions[r.instructionIndex].operation == "MUL.D") {
                    result = r.Vj * r.Vk;
                } else if (instructions[r.instructionIndex].operation == "DIV.D") {
                    result = r.Vj / r.Vk; // TODO: Remainder!
                } else if (instructions[r.instructionIndex].operation == "L.D") {
                    result = MEM[r.addr / 8];
                } else if (instructions[r.instructionIndex].operation == "S.D") {
                    MEM[r.addr / 8] = r.Vk;
                    return;
                }

                // Check if a register needs its result
                for (int x = 0; x < NUM_OF_FP_REGISTER; x++) {
                    if (registerStat[x].Qi.equals(r.id)) {
                        F[x] = result;
                        registerStat[x].Qi.clear();
                    }
                }

                // Check if the Qj, Qk of an RS needs its result
                for (auto &x: RS) {
                    if (x.Qj.equals(r.id)) {
                        x.Vj = result;
                        x.Qj.clear();
                        x.lastUsedCycle = clockCycle;
                    }

                    if (x.Qk.equals(r.id)) {
                        x.Vk = result;
                        x.Qk.clear();
                        x.lastUsedCycle = clockCycle;
                    }
                }
            }
        }
    }

    int getReservationStationIndexByID(ReservationStationID id) {
        for (int i = 0; i < NUM_OF_ADDER_RS + NUM_OF_MULTIPLIER_RS + NUM_OF_LOAD_BUFFER + NUM_OF_STORE_BUFFER; i++) {
            if (RS[i].id.equals(id)) return i;
        }
        return -1;
    }

    ReservationStationID findEmptyAdderRS() {
        for (int i = 0; i < NUM_OF_ADDER_RS; i++) {
            if (RS[i].lastUsedCycle == clockCycle) {
                continue;
            }
            if (!RS[i].busy) {
                return RS[i].id;
            }
        }
        return ReservationStationID();
    }

    ReservationStationID findEmptyMultiplierRS() {
        for (int i = NUM_OF_ADDER_RS; i < NUM_OF_ADDER_RS + NUM_OF_MULTIPLIER_RS; i++) {
            if (RS[i].lastUsedCycle == clockCycle) {
                continue;
            }
            if (!RS[i].busy) {
                return RS[i].id;
            }
        }
        return ReservationStationID();
    }

    ReservationStationID findEmptyLoadBufferRS() {
        for (int i = NUM_OF_ADDER_RS + NUM_OF_MULTIPLIER_RS;
             i < NUM_OF_ADDER_RS + NUM_OF_MULTIPLIER_RS + NUM_OF_LOAD_BUFFER; i++) {
            if (RS[i].lastUsedCycle == clockCycle) {
                continue;
            }
            if (!RS[i].busy) {
                return RS[i].id;
            }
        }
        return ReservationStationID();
    }

    ReservationStationID findEmptyStoreBufferRS() {
        for (int i = NUM_OF_ADDER_RS + NUM_OF_MULTIPLIER_RS + NUM_OF_LOAD_BUFFER;
             i < NUM_OF_ADDER_RS + NUM_OF_MULTIPLIER_RS + NUM_OF_LOAD_BUFFER + NUM_OF_STORE_BUFFER; i++) {
            if (RS[i].lastUsedCycle == clockCycle) {
                continue;
            }
            if (!RS[i].busy) {
                return RS[i].id;
            }
        }
        return ReservationStationID();
    }
};

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Wrong arguments!" << std::endl;
        exit(1);
    }

    Tomasulo tomasulo = Tomasulo();
    tomasulo.R[1] = 16; // Set R1 to 16 as requested
    tomasulo.loadInstructionsFromFile(argv[1]);

    // Run until all the results are written
    while (tomasulo.hasRemainingInstruction()) {
        tomasulo.runNextCycle();
        tomasulo.writeCurrentCycleOutputToFile("output.txt");
    }
    tomasulo.printCurrentInstructionStatus();

    return 0;
}
