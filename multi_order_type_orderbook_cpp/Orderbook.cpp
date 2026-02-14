#include <iostream>
#include <map>
#include <set>
#include <list>
#include <cmath>
#include <ctime>
#include <deque>
#include <queue>
#include <stack>
#include <limits>
#include <string>
#include <vector>
#include <numeric>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <memory>  
#include <variant>
#include <optional>
#include <tuple>
#include <sstream>
#include <stdexcept>

enum class OrderType {
    GoodTillCancel,
    FillandKill,
};

enum class Side {
    Buy,
    Sell,
};

using Price    = std::int32_t;
using Quantity = std::uint32_t;
using OrderId  = std::uint64_t;

struct LevelInfo {
    Price price_;
    Quantity quantity_;
};

using LevelInfos = std::vector<LevelInfo>;

class OrderbookLevelInfos {
    public:                                                                                                                           
        OrderbookLevelInfos (const LevelInfos& bids, const LevelInfos& asks)
            : bids_(bids), asks_(asks) 
        {}

        const LevelInfos& GetBids () const { return bids_; }
        const LevelInfos& GetAsks () const { return asks_; }

    private:
        LevelInfos bids_;
        LevelInfos asks_;
};

class Order {
    public:
        Order (OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity)
            : orderType_ {orderType}
            , orderId_ {orderId}
            , side_ {side}
            , price_ {price}
            , quantity_ {quantity}
            , initialQuantity_ {quantity}
            , remainingQuantity_ {quantity}
        {}

        OrderId GetOrderId () const { return orderId_; }
        Side GetSide() const { return side_; }
        Price GetPriice() const {return price_;}
        OrderType GetOrderType() const { return orderType_; }
        Quantity GetInitialQuantity() const { return initialQuantity_; }
        Quantity GetRemainingQuantity() const {return remainingQuantity_;}
        Quantity GetFilledQuantity() const { return GetInitialQuantity() - GetRemainingQuantity(); }

        void Fill (Quantity quantity) {
            if (quantity > GetRemainingQuantity()) {
                std::stringstream ss;
                ss << "Order (" << orderId_ << ") cannot be filled with quantity (" << quantity << ") greater than remaining quantity (" << GetRemainingQuantity() << ")";
                throw std::logic_error(ss.str());
            }

            remainingQuantity_ -= quantity;
        }

    private:
        OrderType orderType_;
        OrderId orderId_;
        Side side_;
        Price price_;
        Quantity quantity_;
        Quantity initialQuantity_;
        Quantity remainingQuantity_;
        
};      

using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;

class OrderModify {
    public:
        OrderModify (OrderId orderId, Side side, Price price, Quantity quantity)
            : orderId_ { orderId }
            , price_ {price }
            , side_ { side }
            , quantity_ { quantity }

        { }

        OrderId GetOrderId() const { return orderId_; }
        Price GetPrice() const { return price_; }
        Side GetSide() const { return side_; }
        Quantity GetQuantity() const { return quantity_; }

        OrderPointer ToOrderPointer (OrderType type) const {
            return std::make_shared<Order> (type, GetOrderId, GetSide(), GetPrice(), GetQuantity());
        }

    private:
        orderId_;
};

int main() {
    return 0;
}