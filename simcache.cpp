/*
CS-UY 2214
Jeff Epstein
Starter code for E20 cache assembler
simcache.cpp
*/

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <limits>
#include <iomanip>
#include <regex>
#include <cmath>

using namespace std;

size_t const static MEM_SIZE = 1<<13;

/*
    Prints out the correctly-formatted configuration of a cache.

    @param cache_name The name of the cache. "L1" or "L2"

    @param size The total size of the cache, measured in memory cells.
        Excludes metadata

    @param assoc The associativity of the cache. One of [1,2,4,8,16]

    @param blocksize The blocksize of the cache. One of [1,2,4,8,16,32,64])

    @param num_rows The number of rows in the given cache.
*/
void print_cache_config(const string &cache_name, int size, int assoc, int blocksize, int num_rows) {
    cout << "Cache " << cache_name << " has size " << size <<
        ", associativity " << assoc << ", blocksize " << blocksize <<
        ", rows " << num_rows << endl;
}

/*
    Prints out a correctly-formatted log entry.

    @param cache_name The name of the cache where the event
        occurred. "L1" or "L2"

    @param status The kind of cache event. "SW", "HIT", or
        "MISS"

    @param pc The program counter of the memory
        access instruction

    @param addr The memory address being accessed.

    @param row The cache row or set number where the data
        is stored.
*/
void print_log_entry(const string &cache_name, const string &status, int pc, int addr, int row) {
    cout << left << setw(8) << cache_name + " " + status <<  right <<
        " pc:" << setw(5) << pc <<
        "\taddr:" << setw(5) << addr <<
        "\trow:" << setw(4) << row << endl;
}

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
    create_vector_of_rows(num_of_rows)
    creates a cache containing empty rows (vectors)
    parameters:
        num_of_rows = number of rows in cache being created
 */
vector<vector<int>> create_cache(int num_of_rows) {
    // create vector of rows
        // each row stores tag
    vector<vector<int>> L1;
    vector<int> empty;
    for (int i = 0; i < num_of_rows; i++) {
        L1.push_back(empty);
    }
    return L1;
}

/*
    add_tag(cache_row, assoc, tag)
    checks if cache_row is full
        if it is -> evict LRU
    adds tag to cache_row
    parameters:
        cache_row = cache row tag is being added to
        assoc = associativity of cache which cache row comes from
        tag = val being added to cache_row
 */
void add_tag(vector<int>& cache_row, int assoc, int tag) {
    // if row is full, an eviction needs to happen
    if (cache_row.size() == assoc) {
        // evict LRU, aka first element in L1 vector
        cache_row.erase(cache_row.begin());
    }
    // add current element to end of cache_row
    cache_row.push_back(tag);
}

/*
    cache_lw(mem_addr, blocksize, num_rows, cache, cache_name, pc, assoc)
    calculates the desired tag/row for a lw operation and searches a given cache for that tag/row combination
        if the tag/row combination exists in the cache -> cache hit is recorded
        else -> cache miss is recorded
            if cache row is full, evicts LRU element
            loads desired tag to desired cache row
    returns cache row containing desired tag, bool indicating if it was a hit or miss, and int indicating which cache row is being returned
    parameters:
        mem_addr = memory address being loaded in lw operation
        blocksize = size of blocks stored in/loaded to cache
        num_rows = number of rows in cache
        cache = cache being searched for row/tag combination
        cache_name = name of cache being searched
        pc = program counter
        assoc = associativity of cache being searched
 */
tuple<vector<int>, bool, int> cache_lw(int mem_addr, int blocksize, int num_rows, vector<vector<int>> cache, const string& cache_name, int pc, int assoc) {
    int blockid = floor(mem_addr / blocksize);
    int row = blockid % num_rows;
    int tag = floor(blockid / num_rows);
    // find row in cache we're searching
    vector<int> cache_row = cache[row];
    // track if tag is found in cache_row
    bool hit = false;
    // check if tag is already in row
    for (int i = 0; i < cache_row.size(); i++) {
        if (cache_row[i] == tag) { // cache hit
            hit = true;
            // remove found tag- need to add to the end of cache_row to preserve LRU
            cache_row.erase(cache_row.begin() + i);
            // add the entry back to the end of cache_row
            cache_row.push_back(tag);
            print_log_entry(cache_name, "HIT", pc, mem_addr, row);
            break;
        }
    }
    if (hit == false) { // cache miss
        print_log_entry(cache_name, "MISS", pc, mem_addr, row);
        add_tag(cache_row, assoc, tag);
    }
    return {cache_row, hit, row};
}

