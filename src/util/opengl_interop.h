#pragma once

#include <halley.hpp>

#include "dll.h"
class OpenGLInteropPixelCopy;
class CPUUpdateTexture;
using namespace Halley;

class OpenGLInteropObject;
class OpenGLInteropRenderTarget;

class OpenGLInterop {
    friend class OpenGLInteropRenderTarget;
public:
    OpenGLInterop(std::shared_ptr<GLContext> context, VideoAPI& video);
    ~OpenGLInterop();

	void bindGLContext();
    void* getGLProcAddress(const char* name);
    void* getGLProcAddressOrDie(const char* name);

    std::shared_ptr<OpenGLInteropRenderTarget> makeNativeRenderTarget(Vector2i size);
    std::shared_ptr<OpenGLInteropPixelCopy> makeCopyPixelRenderTarget(Vector2i size);

private:
    VideoAPI& video;
    std::shared_ptr<GLContext> context;
    DLL openGLDll;

    void* deviceHandle = nullptr;
};

class OpenGLInteropObject {
public:
    virtual ~OpenGLInteropObject() = default;

    virtual uint32_t lock() = 0;
    virtual void unlock() = 0;
    virtual void unlockAll() = 0;
    virtual std::shared_ptr<Texture> getTexture() = 0;
};

class OpenGLInteropRenderTarget final : public OpenGLInteropObject {
    friend class OpenGLInterop;
public:
    ~OpenGLInteropRenderTarget() override;

    uint32_t lock() override;
    void unlock() override;
    void unlockAll() override;
    std::shared_ptr<Texture> getTexture() override;

private:
    OpenGLInteropRenderTarget(OpenGLInterop& parent, std::shared_ptr<RenderSurface> renderSurface);

    void init();
	void* getGLProcAddress(const char* name);
	void* getGLProcAddressOrDie(const char* name);

    OpenGLInterop& parent;
    std::array<void*, 2> handle;
    std::array<uint32_t, 2> glRenderbuffer;
    uint32_t glFramebuffer = 0;
    int lockCount = 0;
    std::shared_ptr<RenderSurface> renderSurface;
};


class OpenGLInteropPixelCopy final : public OpenGLInteropObject {
    friend class OpenGLInterop;
public:
    ~OpenGLInteropPixelCopy() override;

	uint32_t lock() override;
	void unlock() override;
	void unlockAll() override;
    std::shared_ptr<Texture> getTexture() override;

private:
    OpenGLInteropPixelCopy(std::shared_ptr<CPUUpdateTexture> cpuUpdateTexture);

    std::shared_ptr<CPUUpdateTexture> cpuUpdateTexture;
};
