#pragma once

/**
 * @brief Container for a snapshot of the order book's bid and ask levels.
 * 
 * Holds two collections of LevelInfo (price and aggregated quantity) for
 * the buy side and sell side, typically used to display the current market depth.
 */
#include "LevelInfo.h"

class OrderbookLevelInfos {
public:
    /**
     * @brief Constructs a snapshot with given bid and ask levels.
     * @param bids Collection of bid levels (highest price first).
     * @param asks Collection of ask levels (lowest price first).
     */
    OrderbookLevelInfos(const LevelInfos& bids, const LevelInfos& asks)
        : bids_{ bids }
        , asks_{ asks }
    { }

    /** @return Immutable reference to the bid levels. */
    const LevelInfos& GetBids() const { return bids_; }

    /** @return Immutable reference to the ask levels. */
    const LevelInfos& GetAsks() const { return asks_; }

private:
    LevelInfos bids_;
    LevelInfos asks_;
};