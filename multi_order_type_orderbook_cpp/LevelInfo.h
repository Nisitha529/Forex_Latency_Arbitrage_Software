#pragma once

#include "Usings.h"

// Represents a price level (price and quantity) and a collection of levels.

struct LevelInfo {
    Price price_;
    Quantity quantity_;
};

using LevelInfos = std::vector<LevelInfo>;