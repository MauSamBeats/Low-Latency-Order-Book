
//still uses unordered_map<int,int>, id->idx(in pool)
//still uses buy_idx and sell_idx, gives head/tail of the order list, with best bid/ask in O(1)
//replaces vector<list<Order>> with 
    //storing the heads and tails of all order lists, price wise
    //using a pool, here is where all the order lists are stored, this is where the heads/intermediate/tails point to
    //this is used inplace of computer RAM
//list uses 'new' under the hood, which allocates heap
//meaning, whenever you add a order, the program asks the OS to find free block of RAM for it
//this is highly unpredictable operastion, can vary from 10-2000ns, so we eliminate it by not using list
//we pre allocate a massive memory pool, so no heap allocations are required later
//we manage all our storage by ourself, meaning a single pool array is all that we use and modify, right here

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
class OrderBook{
    private: 
        vector<int> buy_heads;
        vector<int> buy_tails;
        vector<int> sell_heads;
        vector<int> sell_tails;
        vector<OrderNode> pool;
        int free_head=0; 
        unordered_map<int,int> mapping;
        int sell_idx=-1; 
        int buy_idx=-1;

    public:
        OrderBook(){
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
                            mapping.erase(pool[temp].id);
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
                        mapping[pool[idx].id]=idx;
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
                            mapping.erase(pool[temp].id);
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
                        mapping[pool[idx].id]=idx;
                        add_to_list(idx); 
                        if(sell_idx==-1 || sell_idx>incoming.price) sell_idx=incoming.price;
                }
            }
        }
        void cancel_order(int id){
            if(!mapping.count(id)) return;
            int idx = mapping[id];
            int price = pool[idx].price;
            int side = pool[idx].side;
            remove_from_list(idx);
            freeNode(idx);
            mapping.erase(id);
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
