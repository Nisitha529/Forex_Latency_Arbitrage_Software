#pragma once

#include <limits>

#include "Usings.h"

// Common constants, including an invalid price sentinel (NaN).

struct Constants
{
    static const Price InvalidPrice = std::numeric_limits<Price>::quiet_NaN();
};
