#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstdio>
#include <string>
#include <sstream>

using namespace std;

// Instruction constants
#define BR   0x0
#define ADD  0x1
#define LD   0x2
#define ST   0x3
#define JSR  0x4
#define AND  0x5
#define LDR  0x6
#define STR  0x7
#define RTI  0x8
#define NOT  0x9
#define LDI  0xA
#define STI  0xB
#define RET  0xC
#define LEA  0xE
#define TRAP 0xF

namespace lc3 {
    // For easier programming because Samū is lazy
    typedef uint16_t UInt;
    typedef int16_t Int;

    /// @brief Get the specified range of bits from an opcode. Both ends of the range are inclusive.
    ///        MSB is 15, LSB is 0.
    /// @param op    The opcode
    /// @param from  The most significant bit that must be returned
    /// @param to    The least significant bit that must be returned
    /// @return      The bits, in a 0 padded integer
    inline constexpr UInt getBits(UInt op, UInt from, UInt to) {
        const UInt mask = ~(0xFFFF << ((from + 1) - to));
        return (op >> to) & mask;
    }

    /// @brief Get the specified bit from an opcode. MSB is 15, LSB is 0.
    /// @param op   The opcode
    /// @param bit  The bit
    /// @return     The bit
    inline constexpr bool getBit(UInt op, UInt bit) {
        return (op >> bit) & 1;
    }

    /// @brief Sign-extend a range of bits, given the bitlength.
    /// @param op        The bits
    /// @param bitlength The amount of bits that matter
    /// @return          The sign-extended bits
    inline constexpr Int sext(UInt op, UInt bitlength) {
        if (op >> (bitlength - 1)) { // Sign
            return ((0xFFFF << bitlength) | op);
        } else {
            return op;
        }
    }

    struct Instruction {
        const UInt name;
        const UInt instruction;

        Instruction(UInt instruction) : name(getBits(instruction, 16, 12)), instruction(instruction) {
        }

        /// @brief Converts the instruction into a binary string of 16 bits
        /// @return The binary string
        string binaryString() {
            string str = "";

            UInt insn = instruction;
            for (int i = 0; i < 16; i ++) {
                if (insn & 0x8000) {
                    str += "1";
                } else {
                    str += "0";
                }

                insn <<= 1;
            }

            return str;
        }

        /// @brief Converts the instruction into a hexadecimal string of 4 digits, prepended with 'x'.
        /// @return The hexadecimal string
        string hexString() {
            char hex[6];
            sprintf((char *) &hex, "x%04X", instruction);
            return string(hex);
        }

