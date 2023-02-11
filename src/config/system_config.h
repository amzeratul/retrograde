#include <halley.hpp>
using namespace Halley;

class SystemConfig {
public:
    SystemConfig() = default;
    SystemConfig(const ConfigNode& node);

	const Vector<String>& getCores() const;

private:
    Vector<String> cores;
};
