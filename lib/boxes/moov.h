#include "../smiencoder.h"
#include "../util.h"
#include "trak.h"
#include <time.h>

#define MOOV_TYPE 'moov'
#define MOOV_DEFAULT_SIZE 8
#define MVHD_TYPE 'mvhd'
#define MVHD_SIZE 108
#define MVHD_TIMESCALE 1000
#define MVHD_VOLUME 1

int MovieBox_new(MovieBox *, Track[], int, int, int);
int MovieBox_write(MovieBox, FILE *, Track[], int, int);
int MovieHeaderBox_new(MovieHeaderBox *, int, int);
int MovieHeaderBox_write(MovieHeaderBox, FILE *);
int findChunkOffset(Track[], int, int);

//trak -> trackHeader
//     -> media -> mediaHeader
//              -> handler
//              -> mediaInformation -> soundMediaHeader
//                                  -> nullMediaHeader
//                                  -> dataInformation -> dataReference -> dataEntryUrl
//                                  -> sampleTable -> timeToSample
//                                                 -> sampleDescription -> textSampleEntry -> fontTableBox
//                                                                      -> audioSampleEntry -> es -> esDescriptor -> decoderConfigDescriptor
//                                                                                                                -> slConfigDescriptor
//                                                 -> sampleSize
//                                                 -> sampleToChunk
//                                                 -> chunkOffset

int MovieBox_new(MovieBox *moov, Track tracks[], int durationTrack, int totalTracks, int totalSensors)
{
    u32 sizeMVHD, sizeTRAK, sizePRCO, sizeRUCO;
    int chunkOffset = 0;
    sizeTRAK = 0;
    sizeMVHD = MovieHeaderBox_new(&moov->movieHeaderBox, durationTrack, totalTracks + totalSensors + 1);
    //printf("duration: %d, total tracks: %d, total sensors: %d \n", durationTrack, totalTracks, totalSensors);
    int sensorID = 0;
    int trackOffset = 0;
    for (int i = 0; i < totalTracks; i++)
    {

        chunkOffset = findChunkOffset(tracks, i, -1);
        sizeTRAK += TrackBox_new(&moov->trackBox[i + trackOffset], tracks[i].title, i + trackOffset, durationTrack, chunkOffset, 1, 0, 0);
        if (tracks[i].sensorCount > 0)
        {
            for (int j = 1; j <= tracks[i].sensorCount; j++)
            {
                chunkOffset = findChunkOffset(tracks, i, j - 1);
                sizeTRAK += TrackBox_new(&moov->trackBox[i + trackOffset + j], tracks[i].sensor[j - 1].title, i + trackOffset + j, durationTrack, chunkOffset, 0, i, sensorID);
                sensorID++;
            }
            trackOffset += tracks[i].sensorCount;
        }
    }
    u32 sizeMOOV = (u32)sizeMVHD + (u32)sizeTRAK + MOOV_DEFAULT_SIZE;
    writeReverse(sizeMOOV, &moov->size);
    writeReverse(MOOV_TYPE, &moov->type);
    //printf("written moov\t:%d\n", sizeMOOV);
    return sizeMOOV;
}

int MovieBox_write(MovieBox moov, FILE *smi, Track tracks[], int totalTracks, int totalSensors)
{
    int written = 0;
    written += fwrite(&moov.size, sizeof(moov.size), 1, smi);
    u32 size = 0;
    writeReverse(moov.size, &size);
    //printf("\n[write]moov size: %u\n\n", size);

    written += fwrite(&moov.type, sizeof(moov.type), 1, smi);
    written += MovieHeaderBox_write(moov.movieHeaderBox, smi);
    int trackOffset = 0;
    for (int i = 0; i < totalTracks; i++)
    {
        written += TrackBox_write(moov.trackBox[i + trackOffset], smi, 1);
        for (int j = 1; j <= tracks[i].sensorCount; j++)
        {
            written += TrackBox_write(moov.trackBox[i + trackOffset + j], smi, 0);
        }
        trackOffset += tracks[i].sensorCount;
    }
    //printf("moov written on file: %d\n", written);
    return written;
}

//MovieHeader
int MovieHeaderBox_new(MovieHeaderBox *mvhd, int durationTrack, int trackID)
{
    time_t clock = time(NULL);
    int sizeMVHD = MVHD_SIZE; //Default size of mvhd set to 108
    writeReverse(sizeMVHD, &mvhd->size);
    writeReverse(MVHD_TYPE, &mvhd->type);           //Type of box mvhd
    mvhd->version = 0;                              //Version 0
    writeReverse(clock, &mvhd->creation_time);      //Creation time null
    writeReverse(clock, &mvhd->modification_time);  //Modification time null
    writeReverse(MVHD_TIMESCALE, &mvhd->timescale); //Timescale 1000 = 1000ms
    writeReverse(durationTrack, &mvhd->duration);
    writeReverse(0x00010000, &mvhd->rate);      //Rate 1
    writeReverse16(MVHD_VOLUME, &mvhd->volume); //Volume 1 = 100%
    mvhd->reserved = 0;
    mvhd->reserved2[0] = 0;
    mvhd->reserved2[1] = 0;
    writeReverse(0x00010000, &mvhd->matrix[0]); //Matrix for video
    mvhd->matrix[1] = 0;
    mvhd->matrix[2] = 0;
    mvhd->matrix[3] = 0;
    writeReverse(0x00010000, &mvhd->matrix[4]);
    mvhd->matrix[5] = 0;
    mvhd->matrix[6] = 0;
    mvhd->matrix[7] = 0;
    mvhd->matrix[8] = 0x40000000;
    mvhd->pre_defined[0] = 0;
    mvhd->pre_defined[1] = 0;
    mvhd->pre_defined[2] = 0;
    mvhd->pre_defined[3] = 0;
    mvhd->pre_defined[4] = 0;
    mvhd->pre_defined[5] = 0;
    writeReverse(trackID, &mvhd->next_track_ID);
    //printf("written mvhd\t:%d\n", sizeMVHD);
    return sizeMVHD;
}