        /// @brief Converts the instruction into a readable assembly-like string.
        /// @return The assembly string
        string assemblyString() {
            switch (name) {
                case BR: 
                {
                    // BRnz   [OFFSET +3]
                    // BRp    [OFFSET -1]

                    string insn = "BR";
                    bool n = getBit(instruction, 11);
                    bool z = getBit(instruction, 10);
                    bool p = getBit(instruction, 9);

                    int spaces = 3;
                    if (n) { insn += "n"; spaces --; }
                    if (z) { insn += "z"; spaces --; }
                    if (p) { insn += "p"; spaces --; }

                    for (int i = 0; i < spaces; i ++)
                        insn += " ";

                    Int offset = sext(getBits(instruction, 8, 0), 9);

                    insn += "  ";
                    insn += "[OFFSET ";
                    if (offset < 0) insn += "-" + to_string(- offset);
                    else            insn += "+" + to_string(+ offset);
                    insn += "]";

                    return insn;
                }
                break;

                case ADD:
                case AND:
                {
                    // ADD    R1 R2 #-1
                    // AND    R0 R0 #0
                    // ADD    R3 R3 R1

                    bool imm = getBit(instruction, 5);

                    UInt dest = getBits(instruction, 11, 9);
                    UInt src1 = getBits(instruction, 8, 6);

                    string insn = name == ADD ? "ADD    " : "AND    ";

                    insn += "R" + to_string(dest);
                    insn += " R" + to_string(src1);

                    if (imm) {
                        Int v = sext(getBits(instruction, 4, 0), 5);
                        insn += " #" + to_string(v);
                    } else {
                        UInt src2 = getBits(instruction, 2, 0);
                        insn += " R" + to_string(src2);
                    }

                    return insn;
                }
                break;

                case LD:
                case LDI:
                case ST:
                case STI:
                case LEA:
                {
                    // LD     R0 [OFFSET +3]
                    // LDI    R5 [OFFSET -9]
                    // ST     R1 [OFFSET +7]
                    // STI    R2 [OFFSET +19]
                    // LEA    R4 [OFFSET -2]

                    UInt dest = getBits(instruction, 11, 9);
                    Int  addr = sext(getBits(instruction, 8, 0), 9);

                    string insn = "";

                    switch (name) {
                        case LD:  insn += "LD     "; break;
                        case LDI: insn += "LDI    "; break;
                        case ST:  insn += "ST     "; break;
                        case STI: insn += "STI    "; break;
                        case LEA: insn += "LEA    "; break;
                    }

                    insn += "R" + to_string(dest);

                    insn += " [OFFSET ";
                    if (addr < 0) insn += "-" + to_string(- addr);
                    else          insn += "+" + to_string(+ addr);
                    insn += "]";

                    return insn;
                }
                break;

                case STR:
                case LDR:
                {
                    // STR    R2 R3 #+2
                    // LDR    R4 R1 #+0

                    UInt reg  = getBits(instruction, 11, 9);
                    UInt breg = getBits(instruction,  8, 6);
                    Int  off  = sext(getBits(instruction, 5, 0), 6);

                    string insn = name == STR ? "STR    " : "LDR    ";

                    insn += "R" + to_string(reg);
                    insn += " R" + to_string(breg);

                    insn += " #";
                    if (off < 0) insn += "-" + to_string(- off);
                    else         insn += "+" + to_string(+ off);

                    return insn;
                }
                break;

                case NOT:
                {
                    // NOT    R2 R2

                    UInt dest = getBits(instruction, 11, 9);
                    UInt src1 = getBits(instruction, 8, 6);

                    string insn = "NOT    ";

                    insn += "R" + to_string(dest);
                    insn += " R" + to_string(src1);

                    return insn;
                }
                break;

                case JSR:
                {
                    // JSRR   R1
                    // JSR    [OFFSET +3]

                    bool jsrr = getBit(instruction, 11);
                    string insn = jsrr ? "JSRR   " : "JSR    ";

                    if (!jsrr) {
                        Int offset = sext(getBits(instruction, 10, 0), 11);

                        insn += "[OFFSET ";
                        if (offset < 0) insn += "-" + to_string(- offset);
                        else            insn += "+" + to_string(+ offset);
                        insn += "]";
                    } else {
                        UInt src = getBits(instruction, 8, 6);
                        insn += " R" + to_string(src);
                    }

                    return insn;
                }
                break;

                case TRAP:
                {
                    // TRAP   x25
                    
                    string insn = "TRAP   ";

                    UInt src = getBits(instruction, 7, 0);

                    ostringstream ss;
                    ss << "x" << uppercase << hex << src;

                    insn += ss.str();

                    return insn;
                }
                break;

                // These are pretty straightforward
                case RET: return "RET"; break;
                case RTI: return "RTI"; break;

                default: return "[RESERVED]"; break;
            }
        }
    };
}

