#pragma once

#include <halley.hpp>

#include "miniz.h"
class LibretroVFS;
using namespace Halley;

class ZipFile {
public:
    ZipFile();
    ZipFile(Path path, bool inMemory);
    ~ZipFile();

    bool open(Path path, bool inMemory);
    void close();

    size_t getNumFiles() const;
    String getFileName(size_t idx) const;
    Vector<String> getFileNames() const;
    Bytes extractFile(size_t idx) const;

    void extractAll(const Path& prefix, LibretroVFS& target) const;

    void printDiagnostics() const;

    static bool isZipFile(const Path& path);
    static Bytes readFile(const Path& path);

private:
    Path path;
	bool isOpen = false;

	void* fileHandle = nullptr;
    mutable mz_zip_archive archive;
    Bytes compressedData;
};