int MovieHeaderBox_write(MovieHeaderBox mvhd, FILE *smi)
{
    int written = 0;
    written += fwrite(&mvhd.size, sizeof(mvhd.size), 1, smi);
    written += fwrite(&mvhd.type, sizeof(mvhd.type), 1, smi);
    written += fwrite(&mvhd.version, sizeof(mvhd.version), 1, smi);
    written += fwrite(&mvhd.creation_time, sizeof(mvhd.creation_time), 1, smi);
    written += fwrite(&mvhd.modification_time, sizeof(mvhd.modification_time), 1, smi);
    written += fwrite(&mvhd.timescale, sizeof(mvhd.timescale), 1, smi);
    written += fwrite(&mvhd.duration, sizeof(mvhd.duration), 1, smi);
    written += fwrite(&mvhd.rate, sizeof(mvhd.rate), 1, smi);
    written += fwrite(&mvhd.volume, sizeof(mvhd.volume), 1, smi);
    written += fwrite(&mvhd.reserved, sizeof(mvhd.reserved), 1, smi);
    written += fwrite(&mvhd.reserved2[0], sizeof(mvhd.reserved2[0]), 1, smi);
    written += fwrite(&mvhd.reserved2[1], sizeof(mvhd.reserved2[1]), 1, smi);
    written += fwrite(&mvhd.matrix[0], sizeof(mvhd.matrix[0]), 1, smi);
    written += fwrite(&mvhd.matrix[1], sizeof(mvhd.matrix[1]), 1, smi);
    written += fwrite(&mvhd.matrix[2], sizeof(mvhd.matrix[2]), 1, smi);
    written += fwrite(&mvhd.matrix[3], sizeof(mvhd.matrix[3]), 1, smi);
    written += fwrite(&mvhd.matrix[4], sizeof(mvhd.matrix[4]), 1, smi);
    written += fwrite(&mvhd.matrix[5], sizeof(mvhd.matrix[5]), 1, smi);
    written += fwrite(&mvhd.matrix[6], sizeof(mvhd.matrix[6]), 1, smi);
    written += fwrite(&mvhd.matrix[7], sizeof(mvhd.matrix[7]), 1, smi);
    written += fwrite(&mvhd.matrix[8], sizeof(mvhd.matrix[8]), 1, smi);
    written += fwrite(&mvhd.pre_defined[0], sizeof(mvhd.pre_defined[0]), 1, smi);
    written += fwrite(&mvhd.pre_defined[1], sizeof(mvhd.pre_defined[1]), 1, smi);
    written += fwrite(&mvhd.pre_defined[2], sizeof(mvhd.pre_defined[2]), 1, smi);
    written += fwrite(&mvhd.pre_defined[3], sizeof(mvhd.pre_defined[3]), 1, smi);
    written += fwrite(&mvhd.pre_defined[4], sizeof(mvhd.pre_defined[4]), 1, smi);
    written += fwrite(&mvhd.pre_defined[5], sizeof(mvhd.pre_defined[5]), 1, smi);
    written += fwrite(&mvhd.next_track_ID, sizeof(mvhd.next_track_ID), 1, smi);
    //printf("mvhd written on file: %d achtung\n", written);
    return written;
}

int findChunkOffset(Track tracks[], int trackPos, int sensorPos)
{
    int offset = FTYP_SIZE + BOX_DEFAULT_SIZE;
    for (int i = 0; i <= trackPos; i++)
    {
        if (i == trackPos && sensorPos == -1)
        {
            //printf("[findChunkOffset] offset: %d for track %d sensor %d\n", offset, trackPos, sensorPos);
            return offset;
        }
        offset += tracks[0].size;
        //printf("[findChunkOffse] track size %d \n", tracks[0].size);
        if (i == trackPos)
        {
            for (int j = 0; j < sensorPos; j++)
            {
                offset += tracks[0].sensor[0].size;
                //printf("[findChunkOffse] sensor size %d \n", tracks[0].sensor[0].size);
            }
        }
        else
        {
            for (int j = 0; j < tracks[i].sensorCount; j++)
            {
                offset += tracks[0].sensor[0].size;
            }
        }
    }

    return offset;
}
