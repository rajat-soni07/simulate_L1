#include <iostream>
#include <map>
#include <vector>
#include <cstring>
#define ll long long int
#include "input.cpp"
#include <fstream>


//when running for a core, access its cache's read/write function and provide the core struct as argument
ll counter=0;
// address is 32 bit out of which 2 are offset bits(as data is a word of 4 bytes)
// ---tag---|--index--|--offset--|
// (32-s-b) bits   |  s bits  |  b bits  |

enum mesi_state { I, M, S, E };
enum message {BusRd,BusRdx}; //BusRdx is RWITM  


typedef struct block{
    ll data; //32 bit data
    ll tag;
    ll last_used=-1;//for LRU implementation
    bool dirty_bit=0;
    bool valid_bit=0;
    mesi_state state=I;
} block;

typedef struct core{

    int core_id; // core id
    ll ct_write_instructions = 0; // number of write instructions
    ll ct_read_instructions = 0; // number of read instructions
    ll ct_cache_hits = 0; // number of cache hits
    ll ct_cache_misses = 0; // number of cache misses
    ll ct_execution_cycles = 0; // number of execution cycles
    ll ct_idle_cycles = 0; // number of idle cycles
    ll ct_cache_evictions = 0; // number of cache evictions
    ll ct_writebacks = 0; // number of writebacks
    ll ct_invalidations = 0; // number of invalidations

    ll wait_cycles =0; //number of cycles remained in the work this core is doing
    int s = 0; // s is number of set index bits
    int b = 0; // b is number of block offset bits
    int E = 0; // E is number of lines in a set(associativity)
} core;

typedef struct set{
    std::vector<block> set; // each element of vector is [tag,block(data)]
}set ;



typedef struct L1cache{
    std::vector<set> table; //maps(vector)index to set
    int s=0;int b=0;int E=0;

} L1cache;

typedef struct mesi_data_bus{
    ll data;
    ll address;
    int core_id; // core id
    mesi_state next_state; // at end of bus operation, find this address in core id cache and update the state
    message message; // type of the transaction (read/write)
    bool is_busy=false;
    ll wait_cycles=0;
    std::vector<core*> cores; // vector of cores as per core id
    std::vector<L1cache*> caches;
} mesi_data_bus;





ll hit_or_miss(ll address,core &core, L1cache &this_cache){
    // determines whether it is a hit or a miss
    // if hit - return the block index in the mapped set
    // if miss - return -1
    ll index = (address >> (this_cache.b)) % (1 << this_cache.s);
    ll req_tag = (address>>((this_cache.b) + (this_cache.s)));
    set this_set = this_cache.table[index];
    for(int i=0;i<this_set.set.size();i++){
        if((this_set.set)[i].tag==req_tag && this_set.set[i].valid_bit==1){
            return i;
        }
    }
    return -1;
}


std::vector<ll> add_new_block(ll address,ll new_data, set &this_set, L1cache &this_cache){
    //adds new block in a given set res[0]=1 if there is a eviction writeback, res[1] gives the index of block in this set
    ll ind=0;
    ll min=INT_MAX;
    for(int i=0;i<this_set.set.size();i++){
        if(this_set.set[i].last_used<min){
            min=this_set.set[i].last_used;
            ind=i;
        }
    }
    ll eviction_type=0;//0 - no eviction 1-normal eviction 2- writeback eviction
    if(this_set.set[ind].valid_bit==1){
        //Valid and dirty block needs to be evicted
        if(this_set.set[ind].dirty_bit==1){
            eviction_type=2;
        }
        else{
            eviction_type=1;
        }
    }
    block new_block;
    new_block.data=new_data;
    new_block.tag=(address>>(this_cache.b+this_cache.s));
    new_block.valid_bit=1;
    new_block.dirty_bit=0;
    new_block.last_used=counter;
    this_set.set[ind]=new_block;
    std::vector<ll> ans ={eviction_type,ind};
    return ans;
     // return the index of the newly inserted block
    // caller needs to set the state of the newly inserted block
}

