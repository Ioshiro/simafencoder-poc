#ifndef MDIA_H
#define MDIA_H

#include "../smiencoder.h"
#include "../util.h"
#include "minf.h"
#include <time.h>

#define MDIA_TYPE 'mdia'
#define MDIA_DEFAULT_SIZE 8
#define MDHD_TYPE 'mdhd'
#define MDHD_SIZE 32
#define HDLR_TYPE 'hdlr'
#define HDLR_SIZE 37
#define HDLR_HANDLER_TYPE_AUDIO 'soun'
#define HDLR_HANDLER_TYPE_META 'meta'

int MediaBox_new(MediaBox *, char[], int, int, int);
int MediaBox_write(MediaBox, FILE *, int);
int MediaHeaderBox_new(MediaHeaderBox *, int, int);
int MediaHeaderBox_write(MediaHeaderBox, FILE *);
int HandlerBox_new(HandlerBox *, int);
int HandlerBox_write(HandlerBox, FILE *);
WavHeader readWavHeader(char[]);

//     . media . mediaHeader
//              . handler
//              . mediaInformation . soundMediaHeader
//                                  . baseMediaHeader
//                                  . dataInformation . dataReference . dataEntryUrl
//                                  . sampleTable . timeToSample
//                                                 . sampleDescription . textSampleEntry . fontTableBox
//                                                                      . audioSampleEntry . es . esDescriptor . decoderConfigDescriptor
//                                                                                                                . slConfigDescriptor
//                                                 . sampleSize
//                                                 . sampleToChunk
//                                                 . chunkOffset

int MediaBox_new(MediaBox *mdia, char fileName[], int chunkOffset, int isAudio, int sensorID)
{
    int sizeMDIA = MDIA_DEFAULT_SIZE;
    writeReverse(MDIA_TYPE, &mdia->type);
    WavHeader wavHeader = readWavHeader(fileName);
    u32 duration = wavHeader.Chunk2_data_Size / wavHeader.byte_rate;
    sizeMDIA += MediaHeaderBox_new(&mdia->mediaHeaderBox, wavHeader.Sample_rate, wavHeader.Sample_rate * duration); // *** IF WAV ***
    sizeMDIA += HandlerBox_new(&mdia->handlerBox, isAudio);
    sizeMDIA += MediaInformationBox_new(&mdia->mediaInformationBox, wavHeader, chunkOffset, isAudio, sensorID);
    writeReverse(sizeMDIA, &mdia->size);
    //printf("written mdia\t:%d\n", sizeMDIA);
    return sizeMDIA;
}

int MediaBox_write(MediaBox mdia, FILE *smi, int isAudio)
{
    int written = 0;
    written += fwrite(&mdia.size, sizeof(mdia.size), 1, smi);
    u32 size = 0;
    writeReverse(mdia.size, &size);
    //printf("\n[write]mdia size: %u\n\n", size);
    written += fwrite(&mdia.type, sizeof(mdia.type), 1, smi);
    written += MediaHeaderBox_write(mdia.mediaHeaderBox, smi);
    written += HandlerBox_write(mdia.handlerBox, smi);
    written += MediaInformationBox_write(mdia.mediaInformationBox, smi, isAudio);
    //printf("mdia written on file: %d\n", written);
    return written;
}

int MediaHeaderBox_new(MediaHeaderBox *mdhd, int timescale, int duration)
{
    int sizeMDHD = MDHD_SIZE;
    time_t clock = time(NULL);
    writeReverse(MDHD_TYPE, &mdhd->type);
    writeReverse(sizeMDHD, &mdhd->size);
    writeReverse(clock, &mdhd->creation_time);
    writeReverse(clock, &mdhd->modification_time);
    writeReverse(timescale, &mdhd->timescale);
    writeReverse(duration, &mdhd->duration);
    mdhd->language = 0xC455;
    mdhd->version = 0;
    mdhd->pre_defined = 0;
    //printf("written mdhd\t:%d\n", sizeMDHD);
    return sizeMDHD;
}

int MediaHeaderBox_write(MediaHeaderBox mdhd, FILE *smi)
{
    int written = 0;
    written += fwrite(&mdhd, sizeof(mdhd), 1, smi);
    //printf("mdhd written on file: %d\n", written);
    return written;
}

int HandlerBox_new(HandlerBox *hdlr, int isAudio)
{
    //Handler Box//
    int sizeHDLR = HDLR_SIZE;
    writeReverse(HDLR_TYPE, &hdlr->type);
    writeReverse(sizeHDLR, &hdlr->size);
    hdlr->version = 0;
    hdlr->pre_defined = 0;
    hdlr->reserved[0] = 0;
    hdlr->reserved[1] = 0;
    hdlr->reserved[2] = 0;
    if (isAudio)
    {
        writeReverse(HDLR_HANDLER_TYPE_AUDIO, &hdlr->handler_type);
        hdlr->data[0] = 's';
        hdlr->data[1] = 'o';
        hdlr->data[2] = 'u';
        hdlr->data[3] = 'n';
        hdlr->data[4] = '\0';
    }
    else
    {
        writeReverse(HDLR_HANDLER_TYPE_META, &hdlr->handler_type);
        hdlr->data[0] = 'm';
        hdlr->data[1] = 'e';
        hdlr->data[2] = 't';
        hdlr->data[3] = 'a';
        hdlr->data[4] = '\0';
    }
    //printf("written hdlr\t:%d\n", sizeHDLR);
    return sizeHDLR;
}

int HandlerBox_write(HandlerBox hdlr, FILE *smi)
{
    int written = 0;
    written += fwrite(&hdlr.size, sizeof(hdlr.size), 1, smi);
    written += fwrite(&hdlr.type, sizeof(hdlr.type), 1, smi);
    written += fwrite(&hdlr.version, sizeof(hdlr.version), 1, smi);
    written += fwrite(&hdlr.pre_defined, sizeof(hdlr.pre_defined), 1, smi);
    written += fwrite(&hdlr.handler_type, sizeof(hdlr.handler_type), 1, smi);
    written += fwrite(&hdlr.reserved[0], sizeof(hdlr.reserved[0]), 1, smi);
    written += fwrite(&hdlr.reserved[1], sizeof(hdlr.reserved[1]), 1, smi);
    written += fwrite(&hdlr.reserved[2], sizeof(hdlr.reserved[2]), 1, smi);
    written += fwrite(&hdlr.data[0], sizeof(hdlr.data[0]), 1, smi);
    written += fwrite(&hdlr.data[1], sizeof(hdlr.data[1]), 1, smi);
    written += fwrite(&hdlr.data[2], sizeof(hdlr.data[2]), 1, smi);
    written += fwrite(&hdlr.data[3], sizeof(hdlr.data[3]), 1, smi);
    written += fwrite(&hdlr.data[4], sizeof(hdlr.data[4]), 1, smi);
    //printf("hdlr written on file: %d\n", written);
    return written;
}

WavHeader readWavHeader(char fileName[])
{
    WavHeader header;
    FILE *file;
    file = fopen(fileName, "r");
    fread(&header, sizeof(WavHeader), 1, file);
    return header;
}

#endif