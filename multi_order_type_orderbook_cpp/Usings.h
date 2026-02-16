/**
 * @file Usings.h
 * @brief Type aliases for common domain types to improve code readability and maintainability.
 */

#pragma once
#include <cstdint>

#include <vector>

using Price    = std::int32_t;
using Quantity = std::uint32_t;
using OrderId  = std::uint64_t;
using OrderIds = std::vector<OrderId>;