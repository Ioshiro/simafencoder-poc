#ifndef MDAT_H
#define MDAT_H

#include "../smiencoder.h"
#include "../util.h"
#include <string.h>

#define MDAT_TYPE 'mdat'
#define MDAT_DEFAULT_SIZE 8 //size + type fields

int MediaDataBox_new(MediaDataBox *, Track[], int, int, int, u32 *);
int MediaDataBox_write(MediaDataBox, FILE *, Track[], int, int, int);
int writeTrackMediaData_WAV(MediaDataBox, FILE *, char[]);
int writeTrackMediaData_MP3(MediaDataBox, FILE *, char[]);
int findTrackSize(char[], int, u32 *);

int MediaDataBox_new(MediaDataBox *mdat, Track tracks[], int totalTracks, int totalSensors, int typeTrack, u32 *duration)
{
    int MDATSize = MDAT_DEFAULT_SIZE;
    // Calculate size of all media
    tracks[0].size = findTrackSize(tracks[0].title, typeTrack, duration);
    if (tracks[0].size == -1)
    {
        return -1;
    }
    MDATSize += tracks[0].size * totalTracks; // Supposing every track has equal size
    if (totalSensors > 0)
        tracks[0].sensor[0].size = findTrackSize(tracks[0].sensor[0].title, 1, duration); // Our sensor data is in .wav
    if (tracks[0].sensor[0].size == -1)
    {
        return -1;
    }
    MDATSize += tracks[0].sensor[0].size * totalSensors; // Same for the sensor data

    // Write default fields
    writeReverse(MDAT_TYPE, &mdat->type);
    writeReverse(MDATSize, &mdat->size);

    return MDATSize;
}

int MediaDataBox_write(MediaDataBox mdat, FILE *smi, Track tracks[], int totalTracks, int totalSensors, int typeTrack)
{

    int dataSize = MDAT_DEFAULT_SIZE;
    fwrite(&mdat.size, sizeof(u32), 1, smi);
    fwrite(&mdat.type, sizeof(u32), 1, smi);

    for (int i = 0; i < totalTracks; i++)
    {
        if (typeTrack == 1)
        {
            tracks[i].size = writeTrackMediaData_WAV(mdat, smi, tracks[i].title);
            printf("[MediaDataBox_writeData] track %d written %d bytes\n", i, tracks[i].size);
        }
        else if (typeTrack == 2)
            tracks[i].size = writeTrackMediaData_MP3(mdat, smi, tracks[i].title);
        if (tracks[i].size == -1)
        {
            return -1;
        }
        dataSize += tracks[i].size;
        for (int j = 0; j < tracks[i].sensorCount; j++)
        {
            tracks[i].sensor[j].size = writeTrackMediaData_WAV(mdat, smi, tracks[i].sensor[j].title);
            printf("[MediaDataBox_writeData] sensor %d written %d bytes\n", j, tracks[i].sensor[j].size);
            if (tracks[i].sensor[j].size == -1)
            {
                return -1;
            }
            dataSize += tracks[i].sensor[j].size;
        }
    }

    return dataSize;
}

int writeTrackMediaData_WAV(MediaDataBox mdat, FILE *smi, char fileName[])
{
    WavHeader wavHeader;
    FILE *file = fopen(fileName, "r");

    // Write media data
    printf("[writeTrackMediaData_WAV] opening: %s \n", fileName);
    fread(&wavHeader, 1, sizeof(wavHeader), file);
    int bytesPerSample = wavHeader.bits_per_sample / 8;
    int samples = (int)wavHeader.Chunk2_data_Size / bytesPerSample;
    printf("[writeTrackMediaData_WAV] data size: %u \n", wavHeader.Chunk2_data_Size);
    for (int i = 0; i < samples; i++)
    {
        fread(&mdat.data, bytesPerSample, 1, file);
        fwrite(&mdat.data, bytesPerSample, 1, smi);
    }
    fclose(file);
    return wavHeader.Chunk2_data_Size;
}

int writeTrackMediaData_MP3(MediaDataBox mdat, FILE *smi, char fileName[])
{
    return -1;
}

int findTrackSize(char fileName[], int typeTrack, u32 *duration)
{
    FILE *file = fopen(fileName, "r");
    if (typeTrack == 1)
    {
        WavHeader wavHeader;
        printf("[findTrackSize] opening: %s \n", fileName);
        fread(&wavHeader, 1, sizeof(wavHeader), file);
        fclose(file);
        if (wavHeader.Chunk_ID[0] == 'R' && wavHeader.Chunk_ID[1] == 'I' && wavHeader.Chunk_ID[2] == 'F' && wavHeader.Chunk_ID[3] == 'F')
        {
            printf("[findTrackSize] size: %u \n", wavHeader.Chunk2_data_Size);
            printf("[findTrackSize] byte rate: %u \n", wavHeader.byte_rate);
            *duration = (uint32_t)(((uint64_t)wavHeader.Chunk2_data_Size * (uint64_t)1000) / ((uint64_t)wavHeader.byte_rate)); // strange casts to ensure enough space is available to determine duration
            return wavHeader.Chunk2_data_Size;
        }
        else
        {
            printf("[findTrackSize] Not a wav header.\n");
        }
    }
    else if (typeTrack == 2)
    {
        // TODO: MP3 CASE
    }
    else
    {
        printf("[findTrackSize] Unsupported file type.\n");
        return -1;
    }
    return -1;
}

#endif