ll read_miss(ll address,core &core,L1cache &cache,mesi_data_bus &mesi_data_bus){
    core.ct_cache_misses+=1;
    ll which_case=0; //0 - no hit, 1- exclusive , 2- shared , 3 - Modified
    block *hit_block;
    
    for(int i=0;i<mesi_data_bus.caches.size();i++){
        // assume S and M cannot coexist
        ll index = (address >> (mesi_data_bus.caches[i]->b)) % (1 << mesi_data_bus.caches[i]->s);
        ll temp=hit_or_miss(address,core,*mesi_data_bus.caches[i]);

        if(temp!=-1){
            // hit
            if(mesi_data_bus.caches[i]->table[index].set[temp].state ==M){
                hit_block=&mesi_data_bus.caches[i]->table[index].set[temp];
                mesi_data_bus.cores[i]->ct_writebacks+=1;
                which_case=3;break;
            }

            else if(mesi_data_bus.caches[i]->table[index].set[temp].state ==E){
                hit_block=&mesi_data_bus.caches[i]->table[index].set[temp];
                which_case=1;break;
            }
            else if(mesi_data_bus.caches[i]->table[index].set[temp].state ==S){
                hit_block=&mesi_data_bus.caches[i]->table[index].set[temp];
                which_case=2;break;
            }
        } 

    }

    mesi_data_bus.core_id=core.core_id;mesi_data_bus.address=address;
    mesi_data_bus.message=BusRd;
    if(which_case==0){
        // no hit
        core.wait_cycles=100 + 1; 
        mesi_data_bus.wait_cycles=100 + 1;
        mesi_data_bus.is_busy=true;

    }
    else if(which_case==1){
        // exclusive found
        hit_block->state=S; // the hitted block's state becomes shared
        core.wait_cycles=2*(core.b) + 1;
        mesi_data_bus.wait_cycles=2*(core.b) +1;
        mesi_data_bus.is_busy=true;
    }

    else if(which_case==2){
        // shared found , use the first cache in order to use data
        core.wait_cycles=2*(core.b) + 1;
        mesi_data_bus.wait_cycles=2*(core.b) +1; 
        mesi_data_bus.is_busy=true;
    }
    else{
        // M found
        hit_block->state=S;
        hit_block->dirty_bit=0;//No more dirty,it matches with memory
        core.wait_cycles= 2*(core.b) + 100 +1 ;
        mesi_data_bus.wait_cycles=2*(core.b)+100+1;
        mesi_data_bus.is_busy=true;
    }

    return which_case; //return which_case of readmiss so that the state of newly inserted block can be determined
}

bool read(ll address, core &core, L1cache &this_cache, mesi_data_bus &mesi_data_bus){
    // core.ct_cache_hits+=1;
    ll temp=hit_or_miss(address,core,this_cache);
    ll index = (address >> (this_cache.b)) % (1 << this_cache.s);
    if(temp==-1){
        //miss
        if(mesi_data_bus.is_busy){
            // bus is busy, so read is not possible
            return false;
        }


        ll which_case = read_miss(address,core,this_cache,mesi_data_bus);
        if(which_case==0){
            // no hit
            mesi_data_bus.next_state=E;
        }
        else{
            mesi_data_bus.next_state=S;
        }
        std::vector<ll> res = add_new_block(address,0,this_cache.table[index],this_cache);
        if(res[0]==2){
            //write_back during eviction
            core.ct_cache_evictions+=1;
            core.ct_writebacks+=1;
            core.wait_cycles+=100;
            mesi_data_bus.wait_cycles+=100;
        }
        else if(res[0]==1){
            //normal eviction
            core.ct_cache_evictions+=1;
        }
        this_cache.table[index].set[res[1]].last_used=counter; //update last used for lru policy
        
        return true;
    }
    else{
        //hit 
        //make last used of tempth way of mapped set = counter
        core.ct_cache_hits+=1;
        this_cache.table[index].set[temp].last_used=counter;
        core.wait_cycles=1;
        return true;
    }
}


