#ifndef STSD_H
#define STSD_H

#include "../smiencoder.h"
#include "../util.h"
#include <time.h>

#define STSD_TYPE 'stsd'
#define STSD_DEFAULT_SIZE 16
#define WAVE_TYPE 'mp4a'
#define WAVE_SIZE 36 //18
#define ESDS_TYPE 'esds'
#define ESDS_SIZE 35
#define MEBX_TYPE 'mebx'
#define MEBX_DEFAULT_SIZE 16
#define KEYS_TYPE 'keys'
#define KEYD_TYPE 'keyd'
#define KEYD_SIZE 16
#define DTYP_TYPE 'dtyp'
#define DTYP_SIZE 16

int AudioSampleEntryBox_write2(AudioSampleEntryBox, FILE *);

int SampleDescriptionBox_new(SampleDescriptionBox *, WavHeader, int, int);
int SampleDescriptionBox_write(SampleDescriptionBox, FILE *, int);
int AudioSampleEntryBox_new(AudioSampleEntryBox *, u16, u16, u32);
int AudioSampleEntryBox_write(AudioSampleEntryBox, FILE *);
int ElementaryStreamBox_new(ESBox *);
int ElementaryStreamBox_write(ESBox, FILE *);
int SampleDescriptionTableBox_new(SampleDescriptionTableBox *, int);
int SampleDescriptionTableBox_write(SampleDescriptionTableBox, FILE *);
int MetadataKeyTableBox_new(MetadataKeyTableBox *, int);
int MetadataKeyTableBox_write(MetadataKeyTableBox, FILE *);
int MetadataKeyBox_new(MetadataKeyBox *, int);
int MetadataKeyBox_write(MetadataKeyBox, FILE *);
int MetadataKeyDeclarationBox_new(MetadataKeyDeclarationBox *);
int MetadataKeyDeclarationBox_write(MetadataKeyDeclarationBox, FILE *);
int MetadataDatatypeDefinitionBox_new(MetadataDatatypeDefinitionBox *);
int MetadataDatatypeDefinitionBox_write(MetadataDatatypeDefinitionBox, FILE *);

int SampleDescriptionBox_new(SampleDescriptionBox *stsd, WavHeader wavHeader, int isAudio, int sensorID)
{
    int sizeSTSD = STSD_DEFAULT_SIZE;
    writeReverse(STSD_TYPE, &stsd->type);
    writeReverse(1, &stsd->entry_count);
    stsd->version = 0;
    if (isAudio)
        sizeSTSD += AudioSampleEntryBox_new(&stsd->audioSampleEntryBox, wavHeader.Num_Channels, wavHeader.bits_per_sample, wavHeader.Sample_rate);
    else
        sizeSTSD += SampleDescriptionTableBox_new(&stsd->sampleDescriptionTableBox, sensorID);
    writeReverse(sizeSTSD, &stsd->size);
    //printf("written stsd\t:%d\n", sizeSTSD);
    return sizeSTSD;
}

int SampleDescriptionBox_write(SampleDescriptionBox stsd, FILE *smi, int isAudio)
{
    int written = 0;
    written += fwrite(&stsd.size, sizeof(stsd.size), 1, smi);
    written += fwrite(&stsd.type, sizeof(stsd.type), 1, smi);
    written += fwrite(&stsd.version, sizeof(stsd.version), 1, smi);
    written += fwrite(&stsd.entry_count, sizeof(stsd.entry_count), 1, smi);
    if (isAudio)
        written += AudioSampleEntryBox_write2(stsd.audioSampleEntryBox, smi);
    else
        written += SampleDescriptionTableBox_write(stsd.sampleDescriptionTableBox, smi);
    //printf("stsd written on file: %d\n", written);
    return written;
}

