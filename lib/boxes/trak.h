#ifndef MOOV_H
#define MOOV_H

#include "../smiencoder.h"
#include "../util.h"
#include "mdia.h"
#include <time.h>

#define TRAK_TYPE 'trak'
#define TRAK_DEFAULT_SIZE 8
#define TKHD_TYPE 'tkhd'
#define TKHD_SIZE 92
#define TKHD_AUDIO_VOLUME 256
#define TREF_TYPE 'tref'
#define CDSC_TYPE 'cdsc'

int TrackBox_new(TrackBox *, char[], int, int, int, int, int, int);
int TrackBox_write(TrackBox, FILE *, int);
int TrackHeaderBox_new(TrackHeaderBox *, int, int, int);
int TrackHeaderBox_write(TrackHeaderBox, FILE *);
int TrackReferenceBox_new(TrackReferenceBox *, int);
int TrackReferenceBox_write(TrackReferenceBox, FILE *);
int TrackReferenceTypeBox_new(TrackReferenceTypeBox *, int);
int TrackReferenceTypeBox_write(TrackReferenceTypeBox, FILE *);
//Contains methods to construct TrackBox, TrackHeaderBox and TrackReferenceBox

int TrackBox_new(TrackBox *trak, char fileName[], int numTrack, int durationTrack, int chunkOffset, int isAudio, int referenceTrack, int sensorID)
{
    int sizeTRAK = TRAK_DEFAULT_SIZE; //default size 8

    writeReverse(TRAK_TYPE, &trak->type);
    sizeTRAK += TrackHeaderBox_new(&trak->trackHeaderBox, numTrack, durationTrack, isAudio);
    if (!isAudio)
    {
        sizeTRAK += TrackReferenceBox_new(&trak->trackReferenceBox, referenceTrack);
    }
    sizeTRAK += MediaBox_new(&trak->mediaBox, fileName, durationTrack, chunkOffset, isAudio, sensorID);
    //trak . trackHeader
    //     . media . mediaHeader
    //              . handler
    //              . mediaInformation . soundMediaHeader
    //                                  . nullMediaHeader
    //                                  . dataInformation . dataReference . dataEntryUrl
    //                                  . sampleTable . timeToSample
    //                                                 . sampleDescription . textSampleEntry . fontTableBox
    //                                                                      . audioSampleEntry . es . esDescriptor . decoderConfigDescriptor
    //                                                                                                                . slConfigDescriptor
    //                                                 . sampleSize
    //                                                 . sampleToChunk
    //                                                 . chunkOffset

    writeReverse(sizeTRAK, &trak->size);
    //printf("written trak\t:%d\n", sizeTRAK);
    return sizeTRAK;
}

int TrackBox_write(TrackBox trak, FILE *smi, int isAudio)
{
    int written = 0;
    written += fwrite(&trak.size, sizeof(trak.size), 1, smi);
    written += fwrite(&trak.type, sizeof(trak.type), 1, smi);
    written += TrackHeaderBox_write(trak.trackHeaderBox, smi);
    if (!isAudio)
        written += TrackReferenceBox_write(trak.trackReferenceBox, smi);
    written += MediaBox_write(trak.mediaBox, smi, isAudio);
    //printf("trak written on file: %d\n", written);
    return written;
}

int TrackHeaderBox_new(TrackHeaderBox *tkhd, int numTrack, int durationTrack, int isAudio)
{
    //Track Header//
    time_t clock = time(NULL);
    int sizeTKHD = TKHD_SIZE; //default size 92
    writeReverse(sizeTKHD, &tkhd->size);
    writeReverse(TKHD_TYPE, &tkhd->type);
    writeReverse(0x00000003, &tkhd->version);
    //tkhd->version = 0;
    writeReverse(clock, &tkhd->creation_time);
    writeReverse(clock, &tkhd->modification_time);
    writeReverse((numTrack + 1), &tkhd->track_ID);
    tkhd->reserved = 0;
    writeReverse(durationTrack, &tkhd->duration);
    tkhd->reserved2[0] = 0;
    tkhd->reserved2[1] = 0;
    tkhd->layer = 0;
    tkhd->alternate_group = 0;
    if (isAudio)
        writeReverse16(TKHD_AUDIO_VOLUME, &tkhd->volume);
    else
        tkhd->volume = 0;
    tkhd->reserved3 = 0;
    writeReverse(0x00010000, &tkhd->matrix[0]);
    tkhd->matrix[1] = 0;
    tkhd->matrix[2] = 0;
    tkhd->matrix[3] = 0;
    writeReverse(0x00010000, &tkhd->matrix[4]);
    tkhd->matrix[5] = 0;
    tkhd->matrix[6] = 0;
    tkhd->matrix[7] = 0;
    writeReverse(0x40000000, &tkhd->matrix[8]);
    tkhd->width = 0;  //just for video
    tkhd->height = 0; //just for video
    //printf("written tkhd\t:%d\n", sizeTKHD);
    return sizeTKHD;
}

int TrackHeaderBox_write(TrackHeaderBox tkhd, FILE *smi)
{
    int written = 0;
    written += fwrite(&tkhd, sizeof(tkhd), 1, smi);
    //printf("tkhd written on file: %d\n", written);
    return written;
}

int TrackReferenceBox_new(TrackReferenceBox *tref, int referenceTrack)
{
    int sizeTREF = BOX_DEFAULT_SIZE;
    writeReverse(TREF_TYPE, &tref->type);
    sizeTREF += TrackReferenceTypeBox_new(&tref->trackReferenceTypeBox, referenceTrack);
    writeReverse(sizeTREF, &tref->size);
    //printf("written tref\t:%d\n", sizeTREF);
    return sizeTREF;
}

int TrackReferenceBox_write(TrackReferenceBox tref, FILE *smi)
{
    int written = 0;
    written += fwrite(&tref.size, sizeof(tref.size), 1, smi);
    written += fwrite(&tref.type, sizeof(tref.type), 1, smi);
    written += TrackReferenceTypeBox_write(tref.trackReferenceTypeBox, smi);
    //printf("tref written on file: %d\n", written);
    return written;
}

int TrackReferenceTypeBox_new(TrackReferenceTypeBox *cdsc, int referenceTrack)
{
    int sizeCDSC = BOX_DEFAULT_SIZE + 4;
    writeReverse(CDSC_TYPE, &cdsc->type);
    writeReverse(sizeCDSC, &cdsc->size);
    writeReverse(referenceTrack + 1, &cdsc->trackIDReference);
    //printf("written cdsc\t:%d\n", sizeCDSC);
    return sizeCDSC;
}

int TrackReferenceTypeBox_write(TrackReferenceTypeBox cdsc, FILE *smi)
{
    int written = 0;
    written += fwrite(&cdsc, sizeof(cdsc), 1, smi);
    //printf("cdsc written on file: %d\n", written);
    return written;
}

#endif