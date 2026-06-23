//used right datastructures
//map<int,list<Order>>, unordered_map<int,list<Order>::iterator>

#include <bits/stdc++.h>
using namespace std;
typedef long long ll;

enum Side {Buy, Sell};
struct Order{
    int id; 
    Side side;
    int price;
    int qty;
};
class OrderBook{
    private: 
        map<int,list<Order>,greater<int>> buy;
        map<int,list<Order>> sell;
        unordered_map<int,list<Order>::iterator> mapping;
    public:
        void add_order(int id, int qty, int price, Side side){
            Order incoming = {id,side,price,qty};
            if(incoming.side==0){
                while(incoming.qty){
                    if(sell.empty() || (*sell.begin()).first>incoming.price) break;
                    list<Order>& target = (*sell.begin()).second; 
                    while(incoming.qty && !target.empty()){
                        int amount = min(incoming.qty,target.front().qty);
                        incoming.qty-=amount; target.front().qty-=amount;
                        if(!target.front().qty){
                            mapping.erase(target.front().id);
                            target.pop_front();
                        }
                        if(!incoming.qty) break;
                    }
                    if((*sell.begin()).second.empty()) sell.erase(sell.begin());
                    if(!incoming.qty) break;
                }
                if(incoming.qty) {buy[incoming.price].push_back(incoming); mapping[incoming.id]=prev(buy[incoming.price].end());}
            }
            else{
                while(incoming.qty){
                    if(buy.empty() || (*buy.begin()).first<incoming.price) break;
                    list<Order>& target = (*buy.begin()).second; 
                    while(incoming.qty && !target.empty()){
                        int amount = min(incoming.qty,target.front().qty);
                        incoming.qty-=amount; target.front().qty-=amount;
                        if(!target.front().qty){
                            mapping.erase(target.front().id);
                            target.pop_front();
                        }
                        if(!incoming.qty) break;
                    }
                    if((*buy.begin()).second.empty()) buy.erase(buy.begin());
                    if(!incoming.qty) break;
                }
                if(incoming.qty) {sell[incoming.price].push_back(incoming); mapping[incoming.id]=prev(sell[incoming.price].end());}
            }
        }
        void cancel_order(int id){
            if(!mapping.count(id)) return;
            list<Order>::iterator it = mapping[id];
            int price = it->price;
            int side = it->side;
            if(side==0) {
                buy[price].erase(it);
                if(buy[price].empty()) buy.erase(price);
            }
            else{
                sell[price].erase(it);
                if(sell[price].empty()) sell.erase(price);
            }
            mapping.erase(id);
        }
        Order best_seller(){
            if(sell.empty()) {cout << "no sellers currently" << endl; return {};}
            return (*sell.begin()).second.front();
        }
        Order best_buyer(){
            if(buy.empty()) {cout << "no buyers currently" << endl; return {};}
            return (*buy.begin()).second.front();
        }
};

int main(){
    OrderBook market; 
    int id = 0;
    market.add_order(id++,2,10,Sell);
    market.add_order(id++,10,15,Buy);
    market.add_order(id++,20,20,Sell);
    cout << market.best_buyer().price << endl;
}