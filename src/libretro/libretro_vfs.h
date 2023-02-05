#pragma once

#include <halley.hpp>

#include "libretro.h"
using namespace Halley;

class LibretroVFS;

class LibretroVFSFileHandle {
public:
	virtual ~LibretroVFSFileHandle() = default;

	virtual const char* getPath() const = 0;

	virtual int close() = 0;
	virtual int flush() = 0;

	virtual int64_t size() const = 0;
	virtual int64_t tell() const = 0;
	virtual int64_t seek(int64_t offset, int position) = 0;
	virtual int64_t read(gsl::span<std::byte> span) = 0;
	virtual int64_t write(gsl::span<const std::byte> span) = 0;
	virtual int64_t truncate(int64_t size) = 0;
};

class LibretroVFSFileHandleSTDIO : public LibretroVFSFileHandle {
public:
	LibretroVFSFileHandleSTDIO(FILE* fp, String path);

	const char* getPath() const;

	int close();
	int flush();

	int64_t size() const;
	int64_t tell() const;
	int64_t seek(int64_t offset, int position);
	int64_t read(gsl::span<std::byte> span);
	int64_t write(gsl::span<const std::byte> span);
	int64_t truncate(int64_t size);

private:
	String path;
	FILE* fp = nullptr;
	size_t fpSize = 0;
};

class LibretroVFSDirHandle {
public:
	int close();
	bool read();
	const char* dirEntGetName() const;
	bool dirEntIsDir() const;
};

class LibretroVFS {
public:
	static retro_vfs_interface* getLibretroInterface();

	LibretroVFSFileHandle* open(std::string_view path, uint32_t mode, uint32_t hints);
	LibretroVFSDirHandle* openDir(std::string_view dir, bool includeHidden);
	int remove(std::string_view path);
	int rename(std::string_view old_path, std::string_view new_path);
	int stat(std::string_view path, int32_t* size);
	int mkdir(std::string_view dir);
};
