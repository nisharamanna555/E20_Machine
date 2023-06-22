/*
CS-UY 2214
Jeff Epstein
Starter code for E20 assembler
asm.cpp
*/

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <bitset>
// using map
#include <map>

using namespace std;

/*
    print_line(address, num)
    Print a line of machine code in the required format.
    Parameters:
        address = RAM address of the instructions
        num = numeric value of machine instruction
*/
void print_machine_code(unsigned address, unsigned num) {
    bitset<16> instruction_in_binary(num);
    cout << "ram[" << address << "] = 16'b" << instruction_in_binary <<";"<<endl;
}

/*
    make_line_upper(line)
    makes every character in a string upper case
    parameters:
        line = a string read from the file- passed by reference so we can modify the actual given line
 */
void make_line_upper(string& line) {
    for (size_t i = 0; i < line.size(); i++) {
        line[i] = toupper(line[i]);
    }
}

/*
    get_label(line, labels, pc)
    finds all labels in a line
        adds the label and its corresponding value to a map named labels
        increments program counter if needeed
            returns the unchanged/changed pc
    parameters:
        line = a string read from the file
        labels = a map containing labels and its corresponding value
        pc = program counter, keeps track of RAM address of instruction
 */
int16_t get_label(string& line, map<string, int>& labels, int16_t pc) {
    bool cont = true;
    size_t pos = 0;
    while (cont == true) {
        // get rid of spaces before/inbetween labels
        while (line[pos] == ' ') {
            pos += 1;
        }
        // find colons, b/c labels are always followed by colon
        size_t new_pos = line.find(":");
        // if there is no label in the line, then the line must only contain machine code
            // increment PC and end loop
        if (new_pos == string::npos) {
            pc += 1;
            cont = false;
        }
        else {
            // first parameter of str.substr = beginning position of substring
            // second parameter of str.substr = num of characters to include in substring
            string curr_label = line.substr(pos, (new_pos + 1 - pos));
            labels.insert({curr_label, pc});
            // check if we've reached end of line
            if ((new_pos + 1) == line.length()) {
                // if so, finish loop
                cont = false;
            }
            // otherwise, evaluate the rest of the line for more labels/instructions
            else {
                line = line.substr(new_pos + 1, line.length());
            }
        }
    }
    return pc;
}

/*
    split_line(line)
    delimits a line by characters ',', ' ', ':', '(', ')'
        adds the new substrings of the line to a vector
            returns the vector
    Parameters:
        line = a string read from a file
 */
// source = https://stackoverflow.com/questions/7621727/split-a-string-into-words-by-multiple-delimiters
vector<string> split_line(string& line) {
    size_t prev = 0;
    size_t pos;
    vector<string> parsed_line;
    // add space to end of line
        // lines w/ no comment have no space at the end
        // thus, final element of string won't be parsed b/c there will be no space to detect
    line = line + " ";
    // delimiting line by " ", ":", "(", or ")"
    while ((pos = line.find_first_of(", :()", prev)) != string::npos) {
        if (pos > prev) {
            // when delimiting a line by a character it deletes those characters
                // we want to keep our colons tho, to be able to later identify a label
            if (line[pos] == ':') {
                parsed_line.push_back((line.substr(prev, pos-prev)) + ":");
            }
            else {
                parsed_line.push_back(line.substr(prev, pos-prev));
            }
        }
        prev = pos+1;
    }
    return parsed_line;
}

/*
    three_register_arguments(reg1, reg2, reg3, func)
        handles add, sub, or, and, slt, jr
    adds 3 register values and one func value, creating a machine code value
        returns machine code as an int
        register values are passed as chars
            turns these char values into ints
            then shifts them to their correct positions
    Parameters:
        reg1 = tells us which register is stored in the first register position as a char value
        reg2 = tells us which register is stored in the second register position as a char value
        reg3 = tells us which register is stored in the third register position as a char value
 */
