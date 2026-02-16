/**
 * @brief Defines the lifespan and execution behavior of an order.
 * 
 * - GoodTillCancel: Remains active until explicitly cancelled.
 * - FillAndKill: Must be filled immediately; any unfilled portion is cancelled.
 * - FillOrKill: Must be filled completely immediately; otherwise cancelled.
 * - GoodForDay: Active only until the end of the trading day.
 * - Market: Executes immediately at the best available price (no limit).
 */
enum class OrderType {
    GoodTillCancel,
    FillAndKill,
    FillOrKill,
    GoodForDay,
    Market
};