int AudioSampleEntryBox_new(AudioSampleEntryBox *wave, u16 numChannels, u16 sampleSize, u32 sampleRate)
{
    int sizeWAVE = WAVE_SIZE;
    writeReverse(WAVE_TYPE, &wave->type);

    // writeReverse16(numChannels, &wave->num_channels);
    //  writeReverse16(sampleSize, &wave->sample_size);
    //writeReverse(sampleRate, &wave->sample_rate);
    wave->num_channels = 512;
    wave->sample_size = 4096;
    u32 swap = 44100 << 16;
    writeReverse(swap, &wave->sample_rate);
    wave->reserved[0] = 0;
    wave->reserved[1] = 0;
    wave->reserved[2] = 0;
    wave->reserved[3] = 0;
    wave->reserved[4] = 0;
    wave->reserved[5] = 0;
    wave->data_reference_index = 256; // suspicious (maybe try writeReverse16(1, &wave->data_reference_index);?)
    wave->version = 0;
    wave->revision_level = 0;
    wave->vendor = 0;
    wave->compression_ID = 0;
    wave->packet_size = 0;
    sizeWAVE += ElementaryStreamBox_new(&wave->esBox);
    writeReverse(sizeWAVE, &wave->size);
    //printf("written wave\t:%d, channels %u, samplesize %u, samplerate %u\n", sizeWAVE, numChannels, sampleSize, sampleRate);
    return sizeWAVE;
}

int AudioSampleEntryBox_write(AudioSampleEntryBox wave, FILE *smi)
{
    int written = 0;
    written += fwrite(&wave, sizeof(wave), 1, smi); // reserved may be written as 6 zero string terminated by \0, so check size = 36
    //printf("wave written on file: %d  (36?)\n", written);
    return written;
}

int AudioSampleEntryBox_write2(AudioSampleEntryBox wave, FILE *smi)
{
    int written = 0;
    written += fwrite(&wave.size, sizeof(wave.size), 1, smi);
    written += fwrite(&wave.type, sizeof(wave.type), 1, smi);
    written += fwrite(&wave.reserved[0], sizeof(wave.reserved[0]), 1, smi);
    written += fwrite(&wave.reserved[1], sizeof(wave.reserved[1]), 1, smi);
    written += fwrite(&wave.reserved[2], sizeof(wave.reserved[2]), 1, smi);
    written += fwrite(&wave.reserved[3], sizeof(wave.reserved[3]), 1, smi);
    written += fwrite(&wave.reserved[4], sizeof(wave.reserved[4]), 1, smi);
    written += fwrite(&wave.reserved[5], sizeof(wave.reserved[5]), 1, smi);
    written += fwrite(&wave.data_reference_index, sizeof(wave.data_reference_index), 1, smi);
    written += fwrite(&wave.vendor, sizeof(wave.vendor), 1, smi);
    written += fwrite(&wave.vendor, sizeof(wave.vendor), 1, smi);
    written += fwrite(&wave.num_channels, sizeof(wave.num_channels), 1, smi);
    written += fwrite(&wave.sample_size, sizeof(wave.sample_size), 1, smi);
    written += fwrite(&wave.vendor, sizeof(wave.vendor), 1, smi);
    written += fwrite(&wave.sample_rate, sizeof(wave.sample_rate), 1, smi);
    u32 size;
    writeReverse(wave.size, &size);
    ElementaryStreamBox_write(wave.esBox, smi);
    //printf("\n\n wave size: %u \n\n", size);
    //printf("wave written on file: %d  (36?)\n", written);
    return written;
}

int ElementaryStreamBox_new(ESBox *esds)
{
    int sizeESDS = ESDS_SIZE;
    writeReverse(ESDS_TYPE, &esds->type);
    writeReverse(sizeESDS, &esds->size);
    esds->version = 0;

    esds->esDescriptorBox.tag = 3;
    esds->esDescriptorBox.length = 21;
    esds->esDescriptorBox.ES_ID = 0;
    esds->esDescriptorBox.mix = 0;

    esds->esDescriptorBox.decoderConfigDescriptorBox.tag = 4;
    esds->esDescriptorBox.decoderConfigDescriptorBox.length = 13;
    esds->esDescriptorBox.decoderConfigDescriptorBox.objectProfileInd = 0x6B;
    writeReverse(0x150036B0, &esds->esDescriptorBox.decoderConfigDescriptorBox.mix);
    writeReverse(1411, &esds->esDescriptorBox.decoderConfigDescriptorBox.maxBitRate);
    writeReverse(1411, &esds->esDescriptorBox.decoderConfigDescriptorBox.avgBitrate);

    esds->esDescriptorBox.slConfigDescriptorBox.tag = 6;
    esds->esDescriptorBox.slConfigDescriptorBox.length = 1;
    esds->esDescriptorBox.slConfigDescriptorBox.predifined = 2;
    return sizeESDS;
}

