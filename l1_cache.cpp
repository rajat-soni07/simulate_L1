#include <iostream>
#include <map>
#include <vector>
#define ll long long int


//when running for a core, access its cache's read/write function and provide the core struct as argument
ll counter=0;
// address is 32 bit out of which 2 are offset bits(as data is a word of 4 bytes)
// ---tag---|--index--|--offset--|
// (32-s-b) bits   |  s bits  |  b bits  |

enum mesi_state { I, M, S, E };


struct block{
    ll data; //32 bit data
    ll tag;
    ll last_used=-1;//for LRU implementation
    bool dirty_bit=0;
    bool valid_bit=0;
    mesi_state state=I;
};

struct core{

    int core_id; // core id
    ll ct_write_instructions = 0; // number of write instructions
    ll ct_read_instructions = 0; // number of read instructions
    ll ct_cache_hits = 0; // number of cache hits
    ll ct_cache_misses = 0; // number of cache misses
    ll ct_execution_cycles = 0; // number of execution cycles
    ll ct_idle_cycles = 0; // number of idle cycles
    ll ct_cache_evictions = 0; // number of cache evictions
    ll ct_writebacks = 0; // number of writebacks
    ll wait_cycles =0; //number of cycles remained in the work this core is doing
    int s = 0; // s is number of set index bits
    int b = 0; // b is number of block offset bits
    int E = 0; // E is number of lines in a set(associativity)
};

struct set{
    std::vector<block> set; // each element of vector is [tag,block(data)]
};

class Memory {
    private:
        std::map<ll, ll> mem;
};

struct L1cache{
    std::vector<set> table; //maps(vector)index to set
    int s=0;int b=0;int E=0;

};

struct mesi_data_bus{
    ll data;
    ll address;
    int core_id; // core id
    int type; // type of the transaction (read/write)
    std::vector<core> cores; // vector of cores as per core id
    std::vector<L1cache> caches;
};




ll hit_or_miss(ll address,struct core &core, struct L1cache &this_cache){
    // determines whether it is a hit or a miss
    // if hit - return the block index in the mapped set
    // if miss - return -1
    ll index = (address >> (this_cache.b)) % (1 << this_cache.s);
    ll req_tag = (address>>((this_cache.b) + (this_cache.s)));
    set this_set = this_cache.table[index];
    for(int i=0;i<this_set.set.size();i++){
        if((this_set.set)[i].tag==req_tag){
            return i;
        }
    }
    return -1;
}

ll read(ll address, struct core &core, struct L1cache &this_cache,struct mesi_data_bus mesi){
    
    ll temp=hit_or_miss(address,core,this_cache);
    if(temp==-1){
        return read_miss(address,core);
    }
    else{
        ll index = (address >> this_cache.b) % (1 << this_cache.s);
        return this_cache.table[index].set[temp].data; // return the data of the block
    }
}

ll read_miss(ll address,struct core core){
    

    
}

ll add_new_block(ll address,ll new_data, set &this_set,struct L1cache this_cache){
    ll ind=0;
    ll min=INT_MAX;
    for(int i=0;i<this_set.set.size();i++){
        if(this_set.set[i].last_used<min){
            min=this_set.set[i].last_used;
            ind=i;
        }
    }
    block new_block;
    new_block.data=new_data;
    new_block.tag=(address>>(this_cache.b+this_cache.s));
    new_block.valid_bit=1;
    new_block.dirty_bit=0;
    new_block.last_used=counter;
    this_set.set[ind]=new_block;
    return ind; // return the index of the newly inserted block
    // caller needs to set the state of the newly inserted block
}


int main(){

}

