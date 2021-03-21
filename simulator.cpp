/*input
fast 10 2
*/
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <iomanip>
#include <regex>
using namespace std;

vector<vector<int>> memory(1024, vector<int>(1024));
map<string, int> registers;
vector<vector<string>> instructions;
map<string, int> labels;
set<string> commands = {"add", "sub", "mul", "beq", "bne", "slt", "j", "lw", "sw", "addi"};
vector<string> regs = {"$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$s8", "$ra"};
map<int, int> line_map; // Instruction number to line number
map<int, int> count_map;

vector<int> buffer(1024, -1);
int buffer_ind = -1;
int buffer_updates = 0;
bool loading = 0, storing = 0;
string lr = "";
int cycle = 1;
int row_delay, col_delay;
int loading_cycle = 0;
int output_size = 0;
string mode;

vector<vector<string>> output;
vector<vector<string>> temp_output;

void print(vector<vector<string>> v){
    for(auto u:v){
        cout << left << setw(15) << setfill(' ') << u[0];
        cout << left << setw(25) << setfill(' ') << u[1];
        cout << left << setw(20) << setfill(' ') << u[2];
        cout << left << setw(35) << setfill(' ') << u[3];
        cout << left << setw(15) << setfill(' ') << u[4];
        cout<<endl;
    }
    cout<<endl;
}

string getstr(vector<string> v){
    string ans = v[0]+" ";
    for(int i=1;i<v.size();i++){
        ans += v[i];
    }
    return ans;
}

void compress(){
    if(output.size() == 0) return;
    vector<vector<string>> ans;
    vector<string> init = output[0];
    int ct = 0;
    for(int i=1;i<=output.size();i++){
        if(i < output.size() and (vector<string>){output[i][1], output[i][2], output[i][3], output[i][4]}  == (vector<string>){init[1], init[2], init[3], init[4]} and init[1][0] == 'D'){
            ct++;
            continue;
        }
        else{
            if(ct != 0) ans.push_back({init[0]+"-"+output[i-1][0].substr(6, output[i-1][0].size()), init[1], init[2], init[3], init[4]});
            else ans.push_back({init[0], init[1], init[2], init[3], init[4]});
            ct = 0;
            if(i != output.size()) init = output[i];
        }
    }
    output = ans;
}

void init_registers()
{
    for (string reg : regs)
    {
        registers[reg] = 0;
    }
    registers["$sp"] = 1048532; // stack pointer
    registers["$gp"] = 524286;  // global pointer
}

int load_word_from_buffer(int address)
{
    int n = 0;
    for (int i = 0; i < 4; i++)
    {
        n = n << 8;
        n += buffer[(address + i)%1024];
    }
    return n;
}
int get_word_from_memory(int address){
    int n = 0;
    for (int i = 0; i < 4; i++)
    {
        n = n << 8;
        n += memory[(address + i)/1024][(address + i)%1024];
    }
    return n;
}
void store_word_to_buffer(int n, int address)
{
    for (int i = 0; i < 4; i++)
    {
        buffer[(address + i)%1024] = ((n >> (24 - 8 * i)) & 0xFF);
    }
}