/*
    cache_sw(mem_addr, blocksize, num_rows, cache, assoc, cache_name, pc)
    calculates the desired tag/row for a sw operation
        adds the desired tag to the desired row of the cache
    returns cache row containing which now contains desired tag and int indicating which cache row is being returned
    parameters:
        mem_addr = memory address being loaded in lw operation
        blocksize = size of blocks stored in/loaded to cache
        num_rows = number of rows in cache
        cache = cache being searched for row/tag combination
        assoc = associativity of cache being searched
        cache_name = name of cache being searched
        pc = program counter
 */
tuple<vector<int>, int> cache_sw(int mem_addr, int blocksize, int num_rows, vector<vector<int>> cache, int assoc, const string& cache_name, int pc) {
    int blockid = floor(mem_addr / blocksize);
    int row = blockid % num_rows;
    int tag = floor(blockid / num_rows);
    vector<int> cache_row = cache[row];
    add_tag(cache_row, assoc, tag);
    print_log_entry(cache_name, "SW", pc, mem_addr, row);
    return {cache_row, row};
}

/*
    execute(mem, pc, regs, num_of_cache)
    gets the current instruction to execute mem[pc]
        based on the opcode of the current instruction, executes the current sequence of operations
        updates cache if current opcode is lw or sw
    parameters:
        mem[] = array containing the memory cells 0-8191
        pc = program counter, tracks which mem cell should currently execute
        regs[] = array containing the values of regs 1-7
        blocksize[] = array containing blocksize of L1 cache (and L2 cache, if applicable)
        num_rows[] = array containing number of rows in L1 cache (and L2 cache, if applicable)
        assoc[] = array containing associativity of L1 cache (and L2 cache, if applicable)
        num_of_cache = indicates the number of caches
        vector<vector<int>>& L1 = vector representing L1 cache
        vector<vector<int>>& L2 = vector representing L2 cache
 */
 // when passing an array by name, you're actually passing a pointer to the first element in the array
    // thus, it'll modify the original array you passed in, not a copy
    // passing an array by reference isn't a thing