int16_t three_register_arguments(char reg1, char reg2, char reg3, int func) {
    // opcode, aka 3 msb, is 000 for ALL 3 register arguments
    // reg1, reg2, reg3 corresponds to the first, second, and third register in the machine code breakdown
    // func refers to 4 lsb of machine code
    
    // convert registers from char to int
        // convert char to str, use stoi on str
    string reg1str(1, reg1);
    int reg1int = stoi(reg1str);
    string reg2str(1, reg2);
    int reg2int = stoi(reg2str);
    string reg3str(1, reg3);
    int reg3int = stoi(reg3str);
    // correctly shift reg1, reg2, and reg3
    // reg1 occupies bits 12-10
        // shift regA by 11 to get it in right position
    reg1int = reg1int << 10;
    reg2int = reg2int << 7;
    reg3int = reg3int << 4;
    // sum values of shifted registers w/ func to get machine code
    int16_t machine_code = reg1int + reg2int + reg3int + func;
    return machine_code;
}

/*
    two_register_arguments(opc, reg1, reg2, imm)
        handles slti, lw, sw, jeq
    adds an opcode value, 2 register values, and one imm value, creating a machine code value
        returns machine code as an int
        register values are passed as chars
            turns these char values into ints
            then shifts them to their correct positions
        shifts opcode to correct position
    Parameters:
        opc = tells us the opcode for this instruction, given as an int
        reg1 = tells us which register is stored in the first register position as a char value
        reg2 = tells us which register is stored in the second register position as a char value
        imm = immediate value for argument, given as an int
 */
int16_t two_register_arguments(int opc, char reg1, char reg2, int imm) {
    // opc refers to opcode, aka 3 msb of machine code
    // reg1, reg2 corresponds to the first and second register in the machine code breakdown
    // imm refers to immediate value, aka 7 lsb of machine code
    string reg1str(1, reg1);
    int reg1int = stoi(reg1str);
    string reg2str(1, reg2);
    int reg2int = stoi(reg2str);
    opc = opc << 13;
    reg1int = reg1int << 10;
    reg2int = reg2int << 7;
    // and imm with 1111111 (127)
        // imm value must be expressed as a signed 7-bit number
        // so if imm is negative, it doesn't mess things up with overflow
    imm = imm & 127;
    int16_t machine_code = opc + reg1int + reg2int + imm;
    return machine_code;
}

/*
    no_register_arguments(opc, imm)
        handles j, jal
    adds an opcode value and one imm value, creating a machine code value
        returns machine code as an int
        register values are passed as chars
        shifts opcode to correct position
    Parameters:
        opc = tells us the opcode for this instruction, given as an int
        imm = immediate value for argument, given as an int
 */
int16_t no_register_arguments(int opc, int imm) {
    // opc refers to opcode, aka 3 msb of machine code
    // imm refers to immediate value, aka 13 lsb of machine code

    // 1111111111111 = 8191
    imm = imm & 8191;
    opc = opc << 13;
    int16_t machine_code = opc + imm;
    return machine_code;
}

/*
    find_imm_val(imm, labels)
    determines if an immediate value is given as a number or a label
        if its a label, returns the labels corresponding val
        if its a number, returns the number as an int
    Parameters:
        labels = a map containing labels and its corresponding value
        imm = immediate value for argument, given as a string
 */
int find_imm_val(string& imm, map<string, int>& labels) {
    int imm_val;
    // WHAT *********
    // NOTE: if imm is given as a label, it won't have a ":" at the end- add colon
    if (labels.count((imm + ':')) > 0) { // imm value is given as a label
        imm_val = labels[(imm + ':')];
    }
    else { // imm value is given as a register
        imm_val = stoi(imm);
    }
    return imm_val;
}

/*
    generate_machine_code(parsed_line, pc, labels)
    generates the machine code for a given vector containing the components of a single instruction in assembly language
    Parameters:
        parsed_line = vector containing the components of a single instruction in assembly language
        pc = program counter, given as an int by reference (so we modify the actual program counter)
        labels = a map containing labels and its corresponding value
 */
