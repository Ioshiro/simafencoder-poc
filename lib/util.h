#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include "smiencoder.h"

#define BOX_DEFAULT_SIZE 8 // 4bytes for type + 4bytes for size

typedef struct WAV_HEADER
{
    // RIFF Chunk
    u8 Chunk_ID[4];      // 'RIFF'
    u32 Chunk_data_Size; // RIFF Chunk data Size
    u8 RIFF_TYPE_ID[4];  // 'WAVE'
    // format sub-chunk
    u8 Chunk1_ID[4];      // 'fmt '
    u32 Chunk1_data_Size; // Size of the format chunk
    u16 Format_Tag;       // format_Tag 1=PCM
    u16 Num_Channels;     // 1=Mono 2=Stereo
    u32 Sample_rate;      // Sampling Frequency in (44100)Hz
    u32 byte_rate;        // Byte rate
    u16 block_Align;      // 4
    u16 bits_per_sample;  // 16
    /* "data" sub-chunk */
    u8 Chunk2_ID[4];      // 'data'
    u32 Chunk2_data_Size; // Size of the audio data
} WavHeader;

typedef struct SENSOR
{ // Stores the different sensor files for each track
    char title[20];
    int size;
    WavHeader header;
} Sensor;

typedef struct TRACK
{ // Stores the different titles of the tracks
    char title[20];
    int size;
    int sensorCount;
    WavHeader header;
    Sensor sensor[MAX_SENSORS];
} Track;

int byteReverse(int num)
{
    int swapped;
    swapped = ((num >> 24) & 0xff) |      // move byte 3 to byte 0
              ((num << 8) & 0xff0000) |   // move byte 1 to byte 2
              ((num >> 8) & 0xff00) |     // move byte 2 to byte 1
              ((num << 24) & 0xff000000); // byte 0 to byte 3
    return swapped;
}

u16 byteReverse16(u16 num)
{
    u16 swapped;
    swapped = (((num << 8) & 0xff00) | // move byte 1 to byte 2
               ((num >> 8) & 0x00ff)); // byte 2 to byte 1
    return swapped;
}

int writeReverse(int val, u32 *field)
{
    u32 swap = (uint32_t)byteReverse(val);
    *field = swap;
    return sizeof(swap);
}

int writeReverse16(int val, u16 *field)
{
    u16 swap = byteReverse16((u16)val);
    *field = swap;
    return sizeof(swap);
}

u32 fwriteReverse(int val, size_t size, size_t n, FILE *file)
{
    u32 swap = byteReverse(val);
    fwrite(&swap, size, n, file);
    return swap;
}

void Tracks_new(Track *tracks[])
{
    for (int i = 0; i < MAX_TRACKS; i++)
    {
        tracks[i]->sensorCount = 0;
    }
}

#endif