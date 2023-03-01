#pragma once

#include <halley.hpp>
using namespace Halley;

class DX11State {
public:
    class IStateInternal {
    public:
        virtual ~IStateInternal() = default;
    };

	void save(VideoAPI& video);
    void load(VideoAPI& video);

private:
    std::unique_ptr<IStateInternal> state;
};
