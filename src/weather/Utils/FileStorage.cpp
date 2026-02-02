#include "FileStorage.hpp"
#include "FS.h"
#include "LittleFS.h"

FileStorage::FileStorage()
{
}

void FileStorage::saveFile(String filename, uint8_t *data, int size)
{
    File f = LittleFS.open(filename, "wb", true);
    f.write(data, size);
    f.close();
}

bool FileStorage::fileExists(String filename)
{
    return LittleFS.exists(filename);
}

void FileStorage::loadFile(String filename, uint8_t *data, int size)
{
    File f = LittleFS.open(filename, "rb");
    f.read(data, size);
    f.close();
}
