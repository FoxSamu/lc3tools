#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstdio>
#include <string>
#include <sstream>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

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

        Instruction(UInt instruction): name(getBits(instruction, 16, 12)), instruction(instruction) {
        }

        /// @brief Converts the instruction into a binary string of 16 bits
        /// @return The binary string
        string binaryString() {
            string str = "";

            UInt insn = instruction;
            for (int i = 0; i < 16; i++) {
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
            sprintf((char *)&hex, "x%04X", instruction);
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
                    if (n) { insn += "n"; spaces--; }
                    if (z) { insn += "z"; spaces--; }
                    if (p) { insn += "p"; spaces--; }

                    for (int i = 0; i < spaces; i++)
                        insn += " ";

                    Int offset = sext(getBits(instruction, 8, 0), 9);

                    insn += "  ";
                    insn += "[OFFSET ";
                    if (offset < 0) insn += "-" + to_string(-offset);
                    else            insn += "+" + to_string(+offset);
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
                    if (addr < 0) insn += "-" + to_string(-addr);
                    else          insn += "+" + to_string(+addr);
                    insn += "]";

                    return insn;
                }
                break;

                case STR:
                case LDR:
                {
                    // STR    R2 R3 #+2
                    // LDR    R4 R1 #+0

                    UInt reg = getBits(instruction, 11, 9);
                    UInt breg = getBits(instruction, 8, 6);
                    Int  off = sext(getBits(instruction, 5, 0), 6);

                    string insn = name == STR ? "STR    " : "LDR    ";

                    insn += "R" + to_string(reg);
                    insn += " R" + to_string(breg);

                    insn += " #";
                    if (off < 0) insn += "-" + to_string(-off);
                    else         insn += "+" + to_string(+off);

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
                        if (offset < 0) insn += "-" + to_string(-offset);
                        else            insn += "+" + to_string(+offset);
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

#define USAGE(out, name) (out) << "Usage: " << (name) << " [-b] [-a] [-o <offset>] <file>" << endl \
                               << "       " << (name) << " -h" << endl;

int main(int argc, char **argv) {
    class exit {
        public:
        char code;
        exit(): code(0) {
        }
        exit(char c): code(c) {
        }
    };

    class inputError {
        public:
        string problem;
        inputError(string problem): problem(problem) {
        }
    };

    istream *in = nullptr;
    ifstream *rmvIn = nullptr;

    char ec = 0;

    try {
        int mode = 1;
        int output = 0;

        uint16_t insnn = 0x3000;

        // Encountered Input Flags
        class enciFlags {
            private:
            int c;

            public:
            enciFlags(): c(0) {
            }

            void set(int f) {
                c |= f;
            }

            bool check(int f) const {
                return c & f;
            }
        };

        enciFlags enci;

        bool o = false;
        for (int i = 1; i < argc; i++) {
            if (o) {
                o = false;
                char *p;
                unsigned long long n = strtoull(argv[i], &p, 16);

                if (*p != 0) {
                    throw inputError("-o: invalid offset, provide a hexadecimal number");
                }

                insnn = n;

                continue;
            }

            string arg = string(argv[i]);
            if (arg == "-b") {        // Binary input
                if (enci.check(4))
                    throw inputError("-h was specified, use no other flags");
                if (enci.check(1))
                    throw inputError("-b already specified");
                enci.set(1);

                mode = 0;
            } else if (arg == "-a") { // Assembly only
                if (enci.check(4))
                    throw inputError("-h was specified, use no other flags");
                if (enci.check(2))
                    throw inputError("-a already specified");
                enci.set(2);

                output = 1;
            } else if (arg == "-h") { // Help menu
                if (enci.check(0xFFFF & ~4))
                    throw inputError("-h was specified, use no other flags");
                if (enci.check(4))
                    throw inputError("-h already specified");
                enci.set(4);
            } else if (arg == "-o") {   // Offset
                if (enci.check(4))
                    throw inputError("-h was specified, use no other flags");
                if (enci.check(8))
                    throw inputError("-o already specified");
                enci.set(8);

                o = true;
            } else if (arg == "-") {    // Use stdin
                if (enci.check(4))
                    throw inputError("-h was specified, use no other flags");
                if (enci.check(16))
                    throw inputError("input file already specified");
                enci.set(16);

                in = &cin;
            } else if (arg[0] == '-') { // Invalid
                throw inputError("unknown flag: " + arg);
            } else {                    // Use file
                if (enci.check(4))
                    throw inputError("-h was specified, use no other flags");
                if (enci.check(16))
                    throw inputError("input file already specified");
                enci.set(16);

                if (rmvIn != nullptr) {
                    rmvIn->close();
                    delete rmvIn;
                    rmvIn = nullptr;
                }

                fs::path path(arg);

                if (!fs::exists(path))
                    throw inputError(arg + ": no such file");
                if (fs::is_directory(path))
                    throw inputError(arg + ": is a directory");
                if (!fs::is_regular_file(path))
                    throw inputError(arg + ": is not a file");

                rmvIn = new ifstream(arg);
                if (!rmvIn->good())
                    throw inputError(arg + ": permission denied");
                in = rmvIn;
            }
        }

        if (o) {
            throw inputError("-o: expected offset");
        }

        if (enci.check(4)) {
            USAGE(std::cout, argv[0]);
            std::cout << endl;
            std::cout << "Convert hexadecimal or binary LC3 machine code into more friendly, readable" << endl;
            std::cout << "assembly code. Note that the assembly code may not be syntactically valid, it" << endl;
            std::cout << "is just for debugging purposes." << endl;
            std::cout << endl;
            std::cout << "The input must be in hexadecimal format, without a preceding 'x', and each" << endl;
            std::cout << "instruction on a separate line. If -b is specified, it must instead be binary." << endl;
            std::cout << endl;
            std::cout << "The input file can be the standard input, specify it with a dash: '-'. When this" << endl;
            std::cout << "is used, an empty line will stop the program. If a file is read, empty lines will" << endl;
            std::cout << "be ignored." << endl;
            std::cout << endl;
            std::cout << "  -b: Binary input mode. Each line must be a binary number." << endl;
            std::cout << "  -a: Only output the assembly, and not the binary and hexadecimal" << endl;
            std::cout << "      machine code." << endl;
            std::cout << "  -h: Print this menu." << endl;
            std::cout << "  -o: Provide the offset of the program in the LC3 memory in hexadecimal. Default" << endl;
            std::cout << "      is 3000." << endl;

            throw exit(0);
        }

        // No input
        if (in == nullptr) {
            throw inputError("no input file");
        }


        // Using sprintf to format the instruction number in here
        char insnNum[8];

        // For each input line
        for (string line; getline(*in, line);) {
            if (line.length() == 0) { // Empty line
                if (in == &cin) {
                    throw exit(0);
                } else {
                    continue;
                }
            }

            char *p;
            unsigned long long n = strtoull(line.c_str(), &p, mode ? 16 : 2);
            if (*p != 0) {
                std::cerr << "Invalid opcode: " << p << endl;
            } else {
                lc3::Instruction insn = { (lc3::UInt)n };

                switch (output) {
                    default:
                    case 0:
                        sprintf((char *)&insnNum, "x%04X", insnn);
                        std::cout << insnNum << " | " << insn.hexString() << " | " << insn.binaryString() << " | " << insn.assemblyString() << endl;
                        break;

                    case 1:
                        std::cout << insn.assemblyString() << endl;
                        break;
                }
            }

            insnn++;
        }
    } catch (inputError exc) {
        std::cerr << argv[0] << ": " << exc.problem << endl;
        USAGE(std::cerr, argv[0]);
        std::cerr << "Type '" << argv[0] << " -h' for help" << endl;

        ec = 1;
    } catch (exit exc) {
        ec = exc.code;
    }

    if (rmvIn != nullptr) {
        rmvIn->close();
        delete rmvIn;
    }

    return ec;
}

#undef USAGE