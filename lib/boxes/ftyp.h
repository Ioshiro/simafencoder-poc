#ifndef FTYP_H
#define FTYP_H

#include "../smiencoder.h"
#include "../util.h"

#define FTYP_SIZE 24
#define FTYP_TYPE 'ftyp'
#define FTYP_MAJOR_BRAND 'im03'
#define FTYP_COMPATIBLE_BRAND 'isom'

int FileTypeBox_new(FileTypeBox*);
int FileTypeBox_write(FileTypeBox, FILE*);

// Constructor for File Type Box, sets major brand to im03,
// compatible brands to im03 and isom, and minor version to 0
int FileTypeBox_new(FileTypeBox *ftyp)
{
    writeReverse(FTYP_SIZE, &ftyp->size);                             // Default size       24bytes
    writeReverse(FTYP_TYPE, &ftyp->type);                             // Type of box        'ftyp'
    writeReverse(FTYP_MAJOR_BRAND, &ftyp->major_brand);               // Major brand        im03
    writeReverse(FTYP_MAJOR_BRAND, &ftyp->compatible_brands[0]);      // Compatible brands  im03
    writeReverse(FTYP_COMPATIBLE_BRAND, &ftyp->compatible_brands[1]); //                  & isom
    ftyp->minor_version = 0;                        // Minor version      0
    return FTYP_SIZE;
}

int FileTypeBox_write(FileTypeBox ftyp, FILE *smi)
{
    fwrite(&ftyp, sizeof(FileTypeBox), 1, smi);

    return sizeof(FileTypeBox);
}

#endif