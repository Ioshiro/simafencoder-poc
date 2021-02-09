#ifndef STBL_H
#define STBL_H

#include "../smiencoder.h"
#include "../util.h"
#include "stsd.h"
#include <time.h>

#define STBL_TYPE 'stbl'
#define STSD_TYPE 'stsd'
#define STTS_TYPE 'stts'
#define STTS_SIZE 24
#define STSC_TYPE 'stsc'
#define STSC_SIZE 28
#define STSZ_TYPE 'stsz'
#define STSZ_SIZE 20
#define STCO_TYPE 'stco'
#define STCO_SIZE 20

int SampleTableBox_new(SampleTableBox *, WavHeader, int, int, int);
int SampleTableBox_write(SampleTableBox, FILE *, int);
int TimeToSampleBox_new(TimeToSampleBox *, u32);
int TimeToSampleBox_write(TimeToSampleBox, FILE *);
int SampleSizeBox_new(SampleSizeBox *, u32, u32);
int SampleSizeBox_write(SampleSizeBox, FILE *);
int SampleToChunkBox_new(SampleToChunkBox *, u32);
int SampleToChunkBox_write(SampleToChunkBox, FILE *);
int ChunkOffsetBox_new(ChunkOffsetBox *, u32);
int ChunkOffsetBox_write(ChunkOffsetBox, FILE *);

//                                  -> sampleTable -> timeToSample
//                                                 -> sampleDescription -> textSampleEntry -> fontTableBox
//                                                                      -> audioSampleEntry -> es -> esDescriptor -> decoderConfigDescriptor
//                                                                                                                -> slConfigDescriptor
//                                                 -> sampleSize
//                                                 -> sampleToChunk
//                                                 -> chunkOffset

int SampleTableBox_new(SampleTableBox *stbl, WavHeader wavHeader, int chunkOffset, int isAudio, int sensorID)
{
    int sizeSTBL = BOX_DEFAULT_SIZE;
    writeReverse(STBL_TYPE, &stbl->type);
    u32 numSamples = (wavHeader.Chunk2_data_Size / (wavHeader.bits_per_sample / 8)) / (u32)wavHeader.Num_Channels;
    u32 sizeSamples = wavHeader.bits_per_sample / 8;
    sizeSTBL += TimeToSampleBox_new(&stbl->timeToSampleBox, numSamples);
    sizeSTBL += SampleSizeBox_new(&stbl->sampleSizeBox, numSamples, sizeSamples * wavHeader.Num_Channels);
    sizeSTBL += SampleToChunkBox_new(&stbl->sampleToChunkBox, numSamples);
    sizeSTBL += ChunkOffsetBox_new(&stbl->chunkOffsetBox, (u32)chunkOffset);
    sizeSTBL += SampleDescriptionBox_new(&stbl->sampleDescriptionBox, wavHeader, isAudio, sensorID);
    writeReverse(sizeSTBL, &stbl->size);
    //printf("written stbl\t:%d\n", sizeSTBL);
    return sizeSTBL;
}

int SampleTableBox_write(SampleTableBox stbl, FILE *smi, int isAudio)
{
    int written = 0;
    written += fwrite(&stbl.size, sizeof(stbl.size), 1, smi);
    written += fwrite(&stbl.type, sizeof(stbl.type), 1, smi);
    written += TimeToSampleBox_write(stbl.timeToSampleBox, smi);
    written += SampleDescriptionBox_write(stbl.sampleDescriptionBox, smi, isAudio);
    written += SampleSizeBox_write(stbl.sampleSizeBox, smi);
    written += SampleToChunkBox_write(stbl.sampleToChunkBox, smi);
    written += ChunkOffsetBox_write(stbl.chunkOffsetBox, smi);
    //printf("stsz written on file: %d\n", written);
    return written;
}

int TimeToSampleBox_new(TimeToSampleBox *stts, u32 numSamples)
{
    int sizeSTTS = STTS_SIZE;
    writeReverse(sizeSTTS, &stts->size);
    writeReverse(STTS_TYPE, &stts->type);
    writeReverse(1, &stts->sample_delta);
    writeReverse(1, &stts->entry_count);
    writeReverse(numSamples, &stts->sample_count);
    stts->version = 0;
    //printf("written stts\t:%d\n", sizeSTTS);
    return sizeSTTS;
}

int TimeToSampleBox_write(TimeToSampleBox stts, FILE *smi)
{
    int written = 0;
    written += fwrite(&stts, sizeof(stts), 1, smi);
    //printf("stts written on file: %d\n", written);
    return written;
}

int SampleSizeBox_new(SampleSizeBox *stsz, u32 numSamples, u32 sizeSamples)
{
    int sizeSTSZ = STSZ_SIZE;
    writeReverse(STSZ_TYPE, &stsz->type);
    writeReverse(sizeSTSZ, &stsz->size);
    writeReverse(sizeSamples, &stsz->sample_size);
    stsz->version = 0;
    //writeReverse(numSamples, &stsz->entry_count);
    stsz->entry_count = 0;
    //printf("written stsz\t:%d\n", sizeSTSZ);
    return sizeSTSZ;
}

int SampleSizeBox_write(SampleSizeBox stsz, FILE *smi)
{
    int written = 0;
    written += fwrite(&stsz, sizeof(stsz), 1, smi);
    //printf("stsz written on file: %d\n", written);
    return written;
}

int SampleToChunkBox_new(SampleToChunkBox *stsc, u32 numSamples)
{
    int sizeSTSC = STSC_SIZE;
    writeReverse(STSC_TYPE, &stsc->type);
    writeReverse(sizeSTSC, &stsc->size);
    writeReverse(1, &stsc->entry_count);
    writeReverse(1, &stsc->first_chunk);
    writeReverse(1, &stsc->sample_description_index);
    writeReverse(numSamples, &stsc->samples_per_chunk);
    stsc->version = 0;
    //printf("written stsc\t:%d\n", sizeSTSC);
    return sizeSTSC;
}

int SampleToChunkBox_write(SampleToChunkBox stsc, FILE *smi)
{
    int written = 0;
    written += fwrite(&stsc, sizeof(stsc), 1, smi);
    //printf("stsc written on file: %d\n", written);
    return written;
}

int ChunkOffsetBox_new(ChunkOffsetBox *stco, u32 chunkOffset)
{
    int sizeSTCO = STCO_SIZE;
    writeReverse(STCO_TYPE, &stco->type);
    writeReverse(sizeSTCO, &stco->size);
    writeReverse(1, &stco->entry_count);
    writeReverse(chunkOffset, &stco->chunk_offset);
    //printf("[ChunkOffsetBox_new] offset: %u\n", chunkOffset);
    stco->version = 0;
    //printf("written stco\t:%d\n", sizeSTCO);
    return sizeSTCO;
}

int ChunkOffsetBox_write(ChunkOffsetBox stco, FILE *smi)
{
    int written = 0;
    written += fwrite(&stco.size, sizeof(stco.size), 1, smi);
    written += fwrite(&stco.type, sizeof(stco.type), 1, smi);
    written += fwrite(&stco.version, sizeof(stco.version), 1, smi);
    written += fwrite(&stco.entry_count, sizeof(stco.entry_count), 1, smi);
    written += fwrite(&stco.chunk_offset, sizeof(stco.chunk_offset), 1, smi);
    //printf("stco written on file: %d\n", written);
    return written;
}

#endif