void tokenizer(string line, int line_num)
{
    int len = line.length();
    for(int i = 0; i < len; i++)
    {
        if(line[i] == '#') 
        {
            len = i;
            line = line.substr(0, len);
            // cout << "COMMENT: "<< line_num << " " << len << endl;
            break;
        }
    }
    
    vector<string> tokens;
    string curr_token = "";
    int i;
    bool label = false;
    for (i = 0; i < len; i++)
    {
        int start = i;
        int last = -1;
        bool token_start = false;
        for (; i < len; i++)
        {

            if (label && (line[i] != ' ' && line[i] != '\t'))
            {
                cout << "ERROR: Wrong syntax at line " << line_num << endl << endl;
                exit(3);
            }
            else if (line[i] == ':')
            {
                if ((tokens.size() == 0 && token_start || tokens.size() == 1 && !token_start))
                {
                    label = true;
                    token_start = false;
                    break;
                }
                else
                {
                    cout << "ERROR: Wrong syntax at line " << line_num << endl << endl;
                    exit(3);
                }
            }
            else if ((line[i] == ' ' || line[i] == '\t') && token_start)
            {
                token_start = false;
                break;
            }
            else if ((line[i] == ',' || line[i] == '(' || line[i] == ')') && token_start)
            {
                i--;
                token_start = false;
                break;
            }
            else if (line[i] == ',' || line[i] == '(' || line[i] == ')')
            {
                start = i;
                last = 1;
                token_start = false;
                break;
            }
            else if (line[i] == ' ' || line[i] == '\t')
            {
                continue;
            }
            else if (!token_start)
            {
                start = i;
                last = 1;
                token_start = true;
            }
            else
                last++;
        }
        if (token_start)
        {
            tokens.push_back(line.substr(start));
        }
        else if (last != -1)
            tokens.push_back(line.substr(start, last));
    }
    if (label)
    {
        if (labels.find(tokens[0]) != labels.end())
        {
            cout << "ERROR: Duplicate label definition at line " << line_num << endl << endl;
            exit(3);
        }
        regex b("[_a-zA-Z][a-zA-Z_0-9]*");
        if(regex_match(tokens[0].begin(), tokens[0].end(), b)){
            labels[tokens[0]] = instructions.size();
        }
        else{
            cout << "ERROR: Invalid label name at line " << line_num << endl << endl;
            exit(3);
        }
    }
    else if (tokens.size() != 0)
    {
        instructions.push_back(tokens);
    }

    if (!label)
    {
        line_map[instructions.size() - 1] = line_num;
    }
}

void preprocess1(vector<string> &instruction, int num)
{
    if (instruction.size() != 6)
    {
        cout << "ERROR: Wrong syntax at line " << line_map[num] << endl << endl;
        exit(3);
    }
    if (instruction[2] != "," or instruction[4] != ",")
    {
        cout << "ERROR: Wrong syntax at line " << line_map[num] << endl << endl;
        exit(3);
    }
}

void preprocess2(vector<string> &instruction, int num)
{
    if (instruction.size() != 4)
    {
        cout << "ERROR: Wrong syntax at line " << line_map[num] << endl << endl;
        exit(3);
    }
    if (instruction[2] != ",")
    {
        cout << "ERROR: Wrong syntax at line " << line_map[num] << endl << endl;
        exit(3);
    }
}

void preprocess3(vector<string> &instruction, int num)
{
    if (instruction.size() != 7)
    {
        cout << "ERROR: Wrong syntax at line " << line_map[num] << endl << endl;
        exit(3);
    }
    if (instruction[2] != "," || instruction[4] != "(" || instruction[6] != ")")
    {
        cout << "ERROR: Wrong syntax at line " << line_map[num] << endl << endl;
        exit(3);
    }
}

void getRegs(string &s)
{
    for (int i = 0; i < 32; i++)
    {
        if (s == "$" + to_string(i) or s == regs[i])
        {
            s = regs[i];
            return;
        }
    }
    s = "";
}

int isint(string var)
{

    if (var.size() > 7 or var.size() < 1)
        return 0;

    if (var[1] == 'x' || var[2] == 'x')
    {
        int idx = 0;
        if (var[0] == '-')
        {
            idx = 1;
        }
        if (var[idx] != '0')
            return 0;
        for (int i = idx + 2; i < var.size(); i++)
        {
            if (!isxdigit(var[i]))
                return 0;
        }
        int d = stoi(var, 0, 16);
        if (d < -(1 << 15) or d > (1 << 15) - 1)
            return 0;
        return 16;
    }
    else
    {
        if (!isdigit(var[0]) and var[0] != '-')
            return 0;
        for (int i = 1; i < var.size(); i++)
        {
            if (!isdigit(var[i]))
                return 0;
        }
        int d = stoi(var);
        if (d < -(1 << 15) or d > (1 << 15) - 1)
            return 0;
        return 10;
    }
}

