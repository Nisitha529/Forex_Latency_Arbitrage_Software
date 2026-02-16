#pragma once

#include "TradeInfo.h"

/**
 * @brief Represents a single trade resulting from matching a bid and an ask.
 * 
 * Contains the trade details for both the buy side and the sell side.
 */
class Trade {
    public:

        /**
         * @brief Constructs a trade from the bid and ask trade information.
         * @param bidTrade Details of the bid side of the trade.
         * @param askTrade Details of the ask side of the trade.
         */
        Trade (const TradeInfo& bidTrade, const TradeInfo& askTrade)
            : bidTrade_ {bidTrade}
            , askTrade_ {askTrade}
        { }

        /** @return Trade information for the bid side. */
        const TradeInfo& GetBidTrade() const { return bidTrade_; }

        /** @return Trade information for the ask side. */
        const TradeInfo& GetAskTrade() const { return askTrade_; }

    private:
        TradeInfo bidTrade_;
        TradeInfo askTrade_;
};

using Trades = std::vector<Trade>;