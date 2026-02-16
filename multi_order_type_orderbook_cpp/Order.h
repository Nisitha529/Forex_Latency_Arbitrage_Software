#pragma once

#include <list>
#include <exception>
#include <sstream>
#include <memory> 

#include "Constants.h"
#include "OrderType.h"
#include "Usings.h"
#include "Side.h"

/**
 * @brief Represents a limit or market order in the order book.
 * 
 * Tracks order details (type, ID, side, price, quantity) and manages 
 * remaining quantity after fills. Supports converting market orders to 
 * Good‑Till‑Cancel orders.
 */
class Order {
    public:
        /**
         * @brief Constructs a full order (limit or market with price).
         * @param orderType Type of order (Limit, Market, etc.)
         * @param orderId Unique identifier for the order
         * @param side Buy or sell side
         * @param price Price for limit orders, Constants::InvalidPrice for market orders
         * @param quantity Total quantity of the order
         */
        Order (OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity)
            : orderType_ {orderType}
            , orderId_ {orderId}
            , side_ {side}
            , price_ {price}
            , quantity_ {quantity}
            , initialQuantity_ {quantity}
            , remainingQuantity_ {quantity}
        {}

        /**
         * @brief Constructs a market order (price is automatically set to invalid).
         * @param orderId Unique identifier
         * @param side Buy or sell side
         * @param quantity Total quantity
         */
        Order(OrderId orderId, Side side, Quantity quantity)
        : Order(OrderType::Market, orderId, side, Constants::InvalidPrice, quantity)
        {}

        /** @return Unique order ID. */
        OrderId GetOrderId () const { return orderId_; }

        /** @return Buy or sell side. */
        Side GetSide() const { return side_; }

        /** @return Order price (Constants::InvalidPrice for market orders). */
        Price GetPrice() const {return price_;}

        /** @return Type of the order (Market, Limit, etc.). */
        OrderType GetOrderType() const { return orderType_; }

        /** @return Original total quantity of the order. */
        Quantity GetInitialQuantity() const { return initialQuantity_; }

        /** @return Quantity still unfilled. */
        Quantity GetRemainingQuantity() const {return remainingQuantity_;}

        /** @return Quantity that has been filled so far. */
        Quantity GetFilledQuantity() const { return GetInitialQuantity() - GetRemainingQuantity(); }

        /** @return True if the order is completely filled. */
        bool IsFilled() const { return GetRemainingQuantity() == 0; }

        /**
         * @brief Fills a portion of the order.
         * @param quantity Amount to fill (must not exceed remaining quantity)
         * @throws std::logic_error if quantity > remaining quantity
         */
        void Fill (Quantity quantity) {
            if (quantity > GetRemainingQuantity()) {
                std::stringstream ss;
                ss << "Order (" << orderId_ << ") cannot be filled with quantity (" << quantity << ") greater than remaining quantity (" << GetRemainingQuantity() << ")";
                throw std::logic_error(ss.str());
            }

            remainingQuantity_ -= quantity;
        }

        /**
         * @brief Converts a market order into a Good‑Till‑Cancel order with a specified price.
         * @param price New limit price for the order
         * @throws std::logic_error if the order is not a market order
         */
        void ToGoodTillCancel (Price price) {
            if (GetOrderType () != OrderType::Market) {
                std::stringstream ss;
                ss << "Order (" << orderId_ << ") cannot have its price adjusted, only market orders can.";
                throw std::logic_error(ss.str());
            }

            price_ = price;
            orderType_ = OrderType::GoodTillCancel;
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

