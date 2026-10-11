#ifndef BLP_LOADER_H
#define BLP_LOADER_H

#include <stdint.h>

// Forzamos al compilador a no dejar espacios vacíos entre variables 
// (Alineación estricta de 1 byte para lectura binaria directa)
#pragma pack(push, 1)

typedef struct {
    char magic[4];          // Identificador del archivo. Debe ser exactamente "BLP2"
    uint32_t version;         // Versión del formato (típicamente 1 para WoW Pandaria)
    uint8_t colorEncoding;    // Tipo de codificación (1 = JPEG, 2 = Directo/DXT/Paleta)
    uint8_t alphaSize;        // Tamaño en bits del canal alfa (0, 1, 4, 8)
    uint8_t preferredFormat;  // Detalles de compresión (0 = Raw/Paleta, 1 = DXT)
    uint8_t mipmapsFormat;    // Información de mipmaps
    uint32_t width;           // Ancho de la textura en píxeles
    uint32_t height;          // Alto de la textura en píxeles
    uint32_t offsets[16];     // Posiciones en bytes dentro del archivo para cada nivel de mipmap
    uint32_t lengths[16];     // Tamaños en bytes de cada uno de los niveles de mipmap
} BLPHeader;

#pragma pack(pop)

#endif // BLP_LOADER_H
