#pragma once

#include <halley.hpp>

#include "src/util/dll.h"
using namespace Halley;

class ILibretroCoreCallbacks {
public:
	virtual ~ILibretroCoreCallbacks() = default;

	virtual bool onEnvironment(unsigned cmd, void* data) = 0;
	virtual void onVideoRefresh(const void* data, unsigned width, unsigned height, size_t size) = 0;
	virtual void onAudioSample(int16_t left, int16_t int16) = 0;
	virtual size_t onAudioSampleBatch(const int16_t* data, size_t size) = 0;
	virtual void onInputPoll() = 0;
	virtual int16_t onInputState(unsigned port, unsigned device, unsigned index, unsigned id) = 0;
};

class LibretroCore : protected ILibretroCoreCallbacks {
public:
	static std::unique_ptr<LibretroCore> load(std::string_view filename);
	~LibretroCore() override;

	void run();

protected:
	bool onEnvironment(unsigned cmd, void* data) override;
	void onVideoRefresh(const void* data, unsigned width, unsigned height, size_t size) override;
	void onAudioSample(int16_t left, int16_t int16) override;
	size_t onAudioSampleBatch(const int16_t* data, size_t size) override;
	void onInputPoll() override;
	int16_t onInputState(unsigned port, unsigned device, unsigned index, unsigned id) override;

private:
	DLL dll;

	explicit LibretroCore(DLL dll);

	void init();
	void deInit();
};
