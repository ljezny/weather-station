#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "../Logging/Logging.hpp"

#define RADAR_TILE_DIR "/radar"
#define MAX_STORED_TILES 4

class RadarStorage
{
public:
    RadarStorage() : initialized(false), storedTileCount(0) {}
    
    bool begin()
    {
        if (initialized) return true;
        
        if (!LittleFS.begin(true)) // true = format if mount fails
        {
            LOGD("LittleFS mount failed");
            return false;
        }
        
        LOGD("LittleFS mounted, total: " + String(LittleFS.totalBytes()) + ", used: " + String(LittleFS.usedBytes()));
        
        // Vytvoříme adresář pro radarové dlaždice
        if (!LittleFS.exists(RADAR_TILE_DIR))
        {
            LittleFS.mkdir(RADAR_TILE_DIR);
        }
        
        initialized = true;
        return true;
    }
    
    void end()
    {
        if (initialized)
        {
            LittleFS.end();
            initialized = false;
        }
    }
    
    // Smaže všechny uložené dlaždice
    void clearTiles()
    {
        if (!initialized) return;
        
        LOGD("Clearing stored radar tiles");
        
        File dir = LittleFS.open(RADAR_TILE_DIR);
        if (!dir || !dir.isDirectory())
        {
            return;
        }
        
        File file = dir.openNextFile();
        while (file)
        {
            String path = String(RADAR_TILE_DIR) + "/" + file.name();
            file.close();
            LittleFS.remove(path);
            file = dir.openNextFile();
        }
        dir.close();
        
        storedTileCount = 0;
        LOGD("Tiles cleared");
    }
    
    // Získá cestu k souboru pro dlaždici
    String getTilePath(int tileIndex)
    {
        return String(RADAR_TILE_DIR) + "/tile_" + String(tileIndex) + ".png";
    }
    
    // Otevře soubor pro zápis dlaždice (pro streaming z HTTP)
    File openTileForWrite(int tileIndex)
    {
        if (!initialized) return File();
        
        String path = getTilePath(tileIndex);
        LOGD("Opening tile for write: " + path);
        
        File file = LittleFS.open(path, FILE_WRITE);
        if (file)
        {
            storedTileCount = max(storedTileCount, tileIndex + 1);
        }
        return file;
    }
    
    // Otevře soubor pro čtení dlaždice (pro streaming při vykreslování)
    File openTileForRead(int tileIndex)
    {
        if (!initialized) return File();
        
        String path = getTilePath(tileIndex);
        if (!LittleFS.exists(path))
        {
            LOGD("Tile not found: " + path);
            return File();
        }
        
        return LittleFS.open(path, FILE_READ);
    }
    
    // Zkontroluje jestli dlaždice existuje
    bool tileExists(int tileIndex)
    {
        if (!initialized) return false;
        return LittleFS.exists(getTilePath(tileIndex));
    }
    
    // Získá velikost dlaždice
    size_t getTileSize(int tileIndex)
    {
        if (!initialized) return 0;
        
        File file = openTileForRead(tileIndex);
        if (!file) return 0;
        
        size_t size = file.size();
        file.close();
        return size;
    }
    
    int getStoredTileCount() { return storedTileCount; }
    bool isInitialized() { return initialized; }
    
private:
    bool initialized;
    int storedTileCount;
};
