#include <vector>
#include <array>

namespace GameMap {
    const size_t LEVEL_SEED = 42;
    const size_t MAP_W = 10;
    const size_t MAP_H = 7;
    const std::array<bool, 7*10> LEVEL_1 {{
		1, 1, 1, 0, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 1, 1,
		1, 1, 0, 0, 0, 1, 1, 0, 0, 0,
		1, 1, 0, 0, 0, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 1, 1, 1, 1, 1, 0, 0,
    }};
}
