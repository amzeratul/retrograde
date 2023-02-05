#include "libretro_vfs.h"
#include "libretro.h"
#include "libretro_core.h"

namespace {

	LibretroVFS& getVFS()
	{
		return ILibretroCoreCallbacks::curInstance->getVFS();
	}

	const char* retro_vfs_get_path(retro_vfs_file_handle* stream)
	{
		return reinterpret_cast<LibretroVFSFileHandle*>(stream)->getPath();
	}

	retro_vfs_file_handle* retro_vfs_open(const char* path, uint32_t mode, uint32_t hints)
	{
		return reinterpret_cast<retro_vfs_file_handle*>(getVFS().open(path, mode, hints));
	}

	int retro_vfs_close(retro_vfs_file_handle* stream)
	{
		return reinterpret_cast<LibretroVFSFileHandle*>(stream)->close();
	}

	int64_t retro_vfs_size(retro_vfs_file_handle* stream)
	{
		return reinterpret_cast<LibretroVFSFileHandle*>(stream)->size();
	}

	int64_t retro_vfs_tell(retro_vfs_file_handle* stream)
	{
		return reinterpret_cast<LibretroVFSFileHandle*>(stream)->tell();
	}

	int64_t retro_vfs_seek(retro_vfs_file_handle* stream, int64_t offset, int seek_position)
	{
		return reinterpret_cast<LibretroVFSFileHandle*>(stream)->seek(offset, seek_position);
	}

	int64_t retro_vfs_read(retro_vfs_file_handle* stream, void* s, uint64_t len)
	{
		return reinterpret_cast<LibretroVFSFileHandle*>(stream)->read(gsl::as_writable_bytes(gsl::span<char>(static_cast<char*>(s), len)));
	}

	int64_t retro_vfs_write(retro_vfs_file_handle* stream, const void* s, uint64_t len)
	{
		return reinterpret_cast<LibretroVFSFileHandle*>(stream)->write(gsl::as_bytes(gsl::span<const char>(static_cast<const char*>(s), len)));
	}

	int retro_vfs_flush(retro_vfs_file_handle* stream)
	{
		return reinterpret_cast<LibretroVFSFileHandle*>(stream)->flush();
	}

	int retro_vfs_remove(const char* path)
	{
		return getVFS().remove(path);
	}

	int retro_vfs_rename(const char* old_path, const char* new_path)
	{
		return getVFS().rename(old_path, new_path);
	}

	int64_t retro_vfs_truncate(retro_vfs_file_handle* stream, int64_t length)
	{
		return reinterpret_cast<LibretroVFSFileHandle*>(stream)->truncate(length);
	}

	int retro_vfs_stat(const char *path, int32_t *size)
	{
		return getVFS().stat(path, *size);
	}

	int retro_vfs_mkdir(const char *dir)
	{
		return getVFS().mkdir(dir);
	}

	retro_vfs_dir_handle* retro_vfs_opendir(const char *dir, bool include_hidden)
	{
		return reinterpret_cast<retro_vfs_dir_handle*>(getVFS().openDir(dir, include_hidden));
	}

	bool retro_vfs_readdir(retro_vfs_dir_handle *dirstream)
	{
		return reinterpret_cast<LibretroVFSDirHandle*>(dirstream)->read();
	}

	const char* retro_vfs_dirent_get_name(retro_vfs_dir_handle *dirstream)
	{
		return reinterpret_cast<LibretroVFSDirHandle*>(dirstream)->dirEntGetName();
	}

	bool retro_vfs_dirent_is_dir(retro_vfs_dir_handle *dirstream)
	{
		return reinterpret_cast<LibretroVFSDirHandle*>(dirstream)->dirEntIsDir();
	}

	int retro_vfs_closedir(retro_vfs_dir_handle *dirstream)
	{
		return reinterpret_cast<LibretroVFSDirHandle*>(dirstream)->close();
	}

	retro_vfs_interface makeRetroVFSInterface()
	{
		retro_vfs_interface result = {};

		// VFS API v1
		result.get_path = &retro_vfs_get_path;
		result.open = &retro_vfs_open;
		result.close = &retro_vfs_close;
		result.size = &retro_vfs_size;
		result.tell = &retro_vfs_tell;
		result.seek = &retro_vfs_seek;
		result.read = &retro_vfs_read;
		result.write = &retro_vfs_write;
		result.flush = &retro_vfs_flush;
		result.remove = &retro_vfs_remove;
		result.rename = &retro_vfs_rename;

		// VFS API v2
		result.truncate = &retro_vfs_truncate;

		// VFS API v3
		result.stat = &retro_vfs_stat;
		result.mkdir = &retro_vfs_mkdir;
		result.opendir = &retro_vfs_opendir;
		result.readdir = &retro_vfs_readdir;
		result.dirent_get_name = &retro_vfs_dirent_get_name;
		result.dirent_is_dir = &retro_vfs_dirent_is_dir;
		result.closedir = &retro_vfs_closedir;

		return result;
	}
}

retro_vfs_interface* LibretroVFS::getLibretroInterface()
{
	static retro_vfs_interface retroVFSInterface = makeRetroVFSInterface();
	return &retroVFSInterface;
}