void check_error(vector<string> &instruction, int num)
{
    if (commands.find(instruction[0]) == commands.end())
    {
        cout << "ERROR: Command \"" + instruction[0] + "\" not recognized at line " << line_map[num] << endl << endl;
        exit(3);
    }
    if (instruction[0] == "add")
    {
        preprocess1(instruction, num);
        string a = instruction[1], b = instruction[3], c = instruction[5];
        getRegs(a);
        getRegs(b);
        getRegs(c);
        if (a == "" or b == "" or c == "")
        {
            cout << "ERROR: Invalid register name(s) at line " << line_map[num] << endl << endl;
            exit(3);
        }
        if (a == "$at" or b == "$at" or c == "$at")
        {
            cout << "ERROR: Register $at is reserved for the assembler at line " << line_map[num] << endl << endl;
            exit(3);
        }
    }
    else if (instruction[0] == "sub")
    {
        preprocess1(instruction, num);
        string a = instruction[1], b = instruction[3], c = instruction[5];
        getRegs(a);
        getRegs(b);
        getRegs(c);
        if (a == "" or b == "" or c == "")
        {
            cout << "ERROR: Invalid register name(s) at line " << line_map[num] << endl << endl;
            exit(3);
        }
        if (a == "$at" or b == "$at" or c == "$at")
        {
            cout << "ERROR: Register $at is reserved for the assembler at line " << line_map[num] << endl << endl;
            exit(3);
        }
    }
    else if (instruction[0] == "mul")
    {
        preprocess1(instruction, num);
        string a = instruction[1], b = instruction[3], c = instruction[5];
        getRegs(a);
        getRegs(b);
        getRegs(c);
        if (a == "" or b == "" or c == "")
        {
            cout << "ERROR: Invalid register name(s) at line " << line_map[num] << endl << endl;
            exit(3);
        }
        if (a == "$at" or b == "$at" or c == "$at")
        {
            cout << "ERROR: Register $at is reserved for the assembler at line " << line_map[num] << endl << endl;
            exit(3);
        }
    }
    else if (instruction[0] == "addi")
    {
        preprocess1(instruction, num);
        string a = instruction[1], b = instruction[3], c = instruction[5];
        getRegs(a);
        getRegs(b);
        if (a == "" or b == "")
        {
            cout << "ERROR: Invalid register name(s) at line " << line_map[num] << endl << endl;
            exit(3);
        }
        if (a == "$at" or b == "$at")
        {
            cout << "ERROR: Register $at is reserved for the assembler at line " << line_map[num] << endl << endl;
            exit(3);
        }
        if (!isint(c))
        {
            cout << "ERROR: Invalid integer at line " << line_map[num] << endl << endl;
            exit(3);
        }
    }
    else if (instruction[0] == "beq")
    {
        preprocess1(instruction, num);
        string a = instruction[1], b = instruction[3], c = instruction[5];
        getRegs(a);
        getRegs(b);
        if (a == "" or b == "")
        {
            cout << "ERROR: Invalid register name(s) at line " << line_map[num] << endl << endl;
            exit(3);
        }
        if (a == "$at" or b == "$at")
        {
            cout << "ERROR: Register $at is reserved for the assembler at line " << line_map[num] << endl << endl;
            exit(3);
        }
        if (labels.find(c) == labels.end())
        {
            cout << "ERROR: Invalid label name at line " << line_map[num] << endl << endl;
            exit(3);
        }
    }
    else if (instruction[0] == "bne")
    {
        preprocess1(instruction, num);
        string a = instruction[1], b = instruction[3], c = instruction[5];
        getRegs(a);
        getRegs(b);
        if (a == "" or b == "")
        {
            cout << "ERROR: Invalid register name(s) at line " << line_map[num] << endl << endl;
            exit(3);
        }
        if (a == "$at" or b == "$at")
        {
            cout << "ERROR: Register $at is reserved for the assembler at line " << line_map[num] << endl << endl;
            exit(3);
        }
        if (labels.find(c) == labels.end())
        {
            cout << "ERROR: Invalid label name at line " << line_map[num] << endl << endl;
            exit(3);
        }
    }
    else if (instruction[0] == "slt")
    {
        preprocess1(instruction, num);
        string a = instruction[1], b = instruction[3], c = instruction[5];
        getRegs(a);
        getRegs(b);
        getRegs(c);
        if (a == "" or b == "" or c == "")
        {
            cout << "ERROR: Invalid register name(s) at line " << line_map[num] << endl << endl;
            exit(3);
        }
        if (a == "$at" or b == "$at" or c == "$at")
        {
            cout << "ERROR: Register $at is reserved for the assembler at line " << line_map[num] << endl << endl;
            exit(3);
        }
    }
    else if (instruction[0] == "j")
    {
        if (instruction.size() != 2)
        {
            cout << "ERROR: Wrong syntax at line " << line_map[num] << endl << endl;
            exit(3);
        }
        if (labels.find(instruction[1]) == labels.end())
        {
            cout << "ERROR: Invalid label name at line " << line_map[num] << endl << endl;
            exit(3);
        }
    }
    else if (instruction[0] == "lw")
    {
        preprocess3(instruction, num);
        string a = instruction[1], b = instruction[5], c = instruction[3];
        getRegs(a);
        getRegs(b);
        if (a == "" or b == "")
        {
            cout << "ERROR: Invalid register name(s) at line " << line_map[num] << endl << endl;
            exit(3);
        }
        if (a == "$at" or b == "$at")
        {
            cout << "ERROR: Register $at is reserved for the assembler at line " << line_map[num] << endl << endl;
            exit(3);
        }
        if (!isint(c))
        {
            cout << "ERROR: Invalid integer at line " << line_map[num] << endl << endl;
            exit(3);
        }
    }
    else if (instruction[0] == "sw")
    {
        preprocess3(instruction, num);
        string a = instruction[1], b = instruction[5], c = instruction[3];
        getRegs(a);
        getRegs(b);
        if (a == "" or b == "")
        {
            cout << "ERROR: Invalid register name(s) at line " << line_map[num] << endl << endl;
            exit(3);
        }
        if (a == "$at" or b == "$at")
        {
            cout << "ERROR: Register $at is reserved for the assembler at line " << line_map[num] << endl << endl;
            exit(3);
        }
        if (!isint(c))
        {
            cout << "ERROR: Invalid integer at line " << line_map[num] << endl << endl;
            exit(3);
        }
    }
}

