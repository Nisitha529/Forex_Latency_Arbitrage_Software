/**
 * @file Orderbook.cpp
 * @brief Implementation of the Orderbook class, handling order management,
 *        matching, and pruning of Good‑For‑Day orders.
 */
#include "Orderbook.h"

#include <numeric>
#include <chrono>
#include <ctime>
#include <optional>
#include <atomic>
#include <thread> 
#include <condition_variable>
#include <cstdint>
#include <mutex>

/**
 * @brief Thread-safe wrapper for localtime (Windows/POSIX compatible).
 * @param t Time to convert.
 * @return std::tm structure with broken-down local time.
 */
std::tm safe_localtime(std::time_t t) {
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);   // POSIX
#endif
    return tm;
}

/**
 * @brief Background thread routine that cancels all Good‑For‑Day orders at 16:00 each day.
 * 
 * Sleeps until the next 16:00, then collects and cancels all GFD orders.
 */
void Orderbook::PruneGoodForDayOrders()
{    
    using namespace std::chrono;
    const auto end = hours(16);

	while (true)
	{
		const auto now = system_clock::now();
		const auto now_c = system_clock::to_time_t(now);
		std::tm now_parts = safe_localtime(now_c);

		if (now_parts.tm_hour >= end.count())
			now_parts.tm_mday += 1;

		now_parts.tm_hour = end.count();
		now_parts.tm_min = 0;
		now_parts.tm_sec = 0;

		auto next = system_clock::from_time_t(mktime(&now_parts));
		auto till = next - now + milliseconds(100);

		{
			std::unique_lock ordersLock{ ordersMutex_ };

			if (shutdown_.load(std::memory_order_acquire) ||
				shutdownConditionVariable_.wait_for(ordersLock, till) == std::cv_status::no_timeout)
				return;
		}

		OrderIds orderIds;

		{
			std::scoped_lock ordersLock{ ordersMutex_ };

			for (const auto& [_, entry] : orders_)
			{
				const auto& [order, _] = entry;

				if (order->GetOrderType() != OrderType::GoodForDay)
					continue;

				orderIds.push_back(order->GetOrderId());
			}
		}

		CancelOrders(orderIds);
	}
}

/**
 * @brief Cancels a list of orders by ID.
 * @param orderIds Collection of order IDs to cancel.
 */
void Orderbook::CancelOrders(OrderIds orderIds)
{
	std::scoped_lock ordersLock{ ordersMutex_ };

	for (const auto& orderId : orderIds)
		CancelOrderInternal(orderId);
}

/**
 * @brief Internal cancellation logic (assumes ordersMutex_ is held).
 * @param orderId ID of the order to cancel.
 */
void Orderbook::CancelOrderInternal(OrderId orderId)
{
	if (!orders_.contains(orderId))
		return;

	const auto [order, iterator] = orders_.at(orderId);
	orders_.erase(orderId);

	if (order->GetSide() == Side::Sell)
	{
		auto price = order->GetPrice();
		auto& orders = asks_.at(price);
		orders.erase(iterator);
		if (orders.empty())
			asks_.erase(price);
	}
	else
	{
		auto price = order->GetPrice();
		auto& orders = bids_.at(price);
		orders.erase(iterator);
		if (orders.empty())
			bids_.erase(price);
	}

	OnOrderCancelled(order);
}

/**
 * @brief Callback when an order is cancelled; updates level data.
 */
void Orderbook::OnOrderCancelled(OrderPointer order)
{
	UpdateLevelData(order->GetPrice(), order->GetRemainingQuantity(), LevelData::Action::Remove);
}

/**
 * @brief Callback when an order is added; updates level data.
 */
void Orderbook::OnOrderAdded(OrderPointer order)
{
	UpdateLevelData(order->GetPrice(), order->GetInitialQuantity(), LevelData::Action::Add);
}

/**
 * @brief Callback when an order is matched (partially or fully).
 * @param price Price level where match occurred.
 * @param quantity Quantity matched.
 * @param isFullyFilled True if the order was completely filled.
 */
void Orderbook::OnOrderMatched(Price price, Quantity quantity, bool isFullyFilled)
{
	UpdateLevelData(price, quantity, isFullyFilled ? LevelData::Action::Remove : LevelData::Action::Match);
}

/**
 * @brief Updates aggregated quantity and order count for a price level.
 * @param price The price level.
 * @param quantity Quantity change.
 * @param action Type of action (Add, Remove, Match).
 */
