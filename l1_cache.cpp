#include <iostream>
#include <map>
#include <vector>
#define ll long long int

struct block{
    ll data;
    ll tag;
    bool dirty_bit;
    bool valid_bit;
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
    std::map<ll,set> table; //map from index to set    
};