int ElementaryStreamBox_write(ESBox esds, FILE *smi)
{
    int written;
    writeReverse(esds.size, &written);
    fwrite(&esds.size, sizeof(esds.size), 1, smi);
    fwrite(&esds.type, sizeof(esds.type), 1, smi);
    fwrite(&esds.version, sizeof(esds.version), 1, smi);
    fwrite(&esds.esDescriptorBox.tag, sizeof(esds.esDescriptorBox.tag), 1, smi);
    fwrite(&esds.esDescriptorBox.length, sizeof(esds.esDescriptorBox.length), 1, smi);
    fwrite(&esds.esDescriptorBox.ES_ID, sizeof(esds.esDescriptorBox.ES_ID), 1, smi);
    fwrite(&esds.esDescriptorBox.mix, sizeof(esds.esDescriptorBox.mix), 1, smi);
    fwrite(&esds.esDescriptorBox.decoderConfigDescriptorBox.tag, sizeof(esds.esDescriptorBox.decoderConfigDescriptorBox.tag), 1, smi);
    fwrite(&esds.esDescriptorBox.decoderConfigDescriptorBox.length, sizeof(esds.esDescriptorBox.decoderConfigDescriptorBox.length), 1, smi);
    fwrite(&esds.esDescriptorBox.decoderConfigDescriptorBox.objectProfileInd, sizeof(esds.esDescriptorBox.decoderConfigDescriptorBox.objectProfileInd), 1, smi);
    fwrite(&esds.esDescriptorBox.decoderConfigDescriptorBox.mix, sizeof(esds.esDescriptorBox.decoderConfigDescriptorBox.mix), 1, smi);
    fwrite(&esds.esDescriptorBox.decoderConfigDescriptorBox.maxBitRate, sizeof(esds.esDescriptorBox.decoderConfigDescriptorBox.maxBitRate), 1, smi);
    fwrite(&esds.esDescriptorBox.decoderConfigDescriptorBox.avgBitrate, sizeof(esds.esDescriptorBox.decoderConfigDescriptorBox.avgBitrate), 1, smi);
    fwrite(&esds.esDescriptorBox.slConfigDescriptorBox.tag, sizeof(esds.esDescriptorBox.slConfigDescriptorBox.tag), 1, smi);
    fwrite(&esds.esDescriptorBox.slConfigDescriptorBox.length, sizeof(esds.esDescriptorBox.slConfigDescriptorBox.length), 1, smi);
    fwrite(&esds.esDescriptorBox.slConfigDescriptorBox.predifined, sizeof(esds.esDescriptorBox.slConfigDescriptorBox.predifined), 1, smi);

    //printf("esds written on file: %d \n", written);
    return written;
}

int SampleDescriptionTableBox_new(SampleDescriptionTableBox *mebx, int sensorID)
{
    int sizeMEBX = MEBX_DEFAULT_SIZE;
    writeReverse(MEBX_TYPE, &mebx->type);
    mebx->reserved[0] = 0;
    mebx->reserved[1] = 0;
    mebx->reserved[2] = 0;
    mebx->reserved[3] = 0;
    mebx->reserved[4] = 0;
    mebx->reserved[5] = 0;
    writeReverse16(1, &mebx->data_reference_index);
    sizeMEBX += MetadataKeyTableBox_new(&mebx->metadataKeyTableBox, sensorID);
    writeReverse(sizeMEBX, &mebx->size);
    //printf("written mebx\t:%d\n", sizeMEBX);
    return sizeMEBX;
}

int SampleDescriptionTableBox_write(SampleDescriptionTableBox mebx, FILE *smi)
{
    int written = 0;
    written += fwrite(&mebx.size, sizeof(mebx.size), 1, smi);
    written += fwrite(&mebx.type, sizeof(mebx.type), 1, smi);
    written += fwrite(&mebx.reserved[0], sizeof(mebx.reserved[0]), 1, smi);
    written += fwrite(&mebx.reserved[1], sizeof(mebx.reserved[1]), 1, smi);
    written += fwrite(&mebx.reserved[2], sizeof(mebx.reserved[2]), 1, smi);
    written += fwrite(&mebx.reserved[3], sizeof(mebx.reserved[3]), 1, smi);
    written += fwrite(&mebx.reserved[4], sizeof(mebx.reserved[4]), 1, smi);
    written += fwrite(&mebx.reserved[5], sizeof(mebx.reserved[5]), 1, smi);
    written += fwrite(&mebx.data_reference_index, sizeof(mebx.data_reference_index), 1, smi);
    written += MetadataKeyTableBox_write(mebx.metadataKeyTableBox, smi);
    //printf("mebx written on file: %d\n", written);
    return written;
}