void Orderbook::UpdateLevelData(Price price, Quantity quantity, LevelData::Action action)
{
	auto& data = data_[price];

	data.count_ += action == LevelData::Action::Remove ? -1 : action == LevelData::Action::Add ? 1 : 0;
	if (action == LevelData::Action::Remove || action == LevelData::Action::Match)
	{
		data.quantity_ -= quantity;
	}
	else
	{
		data.quantity_ += quantity;
	}

	if (data.count_ == 0)
		data_.erase(price);
}

/**
 * @brief Checks whether a Fill‑Or‑Kill order can be fully filled.
 * @param side Buy or sell.
 * @param price Limit price.
 * @param quantity Order quantity.
 * @return True if the total quantity across all matching levels is sufficient.
 */
bool Orderbook::CanFullyFill(Side side, Price price, Quantity quantity) const
{
	if (!CanMatch(side, price))
		return false;

	std::optional<Price> threshold;

	if (side == Side::Buy)
	{
		const auto [askPrice, _] = *asks_.begin();
		threshold = askPrice;
	}
	else
	{
		const auto [bidPrice, _] = *bids_.begin();
		threshold = bidPrice;
	}

	for (const auto& [levelPrice, levelData] : data_)
	{
		if (threshold.has_value() &&
			(side == Side::Buy && threshold.value() > levelPrice) ||
			(side == Side::Sell && threshold.value() < levelPrice))
			continue;

		if ((side == Side::Buy && levelPrice > price) ||
			(side == Side::Sell && levelPrice < price))
			continue;

		if (quantity <= levelData.quantity_)
			return true;

		quantity -= levelData.quantity_;
	}

	return false;
}

/**
 * @brief Checks whether an order can be matched at all at the given price.
 * @param side Buy or sell.
 * @param price Limit price.
 * @return True if there is an opposing order at a matching price.
 */
bool Orderbook::CanMatch(Side side, Price price) const
{
	if (side == Side::Buy)
	{
		if (asks_.empty())
			return false;

		const auto& [bestAsk, _] = *asks_.begin();
		return price >= bestAsk;
	}
	else
	{
		if (bids_.empty())
			return false;

		const auto& [bestBid, _] = *bids_.begin();
		return price <= bestBid;
	}
}

/**
 * @brief Matches orders at the current best bid/ask until no further matches are possible.
 * @return List of trades generated.
 */
Trades Orderbook::MatchOrders()
{
	Trades trades;
	trades.reserve(orders_.size());

	while (true)
	{
		if (bids_.empty() || asks_.empty())
			break;

		auto& [bidPrice, bids] = *bids_.begin();
		auto& [askPrice, asks] = *asks_.begin();

		if (bidPrice < askPrice)
			break;

		while (!bids.empty() && !asks.empty())
		{
			auto bid = bids.front();
			auto ask = asks.front();

			Quantity quantity = std::min(bid->GetRemainingQuantity(), ask->GetRemainingQuantity());

			bid->Fill(quantity);
			ask->Fill(quantity);

			if (bid->IsFilled())
			{
				bids.pop_front();
				orders_.erase(bid->GetOrderId());
			}

			if (ask->IsFilled())
			{
				asks.pop_front();
				orders_.erase(ask->GetOrderId());
			}


			trades.push_back(Trade{
				TradeInfo{ bid->GetOrderId(), bid->GetPrice(), quantity },
				TradeInfo{ ask->GetOrderId(), ask->GetPrice(), quantity } 
				});

			OnOrderMatched(bid->GetPrice(), quantity, bid->IsFilled());
			OnOrderMatched(ask->GetPrice(), quantity, ask->IsFilled());
		}

        if (bids.empty())
        {
            bids_.erase(bidPrice);
            data_.erase(bidPrice);
        }

        if (asks.empty())
        {
            asks_.erase(askPrice);
            data_.erase(askPrice);
        }
	}

	// Cancel any remaining Fill‑And‑Kill orders at the top of the book
	if (!bids_.empty())
	{
		auto& [_, bids] = *bids_.begin();
		auto& order = bids.front();
		if (order->GetOrderType() == OrderType::FillAndKill)
			CancelOrder(order->GetOrderId());
	}

	if (!asks_.empty())
	{
		auto& [_, asks] = *asks_.begin();
		auto& order = asks.front();
		if (order->GetOrderType() == OrderType::FillAndKill)
			CancelOrder(order->GetOrderId());
	}

	return trades;
}

