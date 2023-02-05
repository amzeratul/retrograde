#include "libretro_vfs.h"
#include "libretro.h"
#include "libretro_core.h"
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <filesystem>

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
		return getVFS().stat(path, size);
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

void LibretroVFS::setVirtualFile(Path path, Bytes data)
{
	virtualFiles[path.string()] = std::move(data);
}

void LibretroVFS::clearVirtualFiles()
{
	virtualFiles.clear();
}

LibretroVFSFileHandle* LibretroVFS::open(std::string_view path, uint32_t mode, uint32_t hints)
{
	const bool read = mode & RETRO_VFS_FILE_ACCESS_READ;
	const bool write = mode & RETRO_VFS_FILE_ACCESS_WRITE;
	const bool update = mode & RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING;
	const bool frequentAccess = hints & RETRO_VFS_FILE_ACCESS_HINT_FREQUENT_ACCESS;

	const auto iter = virtualFiles.find(path);
	if (iter != virtualFiles.end()) {
		return openVFile(path, read, write, update, frequentAccess, iter->second);
	} else {
		return openSTDIO(path, read, write, update, frequentAccess);
	}
}

LibretroVFSFileHandleSTDIO* LibretroVFS::openSTDIO(std::string_view path, bool read, bool write, bool update, bool frequentAccess)
{
	const char* modeStr = nullptr;
	if (write) {
		if (read && update) {
			modeStr = "r+b";
		} else if (update) {
			modeStr = "r+b";
		} else if (read) {
			modeStr = "w+b";
		} else {
			modeStr = "wb";
		}
	} else {
		modeStr = "rb";
	}

#ifdef _WIN32
	FILE* fp;
	auto error = _wfopen_s(&fp, String(path).getUTF16().c_str(), String(modeStr).getUTF16().c_str());
	if (error != 0) {
		return nullptr;
	}
#else
	FILE* fp = fopen(path.c_str(), modeStr);
#endif
	if (fp) {
		return new LibretroVFSFileHandleSTDIO(fp, path);
	} else {
		return nullptr;
	}
}

LibretroVFSFileHandleVFile* LibretroVFS::openVFile(std::string_view path, bool read, bool write, bool update, bool frequentAccess, Bytes& data)
{
	if (write && !update) {
		data.clear();
	}
	return new LibretroVFSFileHandleVFile(data, path, read, write);
}

LibretroVFSDirHandle* LibretroVFS::openDir(std::string_view dir, bool includeHidden)
{
	Logger::logWarning("LibretroVFS::openDir is untested!!");

	const Path dirPath = String(dir);
	Vector<LibretroVFSDirHandle::Entry> entries;

	// Eh, this is not great
	for (auto& file: virtualFiles) {
		const auto p = Path(file.first);
		if (p.parentPath() == dirPath) {
			entries.emplace_back(LibretroVFSDirHandle::Entry{ p.getFilename().string(), false });
		}
	}

	for (const auto& e: std::filesystem::directory_iterator(dir)) {
		if (e.is_directory() || e.is_regular_file()) {
			entries.emplace_back(LibretroVFSDirHandle::Entry{ e.path().filename().string(), e.is_directory() });
		}
	}

	return new LibretroVFSDirHandle(std::move(entries));
}

int LibretroVFS::remove(std::string_view path)
{
	std::error_code ec;
	return std::filesystem::remove(path, ec) ? 0 : -1;
}

int LibretroVFS::rename(std::string_view old_path, std::string_view new_path)
{
	std::error_code ec;
	std::filesystem::rename(old_path, new_path, ec);
	return ec.value() == 0 ? 0 : -1;	
}

int LibretroVFS::stat(std::string_view path, int32_t* size)
{
	std::error_code ec;
	const auto status = std::filesystem::status(path, ec);
	if (!ec) {
		return 0;
	}

	if (size) {
		const auto sz = std::filesystem::file_size(path, ec);
		if (!ec) {
			return 0;
		}
		*size = static_cast<int32_t>(sz);
	}

	int result = RETRO_VFS_STAT_IS_VALID;
	if (status.type() == std::filesystem::file_type::directory) {
		result |= RETRO_VFS_STAT_IS_DIRECTORY;
	}

	return result;
}

int LibretroVFS::mkdir(std::string_view dir)
{
	std::error_code ec;
	std::filesystem::create_directories(dir, ec);
	return ec.value() == 0 ? 0 : -1;	
}



LibretroVFSFileHandleSTDIO::LibretroVFSFileHandleSTDIO(FILE* fp, String path)
	: path(std::move(path))
	, fp(fp)
{
#ifdef _WIN32
	_fseeki64(fp, 0, SEEK_END);
	fpSize = _ftelli64(fp);
#else
	fseeko(fp, 0, SEEK_END);
	fpSize = ftello(fp);
#endif
}

