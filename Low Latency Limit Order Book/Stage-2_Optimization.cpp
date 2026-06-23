//went from using maps to pre size allocated vectors
//give O(1) access for order list at a price
//mp.begin() if fetched from ram, while arr[i] is fetched from cpu l1/l2 cache
//when a price (index) holds no order in map, we have to erase, causing lots of operations to keep RB tree balanced
//in arr, we can have empty indexes, no need of erase operations, as we are storing the buy_idx and sell_idx

#include <bits/stdc++.h>
using namespace std;
typedef long long ll;

enum Side {Buy, Sell};
struct Order{
    int id; 
    int qty;
    int price;
    Side side;
};
class OrderBook{
    private: 
        vector<list<Order>> buy;
        vector<list<Order>> sell;
        unordered_map<int,list<Order>::iterator> mapping;
        int sell_idx = -1;
        int buy_idx = -1;
    public:
        OrderBook(){
            buy.resize(20000);
            sell.resize(20000);
        }
        void add_order(int id, int qty, int price, Side side){
            Order incoming = {id,qty,price,side};
            if(incoming.side==0){
                while(incoming.qty){
                    if(sell_idx==-1 || sell_idx>incoming.price) break;
                    list<Order>& target = sell[sell_idx];
                    while(!target.empty() && incoming.qty){
                        int amount = min(target.front().qty,incoming.qty);
                        target.front().qty-=amount; incoming.qty-=amount;
                        if(!target.front().qty) {mapping.erase(target.front().id); target.pop_front();}
                        if(!incoming.qty) break;
                    }
                    if(target.empty()){
                        for(int i=sell_idx+1; i<sell.size(); i++){
                            if(!sell[i].empty()) {sell_idx=i; break;}
                        }
                        if(sell[sell_idx].empty()) sell_idx=-1;
                    }
                    if(!incoming.qty) break;
                }
                if(incoming.qty){
                    if(buy_idx==-1 || buy_idx<incoming.price) buy_idx=incoming.price;
                    buy[incoming.price].push_back(incoming);
                    mapping[incoming.id]=prev(buy[incoming.price].end());
                }
            }
            else{
                while(incoming.qty){
                    if(buy_idx==-1 || buy[buy_idx].front().price<incoming.price) break;
                    list<Order>& target = buy[buy_idx];
                    while(!target.empty() && incoming.qty){
                        int amount = min(target.front().qty,incoming.qty);
                        target.front().qty-=amount; incoming.qty-=amount;
                        if(!target.front().qty) {mapping.erase(target.front().id); target.pop_front();}
                        if(!incoming.qty) break;
                    }
                    if(target.empty()){
                        for(int i=buy_idx-1; i>=0; i--){
                            if(!buy[i].empty()) {buy_idx=i; break;}
                        }
                        if(buy[buy_idx].empty()) buy_idx=-1;
                    }
                    if(!incoming.qty) break;
                }
                if(incoming.qty){
                    if(sell_idx==-1 || sell_idx>incoming.price) sell_idx=incoming.price;
                    sell[incoming.price].push_back(incoming);
                    mapping[incoming.id]=prev(sell[incoming.price].end());
                }
            }
        }
        void cancel_order(int id){
            if(!mapping.count(id)) return;
            list<Order>::iterator it = mapping[id];
            int price = it->price;
            int side = it->side;
            if(side==0){
                buy[price].erase(it);
                if(price==buy_idx && buy[buy_idx].empty()){
                    for(int i=buy_idx-1; i>=0; i--){
                        if(!buy[i].empty()) {buy_idx=i; break;}
                    }
                    if(buy[buy_idx].empty()) buy_idx=-1; 
                }
            }
            else{
                sell[price].erase(it);
                if(price==sell_idx && sell[sell_idx].empty()){
                    for(int i=sell_idx+1; i<sell.size(); i++){
                        if(!sell[i].empty()) {sell_idx=i; break;}
                    }
                    if(sell[sell_idx].empty()) sell_idx=-1;
                }
            }
            mapping.erase(id);
        }
        Order best_seller(){
            if(sell_idx==-1) {cout << "no sell orders currently" << endl; return {};}
            return sell[sell_idx].front();
        }
        Order best_buyer(){
            if(buy_idx==-1) {cout << "no buy orders currently" << endl; return {};}
            return buy[buy_idx].front();
        }
};

int main(){
    OrderBook market; 
    int id = 0;
    market.add_order(id++,2,10,Sell);
    market.add_order(id++,10,15,Buy);
    market.add_order(id++,20,10,Sell);
    market.add_order(id++,10,5,Sell);
    // market.cancel_order(3);
    cout << market.best_seller().price << endl;
}