ll write_miss(ll address,core &core,L1cache &this_cache,mesi_data_bus &mesi_data_bus){
    core.ct_cache_misses+=1;
    ll which_case=0; //0 - no hit, 1- exclusive , 2- shared , 3 - Modified
    block hit_block;
    
    for(int i=0;i<mesi_data_bus.caches.size();i++){
        // assume S and M cannot coexist
        ll index = (address >> (mesi_data_bus.caches[i]->b)) % (1 << mesi_data_bus.caches[i]->s);
        ll temp=hit_or_miss(address,core,*mesi_data_bus.caches[i]);

        if(temp!=-1){
            // hit
            if(mesi_data_bus.caches[i]->table[index].set[temp].state ==M){
                mesi_data_bus.cores[i]->ct_writebacks+=1;
                hit_block=mesi_data_bus.caches[i]->table[index].set[temp];
                which_case=3;break;
            }

            else if(mesi_data_bus.caches[i]->table[index].set[temp].state ==E){
                hit_block=mesi_data_bus.caches[i]->table[index].set[temp];
                which_case=1;break;
            }
            else{
                hit_block=mesi_data_bus.caches[i]->table[index].set[temp];
                which_case=2;break;
            }
        } 

    }
    ll index = (address >> (this_cache.b)) % (1 << this_cache.s);
    mesi_data_bus.core_id=core.core_id;mesi_data_bus.address=address;
    mesi_data_bus.message=BusRdx;mesi_data_bus.is_busy=true;
    //Snooping in occurs as caches see BusRdx on data_bu
    if(which_case==0){
        core.wait_cycles=100+1;
        mesi_data_bus.wait_cycles=100+1;
        return 0;
    }
    else if(which_case==1 or which_case==2){
        core.wait_cycles=2*(core.b) +1 ;
        mesi_data_bus.wait_cycles=2*(core.b) +1;
        //Invalidate all states
        for(int i=0;i<mesi_data_bus.caches.size();i++){
            ll temp=hit_or_miss(address,core,this_cache);
            if(temp!=-1){
                mesi_data_bus.caches[i]->table[index].set[temp].state=I;
                mesi_data_bus.cores[i]->ct_invalidations+=1;
                mesi_data_bus.caches[i]->table[index].set[temp].valid_bit=0;
            }
        }
        return 0;
    }
    else{
        //in simulator check if mesi_bus is in Rdx and the state change in final cycle is imposed on M->I or not
        //If it is M->I, dont go on next instruction for the core and do this instruction on priority
        mesi_data_bus.wait_cycles=100;
        core.wait_cycles=100; //only perform write back from this M block to memory
        return 1;
    }

}

bool write(ll address,core &core,L1cache &this_cache, mesi_data_bus &mesi_data_bus){
    ll temp = hit_or_miss(address,core,this_cache);
    ll index = (address >> (this_cache.b)) % (1 << this_cache.s);
    if(temp!=-1){
        core.ct_cache_hits+=1;
        //hit
        block &this_block = this_cache.table[index].set[temp];
        this_block.dirty_bit=1;
        core.wait_cycles=1;
        if(this_block.state==S){
            for(int i=0;i<mesi_data_bus.caches.size();i++){
                ll temp=hit_or_miss(address,core,this_cache);
                if(temp!=-1){
                    mesi_data_bus.caches[i]->table[index].set[temp].state=I;
                    mesi_data_bus.cores[i]->ct_invalidations+=1;
                    mesi_data_bus.caches[i]->table[index].set[temp].valid_bit=0;
                }
            }
        }
        this_block.state=M;
        this_block.last_used=counter;
        return true;
    }
    else{
        //NOTE- IN case of write miss, at end of transaction, along with making state of the block M, also set the dirty bit
        if(mesi_data_bus.is_busy){return false;}

        ll is_writeback_from_M=write_miss(address,core,this_cache,mesi_data_bus);
        if(!is_writeback_from_M){
        mesi_data_bus.next_state=M;}
        else{
            mesi_data_bus.next_state=I;
        }
        std::vector<ll> res = add_new_block(address,0,this_cache.table[index],this_cache);
        // this_cache.table[index].set[res[1]].dirty_bit=1;
        if(res[0]==2){
            //write_back during eviction
            core.ct_cache_evictions+=1;
            core.ct_writebacks+=1;
            core.wait_cycles+=100;
            mesi_data_bus.wait_cycles+=100;
        }
        else if(res[0]==1){
            //normal eviction
            core.ct_cache_evictions+=1;
        }
        this_cache.table[index].set[res[1]].last_used=counter; //update last used for lru policy
        return true;
    }
}

void print_help() {
    std::cout << "Usage: ./l1_cache [options]\n"
              << "-t <tracefile>: name of parallel application\n"
              << "-s <s>: number of set index bits\n"
              << "-E <E>: associativity (number of cache lines per set)\n"
              << "-b <b>: number of block bits\n"
              << "-o <outfilename>: logs output to file\n"
              << "-h: prints this help\n";
}