/**
 * @brief Constructor – starts the background pruning thread.
 */
Orderbook::Orderbook() : ordersPruneThread_{ [this] { PruneGoodForDayOrders(); } } { }

/**
 * @brief Destructor – signals shutdown and joins the pruning thread.
 */
Orderbook::~Orderbook()
{
    shutdown_.store(true, std::memory_order_release);
	shutdownConditionVariable_.notify_one();
	ordersPruneThread_.join();
}

/**
 * @brief Adds an order to the book, performs necessary conversions, and attempts to match.
 * @param order The order to add.
 * @return List of trades resulting from this addition.
 */
Trades Orderbook::AddOrder(OrderPointer order)
{
	std::scoped_lock ordersLock{ ordersMutex_ };

	if (orders_.contains(order->GetOrderId()))
		return { };

	// Convert market orders to Good‑Till‑Cancel with the worst opposite price
	if (order->GetOrderType() == OrderType::Market)
	{
		if (order->GetSide() == Side::Buy && !asks_.empty())
		{
			const auto& [worstAsk, _] = *asks_.rbegin();
			order->ToGoodTillCancel(worstAsk);
		}
		else if (order->GetSide() == Side::Sell && !bids_.empty())
		{
			const auto& [worstBid, _] = *bids_.rbegin();
			order->ToGoodTillCancel(worstBid);
		}
		else
			return { };
	}

	// Immediate‑or‑cancel checks
	if (order->GetOrderType() == OrderType::FillAndKill && !CanMatch(order->GetSide(), order->GetPrice()))
		return { };

	if (order->GetOrderType() == OrderType::FillOrKill && !CanFullyFill(order->GetSide(), order->GetPrice(), order->GetInitialQuantity()))
		return { };

	// Insert order into the appropriate side's price level
	OrderPointers::iterator iterator;

	if (order->GetSide() == Side::Buy)
	{
		auto& orders = bids_[order->GetPrice()];
		orders.push_back(order);
		iterator = std::prev(orders.end());
	}
	else
	{
		auto& orders = asks_[order->GetPrice()];
		orders.push_back(order);
		iterator = std::prev(orders.end());
	}

	orders_.insert({ order->GetOrderId(), OrderEntry{ order, iterator } });
	
	OnOrderAdded(order);
	
	return MatchOrders();

}

/**
 * @brief Cancels a single order by ID.
 * @param orderId ID of the order to cancel.
 */
void Orderbook::CancelOrder(OrderId orderId)
{
	std::scoped_lock ordersLock{ ordersMutex_ };

	CancelOrderInternal(orderId);
}

/**
 * @brief Modifies an existing order (cancel + add new).
 * @param order Modification details.
 * @return Trades resulting from the modified order.
 */
Trades Orderbook::ModifyOrder(OrderModify order)
{
	OrderType orderType;

	{
		std::scoped_lock ordersLock{ ordersMutex_ };

		if (!orders_.contains(order.GetOrderId()))
			return { };

		const auto& [existingOrder, _] = orders_.at(order.GetOrderId());
		orderType = existingOrder->GetOrderType();
	}

	CancelOrder(order.GetOrderId());
	return AddOrder(order.ToOrderPointer(orderType));
}

/**
 * @brief Returns the total number of orders currently in the book.
 */
std::size_t Orderbook::Size() const
{
	std::scoped_lock ordersLock{ ordersMutex_ };
	return orders_.size(); 
}

/**
 * @brief Constructs a snapshot of the current order book (bids and asks with aggregated quantities).
 * @return OrderbookLevelInfos containing bid and ask levels.
 */
OrderbookLevelInfos Orderbook::GetOrderInfos() const
{
	LevelInfos bidInfos, askInfos;
	bidInfos.reserve(orders_.size());
	askInfos.reserve(orders_.size());

	auto CreateLevelInfos = [](Price price, const OrderPointers& orders)
	{
		return LevelInfo{ price, std::accumulate(orders.begin(), orders.end(), (Quantity)0,
			[](Quantity runningSum, const OrderPointer& order)
			{ return runningSum + order->GetRemainingQuantity(); }) };
	};

	for (const auto& [price, orders] : bids_)
		bidInfos.push_back(CreateLevelInfos(price, orders));

	for (const auto& [price, orders] : asks_)
		askInfos.push_back(CreateLevelInfos(price, orders));

	return OrderbookLevelInfos{ bidInfos, askInfos };

}