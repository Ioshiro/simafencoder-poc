#ifndef MINF_H
#define MINF_H

#include "../smiencoder.h"
#include "../util.h"
#include "stbl.h"
#include <time.h>

#define MINF_TYPE 'minf'
#define SMHD_TYPE 'smhd'
#define SMHD_SIZE 16
#define GMHD_TYPE 'gmhd'
#define GMIN_TYPE 'gmin'
#define GMIN_SIZE 24
#define DINF_TYPE 'dinf'
#define DREF_TYPE 'dref'
#define URL_TYPE 'url '

int MediaInformationBox_new(MediaInformationBox *, WavHeader, int, int, int);
int MediaInformationBox_write(MediaInformationBox, FILE *, int);
int SoundMediaHeaderBox_new(SoundMediaHeaderBox *);
int SoundMediaHeaderBox_write(SoundMediaHeaderBox, FILE *);
int BaseMediaInformationHeaderBox_new(BaseMediaInformationHeaderBox *);
int BaseMediaInformationHeaderBox_write(BaseMediaInformationHeaderBox, FILE *);
int BaseMediaInformationBox_new(BaseMediaInformationBox *);
int BaseMediaInformationBox_write(BaseMediaInformationBox, FILE *);
int DataInformationBox_new(DataInformationBox *);
int DataInformationBox_write(DataInformationBox, FILE *);
int DataReferenceBox_new(DataReferenceBox *);
int DataReferenceBox_write(DataReferenceBox, FILE *);
int DataEntryUrlBox_new(DataEntryUrlBox *);
int DataEntryUrlBox_write(DataEntryUrlBox, FILE *);

int MediaInformationBox_new(MediaInformationBox *minf, WavHeader wavHeader, int chunkOffset, int isAudio, int sensorID)
{
    int sizeMINF = BOX_DEFAULT_SIZE;
    writeReverse(MINF_TYPE, &minf->type);
    if (isAudio)
        sizeMINF += SoundMediaHeaderBox_new(&minf->soundMediaHeaderBox);
    else
        sizeMINF += BaseMediaInformationHeaderBox_new(&minf->baseMediaInformationHeaderBox);
    sizeMINF += DataInformationBox_new(&minf->dataInformationBox);
    sizeMINF += SampleTableBox_new(&minf->sampleTableBox, wavHeader, chunkOffset, isAudio, sensorID);
    writeReverse(sizeMINF, &minf->size);
    //printf("written minf\t:%d\n", sizeMINF);
    return sizeMINF;
}

int MediaInformationBox_write(MediaInformationBox minf, FILE *smi, int isAudio)
{
    int written = 0;
    written += fwrite(&minf.size, sizeof(minf.size), 1, smi);
    written += fwrite(&minf.type, sizeof(minf.type), 1, smi);
    if (isAudio)
        written += SoundMediaHeaderBox_write(minf.soundMediaHeaderBox, smi);
    else
        written += BaseMediaInformationHeaderBox_write(minf.baseMediaInformationHeaderBox, smi);
    written += DataInformationBox_write(minf.dataInformationBox, smi);
    written += SampleTableBox_write(minf.sampleTableBox, smi, isAudio);
    //printf("minf written on file: %d\n", written);
    return written;
}

int SoundMediaHeaderBox_new(SoundMediaHeaderBox *smhd)
{
    int sizeSMHD = SMHD_SIZE;
    writeReverse(SMHD_TYPE, &smhd->type);
    writeReverse(sizeSMHD, &smhd->size);
    smhd->reserved = 0;
    smhd->balance = 0;
    smhd->version = 0;
    //printf("written smhd\t:%d\n", sizeSMHD);
    return sizeSMHD;
}

int SoundMediaHeaderBox_write(SoundMediaHeaderBox smhd, FILE *smi)
{
    int written = 0;
    written += fwrite(&smhd, sizeof(smhd), 1, smi);
    //printf("smhd written on file: %d\n", written);
    return written;
}