vector<int16_t> generate_machine_code(vector<string>& parsed_line, int16_t& pc, map<string, int>& labels) {
    // machine_code_is_label[0] will hold the machine code for the line of assembly language
    // machine_code_is_label[1] will hold 0 if the line is just labels
        // will hold 1 if the line contains instructions
    // begin with the assumption that parsed_line contains instructions
    vector<int16_t> machine_code_is_label{0, 0};
    // if last char in first item of parsed_line is a colon, we're dealing with labels
    if ((parsed_line[0]).back() == ':') {
        // don't increment PC if line is just labels
        // iterate thru parsed_line to see where label ends
        bool is_label;
        for(size_t i = 0; i < parsed_line.size(); i++) {
            if (labels.count(parsed_line[i]) > 0) { // label has been found
                is_label = true;
            }
            else {
                is_label = false;
            }
            if (is_label == true) {// FIX &*******
                // if line so far just contains a label, set machine_code[1] = 1
                machine_code_is_label[1] = 1;
            }
            else { // is_label == false
                // if we find line contains machine_code, set machine_code[1] = 0
                machine_code_is_label[1] = 0;
                // create new vector starting from instruction
                vector<string> new_parsed_line((parsed_line.begin() + i), parsed_line.end());
                // pass just instruction to generate_machine_code to find machine code
                machine_code_is_label[0] = (generate_machine_code(new_parsed_line, pc, labels))[0];
                break; // FIX
            }
        }
    }
    else if (parsed_line[0] == "ADD" || parsed_line[0] == "SUB" || parsed_line[0] == "OR" || parsed_line[0] == "AND" || parsed_line[0] == "SLT") {
        // index parsed_line to get desired register string
            // index register string to access second char (b/c register = $num)
        char regA = (parsed_line[2])[1];
        char regB = (parsed_line[3])[1];
        char regDst = (parsed_line[1])[1];
        // depending on opcode, final 4 bits of machine canguage varies
        int func;
        if (parsed_line[0] == "ADD") {
            // 3 lsb = 000
            func = 0;
        }
        else if (parsed_line[0] == "SUB") {
            // 3 lsb = 001
            func = 1;
        }
        else if (parsed_line[0] == "OR") {
            // 3 lsb = 010
            func = 2;
        }
        else if (parsed_line[0] == "AND") {
            // 3 lsb = 011
            func = 3;
        }
        else { // parsed_line[0] == "SLT"
            // 3 lsb = 100
            func = 4;
        }
        machine_code_is_label[0] = three_register_arguments(regA, regB, regDst, func);
        pc += 1;
    }
    else if (parsed_line[0] == "JR") {
        char reg = (parsed_line[1])[1];
        char filler = '0';
        int func = 8;
        machine_code_is_label[0] = three_register_arguments(reg, filler, filler, func);
        pc += 1;
    }
    else if (parsed_line[0] == "SLTI" || parsed_line[0] == "ADDI") {
        char regSrc = (parsed_line[2])[1];
        char regDst = (parsed_line[1])[1];
        int imm = find_imm_val(parsed_line[3], labels);
        int opc;
        if (parsed_line[0] == "SLTI") {
            // 3 msb = 111
            opc = 7;
        }
        else { // parsed_line[0] == "ADDI"
            // 3 msb = 001
            opc = 1;
        }
        machine_code_is_label[0] = two_register_arguments(opc, regSrc, regDst, imm);
        pc += 1;
    }
    else if (parsed_line[0] == "LW" || parsed_line[0] == "SW") {
        char regAddr = (parsed_line[3])[1];
        char regDst = (parsed_line[1])[1];
        int imm = find_imm_val(parsed_line[2], labels);
        int opc;
        if (parsed_line[0] == "LW") {
            // 3 msb = 100
            opc = 4;
        }
        else { // parsed_line[0] == "SW"
            // 3 msb = 101
            opc = 5;
        }
        machine_code_is_label[0] = two_register_arguments(opc, regAddr, regDst, imm);
        pc += 1;
    }
    else if (parsed_line[0] == "JEQ") {
        char regA = (parsed_line[1])[1];
        char regB = (parsed_line[2])[1];
        int imm = find_imm_val(parsed_line[3], labels);
        int rel_imm = imm - pc - 1;
        // 3 msb = 110
        int opc = 6;
        machine_code_is_label[0] = two_register_arguments(opc, regA, regB, rel_imm);
        pc += 1;
    }
    else if (parsed_line[0] == "J" || parsed_line[0] == "JAL") {
        int imm = find_imm_val(parsed_line[1], labels);
        int opc;
        if (parsed_line[0] == "J") {
            // 3 msb = 010
            opc = 2;
        }
        else { // parsed_line[0] == "JAL"
            // 3 msb = 011
            opc = 3;
        }
        machine_code_is_label[0] = no_register_arguments(opc, imm);
        pc += 1;
    }
    else if (parsed_line[0] == "MOVI") {
        // movi $reg, imm == addi $reg, $0, imm
        char regSrc = '0';
        char regDst = (parsed_line[1])[1];
        int imm = find_imm_val(parsed_line[2], labels);
        // 3 msb = 001
        int opc = 1;
        machine_code_is_label[0] = two_register_arguments(opc, regSrc, regDst, imm);
        pc += 1;
    }
    else if (parsed_line[0] == "NOP") {
        // nop == add $0, $0, $0
        char reg = '0';
        // 3 lsb = 000
        int func = 0;
        machine_code_is_label[0] = three_register_arguments(reg, reg, reg, func);
        pc += 1;
    }
    else if (parsed_line[0] == "HALT") {
        // halt == j pc
            // 3 msb = 2
        machine_code_is_label[0] = no_register_arguments(2, pc);
        pc += 1;
    }
    else { // parsed_line[0] == ".FILL"
        int val = find_imm_val(parsed_line[1], labels);
        machine_code_is_label[0] = val;
        pc += 1;
    }
    return machine_code_is_label;
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
    // Display error message if appropriate
//    if (arg_error || do_help || filename == nullptr) {
//        cerr << "usage " << argv[0] << " [-h] filename" << endl << endl;
//        cerr << "Assemble E20 files into machine code" << endl << endl;
//        cerr << "positional arguments:" << endl;
//        cerr << "  filename    The file containing assembly language, typically with .s suffix" << endl<<endl;
//        cerr << "optional arguments:"<<endl;
//        cerr << "  -h, --help  show this help message and exit"<<endl;
//        return 1;
//    }
    
    /* go through file to look for all labels
        add them & their corresponding val to a vector */
    // open file
    ifstream f_label("hw7q1.txt");
    // check if that was valid
    if (!f_label.is_open()) {
        cerr << "Can't open file "<<filename<<endl;
        return 1;
    }
    map<string, int> labels;
    string line_label;
    // initialize program counter as 0
        // have to keep track of pc while reading file to give values for label
        // give pc_label type int16_t b/c E20's program counter is 16-bit
            // this enforces overflow when PC's value > 16 bit
    int16_t pc_label = 0;
    while (getline(f_label, line_label)) {
        // getting rid of comments
        size_t pos = line_label.find("#");
        if (pos != string::npos) {
            line_label = line_label.substr(0, pos);
        }
        make_line_upper(line_label);
        if (line_label.length() != 0) { // only consider none empty lines
            pc_label = get_label(line_label, labels, pc_label);
        }
    }
    // close file
    f_label.close();
    
    /* iterate through the line in the file, construct a list
       of numeric values representing machine code */
    // reopen file to process it again
    ifstream f("hw7q1.txt");
    // check if that was valid
    if (!f.is_open()) {
        cerr << "Can't open file "<<filename<<endl;
        return 1;
    }
    /* our final output is a list of ints values representing
       machine code instructions */
    vector<int16_t> instructions;
    string line;
    // initialize program counter as 0
        // have to keep track of pc while reading file for jeq
    int16_t pc = 0;
    // taking lines from file f, storing in line
    while (getline(f, line)) {
        // getting rid of comments
        size_t pos = line.find("#");
        if (pos != string::npos) {
            line = line.substr(0, pos);
        }
        make_line_upper(line);
        vector<string> parsed_line = split_line(line);
        // check if line is empty
        if (parsed_line.size() != 0) {
            // each line of machine code can be 16 bits
                // if machine_code_is_label[1] == 1, then this line corresponds to only a label
                // doesn't need to be added to instructions
            vector<int16_t> machine_code_is_label = generate_machine_code(parsed_line, pc, labels);
            if (machine_code_is_label[1] != 1) {
                instructions.push_back(machine_code_is_label[0]);
            }
        }
    }
    f.close();



    /* print out each instruction in the required format */
    unsigned address = 0;
    for (unsigned instruction : instructions) {
        print_machine_code(address, instruction);
        address ++;
    }
    
    
    return 0;
}

//ra0Eequ6ucie6Jei0koh6phishohm9