int process(vector<string> &instruction, int num)
{
    int next_instruction;
    if (instruction[0] == "add")
    {
        string a = instruction[1], b = instruction[3], c = instruction[5];
        getRegs(a);
        getRegs(b);
        getRegs(c);
        if (a != "$zero")
            registers[a] = registers[b] + registers[c];
        if(loading == true and (a == lr or b == lr or c == lr)){
            output_size += temp_output.size();
            // compress();
            for(auto u:temp_output){
                output.push_back(u);
            }
            cycle = output.size()+1;
            temp_output.clear();
            loading_cycle = 0;
            output_size++;
            loading = false;
            output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), a+" = "+to_string(registers[a]), "-", "-"});
        }
        else if(loading == true or storing == true){
            vector<string> init = temp_output[loading_cycle];
            temp_output[loading_cycle] = {init[0], getstr(instructions[num]), a+" = "+to_string(registers[a]), init[3], init[4]};
            loading_cycle++;
        }
        else{
            output_size++;
            output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), a+" = "+to_string(registers[a]), "-", "-"});
        }
        next_instruction = num + 1;
    }
    else if (instruction[0] == "sub")
    {

        string a = instruction[1], b = instruction[3], c = instruction[5];
        getRegs(a);
        getRegs(b);
        getRegs(c);
        if (a != "$zero")
            registers[a] = registers[b] - registers[c];
        if(loading == true and (a == lr or b == lr or c == lr)){
            output_size += temp_output.size();
            // compress();
            for(auto u:temp_output){
                output.push_back(u);
            }
            cycle = output.size()+1;
            temp_output.clear();
            loading_cycle = 0;
            output_size++;
            loading = false;
            output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), a+" = "+to_string(registers[a]), "-", "-"});
        }
        else if(loading == true or storing == true){
            vector<string> init = temp_output[loading_cycle];
            temp_output[loading_cycle] = {init[0], getstr(instructions[num]), a+" = "+to_string(registers[a]), init[3], init[4]};
            loading_cycle++;
        }
        else{
            output_size++;
            output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), a+" = "+to_string(registers[a]), "-", "-"});
        }
        next_instruction = num + 1;
    }
    else if (instruction[0] == "mul")
    {
        string a = instruction[1], b = instruction[3], c = instruction[5];
        getRegs(a);
        getRegs(b);
        getRegs(c);
        if (a != "$zero")
            registers[a] = registers[b] * registers[c];
        if(loading == true and (a == lr or b == lr or c == lr)){
            output_size += temp_output.size();
            // compress();
            for(auto u:temp_output){
                output.push_back(u);
            }
            cycle = output.size()+1;
            temp_output.clear();
            loading_cycle = 0;
            output_size++;
            loading = false;
            output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), a+" = "+to_string(registers[a]), "-", "-"});
        }
        else if(loading == true or storing == true){
            vector<string> init = temp_output[loading_cycle];
            temp_output[loading_cycle] = {init[0], getstr(instructions[num]), a+" = "+to_string(registers[a]), init[3], init[4]};
            loading_cycle++;
        }
        else{
            output_size++;
            output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), a+" = "+to_string(registers[a]), "-", "-"});
        }
        next_instruction = num + 1;
    }
    else if (instruction[0] == "addi")
    {
        string a = instruction[1], b = instruction[3], c = instruction[5];
        getRegs(a);
        getRegs(b);
        if (a != "$zero")
            registers[a] = registers[b] + stoi(c, 0, isint(c));
        if(loading == true and (a == lr or b == lr)){
            output_size += temp_output.size();
            // compress();
            for(auto u:temp_output){
                output.push_back(u);
            }
            cycle = output.size()+1;
            temp_output.clear();
            loading_cycle = 0;
            output_size++;
            loading = false;
            output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), a+" = "+to_string(registers[a]), "-", "-"});
        }
        else if(loading == true or storing == true){
            vector<string> init = temp_output[loading_cycle];
            temp_output[loading_cycle] = {init[0], getstr(instructions[num]), a+" = "+to_string(registers[a]), init[3], init[4]};
            loading_cycle++;
        }
        else{
            output_size++;
            output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), a+" = "+to_string(registers[a]), "-", "-"});
        }
        next_instruction = num + 1;
    }
    else if (instruction[0] == "beq")
    {
        string a = instruction[1], b = instruction[3], c = instruction[5];
        getRegs(a);
        getRegs(b);
        if(loading == true and (a == lr or b == lr)){
            output_size += temp_output.size();
            // compress();
            for(auto u:temp_output){
                output.push_back(u);
            }
            cycle = output.size()+1;
            temp_output.clear();
            loading_cycle = 0;
            output_size++;
            loading = false;
            output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), "-", "-", "-"});
        }
        else if(loading == true or storing == true){
            vector<string> init = temp_output[loading_cycle];
            temp_output[loading_cycle] = {init[0], getstr(instructions[num]), init[2], init[3], init[4]};
            loading_cycle++;
        }
        else{
            output_size++;
            output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), "-", "-", "-"});
        }
        if (registers[a] == registers[b])
            next_instruction = labels[c];
        else
            next_instruction = num + 1;
    }
    else if (instruction[0] == "bne")
    {
        string a = instruction[1], b = instruction[3], c = instruction[5];
        getRegs(a);
        getRegs(b);
        if(loading == true and (a == lr or b == lr)){
            output_size += temp_output.size();
            // compress();
            for(auto u:temp_output){
                output.push_back(u);
            }
            cycle = output.size()+1;
            temp_output.clear();
            loading_cycle = 0;
            output_size++;
            loading = false;
            output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), "-", "-", "-"});
        }
        else if(loading == true or storing == true){
            vector<string> init = temp_output[loading_cycle];
            temp_output[loading_cycle] = {init[0], getstr(instructions[num]), init[2], init[3], init[4]};
            loading_cycle++;
        }
        else{
            output_size++;
            output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), "-", "-", "-"});
        }
        if (registers[a] != registers[b])
            next_instruction = labels[c];
        else
            next_instruction = num + 1;
    }
    else if (instruction[0] == "slt")
    {
        string a = instruction[1], b = instruction[3], c = instruction[5];
        getRegs(a);
        getRegs(b);
        getRegs(c);
        if (registers[b] < registers[c])
            registers[a] = 1;
        else
            registers[a] = 0;
        if(loading == true and (a == lr or b == lr or c == lr)){
            output_size += temp_output.size();
            // compress();
            for(auto u:temp_output){
                output.push_back(u);
            }
            cycle = output.size()+1;
            temp_output.clear();
            loading_cycle = 0;
            output_size++;
            loading = false;
            output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), a+" = "+to_string(registers[a]), "-", "-"});
        }
        else if(loading == true or storing == true){
            vector<string> init = temp_output[loading_cycle];
            temp_output[loading_cycle] = {init[0], getstr(instructions[num]), a+" = "+to_string(registers[a]), init[3], init[4]};
            loading_cycle++;
        }
        else{
            output_size++;
            output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), a+" = "+to_string(registers[a]), "-", "-"});
        }
        next_instruction = num + 1;
    }
    else if (instruction[0] == "j")
    {
        if(loading == true or storing == true){
            vector<string> init = temp_output[loading_cycle];
            temp_output[loading_cycle] = {init[0], getstr(instructions[num]), init[2], init[3], init[4]};
            loading_cycle++;
        }
        else{
            output_size++;
            output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), "-", "-", "-"});
        }
        next_instruction = labels[instruction[1]];
    }
    else if (instruction[0] == "lw")
    {
        string a = instruction[1], b = instruction[5], c = instruction[3];
        getRegs(a);
        getRegs(b);
        int memory_access_index = registers[b] + stoi(c, 0, isint(c));
        int row = memory_access_index / 1024;
        if (memory_access_index >= 1048576 or memory_access_index < 0)
        {
            cout << "ERROR: Invalid memory address access attempt at line " << line_map[num] << endl << endl;
            cycle--;
        }
        else if (memory_access_index % 4 != 0)
        {
            cout << "ERROR: Unaligned memory address access attempt at line " << line_map[num] << endl << endl;
            cycle--;
        }
        else
        {
            if(loading == true or storing == true){
                output_size += temp_output.size();
                // compress();
                for(auto u:temp_output){
                    output.push_back(u);
                }
                cycle = output.size()+1;
                temp_output.clear();
                loading_cycle = 0;
            }
            storing = false;
            loading = true;
            lr = a;
            output_size++;
            if(buffer_ind == -1){
                buffer_updates++;
                for(int i=0;i<row_delay;i++){
                    temp_output.push_back({"cycle "+to_string(i+cycle+1), "DRAM: Activate row "+to_string(row), "-", "-", "Activate row "+to_string(row)});
                }
                buffer = memory[row];
                if(a != "$zero") registers[a] = load_word_from_buffer(memory_access_index);
                if(col_delay == 0 and row_delay != 0){
                    temp_output[temp_output.size()-1][3] = a+" = "+to_string(registers[a]);
                }
                for(int i=0;i<col_delay;i++){
                    temp_output.push_back({"cycle "+to_string(i+cycle+row_delay+1), "DRAM: Column access", "-", a+" = "+to_string(registers[a]), "Column access"});
                }
            }
            else if(buffer_ind != row){
                buffer_updates++;
                for(int i=0;i<row_delay;i++){
                    temp_output.push_back({"cycle "+to_string(i+cycle+1), "DRAM: Writeback row "+to_string(buffer_ind), "-", "-", "Writeback row "+to_string(buffer_ind)});
                }
                memory[buffer_ind] = buffer;
                for(int i=0;i<row_delay;i++){
                    temp_output.push_back({"cycle "+to_string(i+cycle+row_delay+1), "DRAM: Activate row "+to_string(row), "-", "-", "Activate row "+to_string(row)});
                }
                buffer = memory[row];
                if(a != "$zero") registers[a] = load_word_from_buffer(memory_access_index);
                if(col_delay == 0 and row_delay != 0){
                    temp_output[temp_output.size()-1][3] = a+" = "+to_string(registers[a]);
                }
                for(int i=0;i<col_delay;i++){
                    temp_output.push_back({"cycle "+to_string(i+cycle+2*row_delay+1), "DRAM: Column access", "-", a+" = "+to_string(registers[a]), "Column access"});
                }
            }
            else{
                if(a != "$zero") registers[a] = load_word_from_buffer(memory_access_index);
                for(int i=0;i<col_delay;i++){
                    temp_output.push_back({"cycle "+to_string(i+cycle+1), "DRAM: Column access", "-", a+" = "+to_string(registers[a]), "Column access"});
                }
            }
            if((col_delay != 0 or row_delay != 0) and buffer_ind != row)
                output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), "-", "-", "DRAM request issued"});
            else if(row_delay == 0 and col_delay == 0)
                output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), "-", a+" = "+to_string(registers[a]), "DRAM request processed"});
            else
                output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), "-", "-", "DRAM request issued"});
            buffer_ind = row;
            if(mode == "slow"){
                for(auto u:temp_output){
                    output.push_back(u);
                }
                cycle = output.size();
                temp_output.clear();
                loading = false;
                storing = false;
                loading_cycle = 0;
            }
        }
        next_instruction = num + 1;
    }
    else if (instruction[0] == "sw")
    {
        storing = true;
        string a = instruction[1], b = instruction[5], c = instruction[3];
        getRegs(a);
        getRegs(b);
        int memory_access_index = registers[b] + stoi(c, 0, isint(c));
        int row = memory_access_index / 1024;
        if (memory_access_index >= 1048576 or memory_access_index < 0)
        {
            cout << "ERROR: Invalid memory address access attempt at line " << line_map[num] << endl << endl;
            cycle--;
        }
        else if (memory_access_index % 4 != 0)
        {
            cout << "ERROR: Unaligned memory address access attempt at line " << line_map[num] << endl << endl;
            cycle--;
        }
        else
        {
            if(loading == true or storing == true){
                for(auto u:temp_output){
                    output.push_back(u);
                }
                cycle = output.size()+1;
                temp_output.clear();
                loading_cycle = 0;
            }
            storing = true;
            loading = false;
            lr = a;
            if(buffer_ind == -1){
                buffer_updates+=2;
                for(int i=0;i<row_delay;i++){
                    temp_output.push_back({"cycle "+to_string(i+cycle+1), "DRAM: Activate row "+to_string(row), "-", "-", "Activate row "+to_string(row)});
                }
                buffer = memory[row];
                store_word_to_buffer(registers[a], memory_access_index);
                if(col_delay == 0 and row_delay !=0){
                    temp_output[temp_output.size()-1][3] = "memory "+to_string(memory_access_index)+"-"+to_string(memory_access_index+3)+" = "+to_string(registers[a]);
                }
                for(int i=0;i<col_delay;i++){
                    temp_output.push_back({"cycle "+to_string(i+cycle+row_delay+1), "DRAM: Column access", "-", "memory "+to_string(memory_access_index)+"-"+to_string(memory_access_index+3)+" = "+to_string(registers[a]), "Column access"});
                }
            }
            else if(buffer_ind != row){
                buffer_updates+=2;
                for(int i=0;i<row_delay;i++){
                    temp_output.push_back({"cycle "+to_string(i+cycle+1), "DRAM: Writeback row "+to_string(buffer_ind), "-", "-", "Writeback row "+to_string(buffer_ind)});
                }
                memory[buffer_ind] = buffer;
                for(int i=0;i<row_delay;i++){
                    temp_output.push_back({"cycle "+to_string(i+cycle+row_delay+1), "DRAM: Activate row "+to_string(row), "-", "-", "Activate row "+to_string(row)});
                }
                buffer = memory[row];
                store_word_to_buffer(registers[a], memory_access_index);
                if(col_delay == 0 and row_delay != 0){
                    temp_output[temp_output.size()-1][3] = "memory "+to_string(memory_access_index)+"-"+to_string(memory_access_index+3)+" = "+to_string(registers[a]);
                }
                for(int i=0;i<col_delay;i++){
                    temp_output.push_back({"cycle "+to_string(i+cycle+2*row_delay+1), "DRAM: Column access", "-", "memory "+to_string(memory_access_index)+"-"+to_string(memory_access_index+3)+" = "+to_string(registers[a]), "Column access"});
                }
            }
            else{
                buffer_updates++;
                store_word_to_buffer(registers[a], memory_access_index);
                for(int i=0;i<col_delay;i++){
                    temp_output.push_back({"cycle "+to_string(i+cycle+1), "DRAM: Column access", "-", "memory "+to_string(memory_access_index)+"-"+to_string(memory_access_index+3)+" = "+to_string(registers[a]), "Column access"});
                }
            }
            if((col_delay != 0 or row_delay != 0) and buffer_ind != row)
                output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), "-", "-", "DRAM request issued"});
            else if(row_delay == 0 and col_delay == 0)
                output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), "-", "memory "+to_string(memory_access_index)+"-"+to_string(memory_access_index+3)+" = "+to_string(registers[a]), "DRAM request processed"});
            else
                output.push_back({"cycle "+to_string(cycle), getstr(instructions[num]), "-", "-", "DRAM request issued"});
            buffer_ind = row;
            if(mode == "slow"){
                for(auto u:temp_output){
                    output.push_back(u);
                }
                cycle = output.size();
                temp_output.clear();
                loading = false;
                storing = false;
                loading_cycle = 0;
            }
        }
        next_instruction = num + 1;
    }

    return next_instruction;
}

