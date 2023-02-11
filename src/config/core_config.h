#include <halley.hpp>
using namespace Halley;

class CoreConfig {
public:
    CoreConfig() = default;
    CoreConfig(const ConfigNode& node);
};
