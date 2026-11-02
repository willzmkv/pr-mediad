/* src/media_detect.h */
#pragma once
#include "platform_db.h"

/* Lê magic bytes do dispositivo/arquivo e identifica plataforma.
 * device_path: caminho do dispositivo ou arquivo ISO. */
PlatformID detect_magic_bytes(const char *device_path);

/* Verifica arquivos específicos no ponto de montagem.
 * mount_point: diretório onde o disco está montado. */
PlatformID detect_filesystem(const char *mount_point);

/* Identifica plataforma de ROM por extensão. */
PlatformID detect_rom_file(const char *filepath);

/* Função principal: tenta magic_bytes primeiro, depois filesystem.
 * Retorna PLATFORM_UNKNOWN se não identificar. */
PlatformID detect_platform(const char *device_path, const char *mount_point);
