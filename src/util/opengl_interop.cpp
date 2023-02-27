#include "opengl_interop.h"

#ifdef _WIN32
#define UUID_DEFINED
#include <d3d11.h>
#include "halley/src/plugins/dx11/src/dx11_texture.h"
#endif
#include "halley/src/plugins/dx11/src/dx11_render_target_texture.h"
#include "halley/src/plugins/dx11/src/dx11_video.h"
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



OpenGLInterop::OpenGLInterop(std::shared_ptr<GLContext> context, VideoAPI& video)
	: video(video)
	, context(std::move(context))
{
	this->context->bind();
	VideoOpenGL::initGLBindings();

	auto& dx11Video = static_cast<DX11Video&>(video);
	deviceHandle = GL_FUNC(wglDXOpenDeviceNV)(&dx11Video.getDevice());
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

std::shared_ptr<OpenGLInteropRenderTarget> OpenGLInterop::makeNativeRenderTarget(Vector2i size)
{
	RenderSurfaceOptions options;
	options.powerOfTwo = false;
	options.canBeUpdatedOnCPU = true;
	auto renderSurface = std::make_shared<RenderSurface>(video, options);
	renderSurface->setSize(size);
	dynamic_cast<DX11TextureRenderTarget&>(renderSurface->getRenderTarget()).update();

	return std::shared_ptr<OpenGLInteropRenderTarget>(new OpenGLInteropRenderTarget(*this, std::move(renderSurface)));
}

std::shared_ptr<OpenGLInteropPixelCopy> OpenGLInterop::makeInterop(std::shared_ptr<CPUUpdateTexture> cpuUpdateTexture)
{
	return std::shared_ptr<OpenGLInteropPixelCopy>(new OpenGLInteropPixelCopy(std::move(cpuUpdateTexture)));
}


OpenGLInteropRenderTarget::OpenGLInteropRenderTarget(OpenGLInterop& parent, std::shared_ptr<RenderSurface> renderSurface)
	: parent(parent)
	, renderSurface(renderSurface)
{
	init();
}

void OpenGLInteropRenderTarget::init()
{
	auto& renderTarget = renderSurface->getRenderTarget();
	const auto dx11TextureCol = dynamic_cast<DX11Texture&>(*renderTarget.getTexture(0)).getTexture();
	const auto dx11TextureDepth = dynamic_cast<DX11Texture&>(*renderTarget.getDepthTexture()).getTexture();

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
	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glCheckError();
	unlock();
}

void* OpenGLInteropRenderTarget::getGLProcAddress(const char* name)
{
	return parent.getGLProcAddress(name);
}


OpenGLInteropRenderTarget::~OpenGLInteropRenderTarget()
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

uint32_t OpenGLInteropRenderTarget::lock()
{
	if (lockCount++ == 0) {
		const bool result = GL_FUNC(wglDXLockObjectsNV)(parent.deviceHandle, 2, handle.data());
		assert(result);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, glFramebuffer);
	return glFramebuffer;
	//return 0;
}

void OpenGLInteropRenderTarget::unlock()
{
	if (--lockCount == 0) {
		const bool result = GL_FUNC(wglDXUnlockObjectsNV)(parent.deviceHandle, 2, handle.data());
		assert(result);
	}
}

void OpenGLInteropRenderTarget::unlockAll()
{
	glFlush();
	if (lockCount > 0) {
		const bool result = GL_FUNC(wglDXUnlockObjectsNV)(parent.deviceHandle, 2, handle.data());
		assert(result);
		lockCount = 0;
	}
}

std::shared_ptr<Texture> OpenGLInteropRenderTarget::getTexture()
{
	return renderSurface->getRenderTarget().getTexture(0);
}


OpenGLInteropPixelCopy::OpenGLInteropPixelCopy(std::shared_ptr<CPUUpdateTexture> cpuUpdateTexture)
{
	
}

OpenGLInteropPixelCopy::~OpenGLInteropPixelCopy()
{
	
}

uint32_t OpenGLInteropPixelCopy::lock()
{
	return 0;
}

void OpenGLInteropPixelCopy::unlock()
{
}

void OpenGLInteropPixelCopy::unlockAll()
{
}

std::shared_ptr<Texture> OpenGLInteropPixelCopy::getTexture()
{
	return {};
}
