/*
CS-UY 2214
Jeff Epstein
Starter code for E20 simulator
sim.cpp
*/

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <regex>
#include <cstdlib>

using namespace std;

// Some helpful constant values that we'll be using.
size_t const static NUM_REGS = 8;
size_t const static MEM_SIZE = 1<<13;


/*
    Loads an E20 machine code file into the list
    provided by mem. We assume that mem is
    large enough to hold the values in the machine
    code file.

    @param f Open file to read from
    @param mem Array represetnting memory into which to read program
*/
void load_machine_code(ifstream &f, uint16_t mem[]) {
    regex machine_code_re("^ram\\[(\\d+)\\] = 16'b(\\d+);.*$");
    size_t expectedaddr = 0;
    string line;
    while (getline(f, line)) {
        smatch sm;
        if (!regex_match(line, sm, machine_code_re)) {
            cerr << "Can't parse line: " << line << endl;
            exit(1);
        }
        size_t addr = stoi(sm[1], nullptr, 10);
        unsigned instr = stoi(sm[2], nullptr, 2);
        if (addr != expectedaddr) {
            cerr << "Memory addresses encountered out of sequence: " << addr << endl;
            exit(1);
        }
        if (addr >= MEM_SIZE) {
            cerr << "Program too big for memory" << endl;
            exit(1);
        }
        expectedaddr ++;
        mem[addr] = instr;
    }
}

/*
    Prints the current state of the simulator, including
    the current program counter, the current register values,
    and the first memquantity elements of memory.

    @param pc The final value of the program counter
    @param regs Final value of all registers
    @param memory Final value of memory
    @param memquantity How many words of memory to dump
*/
void print_state(uint16_t pc, uint16_t regs[], uint16_t memory[], size_t memquantity) {
    cout << setfill(' ');
    cout << "Final state:" << endl;
    cout << "\tpc=" <<setw(5)<< pc << endl;

    for (size_t reg=0; reg<NUM_REGS; reg++)
        cout << "\t$" << reg << "="<<setw(5)<<regs[reg]<<endl;

    cout << setfill('0');
    bool cr = false;
    for (size_t count=0; count<memquantity; count++) {
        cout << hex << setw(4) << memory[count] << " ";
        cr = true;
        if (count % 8 == 7) {
            cout << endl;
            cr = false;
        }
    }
    if (cr)
        cout << endl;
}

/*
    ALU(regs[], val1, val2, dst, op)
    performs an ALU operation (which operation depends on op) on val1 and val2
        stores the result in regs[dst]
    parameters:
        regs[] = an array containing the values of regs 1-7, passed by reference
        val1 = first operand to the ALU
        val2 = second operand to the ALU
        op = opcode determining which ALU operation should be performed
 */
void ALU(uint16_t regs[], int val1, int val2, int dst, const string& op) {
    if ((op == "add") | (op == "addi")) {
        int sum = val1 + val2;
        regs[dst] = sum;
    }
    else if (op == "sub") {
        int diff = val1 - val2;
        regs[dst] = diff;
    }
}

/*
    less_than(regs[], val1, val2, dst)
    checks if val1 is less than val2
        if it is, stores 1 in regs[dst]- otherwise, stores 0
    parameters:
        regs[] = an array containing the values of regs 1-7
        val1 = first operand of less-than operation
        val2 = second operand to less-than operation
        dst = register to store 0 or 1 in, depending on outcome of less-than operation
 */
void less_than(uint16_t regs[], uint16_t val1, uint16_t val2, int dst) {
    if (val1 < val2) {
        regs[dst] = 1;
    }
    else {
        regs[dst] = 0;
    }
}

/*
    sign_extend(imm, msb)
    checks if the msb of imm is 0 or 1
        if its 1, it sign extends imm so its 16 bits
    parameters:
        imm = immediate value
        msb = number indicating which bit is the most significant bit of imm
 */
uint16_t sign_extend(uint16_t imm, int msb) {
    uint16_t check = 1 << (msb - 1);
    if ((imm & check) == check) {
        // sign extend by 1
        if (msb == 7) {
            // 65408 = 1111111110000000
            imm = (imm | 65408);
        }
        else { // msb == 13
            // 57344 = 1110000000000000
            imm = imm | 57344;
        }
    }
    return imm;
}

