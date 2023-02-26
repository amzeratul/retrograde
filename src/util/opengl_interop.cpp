#include "opengl_interop.h"

#ifdef _WIN32
#define UUID_DEFINED
#include <d3d11.h>
#include "halley/src/plugins/dx11/src/dx11_texture.h"
#endif
#include "halley/src/plugins/opengl/src/halley_gl.h"
#include "halley/src/plugins/opengl/src/video_opengl.h"


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
	this->context->bind();
	VideoOpenGL::initGLBindings();
	deviceHandle = GL_FUNC(wglDXOpenDeviceNV)(dx11Device);
}

OpenGLInterop::~OpenGLInterop()
{
	if (deviceHandle) {
		GL_FUNC(wglDXCloseDeviceNV)(deviceHandle);
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

std::shared_ptr<OpenGLInteropObject> OpenGLInterop::makeInterop(std::shared_ptr<TextureRenderTarget> renderTarget)
{
	return std::shared_ptr<OpenGLInteropObject>(new OpenGLInteropObject(*this, std::move(renderTarget)));
}


OpenGLInteropObject::OpenGLInteropObject(OpenGLInterop& parent, std::shared_ptr<TextureRenderTarget> renderTarget)
	: parent(parent)
	, renderTarget(renderTarget)
{
	init();
}

void OpenGLInteropObject::init()
{
	const auto dx11TextureCol = dynamic_cast<DX11Texture&>(*renderTarget->getTexture(0)).getTexture();
	const auto dx11TextureDepth = dynamic_cast<DX11Texture&>(*renderTarget->getDepthTexture()).getTexture();

	//parent.context->bind();

	glGenTextures(2, glRenderbuffer.data());
	handle[0] = GL_FUNC(wglDXRegisterObjectNV)(parent.deviceHandle, dx11TextureCol, glRenderbuffer[0], GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV);
	handle[1] = GL_FUNC(wglDXRegisterObjectNV)(parent.deviceHandle, dx11TextureDepth, glRenderbuffer[1], GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV);
	assert(handle[0]);
	assert(handle[1]);

	lock();

	glGenFramebuffers(1, &glFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, glFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, glRenderbuffer[0], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, glRenderbuffer[1], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, glRenderbuffer[1], 0);
	const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glCheckError();
	assert(status == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, glFramebuffer);
	glCheckError();
	glViewport(0, 0, renderTarget->getTexture(0)->getSize().x, renderTarget->getTexture(0)->getSize().y);
	glCheckError();
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glCheckError();
	glDisable(GL_SCISSOR_TEST | GL_DEPTH_TEST);
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_CLEAR_VALUE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glFlush();
	glCheckError();

	unlock();
}

void* OpenGLInteropObject::getGLProcAddress(const char* name)
{
	return parent.getGLProcAddress(name);
}

OpenGLInteropObject::~OpenGLInteropObject()
{
	if (handle[0]) {
		unlockAll();
		GL_FUNC(wglDXUnregisterObjectNV)(parent.deviceHandle, handle[0]);
	}
	if (handle[1]) {
		GL_FUNC(wglDXUnregisterObjectNV)(parent.deviceHandle, handle[1]);
	}
	glDeleteTextures(2, glRenderbuffer.data());
	glDeleteFramebuffers(1, &glFramebuffer);
	handle.fill(nullptr);
	glRenderbuffer.fill(0);
}

uint32_t OpenGLInteropObject::lock()
{
	if (lockCount++ == 0) {
		const bool result = GL_FUNC(wglDXLockObjectsNV)(parent.deviceHandle, 2, handle.data());
		assert(result);
	}
	return glFramebuffer;
	//return 0;
}

void OpenGLInteropObject::unlock()
{
	if (--lockCount == 0) {
		const bool result = GL_FUNC(wglDXUnlockObjectsNV)(parent.deviceHandle, 2, handle.data());
		assert(result);
	}
}

void OpenGLInteropObject::unlockAll()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if (lockCount > 0) {
		const bool result = GL_FUNC(wglDXUnlockObjectsNV)(parent.deviceHandle, 2, handle.data());
		assert(result);
		lockCount = 0;
	}
}

bool OpenGLInteropObject::isLocked() const
{
	return lockCount > 0;
}