int main(int argc, char** argv) {
    int mode = 1;

    int output = 0;

    istream *in = nullptr;

    ifstream *rmvIn = nullptr;

    uint16_t insnn = 0x3000;

    bool o = false;
    for (int i = 1; i < argc; i++) {
        if (o) {
            o = false;
            char *p;
            unsigned long long n = strtoull(argv[i], &p, 16);

            if (*p != 0) {
                cerr << argv[0] << ": invalid offset, provide a hexadecimal number" << endl;
                cerr << "Usage: " << argv[0] << " [-x] [-b] [-a] [-h] [-o <offset>] <file>" << endl;
                cerr << "Type '" << argv[0] << " -h' for help" << endl;

                if (rmvIn != nullptr) {
                    rmvIn->close();
                    delete rmvIn;
                }
                return 0;
            }

            insnn = n;

            continue;
        }

        string arg = string(argv[i]);
        if (arg == "-x") {        // Hex input
            mode = 1;
        } else if (arg == "-b") { // Binary input
            mode = 0;
        } else if (arg == "-a") { // Assembly only
            output = 1;
        } else if (arg == "-h") { // Help menu
            cout << "Usage: " << argv[0] << " [-x] [-b] [-a] [-h] [-o <offset>] <file>" << endl;
            cout << endl;
            cout << "Convert hexadecimal or binary LC3 machine code into more friendly, readable" << endl;
            cout << "assembly code. Note that the assembly code may not be syntactically valid, it" << endl;
            cout << "is just for debugging purposes." << endl;
            cout << endl;
            cout << "The input file can be the standard input, specify it with a dash: '-'. When this" << endl;
            cout << "is used, an empty line will stop the program. If a file is read, empty lines will" << endl;
            cout << "be ignored." << endl;
            cout << endl;
            cout << "  -x: Hexadecimal input mode (default input mode). Each line must be a" << endl;
            cout << "      hexadecimal number." << endl;
            cout << "  -b: Binary input mode. Each line must be a binary number." << endl;
            cout << "  -a: Only output the assembly, and not the binary and hexadecimal" << endl;
            cout << "      machine code." << endl;
            cout << "  -h: Print this menu." << endl;
            cout << "  -o: Provide the offset of the program in the LC3 memory in hexadecimal. Default" << endl;
            cout << "      is 3000." << endl;

            if (rmvIn != nullptr) {
                rmvIn->close();
                delete rmvIn;
            }
            return 0;
        } else if (arg == "-o") {   // Offset
            o = true;
        } else if (arg == "-") {    // Use stdin
            in = &cin;
        } else {
            if (rmvIn != nullptr) { // Use file
                rmvIn->close();
                delete rmvIn;
            }
            rmvIn = new ifstream();
            rmvIn->open(arg);
            in = rmvIn;
        }
    }

    // No input
    if (in == nullptr) {
        if (rmvIn != nullptr) {
            rmvIn->close();
            delete rmvIn;
        }

        cerr << argv[0] << ": no input file" << endl;
        cerr << "Usage: " << argv[0] << " [-x] [-b] [-a] [-h] [-o <offset>] <file>" << endl;
        cerr << "Type '" << argv[0] << " -h' for help" << endl;
        return 0;
    }


    char insnNum[8];
    for (string line; getline(*in, line);) {
        if (line.length() == 0) {
            if (in == &cin) {
                if (rmvIn != nullptr) {
                    rmvIn->close();
                    delete rmvIn;
                }
                return 0;
            } else {
                continue;
            }
        }

        char *p;
        unsigned long long n = strtoull(line.c_str(), &p, mode ? 16 : 2);
        if (*p != 0) {
            cout << "Invalid opcode: " << p << endl;
        } else {
            lc3::Instruction insn = {(lc3::UInt) n};

            switch (output) {
                default:
                case 0:
                    sprintf((char *) &insnNum, "x%04X", insnn);
                    cout << insnNum << " | " << insn.hexString() << " | " << insn.binaryString() << " | " << insn.assemblyString() << endl;
                break;
                
                case 1:
                    cout << insn.assemblyString() << endl;
                break;
            }
        }

        insnn ++;
    }

    if (rmvIn != nullptr) {
        rmvIn->close();
        delete rmvIn;
    }
}