#include <halley.hpp>
using namespace Halley;

class CoreConfig {
public:
    CoreConfig() = default;
    CoreConfig(const ConfigNode& node);

    const HashMap<String, String>& getOptions() const;

private:
    HashMap<String, String> options;
};
