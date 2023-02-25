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

    std::shared_ptr<OpenGLInteropObject> makeInterop(std::shared_ptr<Texture> texture);

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

private:
    OpenGLInteropObject(OpenGLInterop& parent, std::shared_ptr<Texture> texture);

	void* getGLProcAddress(const char* name);

    OpenGLInterop& parent;
    void* handle = nullptr;
    uint32_t glName = 0;
    int lockCount = 0;
    std::shared_ptr<Texture> texture;
};