int main(int argc, char *argv[]){
    std::string app;
    int s = -1, E = -1, b = -1;
    std::string outfilename;
    
    if (argc == 1) {
        print_help();
        return 0;
    }

    for (int i = 1; i < argc; ++i) {
        // std::cout << std::strcmp(argv[i],"-t") << " ";
        if (std::strcmp(argv[i], "-h") == 0) {
            print_help();
            return 0;
        } else if (std::strcmp(argv[i], "-t") == 0) {
            if (i + 1 < argc) {
                app = argv[++i];
            } else {
                std::cerr << "Error: -t flag needs a tracefile.\n";
                return 1;
            }
        } else if (std::strcmp(argv[i], "-s") == 0) {
            if (i + 1 < argc) {
                s = std::atoi(argv[++i]);
            } else {
                std::cerr << "Error: -s flag needs a value.\n";
                return 1;
            }
        } else if (std::strcmp(argv[i], "-E") == 0) {
            if (i + 1 < argc) {
                E = std::atoi(argv[++i]);
            } else {
                std::cerr << "Error: -E flag needs a value.\n";
                return 1;
            }
        } else if (std::strcmp(argv[i], "-b") == 0) {
            if (i + 1 < argc) {
                b = std::atoi(argv[++i]);
            } else {
                std::cerr << "Error: -b flag needs a value.\n";
                return 1;
            }
        } else if (std::strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                outfilename = argv[++i];
            } else {
                std::cerr << "Error: -o flag needs an output filename.\n";
                return 1;
            }
        } else {
            std::cerr << "Unknown flag: " << argv[i] << "\n";
            return 1;
        }
    }
    b+=2;
    std::vector<L1cache> caches;
    std::vector<core> cores;
    for(int i=0;i<4;i++){
        L1cache this_cache;
        this_cache.b=b;this_cache.E=E;this_cache.s=s;
        for(int j=0;j<(1<<s);j++){
            set temp_set;
            for(int _=0;_<E;_++){
                block this_block;
                temp_set.set.push_back(this_block);
            }
            this_cache.table.push_back(temp_set);
        }

        core this_core;
        this_core.core_id=i;
        this_core.E=E;this_core.s=s;this_core.b=b;
        caches.push_back(this_cache);
        cores.push_back(this_core);
    }
    mesi_data_bus mesi_data_bus;
    for (int i = 0; i < cores.size(); i++) {
        mesi_data_bus.cores.push_back(&(cores[i]));
    }
    for (int i = 0; i < caches.size(); i++) {
        mesi_data_bus.caches.push_back(&(caches[i]));
    }
    std::vector<ll> reads(4,0);
    std::vector<ll> writes(4,0);

    std::vector<std::vector<std::vector<ll>>> commands=input(app,reads,writes);
    for (int i=0;i<4;i++){
        cores[i].ct_read_instructions = reads[i];
        cores[i].ct_write_instructions = writes[i];
    }
    std::vector<int> curr(4,0);
    int n0=commands[0].size();int n1=commands[1].size();int n2=commands[2].size();int n3=commands[3].size();
    counter=0;
    while( curr[0]<n0 || curr[1]<n1 || curr[2]<n2 || curr[3]<n3 ){
        // for (int i=0;i<4;i++){
        //     std::cout<<curr[i]<<" ";
        // }

        int done_core=-1;
        if(mesi_data_bus.wait_cycles>0){
            mesi_data_bus.wait_cycles-=1;
        }
        
        if(mesi_data_bus.wait_cycles==0 && mesi_data_bus.is_busy==true){
            mesi_data_bus.is_busy=false;
            ll index = (mesi_data_bus.address >> (b)) % (1 <<s);

            ll temp=hit_or_miss(mesi_data_bus.address,cores[mesi_data_bus.core_id],caches[mesi_data_bus.core_id]);
            if(temp!=-1){
                caches[mesi_data_bus.core_id].table[index].set[temp].state=mesi_data_bus.next_state;
                if(mesi_data_bus.next_state==M){
                    caches[mesi_data_bus.core_id].table[index].set[temp].dirty_bit=1;
                }

                
            if(mesi_data_bus.next_state==I){
                done_core=mesi_data_bus.core_id;
                write(mesi_data_bus.address,cores[mesi_data_bus.core_id],caches[mesi_data_bus.core_id],mesi_data_bus);
                done_core=mesi_data_bus.core_id;
            }
            }
        }

        for(int i=0;i<4;i++){
            if(curr[i]==commands[i].size()){
                cores[i].ct_idle_cycles+=1;
                continue;
            }
            if(i==done_core){continue;}
            if(cores[i].wait_cycles==0){
                bool temp;
                if(commands[i][curr[i]][0]==1){
                     temp=write(commands[i][curr[i]][1],cores[i],caches[i],mesi_data_bus);
                }
                else{
                    temp=read(commands[i][curr[i]][1],cores[i],caches[i],mesi_data_bus);
                }
                if(temp==false){
                    cores[i].ct_idle_cycles+=1;
                }
                else{
                    cores[i].ct_execution_cycles+=1;
                   
                }
            }
            else{
                cores[i].wait_cycles-=1;
                cores[i].ct_execution_cycles+=1;
                if(cores[i].wait_cycles==0){
                    curr[i]++;
                }
            }
        }
        
        counter++;
    }

    std::ofstream output_file(outfilename);
    std::streambuf *cout_buf = std::cout.rdbuf();
    std::cout.rdbuf(output_file.rdbuf());
    
    std::cout << "Simulation Parameters:\n";
    std::cout << "Trace Prefix: app1\n";
    std::cout << "Set Index Bits: " << s << "\n";
    std::cout << "Associativity: " << E << "\n";
    std::cout << "Block Bits: " << b - 2 << "\n";
    std::cout << "Block Size (Bytes): " << (1 << (b - 2)) << "\n";
    std::cout << "Number of Sets: " << (1 << s) << "\n";
    std::cout << "Cache Size (KB per core): " << ((1 << s) * E * (1 << (b - 2)) / 1024) << "\n";
    std::cout << "MESI Protocol: Enabled\n";
    std::cout << "Write Policy: Write-back, Write-allocate\n";
    std::cout << "Replacement Policy: LRU\n";
    std::cout << "Bus: Central snooping bus\n\n";

    ll total_bus_transactions = 0;
    ll total_bus_traffic = 0;

    for (int i = 0; i < 4; i++) {
        ll total_instructions = cores[i].ct_read_instructions + cores[i].ct_write_instructions;
        ll num_reads = cores[i].ct_read_instructions;
        ll num_writes = cores[i].ct_write_instructions;
        ll total_cycles = cores[i].ct_execution_cycles ;
        ll idle_cycles = cores[i].ct_idle_cycles;
        ll misses = cores[i].ct_cache_misses;
        double miss_rate = (total_instructions > 0) ? (100.0 * misses / total_instructions) : 0.0;
        ll evictions = cores[i].ct_cache_evictions;
        ll writebacks = cores[i].ct_writebacks;
        ll invalidations = cores[i].ct_invalidations;
        ll traffic_bytes = (misses + writebacks) * (1 << (b - 2));

        total_bus_transactions += misses + writebacks;
        total_bus_traffic += traffic_bytes;

        std::cout << "Core " << i << " Statistics:\n";
        std::cout << "Total Instructions: " << total_instructions << "\n";
        std::cout << "Total Reads: " << num_reads << "\n";
        std::cout << "Total Writes: " << num_writes << "\n";
        std::cout << "Total Execution Cycles: " << total_cycles << "\n";
        std::cout << "Idle Cycles: " << idle_cycles << "\n";
        std::cout << "Cache Misses: " << misses << "\n";
        std::cout << "Cache Miss Rate: " << miss_rate << "%\n";
        std::cout << "Cache Evictions: " << evictions << "\n";
        std::cout << "Writebacks: " << writebacks << "\n";
        std::cout << "Bus Invalidations: " << invalidations << "\n";
        std::cout << "Data Traffic (Bytes): " << traffic_bytes << "\n\n";
    }

    std::cout << "Overall Bus Summary:\n";
    std::cout << "Total Bus Transactions: " << total_bus_transactions << "\n";
    std::cout << "Total Bus Traffic (Bytes): " << total_bus_traffic << "\n";

    output_file.close();
    std::cout.rdbuf(cout_buf);

}
