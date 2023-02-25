#include "opengl_interop.h"

#ifdef _WIN32
#define UUID_DEFINED
#include <d3d11.h>
#include "halley/src/plugins/dx11/src/dx11_texture.h"
#endif
#include "halley/src/plugins/opengl/src/halley_gl.h"


// See
// https://registry.khronos.org/OpenGL/extensions/NV/WGL_NV_DX_interop.txt
// https://registry.khronos.org/OpenGL/extensions/NV/WGL_NV_DX_interop2.txt

HANDLE wglDXOpenDeviceNV(void *dxDevice);
BOOL wglDXCloseDeviceNV(HANDLE hDevice);
HANDLE wglDXRegisterObjectNV(HANDLE hDevice, void *dxObject, GLuint name, GLenum type, GLenum access);
BOOL wglDXUnregisterObjectNV(HANDLE hDevice, HANDLE hObject);
BOOL wglDXObjectAccessNV(HANDLE hObject, GLenum access);
BOOL wglDXLockObjectsNV(HANDLE hDevice, GLint count, HANDLE *hObjects);
BOOL wglDXUnlockObjectsNV(HANDLE hDevice, GLint count, HANDLE *hObjects);

namespace {
	constexpr int WGL_ACCESS_READ_ONLY_NV = 0x0000;
	constexpr int WGL_ACCESS_READ_WRITE_NV = 0x0001;
	constexpr int WGL_ACCESS_WRITE_DISCARD_NV = 0x0002;
}

#define GL_FUNC(FUNC_NAME) static_cast<decltype(&(FUNC_NAME))>(getGLProcAddress(#FUNC_NAME))
#define GL_FUNC_PTR(FUNC_NAME) static_cast<decltype(&*(FUNC_NAME))>(getGLProcAddress(#FUNC_NAME))



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
	const auto dx11Texture = dynamic_cast<DX11Texture&>(*texture).getTexture();

	//parent.context->bind();

	GL_FUNC_PTR(glGenRenderbuffers)(1, &glRenderbuffer0);
	handle = GL_FUNC(wglDXRegisterObjectNV)(parent.deviceHandle, dx11Texture, glRenderbuffer0, GL_RENDERBUFFER, WGL_ACCESS_READ_WRITE_NV);
	assert(handle);

	GL_FUNC_PTR(glGenFramebuffers)(1, &glFramebuffer);
	GL_FUNC_PTR(glBindFramebuffer)(GL_FRAMEBUFFER, glFramebuffer);
	GL_FUNC_PTR(glFramebufferRenderbuffer)(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, glRenderbuffer0);
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
	GL_FUNC_PTR(glDeleteRenderbuffers)(1, &glRenderbuffer0);
	GL_FUNC_PTR(glDeleteFramebuffers)(1, &glFramebuffer);
	glRenderbuffer0 = 0;
}

uint32_t OpenGLInteropObject::lock()
{
	if (lockCount++ == 0) {
		const bool result = GL_FUNC(wglDXLockObjectsNV)(parent.deviceHandle, 1, &handle);
		assert(result);
	}
	GL_FUNC_PTR(glBindFramebuffer)(GL_FRAMEBUFFER, glFramebuffer);
	return glFramebuffer;
}

void OpenGLInteropObject::unlock()
{
	GL_FUNC_PTR(glBindFramebuffer)(GL_FRAMEBUFFER, 0);
	if (--lockCount == 0) {
		const bool result = GL_FUNC(wglDXUnlockObjectsNV)(parent.deviceHandle, 1, &handle);
		assert(result);
	}
}

void OpenGLInteropObject::unlockAll()
{
	GL_FUNC_PTR(glBindFramebuffer)(GL_FRAMEBUFFER, 0);
	if (lockCount > 0) {
		const bool result = GL_FUNC(wglDXUnlockObjectsNV)(parent.deviceHandle, 1, &handle);
		assert(result);
		lockCount = 0;
	}
}
