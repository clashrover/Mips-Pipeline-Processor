#include <bits/stdc++.h>
using namespace std;

struct ifidPipe{
	int pc;
	bool nop;
};

struct idexPipe{
	int wb;
	int m1; int m2;
	int ex1; int ex2;
	int pc;
	int rd1;
	int rd2;
	int wb_reg;
	bool nop;
	bool reg_write;
	int rs_no;
	int rt_no;
};

struct exmemPipe{
	int wb;
	int m1; int m2;
	int jump_result;
	int zero;
	int alu_result;
	int write_data;
	int wb_reg;
	bool nop;
	bool reg_write;
};

struct memwbPipe{
	int wb;
	int read_data;
	int alu_result;
	int wb_reg;
	bool reg_write;
	bool nop;
	int m1;
	int m2;
};

int main(){
	map<string,int> reg_map;
	reg_map.insert({"$zero", 0});	
	reg_map.insert({"$at",1});
	reg_map.insert({"$v0",2});
	reg_map.insert({"$v1",3});
	reg_map.insert({"$a0",4});
	reg_map.insert({"$a1",5});
	reg_map.insert({"$a2",6});
	reg_map.insert({"$a3",7});
	reg_map.insert({"$t0",8});
	reg_map.insert({"$t1",9});
	reg_map.insert({"$t2",10});
	reg_map.insert({"$t3",11});
	reg_map.insert({"$t4",12});
	reg_map.insert({"$t5",13});
	reg_map.insert({"$t6",14});
	reg_map.insert({"$t7",15});
	reg_map.insert({"$s0",16});
	reg_map.insert({"$s1",17});
	reg_map.insert({"$s2",18});
	reg_map.insert({"$s3",19});
	reg_map.insert({"$s4",20});
	reg_map.insert({"$s5",21});
	reg_map.insert({"$s6",22});
	reg_map.insert({"$s7",23});
	reg_map.insert({"$t8",24});
	reg_map.insert({"$t9",25});
	reg_map.insert({"$k0",26});
	reg_map.insert({"$k1",27});
	reg_map.insert({"$gp",28});
	reg_map.insert({"$sp",29});
	reg_map.insert({"$fp",30});
	reg_map.insert({"$ra",31});

	vector<vector<string>> inst_mem;
	map<string,int> lookup;
	vector<bitset<32>> reg_file;
	vector<bitset<32>> memory;
	bitset<32> a;
	for(int i=0;i<4096;i++){
		memory.push_back(a);	
	}
	for(int i=0;i<32;i++){
		reg_file.push_back(a);
	}
	reg_file[29] = bitset<32>(4096);

	// ifstream infile("show_branch.txt");
	// ifstream infile("input_show_stalls.txt");
	// ifstream infile("show_forward.txt");
	ifstream infile("show_stall_and_forward.txt");
	string line;
	int pc=0;
	while(getline(infile,line)){
		vector<string> v;
		istringstream iss(line);
		string word;
		while(iss>>word){
			v.push_back(word);
			//cout << word << " ";
		}
		// inst_mem.push_back(v);
		//cout << endl;
		if(v.size()==1){
			string s = v[0];
			s.pop_back();
			pair<string,int> p;
			p.first = s;
			p.second = pc;
			lookup.insert(p);
			// cout << pc+1 << endl;
		}else{
			inst_mem.push_back(v);
			pc++;
		}
	}
	//-------------------------------------------------------
	// lets create pipe line registers.
	struct ifidPipe* ifid_reg = (struct ifidPipe*)malloc(sizeof(struct ifidPipe));
	struct idexPipe* idex_reg = (struct idexPipe*)malloc(sizeof(struct idexPipe));
	struct exmemPipe* exmem_reg = (struct exmemPipe*)malloc(sizeof(struct exmemPipe));
	struct memwbPipe* memwb_reg = (struct memwbPipe*)malloc(sizeof(struct memwbPipe));
	
	// initiate structure fields
	ifid_reg->pc = -1;
	ifid_reg->nop = true;
	
	idex_reg->wb = 0;  // for chosing read data from memory or alu result. 
	idex_reg->m1 = 0;  // mem read
	idex_reg->m2 = 0;  // mem write
	idex_reg->ex1 = 0; //ex1 is for operating with offset
	idex_reg->ex2 = 0; //ex2 is for alu control 0 means add
	idex_reg->pc = 0;
	idex_reg-> rd1 = 0;	// read data 1
	idex_reg-> rd2 = 0; // read data 2 
	idex_reg -> wb_reg = 0;			// write back register number
	idex_reg -> nop = true;			// initially all stages have to non operational
	idex_reg -> reg_write = false;	// would we be writing to reg_file?
	idex_reg -> rs_no = -1;			// read reg no 1
	idex_reg -> rt_no = -1;			// read reg no 2

	exmem_reg->wb = 0;				// transfer the control from prev register
	exmem_reg->m1 = 0;
	exmem_reg->m2 = 0;
	exmem_reg->jump_result = 0;		// no need for now
	exmem_reg->zero = 0;			// no need for now
	exmem_reg-> alu_result = 0;		// for storing alu result
	exmem_reg->write_data = 0;		// data to be stored in memory
	exmem_reg->wb_reg = 0;			// from prev register
	exmem_reg->wb = 0;				// from prev reg
	exmem_reg -> nop = true;		
	exmem_reg -> reg_write = false; // from prev reg

	memwb_reg -> wb = 0;			
	memwb_reg -> read_data = 0;
	memwb_reg -> alu_result = 0;
	memwb_reg -> wb_reg = 0;
	// memwb_reg -> reg_write=0;
	memwb_reg -> nop = true;
	memwb_reg -> reg_write = false;
	memwb_reg -> m1 = 0;
	memwb_reg -> m2 = 0;
	//------------------------------------------------------------
	
	int stall_type = 0;  // no stall, 1 cycle, 2 cycles  
	int stall_no;		 // at which stall are we

	int s = inst_mem.size();
	bool half_cycle = false; // half cycle has not been completed yet
	bool exe_stall = false;
	bool another_mem_stall=false;
	bool next_mem_stall = false;
	bool another_ex_stall = false;
	cout << "========================NEW CYCLE=======================" << endl;
	while((ifid_reg->pc) < s+3){
		if(half_cycle){
			half_cycle = false;
			// IF
			cout << "-----------------by the end of 2nd HALF CYCLE----------------" << endl;
			cout << "pc : "  << ifid_reg->pc + 1 << endl;
			struct ifidPipe* temp_ifid_reg = (struct ifidPipe*)malloc(sizeof(struct ifidPipe));
			temp_ifid_reg->pc = (ifid_reg -> pc) +1;
			temp_ifid_reg->nop = false;
			cout << "Inst Fetch" << endl;
			cout << "---------------" << endl;

			// ID
			struct idexPipe* temp_idex_reg = (struct idexPipe*)malloc(sizeof(struct idexPipe));
			temp_idex_reg->wb = idex_reg->wb;
			temp_idex_reg->m1 = idex_reg->m1;
			temp_idex_reg->m2 = idex_reg->m2;
			temp_idex_reg->ex1 = idex_reg->ex1;
			temp_idex_reg->ex2 = idex_reg->ex2;
			temp_idex_reg->pc = idex_reg->pc;
			temp_idex_reg-> rd1 = idex_reg->rd1;
			temp_idex_reg-> rd2 = idex_reg->rd2;
			temp_idex_reg -> wb_reg = idex_reg->wb_reg ;
			temp_idex_reg -> nop = idex_reg->nop;
			temp_idex_reg -> reg_write = idex_reg->reg_write;
			temp_idex_reg -> rs_no = idex_reg->rs_no;			// read reg no 1
			temp_idex_reg -> rt_no = idex_reg->rt_no;			// read reg no 2
			
			int stall_type = 0;  // no stall, 1 cylcle, 2 cycles  
			int stall_no;		 // at which stall are we
			if(!ifid_reg->nop){
				cout << "ID ";
				// cout << inst_mem[ifid_reg->pc][0] << endl;
				// cout << ifid_reg->pc << endl;
				if(ifid_reg->pc >= s){
					cout << "NOP generated" << endl;
					temp_idex_reg->nop = true;
				}else if(inst_mem[ifid_reg->pc][0]=="nop"){
					cout << "stall" << endl;
					temp_idex_reg -> nop = true;
				}
				else if(inst_mem[ifid_reg->pc][0] == "beq"){
					cout << "branch : ";
					temp_idex_reg->wb = 0;  // for chosing read data from memory or alu result. 
					temp_idex_reg->m1 = 0;  // mem read
					temp_idex_reg->m2 = 0;  // mem write
					temp_idex_reg->ex1 = 0; //ex1 is for operating with offset
					temp_idex_reg->ex2 = 0; //ex2 is for alu control 0 means add
					temp_idex_reg->pc = 0;
					temp_idex_reg-> rd1 = 0;	// read data 1
					temp_idex_reg-> rd2 = 0; // read data 2 
					temp_idex_reg -> wb_reg = 0;			// write back register number
					temp_idex_reg -> nop = true;			// initially all stages have to non operational
					temp_idex_reg -> reg_write = false;	// would we be writing to reg_file?
					temp_idex_reg -> rs_no = -1;			// No rellevance
					temp_idex_reg -> rt_no = -1;			// No relevance

					string rds = inst_mem[ifid_reg->pc][1];
					rds.pop_back();
					string rss = inst_mem[ifid_reg->pc][2];
					rss.pop_back();
					string offset = inst_mem[ifid_reg->pc][3];
					map<string,int>::iterator p = reg_map.find(rds);
					int rd = p->second;
					p = reg_map.find(rss);
					int rs = p->second;
					p = lookup.find(offset);
					int off = p->second;

					if((idex_reg->wb_reg == rs || idex_reg->wb_reg == rd) && idex_reg->reg_write){
						cout << "exe stall generated" << endl;
						temp_idex_reg->nop = true; 
						temp_ifid_reg->pc = (temp_ifid_reg -> pc) -1;

						another_ex_stall = true;
						exe_stall = true;
					}else if(another_ex_stall){
						cout << "exe stall generated" << endl;
						temp_idex_reg->nop = true; 
						temp_ifid_reg->pc = (temp_ifid_reg -> pc) -1;

						another_ex_stall = false;
						exe_stall = false;
					}
					else if((exmem_reg->wb_reg == rs || exmem_reg->wb_reg == rd )&& exmem_reg->reg_write && !exe_stall){
						cout << "mem stall generated" << endl;
						temp_idex_reg->nop = true; 
						temp_ifid_reg->pc = (temp_ifid_reg -> pc) -1;

						// temp_idex_reg -> nop = true;			// initially all stages have to non operational
						another_ex_stall = false;
						exe_stall = false;
					}
					else{
						if(reg_file[rd]==reg_file[rs]){
							temp_ifid_reg->pc=off; 
							cout << "taken" << endl;
						}else{
							cout << "not taken" << endl;
						}
					}
				}
				else if(inst_mem[ifid_reg->pc][0] == "add" || inst_mem[ifid_reg->pc][0] == "sub"){
					temp_idex_reg->wb = 1;
					temp_idex_reg->m1 = 0;
					temp_idex_reg->m2 = 0;
					temp_idex_reg->ex1 = 0;
					// temp_idex_reg->ex2 = 0; //ex2 is for alu control 0 means add
					temp_idex_reg->pc = ifid_reg->pc;
					temp_idex_reg -> nop = false;

					string rds = inst_mem[ifid_reg->pc][1];
					rds.pop_back();
					string rss = inst_mem[ifid_reg->pc][2];
					rss.pop_back();
					string rts = inst_mem[ifid_reg->pc][3];

					map<string,int>::iterator p = reg_map.find(rds);
					int rd = p->second;
					p = reg_map.find(rss);
					int rs = p->second;
					p = reg_map.find(rts);
					int rt = p->second;

					temp_idex_reg-> rd1 = (int)reg_file[rs].to_ulong();
					temp_idex_reg-> rd2 = (int)reg_file[rt].to_ulong();
					temp_idex_reg -> wb_reg = rd;
					temp_idex_reg -> reg_write = true;
					temp_idex_reg->nop = ifid_reg->nop;
					temp_idex_reg -> rs_no = rs;			// read reg no 1
					temp_idex_reg -> rt_no = rt;			// read reg no 2

					if (idex_reg->m1 == 1 && (idex_reg->wb_reg == rs || idex_reg->wb_reg == rt)){
						cout << "stall due to previous load   :" << endl;
						temp_idex_reg->nop = true; 
						temp_ifid_reg->pc = (temp_ifid_reg -> pc) -1;

						temp_idex_reg->wb = 0;  // for chosing read data from memory or alu result. 
						temp_idex_reg->m1 = 0;  // mem read
						temp_idex_reg->m2 = 0;  // mem write
						temp_idex_reg->ex1 = 0; //ex1 is for operating with offset
						temp_idex_reg->ex2 = 0; //ex2 is for alu control 0 means add
						temp_idex_reg->pc = 0;
						temp_idex_reg-> rd1 = 0;	// read data 1
						temp_idex_reg-> rd2 = 0; // read data 2 
						temp_idex_reg -> wb_reg = 0;			// write back register number
						// temp_idex_reg -> nop = true;			// initially all stages have to non operational
						temp_idex_reg -> reg_write = false;	// would we be writing to reg_file?
						// another_ex_stall = true;
						// exe_stall = true;
					}else{
						if (inst_mem[ifid_reg->pc][0] == "add"){
							cout << "add" << endl;
							temp_idex_reg->ex2 = 0;
							exe_stall = false;
						}
						if (inst_mem[ifid_reg->pc][0] == "sub"){
							cout << "sub" << endl;
							temp_idex_reg->ex2 = 1;
							exe_stall = false;
						}
					}

				}
				else if(inst_mem[ifid_reg->pc][0] == "addi" || inst_mem[ifid_reg->pc][0] == "sll" || inst_mem[ifid_reg->pc][0] == "srl"){
					temp_idex_reg->wb = 1;
					temp_idex_reg->m1 = 0;
					temp_idex_reg->m2 = 0;
					temp_idex_reg->ex1 = 1;
					temp_idex_reg->pc = ifid_reg->pc;

					string rts = inst_mem[ifid_reg->pc][1];
					rts.pop_back();
					string rss = inst_mem[ifid_reg->pc][2];
					rss.pop_back();
					string imm = inst_mem[ifid_reg->pc][3];
					int immediate = stoi(imm);
					map<string,int>::iterator p = reg_map.find(rts);
					int rt = p->second;
					p = reg_map.find(rss);
					int rs = p->second;

					temp_idex_reg-> rd1 = (int)reg_file[rs].to_ulong();
					temp_idex_reg-> rd2 = immediate;
					temp_idex_reg -> wb_reg = rt ;
					temp_idex_reg -> nop = ifid_reg->nop;
					temp_idex_reg -> reg_write = true;
					temp_idex_reg -> rs_no = rs;			// read reg no 1
					temp_idex_reg -> rt_no = -1;			// read reg no 2... not applicable here

					if (idex_reg->m1 == 1 && idex_reg->wb_reg == rs){
						cout << "stall due to previous load   :" << endl;
						temp_idex_reg->nop = true; 
						temp_ifid_reg->pc = (temp_ifid_reg -> pc) -1;

						temp_idex_reg->wb = 0;  // for chosing read data from memory or alu result. 
						temp_idex_reg->m1 = 0;  // mem read
						temp_idex_reg->m2 = 0;  // mem write
						temp_idex_reg->ex1 = 0; //ex1 is for operating with offset
						temp_idex_reg->ex2 = 0; //ex2 is for alu control 0 means add
						temp_idex_reg->pc = 0;
						temp_idex_reg-> rd1 = 0;	// read data 1
						temp_idex_reg-> rd2 = 0; // read data 2 
						temp_idex_reg -> wb_reg = 0;			// write back register number
						// temp_idex_reg -> nop = true;			// initially all stages have to non operational
						temp_idex_reg -> reg_write = false;	// would we be writing to reg_file?
						// another_ex_stall = true;
						// exe_stall = true;
					}else{
						if(inst_mem[ifid_reg->pc][0] == "addi"){
							temp_idex_reg->ex2 = 2;
							cout << "addi" << endl;
						}else if(inst_mem[ifid_reg->pc][0] == "sll"){
							temp_idex_reg-> ex2 = 3; //for sll
							cout << "sll" << endl;
						}else if(inst_mem[ifid_reg->pc][0] == "srl"){
							temp_idex_reg-> ex2 = 4; // for srl
							cout << "srl" << endl;
						}
					}
				}else if(inst_mem[ifid_reg->pc][0] == "lw" || inst_mem[ifid_reg->pc][0] == "sw"){
					
					string rts = inst_mem[ifid_reg-> pc][1];
					rts.pop_back();
					map<string,int>::iterator p = reg_map.find(rts);
					int rt = p->second;
					string offset = "";
					string base = "";
					bool x=false;
					for(int t=0;t<inst_mem[ifid_reg-> pc][2].size();t++){
						if(inst_mem[ifid_reg-> pc][2][t]=='('){
							x=true;
							continue;
						}
						if(x){
							if(inst_mem[ifid_reg-> pc][2][t]==')'){
								break;
							}
							base = base + inst_mem[ifid_reg-> pc][2][t];
						}else{
							offset = offset + inst_mem[ifid_reg-> pc][2][t];
						}
					}
					p = reg_map.find(base);
					int b = p->second;
					int ofs = stoi(offset);
					ofs = ofs >> 2;
					// cout << rt << " " << b << " " << ofs << endl;
					temp_idex_reg->wb = 0;
					temp_idex_reg->ex1 = 1;
					temp_idex_reg->pc = ifid_reg->pc;

					temp_idex_reg-> rd1 = (int)reg_file[b].to_ulong();
					temp_idex_reg-> rd2 = ofs;
					temp_idex_reg -> wb_reg = rt ;
					temp_idex_reg -> nop = false;

					temp_idex_reg -> rs_no = b;			// read reg no 1
					temp_idex_reg -> rt_no = -1;			// read reg no 2... not applicable here
	
					if (idex_reg->m1 == 1 && idex_reg->wb_reg == b){
						cout << "stall due to previous load   :" << endl;
						temp_idex_reg->nop = true; 
						temp_ifid_reg->pc = (temp_ifid_reg -> pc) -1;

						temp_idex_reg->wb = 0;  // for chosing read data from memory or alu result. 
						temp_idex_reg->m1 = 0;  // mem read
						temp_idex_reg->m2 = 0;  // mem write
						temp_idex_reg->ex1 = 0; //ex1 is for operating with offset
						temp_idex_reg->ex2 = 0; //ex2 is for alu control 0 means add
						temp_idex_reg->pc = 0;
						temp_idex_reg-> rd1 = 0;	// read data 1
						temp_idex_reg-> rd2 = 0; // read data 2 
						temp_idex_reg -> wb_reg = 0;			// write back register number
						// temp_idex_reg -> nop = true;			// initially all stages have to non operational
						temp_idex_reg -> reg_write = false;	// would we be writing to reg_file?
						// another_ex_stall = true;
						// exe_stall = true;
					}else{
						if(inst_mem[ifid_reg->pc][0] == "sw"){
							cout << "sw" << endl;
							temp_idex_reg->m1 = 0;
							temp_idex_reg->m2 = 1;
							temp_idex_reg -> reg_write = false;
							temp_idex_reg-> ex2 = 6; // for sw
						}else if( inst_mem[ifid_reg->pc][0] == "lw"){
							cout << "lw" << endl;
							temp_idex_reg->m1 = 1;
							temp_idex_reg->m2 = 0;
							temp_idex_reg -> reg_write = true;
							temp_idex_reg-> ex2 = 5; // for lw
						}
					}
				}
				ifid_reg->nop = true;
			}else{
				cout << "ID empty" << endl;
			}
			cout << "---------------" << endl;
			// EX
			struct exmemPipe* temp_exmem_reg = (struct exmemPipe*)malloc(sizeof(struct exmemPipe));
			// temp_exmem_reg->wb = exmem_reg->wb;
			// temp_exmem_reg->m1 = exmem_reg->m1;
			// temp_exmem_reg->m2 = exmem_reg->m2;
			temp_exmem_reg->jump_result = exmem_reg->jump_result;
			temp_exmem_reg->zero = exmem_reg->zero;
			temp_exmem_reg-> alu_result = exmem_reg->alu_result;
			temp_exmem_reg->write_data = exmem_reg->write_data;
			// temp_exmem_reg->wb_reg = exmem_reg->wb_reg;
			// temp_exmem_reg->wb = exmem_reg->wb;
			// temp_exmem_reg -> nop = exmem_reg->nop;
			// temp_exmem_reg -> reg_write = exmem_reg->reg_write;

			temp_exmem_reg->wb = idex_reg->wb;
			temp_exmem_reg->m1 = idex_reg->m1; //mem read
			temp_exmem_reg->m2 = idex_reg->m2; //mem write
			temp_exmem_reg->wb_reg = idex_reg->wb_reg;
			temp_exmem_reg -> nop = idex_reg->nop;
			temp_exmem_reg -> reg_write = idex_reg->reg_write;
			if(!idex_reg->nop){
				cout << "EXE  ";
				// temp_exmem_reg->wb = idex_reg->wb;
				// temp_exmem_reg->m1 = idex_reg->m1; //mem read
				// temp_exmem_reg->m2 = idex_reg->m2; //mem write
				// temp_exmem_reg->wb_reg = idex_reg->wb_reg;
				bool fwex = false; // for telling if you have forwareded from exmem pipeline regs
				if(idex_reg->ex2 <= 6){ // alu op of add
					// temp_exmem_reg->jump_result = 0;
					// temp_exmem_reg->zero = 0;
					// temp_exmem_reg->write_data = 0;
					// temp_exmem_reg -> nop = idex_reg->nop;
					// temp_exmem_reg -> reg_write = idex_reg->reg_write;
					if(idex_reg->ex2 == 0){
						if((idex_reg->rs_no == exmem_reg->wb_reg)&& exmem_reg->reg_write) {
							idex_reg->rd1 = exmem_reg->alu_result;
							cout << "  forwarding rs from exmem " << endl;
							fwex = true;
						}
						if((idex_reg->rt_no == exmem_reg->wb_reg) && exmem_reg->reg_write){
							idex_reg->rd2 = exmem_reg->alu_result;
							cout << "  forwarding rt from exmem " << endl;
							fwex=true;
						}
						if((idex_reg->rs_no == memwb_reg->wb_reg) && memwb_reg->reg_write && !fwex){
							if(memwb_reg->m1 == 1){
								idex_reg->rd1 = memwb_reg->read_data;
							}else{
								idex_reg->rd1 = memwb_reg->alu_result;
							}
							cout << "  forwarding rs from memwb " << endl;
						}
						if((idex_reg->rt_no == memwb_reg->wb_reg) && memwb_reg->reg_write && !fwex){
							if(memwb_reg->m1 == 1){
								idex_reg->rd2 = memwb_reg->read_data;
							}else{
								idex_reg->rd2 = memwb_reg->alu_result;
							}
							cout << "  forwarding rt from memwb " << endl;
						}
						fwex = false;
						temp_exmem_reg-> alu_result = idex_reg->rd1 + idex_reg->rd2;
						cout << "add: " <<  temp_exmem_reg->alu_result << endl;
					}else if(idex_reg->ex2 == 1){
						if((idex_reg->rs_no == exmem_reg->wb_reg)&& exmem_reg->reg_write) {
							idex_reg->rd1 = exmem_reg->alu_result;
							cout << "  forwarding rs from exmem " << endl;
							fwex = true;
						}
						if((idex_reg->rt_no == exmem_reg->wb_reg) && exmem_reg->reg_write){
							idex_reg->rd2 = exmem_reg->alu_result;
							cout << "  forwarding rt from exmem " << endl;
							fwex=true;
						}
						if((idex_reg->rs_no == memwb_reg->wb_reg) && memwb_reg->reg_write && !fwex){
							if(memwb_reg->m1 == 1){
								idex_reg->rd1 = memwb_reg->read_data;
							}else{
								idex_reg->rd1 = memwb_reg->alu_result;
							}
							cout << "  forwarding rs from memwb " << endl;
						}
						if((idex_reg->rt_no == memwb_reg->wb_reg) && memwb_reg->reg_write && !fwex){
							if(memwb_reg->m1 == 1){
								idex_reg->rd2 = memwb_reg->read_data;
							}else{
								idex_reg->rd2 = memwb_reg->alu_result;
							}
							cout << "  forwarding rt from memwb " << endl;
						}
						fwex=false;
						temp_exmem_reg-> alu_result = idex_reg->rd1 - idex_reg->rd2;
						cout << "sub: " <<  temp_exmem_reg->alu_result << endl;
					}else if(idex_reg->ex2 == 2){
						if((idex_reg->rs_no == exmem_reg->wb_reg)&& exmem_reg->reg_write) {
							idex_reg->rd1 = exmem_reg->alu_result;
							cout << "  forwarding rs from exmem " << endl;
							fwex = true;
						}
						if((idex_reg->rs_no == memwb_reg->wb_reg) && memwb_reg->reg_write && !fwex){
							if(memwb_reg->m1 == 1){
								idex_reg->rd1 = memwb_reg->read_data;
							}else{
								idex_reg->rd1 = memwb_reg->alu_result;
							}
							cout << "  forwarding rs from memwb " << endl;
						}
						fwex=false;
						temp_exmem_reg-> alu_result = idex_reg->rd1 + idex_reg->rd2;
						cout << "addi: " <<  temp_exmem_reg->alu_result << endl;
					}else if(idex_reg->ex2 == 3){
						if((idex_reg->rs_no == exmem_reg->wb_reg)&& exmem_reg->reg_write) {
							idex_reg->rd1 = exmem_reg->alu_result;
							cout << "  forwarding rs from exmem " << endl;
							fwex=true;
						}
						if((idex_reg->rs_no == memwb_reg->wb_reg) && memwb_reg->reg_write && !fwex){
							if(memwb_reg->m1 == 1){
								idex_reg->rd1 = memwb_reg->read_data;
							}else{
								idex_reg->rd1 = memwb_reg->alu_result;
							}
							cout << "  forwarding rs from memwb " << endl;
						}
						fwex=false;
						temp_exmem_reg-> alu_result = (idex_reg->rd1) <<= idex_reg->rd2;
						cout << "sll: " <<  temp_exmem_reg->alu_result << endl;
					}else if(idex_reg->ex2 == 4){
						if((idex_reg->rs_no == exmem_reg->wb_reg)&& exmem_reg->reg_write) {
							idex_reg->rd1 = exmem_reg->alu_result;
							cout << "  forwarding rs from exmem " << endl;
							fwex=true;
						}
						if((idex_reg->rs_no == memwb_reg->wb_reg) && memwb_reg->reg_write && !fwex){
							if(memwb_reg->m1 == 1){
								idex_reg->rd1 = memwb_reg->read_data;
							}else{
								idex_reg->rd1 = memwb_reg->alu_result;
							}
							cout << "  forwarding rs from memwb " << endl;
						}
						fwex=false;
						temp_exmem_reg-> alu_result = idex_reg->rd1 >>= idex_reg->rd2;
						cout << "srl: " <<  temp_exmem_reg->alu_result << endl;
					}else if(idex_reg->ex2 == 5){
						if((idex_reg->rs_no == exmem_reg->wb_reg)&& exmem_reg->reg_write) {
							idex_reg->rd1 = exmem_reg->alu_result;
							cout << "  forwarding rs from exmem " << endl;
							fwex=true;
						}
						if((idex_reg->rs_no == memwb_reg->wb_reg) && memwb_reg->reg_write && !fwex){
							if(memwb_reg->m1 == 1){
								idex_reg->rd1 = memwb_reg->read_data;
							}else{
								idex_reg->rd1 = memwb_reg->alu_result;
							}
							cout << "  forwarding rs from memwb " << endl;
						}
						fwex=false;
						temp_exmem_reg-> alu_result = idex_reg->rd1 + idex_reg->rd2;
						cout << "lw address: " <<  temp_exmem_reg->alu_result << endl;
					}else if(idex_reg->ex2 == 6){
						if((idex_reg->rs_no == exmem_reg->wb_reg)&& exmem_reg->reg_write) {
							idex_reg->rd1 = exmem_reg->alu_result;
							cout << "  forwarding rs from exmem " << endl;
							fwex=true;
						}
						if((idex_reg->rs_no == memwb_reg->wb_reg) && memwb_reg->reg_write && !fwex){
							if(memwb_reg->m1 == 1){
								idex_reg->rd1 = memwb_reg->read_data;
							}else{
								idex_reg->rd1 = memwb_reg->alu_result;
							}
							cout << "  forwarding rs from memwb " << endl;
						}
						fwex=false;
						temp_exmem_reg-> alu_result = idex_reg->rd1 + idex_reg->rd2;
						cout << "sw address: " <<  temp_exmem_reg->alu_result << endl;
					}
				}

			}else{
				cout << "EXE empty" << endl;  // force all the values to 0
				temp_exmem_reg->wb = 0;				// transfer the control from prev register
				temp_exmem_reg->m1 = 0;
				temp_exmem_reg->m2 = 0;
				temp_exmem_reg->jump_result = 0;		// no need for now
				temp_exmem_reg->zero = 0;			// no need for now
				temp_exmem_reg-> alu_result = 0;		// for storing alu result
				temp_exmem_reg->write_data = 0;		// data to be stored in memory
				temp_exmem_reg->wb_reg = 0;			// from prev register
				temp_exmem_reg->wb = 0;				// from prev reg
				temp_exmem_reg -> nop = idex_reg->nop;	
				temp_exmem_reg -> reg_write = false; // from prev reg
			}
			cout << "---------------" << endl;
			struct memwbPipe* temp_memwb_reg = (struct memwbPipe*)malloc(sizeof(struct memwbPipe));
			temp_memwb_reg -> wb = memwb_reg -> wb;
			temp_memwb_reg -> read_data = memwb_reg -> read_data;

			temp_memwb_reg -> alu_result = exmem_reg->alu_result;
			temp_memwb_reg -> wb_reg = exmem_reg->wb_reg;
			temp_memwb_reg -> reg_write = exmem_reg->reg_write;
			temp_memwb_reg -> nop = exmem_reg->nop;
			temp_memwb_reg -> wb = exmem_reg->wb ;
			temp_memwb_reg -> m1 = exmem_reg -> m1;
			temp_memwb_reg -> m2 = exmem_reg -> m2;
			if(!exmem_reg->nop){
				cout << "M  ";
				// temp_memwb_reg -> wb = exmem_reg->wb ;
				if(exmem_reg->m1 == 1){
					temp_memwb_reg -> read_data = (int)memory[exmem_reg->alu_result].to_ulong();
					cout << "memory read from memory address: " << exmem_reg->alu_result << " " <<  temp_memwb_reg->read_data << endl; 
				}
				else if(exmem_reg->m2 == 1){
					memory[exmem_reg->alu_result] = reg_file[exmem_reg->wb_reg]; 
					cout << "memory write reg no:" << exmem_reg->wb_reg << " -> " << exmem_reg->alu_result << endl;
				}else{
					cout << "no memory access" << endl;
				}

				temp_memwb_reg -> nop = exmem_reg->nop;
			}else{
				cout << "M empty" << endl;
				temp_memwb_reg -> wb = 0;			
				temp_memwb_reg -> read_data = 0;
				temp_memwb_reg -> alu_result = 0;
				temp_memwb_reg -> wb_reg = 0;
				temp_memwb_reg -> reg_write=0;
				temp_memwb_reg -> nop = exmem_reg->nop;
				temp_memwb_reg -> reg_write = false;
				temp_memwb_reg -> m1 = 0;
				temp_memwb_reg -> m2 = 0;
			}
			free(ifid_reg);
			ifid_reg = temp_ifid_reg;

			free(idex_reg);
			idex_reg = temp_idex_reg;

			free(exmem_reg);
			exmem_reg = temp_exmem_reg;

			free(memwb_reg);
			memwb_reg = temp_memwb_reg;
			cout << "========================NEW CYCLE=======================" << endl;
		}else{
			half_cycle = true;
			cout << "-----------------by the end of 1st Half CYCLE----------------" << endl;
			if(!memwb_reg->nop){
				cout << "WB ";
				if(memwb_reg->reg_write){
					if(memwb_reg->wb == 0){
						reg_file[memwb_reg->wb_reg] = bitset<32>(memwb_reg->read_data);
					}else{
						reg_file[memwb_reg->wb_reg] = bitset<32>(memwb_reg->alu_result);
						// cout << memwb_reg->wb_reg << "   " << reg_file[memwb_reg->wb_reg] << endl; 
					}
					cout << memwb_reg->wb_reg << " : " << reg_file[memwb_reg->wb_reg] << endl;
				}else{
					cout << "no wb" << endl;
				}
			}else{
				cout << "WB empty" << endl;
			}
			cout << "---------------" << endl;
		}
	}

	
	free(ifid_reg);
	free(idex_reg);
	free(exmem_reg);
	free(memwb_reg);



}