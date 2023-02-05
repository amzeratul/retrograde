#include "zip_file.h"
#include "miniz.h"
#include <cstdio>

ZipFile::ZipFile() {}

ZipFile::ZipFile(Path path, bool inMemory)
{
	open(std::move(path), inMemory);
}

ZipFile::~ZipFile()
{
	close();
}

bool ZipFile::open(Path p, bool inMemory)
{
	close();

	path = std::move(p);
	memset(&archive, 0, sizeof(archive));

	if (inMemory) {
		compressedData = Path::readFile(path);
		if (compressedData.empty()) {
			return false;
		}
		isOpen = mz_zip_reader_init_mem(&archive, compressedData.data(), compressedData.size(), 0);
		if (!isOpen) {
			compressedData.clear();
		}
	} else {
#ifdef _WIN32
		FILE* file;
		_wfopen_s(&file, path.toString().getUTF16().c_str(), L"rb");
#else
		file = fopen(path.toString().c_str(), "rb");
#endif
		if (file) {
			fileHandle = file;
			isOpen = mz_zip_reader_init_cfile(&archive, file, 0, 0);
			if (!isOpen) {
				fclose(file);
				file = nullptr;
			}
		} else {
			isOpen = false;
		}
	}

	return isOpen;
}

void ZipFile::close()
{
	compressedData.clear();
	if (isOpen) {
		mz_zip_reader_end(&archive);
		if (fileHandle) {
			fclose(static_cast<FILE*>(fileHandle));
		}
		isOpen = false;
	}
}

size_t ZipFile::getNumFiles() const
{
	if (!isOpen) {
		return 0;
	}

	return mz_zip_reader_get_num_files(&archive);
}

String ZipFile::getFileName(size_t idx) const
{
	if (!isOpen) {
		return {};
	}

	char buffer[512];
	const auto size = mz_zip_reader_get_filename(&archive, mz_uint(idx), buffer, 4096);

	return String(buffer, size - 1);
}

Vector<String> ZipFile::getFileNames() const
{
	if (!isOpen) {
		return {};
	}

	const auto n = getNumFiles();
	Vector<String> result;
	result.reserve(n);

	for (int i = 0; i < n; ++i) {
		result.push_back(getFileName(i));
	}
	return result;
}

Bytes ZipFile::extractFile(size_t idx) const
{
	if (!isOpen) {
		return {};
	}

	mz_zip_archive_file_stat stat;
	auto result = mz_zip_reader_file_stat(&archive, static_cast<mz_uint>(idx), &stat);
	if (!result) {
		return {};
	}

	Bytes bytes;
	bytes.resize(stat.m_uncomp_size);
	result = mz_zip_reader_extract_file_to_mem(&archive, stat.m_filename, bytes.data(), bytes.size(), 0);
	if (result) {
		return bytes;
	} else {
		return {};
	}
}

void ZipFile::printDiagnostics() const
{
	const auto n = getNumFiles();
	for (int i = 0; i < n; ++i) {
		mz_zip_archive_file_stat stat;
		mz_zip_reader_file_stat(&archive, mz_uint(i), &stat);

		union {
			uint32_t crc;
			uint8_t crcBytes[4];
		} c;
		c.crc = stat.m_crc32;
		std::swap(c.crcBytes[0], c.crcBytes[3]);
		std::swap(c.crcBytes[1], c.crcBytes[2]);

		Logger::logDev(String(stat.m_filename) + ": CRC = " + Encode::encodeBase16(gsl::as_bytes(gsl::span<const uint32_t>(&c.crc, 1))));
	}
}

bool ZipFile::isZipFile(const Path& path)
{
	return path.getExtension() == ".zip";
}

Bytes ZipFile::readFile(const Path& path)
{
	const auto zip = ZipFile(path, false);
	return zip.extractFile(0);
}
