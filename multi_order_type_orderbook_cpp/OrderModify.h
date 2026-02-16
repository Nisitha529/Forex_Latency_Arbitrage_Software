#pragma once

#include "Order.h"

/**
 * @brief Represents a request to modify an existing order.
 * 
 * Contains the new parameters (side, price, quantity) for an order identified by its ID.
 * Can be converted back into a full Order object with a specified order type.
 */
class OrderModify {
    public:

        /**
         * @brief Constructs a modification request.
         * @param orderId ID of the order to modify.
         * @param side New side (buy/sell).
         * @param price New price.
         * @param quantity New quantity.
         */
        OrderModify (OrderId orderId, Side side, Price price, Quantity quantity)
            : orderId_ { orderId }
            , price_ {price }
            , side_ { side }
            , quantity_ { quantity }

        { }

        /** @return ID of the order to modify. */
        OrderId GetOrderId() const { return orderId_; }

        /** @return New price for the order. */
        Price GetPrice() const { return price_; }

        /** @return New side for the order. */
        Side GetSide() const { return side_; }

        /** @return New quantity for the order. */
        Quantity GetQuantity() const { return quantity_; }

        /**
         * @brief Creates a new Order object from this modification request.
         * @param type Order type for the new order (e.g., GoodTillCancel, FillAndKill).
         * @return Shared pointer to the newly created Order.
         */
        OrderPointer ToOrderPointer (OrderType type) const {
            return std::make_shared<Order> (type, GetOrderId(), GetSide(), GetPrice(), GetQuantity());
        }

    private:
        OrderId orderId_;
        Price price_;
        Side side_;
        Quantity quantity_;
};