//just replaces unordered_map<int,int> with openHash, a hashmap using open addressing, i.e no heap allocation
//unordered map uses 'seperate chaining' to avoid collisions, which is a linked list
//meaning, unordered map is making new RAM allocations, on every collision that happens
//we need some other way, to map 'order it' with 'order address'
//to overcome this, we use a hashmap, which uses 'open addressing', meaning probing techniques
//this truly makes it a zero allocation hash map
//bitwise modulo
    // doing a%b is equivalent of doing a&(b-1), if b is a power of 2

#include <bits/stdc++.h>
using namespace std;
typedef long long ll;
int max_orders = 1e6;


enum Side {Buy, Sell};
struct OrderNode{
    int id;
    int qty;
    int price;
    Side side;
    int prev_idx;
    int next_idx;
};

class openHash{
    private:
        struct Entry{
            int id;
            int pool_idx;
        };
        vector<Entry> table;
        int mask;
    public:
        openHash(int capacity_in_power_of_two){   //for bitwise modulo (lightning fast)
            mask=capacity_in_power_of_two-1;
            table.resize(capacity_in_power_of_two,{-1,-1});
        }
        void insert(int id, int pool_idx){
            int idx=id&mask;
            while(table[idx].id!=-1 && table[idx].id!=-2 && table[idx].id!=id){ //table never gets full
                idx=(idx+1)&mask;                                               //load factor mantained to .5
            }
            table[idx]={id,pool_idx};
        }
        int get(int id){
            int idx=id&mask;
            while(table[idx].id!=-1){
                if(table[idx].id==id){
                    return table[idx].pool_idx;
                }
                idx=(idx+1)&mask;
            }
            return -1;
        }
        void remove(int id){
            int idx = id&mask;
            while(table[idx].id!=-1){
                if(table[idx].id==id){
                    table[idx].id=-2;
                    return;
                }
                idx=(idx+1)&mask;
            }
        }
};



class OrderBook{
    private: 
        vector<int> buy_heads;
        vector<int> buy_tails;
        vector<int> sell_heads;
        vector<int> sell_tails;
        openHash mapping;
        vector<OrderNode> pool;
        int free_head=0; 
        int sell_idx=-1; 
        int buy_idx=-1;

