#pragma once

#include <Arduino.h>

class FileStorage
{
public:
    FileStorage();
    bool fileExists(String filename);
    void saveFile(String filename, uint8_t* data, int size);
    void loadFile(String filename, uint8_t* data, int size);
};