/*
    find_twos_complement(imm, msb)
    checks if most significant bit of imm is 0 or 1
        if its 1, it finds the twos complement of imm
            sign extends imm
            flips imms bits
            adds 1 to imm
        then multiplies imm by negative 1 to finish the process
    parameters:
        imm = immediate value
        msb = number indicating which bit is the most significant bit of imm
 */
int find_twos_complement(int imm, int msb) {
    // pass in integer
    // sign extend
    // if MSB == 1
    uint16_t check = 1 << (msb - 1);
    // check if imm is negative
    if ((imm & check) == check) {
        // sign extend imm
        imm = sign_extend(imm, msb);
        // flip its bits
        imm = imm ^ 65535;
        // add 1
        imm = imm + 1;
        imm = imm * -1;
    }
    return imm;
}

/*
    execute(mem, pc, regs)
    gets the current instruction to execute mem[pc]
        based on the opcode of the current instruction, executes the current sequence of operations
    parameters:
        mem[] = array containing the memory cells 0-8191
        pc = program counter, tracks which mem cell should currently execute
        regs[] = array containing the values of regs 1-7
 */
 // when passing an array by name, you're actually passing a pointer to the first element in the array
    // thus, it'll modify the original array you passed in, not a copy
    // passing an array by reference isn't a thing
vector<uint16_t> execute(uint16_t mem[], uint16_t pc, uint16_t regs[]) {
    // when accessing memory, only use the 13 lsb of the pc
    // 8191 = 1111111111111
    uint16_t mem_pc = pc & 8191;
    uint16_t curr_instr = mem[mem_pc];
    // tracks if you're instruction jumps or not
    bool jump = false;
    // increment program counter
        // if instruction caused jump, will update new_pc later
    uint16_t new_pc = pc + 1;
    // tracks if instruction was a halt
    bool halt = false;
    // & curr_instr with 11100000000000000 to get 3 msb
    int three_msb = curr_instr & 57344;
    three_msb = three_msb >> 13;
    // 3 register arguments
    if (three_msb == 0) {
        // 15 = 0000000000001111
        int four_lsb = curr_instr & 15;
        // 7168 = 0001110000000000
        int srcA = curr_instr & 7168;
        srcA = srcA >> 10;
        // 896 = 0000001110000000
        int srcB = curr_instr & 896;
        srcB = srcB >> 7;
        int dst = curr_instr & 112;
        dst = dst >> 4;
        // register 0 is immutable
        if (dst != 0) {
            if (four_lsb == 0) {
                // add
                int val1 = regs[srcA];
                int val2 = regs[srcB];
                ALU(regs, val1, val2, dst, "add");
            }
            else if (four_lsb == 1) {
                // sub
                int val1 = regs[srcA];
                int val2 = regs[srcB];
                ALU(regs, val1, val2, dst, "sub");
            }
            else if (four_lsb == 2) {
                // or
                uint16_t bitwise_or = regs[srcA] | regs[srcB];
                regs[dst] = bitwise_or;
            }
            else if (four_lsb == 3) {
                // and
                uint16_t bitwise_and = regs[srcA] & regs[srcB];
                regs[dst] = bitwise_and;
            }
            else if (four_lsb == 4) {
                // slt
                uint16_t val1 = regs[srcA];
                uint16_t val2 = regs[srcB];
                less_than(regs, val1, val2, dst);
            }
        }
        // jr doesn't modify regDst
            // thus, it doesn't matter if regDst == 0
        if (four_lsb == 8) {
            // jr
            new_pc = regs[srcA];
            jump = true;
        }
    }
    // no register arguments
    else if ((three_msb == 2) | (three_msb == 3)) {
        // 8191 = 0001111111111111
        // imm = non-negative absolute address
        int imm = curr_instr & 8191;
        if (three_msb == 2) {
            // j
            new_pc = imm;
            jump = true;
            // check for halt
            if (new_pc == pc) {
                halt = true;
            }
        }
        else { // three_msb == 3
            // jal
            // ****************
            regs[7] = pc + 1;
            new_pc = imm;
            jump = true;
        }
    }
    // two register arguments
    else {
        int src = curr_instr & 7168;
        src = src >> 10;
        int dst = curr_instr & 896;
        dst = dst >> 7;
        // 127 = 0000000001111111
        int imm = curr_instr & 127;
        // slti, lw, addi
        if (dst != 0) {
            if (three_msb == 7) {
                // slti
                // only op where imm = unsigned
                uint16_t val1 = regs[src];
                imm = sign_extend(imm, 7);
                less_than(regs, val1, imm, dst);
            }
            else if (three_msb == 4) {
                // lw
                int addr = regs[src];
                // check if imm is negative
                imm = find_twos_complement(imm, 7);
                // only least significant 13 bits of mem_addr are used to index into memory
                int mem_addr = (imm + addr) & 8191;
                int read_from_memory = mem[mem_addr];
                regs[dst] = read_from_memory;
            }
            else if (three_msb == 1) {
                // addi
                int val1 = regs[src];
                imm = find_twos_complement(imm, 7);
                ALU(regs, val1, imm, dst, "addi");
            }
        }
        // sw & jeq don't modify regDst
            // thus, it doesn't matter if regDst == 0
        if (three_msb == 5) {
            // sw
            int addr = regs[src];
            imm = find_twos_complement(imm, 7);
            int mem_addr = (imm + addr) & 8191;
            int write_to_memory = regs[dst];
            mem[mem_addr] = write_to_memory;
        }
        else if (three_msb == 6){
            // jeq
            int val1 = regs[src];
            int val2 = regs[dst];
            if (val1 == val2) {
                // jump
                // sign extend the rel_imm and find out if its
                // imm = rel_imm
                imm = find_twos_complement(imm, 7);
                new_pc = pc + imm + 1;
                jump = true;
            }
        }

    }
    uint16_t stop;
    if (halt) {
        stop = 1;
    }
    else {
        stop = 0;
    }
    vector<uint16_t> return_vals;
    return_vals.push_back(new_pc);
    return_vals.push_back(stop);
    return return_vals;
}

