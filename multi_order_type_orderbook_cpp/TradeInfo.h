#pragma once

#include "Usings.h"

/**
 * @brief Represents the trade information for one participant (bid or ask).
 * 
 * This struct holds the essential details of a matched order: the order ID,
 * the price at which the trade occurred, and the quantity exchanged.
 */
struct TradeInfo {
    OrderId orderId_;
    Price price_;
    Quantity quantity_;
};