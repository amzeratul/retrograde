#include "opengl_interop.h"

#ifdef _WIN32
#define UUID_DEFINED
#include <d3d11.h>
#include "halley/src/plugins/dx11/src/dx11_texture.h"
#endif
#include "halley/src/plugins/opengl/src/halley_gl.h"


HANDLE wglDXOpenDeviceNV(void *dxDevice);
BOOL wglDXCloseDeviceNV(HANDLE hDevice);
HANDLE wglDXRegisterObjectNV(HANDLE hDevice, void *dxObject, GLuint name, GLenum type, GLenum access);
BOOL wglDXUnregisterObjectNV(HANDLE hDevice, HANDLE hObject);
BOOL wglDXObjectAccessNV(HANDLE hObject, GLenum access);
BOOL wglDXLockObjectsNV(HANDLE hDevice, GLint count, HANDLE *hObjects);
BOOL wglDXUnlockObjectsNV(HANDLE hDevice, GLint count, HANDLE *hObjects);
extern void _glGenRenderbuffers(GLsizei n, GLuint * renderbuffers);

namespace {
	constexpr int WGL_ACCESS_READ_ONLY_NV = 0x0000;
	constexpr int WGL_ACCESS_READ_WRITE_NV = 0x0001;
	constexpr int WGL_ACCESS_WRITE_DISCARD_NV = 0x0002;
}

#define GL_FUNC(FUNC_NAME) static_cast<decltype(&(FUNC_NAME))>(getGLProcAddress(#FUNC_NAME))
#define GL_FUNC_ALT(FUNC_NAME, FUNC_SIGNATURE) static_cast<decltype(&(FUNC_SIGNATURE))>(getGLProcAddress(#FUNC_NAME))


OpenGLInterop::OpenGLInterop(std::shared_ptr<GLContext> context, void* dx11Device)
	: context(std::move(context))
{
	deviceHandle = GL_FUNC(wglDXOpenDeviceNV)(dx11Device);
}

OpenGLInterop::~OpenGLInterop()
{
	if (deviceHandle) {
		GL_FUNC(wglDXOpenDeviceNV)(deviceHandle);
	}
	deviceHandle = nullptr;
	context.reset();
}


void OpenGLInterop::bindGLContext()
{
	context->bind();
}

void* OpenGLInterop::getGLProcAddress(const char* name)
{
	return context->getGLProcAddress(name);
}

std::shared_ptr<OpenGLInteropObject> OpenGLInterop::makeInterop(std::shared_ptr<Texture> texture)
{
	return std::shared_ptr<OpenGLInteropObject>(new OpenGLInteropObject(*this, std::move(texture)));
}


OpenGLInteropObject::OpenGLInteropObject(OpenGLInterop& parent, std::shared_ptr<Texture> texture)
	: parent(parent)
	, texture(texture)
{
	parent.context->bind();
	const auto dx11Texture = dynamic_cast<DX11Texture&>(*texture).getTexture();
	GL_FUNC_ALT(glGenRenderbuffers, _glGenRenderbuffers)(1, &glName);
	handle = GL_FUNC(wglDXRegisterObjectNV)(parent.deviceHandle, dx11Texture, glName, GL_RENDERBUFFER, WGL_ACCESS_READ_WRITE_NV);
	assert(handle);
}

void* OpenGLInteropObject::getGLProcAddress(const char* name)
{
	return parent.getGLProcAddress(name);
}

OpenGLInteropObject::~OpenGLInteropObject()
{
	if (handle) {
		unlockAll();
		GL_FUNC(wglDXUnregisterObjectNV)(parent.deviceHandle, handle);
	}
	handle = nullptr;
	glDeleteRenderbuffers(1, &glName);
	glName = 0;
}

uint32_t OpenGLInteropObject::lock()
{
	if (lockCount++ == 0) {
		const bool result = GL_FUNC(wglDXLockObjectsNV)(parent.deviceHandle, 1, &handle);
		assert(result);
	}
	return glName;
}

void OpenGLInteropObject::unlock()
{
	if (--lockCount == 0) {
		const bool result = GL_FUNC(wglDXUnlockObjectsNV)(parent.deviceHandle, 1, &handle);
		assert(result);
	}
}

void OpenGLInteropObject::unlockAll()
{
	if (lockCount > 0) {
		const bool result = GL_FUNC(wglDXUnlockObjectsNV)(parent.deviceHandle, 1, &handle);
		assert(result);
		lockCount = 0;
	}
}