void printRegisters()
{
    for (int i = 0; i < 32; i++)
    {
        string register_name = regs[i];
        if (i == 0)
            register_name = "$r0";
        cout << left << setw(3) << setfill(' ') << register_name << ": " << setw(12) << setfill(' ') << dec << registers[regs[i]];
        if (i % 4 == 3)
            cout << endl;
        else if (i != 31)
            cout << "   ";
    }
    cout << "------------------------------------------------------";
}

int main()
{
    init_registers();

    string file_name = "test.asm";
    // cout<<"Enter file name: ";
    // cin >> file_name;

    cin >> mode >> row_delay >> col_delay;

    ifstream fin("test/"+file_name);
    string line;
    int line_num = 0;
    while (getline(fin, line))
    {
        tokenizer(line, ++line_num);
    }
    for (int i = 0; i < instructions.size(); i++)
    {
        check_error(instructions[i], i);
    }
    int i = 0;
    int x = 0;
    while (i < instructions.size())
    {
        count_map[i]++;

        i = process(instructions[i], i);
        if(loading_cycle == temp_output.size() and (loading == true or storing == true)){
            for(auto u:temp_output){
                output.push_back(u);
            }
            cycle = output.size()+1;
            temp_output.clear();
            loading = false;
            storing = false;
            loading_cycle = 0;
        }
        else{
            cycle++;
        }
        // if(cycle-1 > 10000){
        //     cout << "Time limit exceeded. Aborting..." << endl << endl;
        //     exit(3);
        // }
    }
    for(auto u:temp_output){
        output.push_back(u);
    }
    cycle = output.size()+1;
    temp_output.clear();

    cout << "Total number of cycles: " << dec << output.size() << endl;
    cout << "Number of instructions: " << dec << instructions.size() << endl;
    cout << "Total number of row buffer updates: " << dec << buffer_updates << endl << endl;
    cout << "Memory content at the end of the execution: " << endl << endl;
    memory[buffer_ind] = buffer;
    for(int i=0;i<1024*1024;i += 4){
        if(get_word_from_memory(i) != 0){
            cout << i << "-" << i+3 << ": " << get_word_from_memory(i) << endl;
        }
    }
    cout << endl << "Every cycle description" << endl << endl;
    cout << left << setw(15) << setfill(' ') << "Cycle Number"<< left << setw(25) << setfill(' ') << "Instruction"<< left << setw(20) << setfill(' ') << "Registers change"<< left << setw(35) << setfill(' ') << "DRAM change"<< left << setw(15) << setfill(' ') << "DRAM Remarks" << endl << endl;
    compress();
    print(output);
    cout << "Every instruction description" << endl << endl;
    for (int i = 0; i < instructions.size(); i++)
    {
        string inst_acc = "";
        cout << "Instruction " << setw(3) << setfill(' ') << i + 1 << ": ";
        inst_acc = instructions[i][0] + " ";
        for (int j = 1; j < instructions[i].size(); j++)
        {
            inst_acc += instructions[i][j];
        }
        cout << left << setw(35) << setfill(' ') << inst_acc << "Number of executions : " << count_map[i] << endl;
    }
    cout << endl;
}