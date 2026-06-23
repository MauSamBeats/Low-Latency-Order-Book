#include <bits/stdc++.h>
using namespace std;
typedef long long ll;

enum Side {Buy, Sell};
struct Order{
    int id; 
    Side side;
    int price;
    int qty;
    int timestamp;
};

class OrderBook{
    private: 
        vector<Order> orders;
        int curr_time=0;
    public:
        void better_index(int& best_idx, Order incoming, int i){
            if(orders[i].side==incoming.side) return;
            if(incoming.side==0){
                if(orders[i].price>incoming.price) return;
                if(best_idx==-1) {best_idx=i; return;}
                bool better = (orders[i].price<orders[best_idx].price);
                bool better_time = (orders[i].price==orders[best_idx].price && orders[i].timestamp<orders[best_idx].timestamp);
                if(better || better_time) best_idx=i;
            }
            else{
                if(orders[i].price<incoming.price) return; 
                if(best_idx==-1) {best_idx=i; return;}
                bool better = (orders[i].price>orders[best_idx].price);
                bool better_time = (orders[i].price==orders[best_idx].price && orders[i].timestamp<orders[best_idx].timestamp);
                if(better || better_time) best_idx=i;
            }
        }
        void add_order(int id, int qty, int price, Side side){
            Order incoming = {id,side,price,qty,curr_time++};
            while(incoming.qty){
                int best_idx=-1;
                for(int i=0; i<orders.size(); i++) better_index(best_idx,incoming,i);
                if(best_idx==-1){orders.push_back(incoming); break;}
                int amount = min(incoming.qty,orders[best_idx].qty); 
                orders[best_idx].qty-=amount; incoming.qty-=amount;
                if(!orders[best_idx].qty) orders.erase(orders.begin()+best_idx);
            }
        }
        void cancel_order(int id){
            for(int i=0; i<orders.size(); i++){
                if(orders[i].id==id){
                    orders.erase(orders.begin()+i);
                    break;
                }
            }
        }
        Order best_seller(){
            int best_idx = -1;
            Order incoming = {-1,Buy,INT_MAX,-1,-1};
            for(int i=0; i<orders.size(); i++) better_index(best_idx,incoming,i);
            if(best_idx==-1) {cout << "no sell orders currently" << endl; return {};}
            return orders[best_idx];
        }
        Order best_buyer(){
            int best_idx = -1;
            Order incoming = {-1,Sell,INT_MIN,-1,-1};
            for(int i=0; i<orders.size(); i++) better_index(best_idx,incoming,i);
            if(best_idx==-1){cout << "no buy orders currently" << endl; return {};}
            return orders[best_idx];
        }
};

int main(){
    OrderBook market; 
    int id = 0;
    market.add_order(id++,2,10,Sell);
    market.add_order(id++,10,15,Buy);
    market.add_order(id++,20,10,Sell);
    cout << market.best_seller().price << endl;
}