/*
    Main function
    Takes command-line args as documented below
*/
int main(int argc, char *argv[]) {
    /*
        Parse the command-line arguments
    */
    char *filename = nullptr;
    bool do_help = false;
    bool arg_error = false;
    for (int i=1; i<argc; i++) {
        string arg(argv[i]);
        if (arg.rfind("-",0)==0) {
            if (arg== "-h" || arg == "--help")
                do_help = true;
            else
                arg_error = true;
        } else {
            if (filename == nullptr)
                filename = argv[i];
            else
                arg_error = true;
        }
    }
    /* Display error message if appropriate */
//    if (arg_error || do_help || filename == nullptr) {
//        cerr << "usage " << argv[0] << " [-h] filename" << endl << endl;
//        cerr << "Simulate E20 machine" << endl << endl;
//        cerr << "positional arguments:" << endl;
//        cerr << "  filename    The file containing machine code, typically with .bin suffix" << endl<<endl;
//        cerr << "optional arguments:"<<endl;
//        cerr << "  -h, --help  show this help message and exit"<<endl;
//        return 1;
//    }
//    ifstream f(filename);
    ifstream f("hw7q1.txt");
    if (!f.is_open()) {
        cerr << "Can't open file "<<filename<<endl;
        return 1;
    }
    // TODO: your code here. Load f and parse using load_machine_code
    // initialize processor state
        // pc, regs, and mem are initialized to 0
        // have max 16 bits (uint16_t)
    uint16_t pc = 0;
    uint16_t regs[8] = {0};
    uint16_t mem[8192] = {0};
    load_machine_code(f, mem);

    // TODO: your code here. Do simulation.
    bool halt = false;
    while (halt == false) {
        vector<uint16_t> return_vals = execute(mem, pc, regs);
        pc = return_vals[0];
        if (return_vals[1] == 1) {
            halt = true;
        }
    }

    // TODO: your code here. print the final state of the simulator before ending, using print_state
    print_state(pc, regs, mem, 128);
    return 0;
}
//ra0Eequ6ucie6Jei0koh6phishohm9
