#include <iostream>
#include <map>
#include <vector>
#define ll long long int
#include "mesi.cpp"

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

struct set{
    std::vector<block> set; // each element of vector is [tag,block(data)]
};

class Memory {
    private:
        std::map<ll, ll> mem;
};

class L1cache{
    private:
    std::vector<set> table; //maps(vector)index to set
    int s=0;int b=0;int E=0;    

    public:
        void build(int s,int b,int E){
            this->s=s;this->b=b;this->E=E;
            ll number_of_sets=1<<s;
            //E- number of blocks in a set
            for(int i=0;i<number_of_sets;i++){
                set v1;
                for(int j=0;i<E;j++){
                    block temp_block;
                    v1.set.push_back(temp_block);
                }
                this->table.push_back(v1);
            }
        }

        ll hit_miss(ll address){
            // determines whether it is a hit or a miss
            // if hit - return the block index in the mapped set
            // if miss - return -1
            ll index = (address >> b) % (1 << s);
            ll req_tag = (address>>(b+s));
            set this_set = this->table[index];
            for(int i=0;i<this_set.set.size();i++){
                if((this_set.set)[i].tag==req_tag){
                    return i;
                }
            }
            return -1;
        }

        ll read(ll address){

            ll temp=hit_miss(address);
            if(temp==-1){
                return read_miss(address);
            }
            else{
                ll index = (address >> b) % (1 << s);
                return this->table[index].set[temp].data; // return the data of the block
            }
        }

        ll read_miss(ll address){

            
        }

        ll add_new_block(set &this_set,ll address,ll new_data){
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
            new_block.tag=(address>>(b+s));
            new_block.valid_bit=1;
            new_block.dirty_bit=0;
            new_block.last_used=counter;
            this_set.set[ind]=new_block;
            return ind; // return the index of the newly inserted block
            // caller needs to set the state of the newly inserted block
        }
};

class Core{
    private:
        L1cache cache;
        int core_id; // core id
        ll ct_write_instructions = 0; // number of write instructions
        ll ct_read_instructions = 0; // number of read instructions
        ll ct_cache_hits = 0; // number of cache hits
        ll ct_cache_misses = 0; // number of cache misses
        ll ct_execution_cycles = 0; // number of execution cycles
        ll ct_idle_cycles = 0; // number of idle cycles
        ll ct_cache_evictions = 0; // number of cache evictions
        ll ct_writebacks = 0; // number of writebacks
        int s = 0; // s is number of set index bits
        int b = 0; // b is number of block offset bits
        int E = 0; // E is number of lines in a set(associativity)

    public:
        Core(int s,int b,int E){
            this->s = s;
            this->b = b;
            this->E = E;
            (this->cache).build(s,b,E);
        }

        ll read(ll address){
            this->cache.read(address);
        }
};