int BaseMediaInformationHeaderBox_new(BaseMediaInformationHeaderBox *gmhd)
{
    int sizeGMHD = BOX_DEFAULT_SIZE;
    writeReverse(GMHD_TYPE, &gmhd->type);
    sizeGMHD += BaseMediaInformationBox_new(&gmhd->baseMediaInformationBox);
    writeReverse(sizeGMHD, &gmhd->size);
    //printf("written gmhd\t:%d\n", sizeGMHD);
    return sizeGMHD;
}

int BaseMediaInformationHeaderBox_write(BaseMediaInformationHeaderBox gmhd, FILE *smi)
{
    int written = 0;
    written += fwrite(&gmhd.size, sizeof(gmhd.size), 1, smi);
    written += fwrite(&gmhd.type, sizeof(gmhd.type), 1, smi);
    written += BaseMediaInformationBox_write(gmhd.baseMediaInformationBox, smi);
    //printf("gmhd written on file: %d\n", written);
    return written;
}

int BaseMediaInformationBox_new(BaseMediaInformationBox *gmin)
{
    int sizeGMIN = GMIN_SIZE;
    writeReverse(GMIN_TYPE, &gmin->type);
    writeReverse(sizeGMIN, &gmin->size);
    gmin->balance = 0;
    gmin->versionAndFlag = 0;
    gmin->opColor[0] = 0;
    gmin->opColor[1] = 0;
    gmin->opColor[2] = 0;
    gmin->graphicsMode = 0;
    gmin->reserved = 0;
    //printf("written gmin\t:%d\n", sizeGMIN);
    return sizeGMIN;
}

int BaseMediaInformationBox_write(BaseMediaInformationBox gmin, FILE *smi)
{
    int written = 0;
    written += fwrite(&gmin, sizeof(gmin), 1, smi);
    //printf("gmin written on file: %d\n", written);
    return written;
}

int DataInformationBox_new(DataInformationBox *dinf)
{
    int sizeDINF = BOX_DEFAULT_SIZE;
    writeReverse(DINF_TYPE, &dinf->type);
    sizeDINF += DataReferenceBox_new(&dinf->dataReferenceBox);
    writeReverse(sizeDINF, &dinf->size);
    //printf("written dinf\t:%d\n", sizeDINF);
    return sizeDINF;
}

int DataInformationBox_write(DataInformationBox dinf, FILE *smi)
{
    int written = 0;
    written += fwrite(&dinf.size, sizeof(dinf.size), 1, smi);
    written += fwrite(&dinf.type, sizeof(dinf.type), 1, smi);
    written += DataReferenceBox_write(dinf.dataReferenceBox, smi);
    //printf("dinf written on file: %d\n", written);
    return written;
}

int DataReferenceBox_new(DataReferenceBox *dref)
{
    int sizeDREF = BOX_DEFAULT_SIZE + 8;
    writeReverse(DREF_TYPE, &dref->type);
    writeReverse(1, &dref->entry_count);
    dref->flags = 0;
    sizeDREF += DataEntryUrlBox_new(&dref->dataEntryUrlBox);
    writeReverse(sizeDREF, &dref->size);
    //printf("written dref\t:%d\n", sizeDREF);
    return sizeDREF;
}

int DataReferenceBox_write(DataReferenceBox dref, FILE *smi)
{
    int written = 0;
    written += fwrite(&dref.size, sizeof(dref.size), 1, smi);
    written += fwrite(&dref.type, sizeof(dref.type), 1, smi);
    written += fwrite(&dref.flags, sizeof(dref.flags), 1, smi);
    written += fwrite(&dref.entry_count, sizeof(dref.entry_count), 1, smi);
    written += DataEntryUrlBox_write(dref.dataEntryUrlBox, smi);
    //printf("dref written on file: %d\n", written);
    return written;
}

int DataEntryUrlBox_new(DataEntryUrlBox *url)
{
    int sizeURL = BOX_DEFAULT_SIZE + 4;
    writeReverse(URL_TYPE, &url->type);
    writeReverse(sizeURL, &url->size);
    url->flags = 0;
    //printf("written url\t:%d\n", sizeURL);
    return sizeURL;
}

int DataEntryUrlBox_write(DataEntryUrlBox url, FILE *smi)
{
    int written = 0;
    written += fwrite(&url, sizeof(url), 1, smi);
    //printf("url written on file: %d\n", written);
    return written;
}

#endif