#pragma once
#include "materialized_tile.hpp"
#include <functional>
#include <optional>
#include <string>

namespace hypervlm {
using GenFn = std::function<std::optional<MaterializedTilePtr>(BlockId const&)>;
} // namespace hypervlm
