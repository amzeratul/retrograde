#pragma once

#include <halley.hpp>

#include "dll.h"
using namespace Halley;

class OpenGLInteropObject;

class OpenGLInterop {
    friend class OpenGLInteropObject;
public:
    OpenGLInterop(std::shared_ptr<GLContext> context, void* dx11Device);
    ~OpenGLInterop();

	void bindGLContext();
    void* getGLProcAddress(const char* name);

    std::shared_ptr<OpenGLInteropObject> makeInterop(std::shared_ptr<TextureRenderTarget> renderTarget);

private:
    std::shared_ptr<GLContext> context;
    DLL openGLDll;

    void* deviceHandle = nullptr;
};


class OpenGLInteropObject {
    friend class OpenGLInterop;
public:
    ~OpenGLInteropObject();

    uint32_t lock();
    void unlock();
    void unlockAll();
    bool isLocked() const;

private:
    OpenGLInteropObject(OpenGLInterop& parent, std::shared_ptr<TextureRenderTarget> renderTarget);

    void init();
	void* getGLProcAddress(const char* name);

    OpenGLInterop& parent;
    std::array<void*, 2> handle;
    std::array<uint32_t, 2> glRenderbuffer;
    uint32_t glFramebuffer = 0;
    int lockCount = 0;
    std::shared_ptr<TextureRenderTarget> renderTarget;
};