    public:
        OrderBook():mapping(1<<21){
            buy_heads.resize(20000,-1);
            buy_tails.resize(20000,-1);
            sell_heads.resize(20000,-1);
            sell_tails.resize(20000,-1);
            pool.resize(max_orders);
            for(int i=0; i<pool.size()-1; i++){
                pool[i].next_idx=i+1; 
            }
            pool[max_orders-1].next_idx=-1;
            free_head = 0;
        }
        int allocate_index(){
            if(free_head==-1) {cout << "Storage full" << endl; return -1;}
            int allocated = free_head;
            free_head = pool[free_head].next_idx;
            return allocated;
        }
        void freeNode(int idx){
            pool[idx].next_idx=free_head;
            free_head=idx;
        }
        void add_to_list(int idx){
            int price = pool[idx].price;
            int side = pool[idx].side;
            int& head = (side==0)?buy_heads[price]:sell_heads[price];
            int& tail = (side==0)?buy_tails[price]:sell_tails[price];
            if(head==-1){
                head=tail=idx;
                pool[idx].prev_idx=-1;
            }
            else{
                pool[tail].next_idx=idx;
                pool[idx].prev_idx=tail;
                tail=idx;
            }
        }
        void remove_from_list(int idx){
            int price = pool[idx].price;
            int side = pool[idx].side;
            int& head = (side==0)?buy_heads[price]:sell_heads[price];
            int& tail = (side==0)?buy_tails[price]:sell_tails[price];
            int prev = pool[idx].prev_idx;
            int next =pool[idx].next_idx;
            if(prev!=-1) pool[prev].next_idx = next;
            else head=next;
            if(next!=-1) pool[next].prev_idx=prev;
            else tail=prev;
        }
        void add_order(int id, int qty, int price, Side side){
            OrderNode incoming = {id,qty,price,side,-1,-1};
            if(incoming.side==0){
                while(incoming.qty){
                    if(sell_idx==-1 || incoming.price<sell_idx) break;
                    int head = sell_heads[sell_idx];
                    int tail = sell_tails[sell_idx];
                    int temp = head;
                    while(temp!=-1 && incoming.qty){
                        int amount = min(pool[temp].qty,incoming.qty);
                        pool[temp].qty-=amount; incoming.qty-=amount;
                        if(!pool[temp].qty){
                            mapping.remove(pool[temp].id);
                            remove_from_list(temp);
                            int next_temp = pool[temp].next_idx;
                            freeNode(temp);
                            temp=next_temp;
                        }
                        if(!incoming.qty) break;
                    }
                    if(sell_heads[sell_idx]==-1){
                        for(int i=sell_idx+1; i<sell_heads.size(); i++){
                            if(sell_heads[i]!=-1) {sell_idx=i; break;}
                        }
                        if(sell_heads[sell_idx]==-1) sell_idx=-1;
                    }
                    if(!incoming.qty) break;
                }
                if(incoming.qty){
                        int idx = allocate_index();
                        pool[idx]=incoming;
                        mapping.insert(pool[idx].id,idx);
                        add_to_list(idx); 
                        if(buy_idx==-1 || buy_idx<incoming.price) buy_idx=incoming.price;
                }
            }
            else{
                while(incoming.qty){
                    if(buy_idx==-1 || incoming.price>buy_idx) break;
                    int head = buy_heads[buy_idx];
                    int tail = buy_tails[buy_idx];
                    int temp = head;
                    while(temp!=-1 && incoming.qty){
                        int amount = min(pool[temp].qty,incoming.qty);
                        pool[temp].qty-=amount; incoming.qty-=amount;
                        if(!pool[temp].qty){
                            mapping.remove(pool[temp].id);
                            remove_from_list(temp);
                            int next_temp = pool[temp].next_idx;
                            freeNode(temp);
                            temp=next_temp;
                        }
                        if(!incoming.qty) break;
                    }
                    if(buy_heads[buy_idx]==-1){
                        for(int i=buy_idx-1; i>=0; i--){
                            if(buy_heads[i]!=-1) {buy_idx=i; break;}
                        }
                        if(buy_heads[buy_idx]==-1) buy_idx=-1;
                    }
                    if(!incoming.qty) break;
                }
                if(incoming.qty){
                        int idx = allocate_index();
                        pool[idx]=incoming;
                        mapping.insert(pool[idx].id,idx);
                        add_to_list(idx); 
                        if(sell_idx==-1 || sell_idx>incoming.price) sell_idx=incoming.price;
                }
            }
        }
        void cancel_order(int id){
            int idx = mapping.get(id);
            if(idx==-1) return;
            int price = pool[idx].price;
            int side = pool[idx].side;
            remove_from_list(idx);
            freeNode(idx);
            mapping.remove(id);
            if(side==0 && buy_idx!=-1 && buy_heads[buy_idx]==-1){
                for(int i=buy_idx-1; i>=0; i--){
                    if(buy_heads[i]!=-1){buy_idx=i; break;}
                }
                if(buy_heads[buy_idx]==-1) buy_idx=-1;
            }
            if(side==1 && sell_idx!=-1 && sell_heads[sell_idx]==-1){
                for(int i=sell_idx+1; i<sell_heads.size(); i++){
                    if(sell_heads[i]!=-1) {sell_idx=i; break;}
                }
                if(sell_heads[sell_idx]==-1) sell_idx=-1;
            }
        }
        OrderNode best_seller(){
            if(sell_idx==-1) {cout << "no sell orders currently" << endl; return {};}
            return pool[sell_heads[sell_idx]];
        }
        OrderNode best_buyer(){
            if(buy_idx==-1) {cout << "no buy orders currently" << endl; return {};}
            return pool[buy_heads[buy_idx]];
        }
};

int main(){
    OrderBook market; 
    int id = 0;
    market.add_order(id++,2,10,Sell);
    market.add_order(id++,10,15,Buy);
    market.add_order(id++,20,10,Sell);
    market.add_order(id++,12,9,Buy);
    cout << market.best_seller().price << endl;
}