vector<uint16_t> execute(uint16_t mem[], uint16_t pc, uint16_t regs[], int blocksize[], int num_rows[], int assoc[], int num_of_cache, vector<vector<int>>& L1, vector<vector<int>>& L2) {
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
                // CACHE
                // cache L1
                tuple<vector<int>, bool, int> return_val_L1 = cache_lw(mem_addr, blocksize[0], num_rows[0], L1, "L1", pc, assoc[0]);
                bool hit = get<1>(return_val_L1);
                int row_L1 = get<2>(return_val_L1);
                L1[row_L1] = get<0>(return_val_L1);
                if (hit == false) { // cache miss on L1
                    if (num_of_cache == 2) { // consult L2 if there is a L1 miss
                        tuple<vector<int>, bool, int> return_val_L2 = cache_lw(mem_addr, blocksize[1], num_rows[1], L2, "L2", pc, assoc[1]);
                        int row_L2 = get<2>(return_val_L2);
                        L2[row_L2] = get<0>(return_val_L2);
                    }
                }
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
            // CACHE
            tuple<vector<int>, int> return_val_L1 = cache_sw(mem_addr, blocksize[0], num_rows[0], L1, assoc[0], "L1", pc);
            int row_L1 = get<1>(return_val_L1);
            L1[row_L1] = get<0>(return_val_L1);
            if (num_of_cache == 2) {
                tuple<vector<int>, int> return_val_L2 = cache_sw(mem_addr, blocksize[1], num_rows[1], L2, assoc[1], "L2", pc);
                int row_L2 = get<1>(return_val_L2);
                L2[row_L2] = get<0>(return_val_L2);
            }
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
    string cache_config;
    for (int i=1; i<argc; i++) {
        string arg(argv[i]);
        if (arg.rfind("-",0)==0) {
            if (arg== "-h" || arg == "--help")
                do_help = true;
            else if (arg=="--cache") {
                i++;
                if (i>=argc)
                    arg_error = true;
                else
                    cache_config = argv[i];
            }
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
    if (arg_error || do_help || filename == nullptr) {
        cerr << "usage " << argv[0] << " [-h] [--cache CACHE] filename" << endl << endl;
        cerr << "Simulate E20 cache" << endl << endl;
        cerr << "positional arguments:" << endl;
        cerr << "  filename    The file containing machine code, typically with .bin suffix" << endl<<endl;
        cerr << "optional arguments:"<<endl;
        cerr << "  -h, --help  show this help message and exit"<<endl;
        cerr << "  --cache CACHE  Cache configuration: size,associativity,blocksize (for one"<<endl;
        cerr << "                 cache) or"<<endl;
        cerr << "                 size,associativity,blocksize,size,associativity,blocksize"<<endl;
        cerr << "                 (for two caches)"<<endl;
        return 1;
    }
    
    // *****************
    // open file here
    ifstream f(filename);
    if (!f.is_open()) {
        cerr << "Can't open file "<<filename<<endl;
        return 1;
    }
    // *****************
    
    // *****************
    // initialize processor state
        // pc, regs, and mem are initialized to 0
        // have max 16 bits (uint16_t)
    uint16_t pc = 0;
    uint16_t regs[8] = {0};
    uint16_t mem[8192] = {0};
    load_machine_code(f, mem);
    // *****************
        
    /* parse cache config */
    if (cache_config.size() > 0) {
        vector<int> parts;
        size_t pos;
        size_t lastpos = 0;
        while ((pos = cache_config.find(",", lastpos)) != string::npos) {
            parts.push_back(stoi(cache_config.substr(lastpos,pos)));
            lastpos = pos + 1;
        }
        parts.push_back(stoi(cache_config.substr(lastpos)));
        if (parts.size() == 3) {
            int L1size = parts[0];
            int L1assoc = parts[1];
            int L1blocksize = parts[2];
            // TODO: execute E20 program and simulate one cache here
            int blocksize[1] = {L1blocksize};
            int rows = (L1size / L1assoc) / L1blocksize;
            int num_rows[1] = {rows};
            int assoc[1] = {L1assoc};
            int num_of_cache = 1;
            vector<vector<int>> L1 = create_cache(rows);
            vector<vector<int>> L2 = {{0}};
            print_cache_config("L1", L1size, L1assoc, L1blocksize, rows);
            bool halt = false;
            while (halt == false) {
                vector<uint16_t> return_vals = execute(mem, pc, regs, blocksize, num_rows, assoc, num_of_cache, L1, L2);
                pc = return_vals[0];
                if (return_vals[1] == 1) {
                    halt = true;
                }
            }
        } else if (parts.size() == 6) {
            int L1size = parts[0];
            int L1assoc = parts[1];
            int L1blocksize = parts[2];
            int L2size = parts[3];
            int L2assoc = parts[4];
            int L2blocksize = parts[5];
            // TODO: execute E20 program and simulate two caches here
            int blocksize[2] = {L1blocksize, L2blocksize};
            int L1_rows = (L1size / L1assoc) / L1blocksize;
            int L2_rows = (L2size / L2assoc) / L2blocksize;
            int num_rows[2] = {L1_rows, L2_rows};
            int assoc[2] = {L1assoc, L2assoc};
            int num_of_cache = 2;
            vector<vector<int>> L1 = create_cache(L1_rows);
            vector<vector<int>> L2 = create_cache(L2_rows);
            print_cache_config("L1", L1size, L1assoc, L1blocksize, L1_rows);
            print_cache_config("L2", L2size, L2assoc, L2blocksize, L2_rows);
            bool halt = false;
            while (halt == false) {
                vector<uint16_t> return_vals = execute(mem, pc, regs, blocksize, num_rows, assoc, num_of_cache, L1, L2);
                pc = return_vals[0];
                if (return_vals[1] == 1) {
                    halt = true;
                }
            }
            
        } else {
            cerr << "Invalid cache config"  << endl;
            return 1;
        }
    }
    
    return 0;
}
//ra0Eequ6ucie6Jei0koh6phishohm9