const char* LibretroVFSFileHandleSTDIO::getPath() const
{
	return path.c_str();
}

int LibretroVFSFileHandleSTDIO::close()
{
	int result = -1;
	if (fp) {
		result = fclose(fp);
	}
	delete this;
	return result;
}

int LibretroVFSFileHandleSTDIO::flush()
{
	if (fp) {
		return fflush(fp);
	}
	return -1;
}

int64_t LibretroVFSFileHandleSTDIO::size() const
{
	if (fp) {
		return static_cast<int64_t>(fpSize);
	}
	return -1;
}

int64_t LibretroVFSFileHandleSTDIO::tell() const
{
	if (fp) {
#ifdef _WIN32
		return _ftelli64(fp);
#else
		return ftello(fp);
#endif
	}
	return -1;
}

int64_t LibretroVFSFileHandleSTDIO::seek(int64_t offset, int position)
{
	if (fp) {
#ifdef _WIN32
		_fseeki64(fp, offset, position);
#else
		fseeko(fp, offset, position);
#endif
		return tell();
	}
	return -1;
}

int64_t LibretroVFSFileHandleSTDIO::read(gsl::span<std::byte> span)
{
	if (fp) {
		return fread(span.data(), 1, span.size(), fp);
	}
	return -1;
}

int64_t LibretroVFSFileHandleSTDIO::write(gsl::span<const std::byte> span)
{
	if (fp) {
		return fwrite(span.data(), 1, span.size(), fp);
	}
	return -1;
}

int64_t LibretroVFSFileHandleSTDIO::truncate(int64_t sz)
{
	if (fp) {
#ifdef _WIN32
		return _chsize_s(_fileno(fp), sz);
#else
		return ftruncate(fileno(fp), sz);
#endif
	}
	return -1;
}



LibretroVFSFileHandleVFile::LibretroVFSFileHandleVFile(Bytes& fileData, String path, bool canRead, bool canWrite)
	: path(path)
	, bytes(fileData)
	, pos(0)
	, canRead(canRead)
	, canWrite(canWrite)
{
}

const char* LibretroVFSFileHandleVFile::getPath() const
{
	return path.c_str();
}

int LibretroVFSFileHandleVFile::close()
{
	delete this;
	return 0;
}

int LibretroVFSFileHandleVFile::flush()
{
	return 0;
}

int64_t LibretroVFSFileHandleVFile::size() const
{
	return static_cast<int64_t>(bytes.size());
}

int64_t LibretroVFSFileHandleVFile::tell() const
{
	return pos;
}

int64_t LibretroVFSFileHandleVFile::seek(int64_t offset, int position)
{
	if (position == RETRO_VFS_SEEK_POSITION_CURRENT) {
		pos = clamp(pos + offset, 0ll, size());
	} else if (position == RETRO_VFS_SEEK_POSITION_START) {
		pos = clamp(offset, 0ll, size());
	} else if (position == RETRO_VFS_SEEK_POSITION_END) {
		pos = clamp(size() + offset, 0ll, size());
	}
	return pos;
}

int64_t LibretroVFSFileHandleVFile::read(gsl::span<std::byte> span)
{
	const int64_t toRead = std::min(size() - pos, static_cast<int64_t>(span.size()));
	memcpy(span.data(), bytes.data() + pos, toRead);
	pos += toRead;
	return toRead;
}

int64_t LibretroVFSFileHandleVFile::write(gsl::span<const std::byte> span)
{
	const auto toWrite = static_cast<int64_t>(span.size());
	if (pos + toWrite > size()) {
		bytes.resize(pos + toWrite);
	}
	memcpy(bytes.data() + pos, span.data(), toWrite);
	pos += toWrite;
	return toWrite;

}

int64_t LibretroVFSFileHandleVFile::truncate(int64_t size)
{
	bytes.resize(size);
	pos = std::min(pos, static_cast<int64_t>(bytes.size()));
	return 0;
}


LibretroVFSDirHandle::LibretroVFSDirHandle(Vector<Entry> entries)
	: entries(std::move(entries))
{
}

int LibretroVFSDirHandle::close()
{
	delete this;
	return 0;
}

bool LibretroVFSDirHandle::read()
{
	++pos;
	return pos == entries.size();
}

const char* LibretroVFSDirHandle::dirEntGetName() const
{
	return entries[pos - 1].name.c_str();
}

bool LibretroVFSDirHandle::dirEntIsDir() const
{
	return entries[pos - 1].isDir;
}
