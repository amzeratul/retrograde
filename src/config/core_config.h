#include <halley.hpp>
using namespace Halley;

class CoreConfig {
public:
    CoreConfig() = default;
    CoreConfig(const ConfigNode& node);

    const String& getId() const;
    const HashMap<String, String>& getOptions() const;

private:
    String id;
    HashMap<String, String> options;
};