int MetadataKeyTableBox_new(MetadataKeyTableBox *keys, int sensorID)
{
    int sizeKEYS = BOX_DEFAULT_SIZE;
    writeReverse(KEYS_TYPE, &keys->type);
    sizeKEYS += MetadataKeyBox_new(&keys->metadataKeyBox, sensorID);
    writeReverse(sizeKEYS, &keys->size);
    //printf("written keys\t:%d\n", sizeKEYS);
    return sizeKEYS;
}

int MetadataKeyTableBox_write(MetadataKeyTableBox keys, FILE *smi)
{
    int written = 0;
    written += fwrite(&keys.size, sizeof(keys.size), 1, smi);
    written += fwrite(&keys.type, sizeof(keys.type), 1, smi);
    written += MetadataKeyBox_write(keys.metadataKeyBox, smi);
    //printf("keys written on file: %d\n", written);
    return written;
}

int MetadataKeyBox_new(MetadataKeyBox *snsr, int sensorID)
{
    int sizeSNSR = BOX_DEFAULT_SIZE;
    snsr->type[0] = 's';
    snsr->type[1] = 'n';
    snsr->type[2] = 's';
    snsr->type[3] = sensorID + '0';
    sizeSNSR += MetadataKeyDeclarationBox_new(&snsr->metadataKeyDeclarationBox);
    sizeSNSR += MetadataDatatypeDefinitionBox_new(&snsr->metadataDatatypeDefinitionBox);
    writeReverse(sizeSNSR, &snsr->size);
    //printf("written snsr\t:%d\n", sizeSNSR);
    return sizeSNSR;
}

int MetadataKeyBox_write(MetadataKeyBox snsr, FILE *smi)
{
    int written = 0;
    written += fwrite(&snsr.size, sizeof(snsr.size), 1, smi);
    written += fwrite(&snsr.type[0], sizeof(snsr.type[0]), 1, smi);
    written += fwrite(&snsr.type[1], sizeof(snsr.type[1]), 1, smi);
    written += fwrite(&snsr.type[2], sizeof(snsr.type[2]), 1, smi);
    written += fwrite(&snsr.type[3], sizeof(snsr.type[3]), 1, smi);
    written += MetadataDatatypeDefinitionBox_write(snsr.metadataDatatypeDefinitionBox, smi);
    written += MetadataKeyDeclarationBox_write(snsr.metadataKeyDeclarationBox, smi);
    //printf("snsr written on file: %d\n", written);
    return written;
}

int MetadataKeyDeclarationBox_new(MetadataKeyDeclarationBox *keyd)
{
    int sizeKEYD = KEYD_SIZE;
    writeReverse(KEYD_TYPE, &keyd->type);
    writeReverse(sizeKEYD, &keyd->size);
    writeReverse('mdta', &keyd->keyNamespace);
    writeReverse('snsr', &keyd->keyValue);
    //printf("written keyd\t:%d\n", sizeKEYD);
    return sizeKEYD;
}

int MetadataKeyDeclarationBox_write(MetadataKeyDeclarationBox keyd, FILE *smi)
{
    int written = 0;
    written += fwrite(&keyd, sizeof(keyd), 1, smi);
    //printf("keyd written on file: %d\n", written);
    return written;
}

int MetadataDatatypeDefinitionBox_new(MetadataDatatypeDefinitionBox *dtyp)
{
    int sizeDTYP = DTYP_SIZE;
    writeReverse(DTYP_TYPE, &dtyp->type);
    writeReverse(sizeDTYP, &dtyp->size);
    writeReverse(76, &dtyp->array); // TODO: understand better how to deal with this
    dtyp->nameSpace = 0;
    //printf("written dtyp\t:%d\n", sizeDTYP);
    return sizeDTYP;
}

int MetadataDatatypeDefinitionBox_write(MetadataDatatypeDefinitionBox dtyp, FILE *smi)
{
    int written = 0;
    written += fwrite(&dtyp, sizeof(dtyp), 1, smi);
    //printf("dtyp written on file: %d\n", written);
    return written;
}

#endif