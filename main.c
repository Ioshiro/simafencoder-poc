//
//  main.c
//  IM_AM Enco§er
//
//  Created by eugenio oñate hospital on 14/06/12.
//  Copyright (c) 2012 QM. All rights reserved.
//
//File input/output 
#include <stdio.h>
//Standard library: numeric conversion, memory allocation...
#include <stdlib.h>
//Operations with strings
#include <string.h>
//Get the creation time: clock
#include <time.h>
#include <stdint.h>
#include "IM_AF Encoder.h"


/*Prototypes*/

void filetypebx(FileTypeBox *ftyp);
int mdatbox(MediaDataBox *mdat, WavHeader*, int, FILE *imf, FILE *song, int);
void moovheaderbox(MovieBox *moov, int, int, int, int, int, int, int);
int trackstructure(MovieBox *moov, int, int, int, int, char name[20]);
int samplecontainer(MovieBox *moov, int, int, char name[20]);
int sampledescription(MovieBox *moov, int);
int presetcontainer(MovieBox *moov, int,nametrack namet);
int rulescontainer(MovieBox *moov);
void writemoovbox(MovieBox moov, int numtrack,int totaltracks, FILE *imf);
int readTrack(MovieBox *moov, int, char name[20]);
int byterevers(int);
FILE* readSensorHeader(char*[], WavHeader*);
int trakSensor (MovieBox *moov, WavHeader, int numtrack, int clock, 
                    int durationTrack, int sizemdat, char name[20], int referenceTrackID);
int sampleSensor(MovieBox *moov, WavHeader header, int numtrack, int sizemdat, char name[20]);
int sampleDescriptionSensor(MovieBox *moov, int numtrack);
int writeSensorTrack(MovieBox moov, int numtrack, int, FILE*);


int main ()
{
    //variables
    FileTypeBox ftyp;
    MediaDataBox mdat;
    MovieBox moov;
    //MetaBox meta;
    nametrack namet;
    WavHeader sensorHeader;

    FILE *imf;
    int numtrack,totaltracks, sizemdat, durationTrack;
    int totalSensors = 1;
        
    /* Obtain current time as seconds elapsed since the Epoch. */
    time_t clock = time(NULL);
    
    printf("\nWelcome to the IM_AF encoder\n");
    printf("This program will allow you to create an IM_AF file with an audio and sensor data file.\n");
    printf("How many audio tracks there will be in your IMAF file? (use 1 for now)\n");
    scanf("%d",&totaltracks); 
    fflush(stdin);
    while (totaltracks > maxtracks) {
        printf("Sorry, for this version the number maximum ot tracks is %d\n",maxtracks);
        printf("How many tracks there will be in your IMAF file:\n");
        scanf("%d",&totaltracks); 
    }
    
    //Create the file
    imf = fopen ("example1.ima","wb"); 
    if (imf == NULL) {
       printf("Error opening input file\n");
       system("pause");
       exit(1);
       }    
    
    //Define the File Type Box
    filetypebx(&ftyp);
    fwrite(&ftyp, sizeof(FileTypeBox),1, imf);

    //Media Data Box - Contains the audio
    FILE *song;
    char nametrack[20];
    //Specify the path directory where there are the songs.
    //If change folder, change the path here (3 times) and in readTrack function!!!
    char pathdir[60] ="";
    int numtr, ex = 0;
    for (numtr=0; numtr<totaltracks; numtr++) {
        
        printf("Name of the track number: %d\n", numtr+1);
        fflush(stdin);
        scanf("%s", nametrack);
        strcpy(pathdir, "");
        strcat(pathdir, nametrack);
        ex = 0;
        //Check if the track exist and then open it.
        while (ex == 0){
            song = fopen(pathdir, "rb");
            if((song)==NULL) {
                printf("Name does not exist. Try again:\n");
                fflush(stdin);
                scanf("%s", nametrack);
                strcpy(pathdir, "");
                strcat(pathdir, nametrack);
            }else{
                ex = 1;
            }
        }
        strcpy(namet[numtr].title, nametrack);
        
        //Extract the samples from the audio file
        sizemdat = mdatbox(&mdat, &sensorHeader, totaltracks, imf, song, numtr);
        
        //Close the audio file
        fclose(song);
    }
   
    //For each track write track information
    u32 sizeTRAK = 0;
    char name[20];
    durationTrack = (int)(1000*(float)(sensorHeader.Chunk2_data_Size*8)/(sensorHeader.Num_Channels*sensorHeader.bits_per_sample*sensorHeader.Sample_rate));
    printf("Duration track\t:%d\n", durationTrack);
    
    for (numtrack = 0; numtrack < totaltracks; numtrack++) {
        strcpy(name,namet[numtrack].title);
        sizeTRAK =  trackstructure(&moov, numtrack, clock, durationTrack,sizemdat, name)+ sizeTRAK;
    }

    //Sensor trak
    sizeTRAK += trakSensor(&moov, sensorHeader, 1, clock, durationTrack, sizemdat, name, 1);
    
    //Presets
    u32 sizePRCO;
    sizePRCO = presetcontainer(&moov, totaltracks, namet); // Creates the preset, returns the size of the box.
    
    //Rules
    u32 sizeRUCO;
    sizeRUCO = rulescontainer(&moov); // Creates the rules, returns the size of the box.
    
    //Movie Header - Overall declarations
    moovheaderbox(&moov, clock, sizeTRAK, sizePRCO, totaltracks, durationTrack, sizeRUCO, totalSensors);
    
    //Writes the movie box into the file
    writemoovbox(moov,numtrack, totaltracks, imf); 
    
    //Close File
    fclose(imf); 
    
    printf("\nFile is created successfully, and ready to use!\n");
    
    return 0;
}

void filetypebx(FileTypeBox *ftyp){
    int swap;

    swap = byterevers (28);
    ftyp->size = swap;
    swap = byterevers ('ftyp');
    ftyp->type = swap;
    swap = byterevers ('qt  ');
    ftyp->major_brand = swap;
    ftyp->minor_version = 0;
    swap = byterevers ('im03');
    ftyp->compatible_brands[0] = swap;
    swap = byterevers ('isom');
    ftyp->compatible_brands[1] = swap;
    swap = byterevers ('qt  ');
    ftyp->compatible_brands[2] = swap;
}

int mdatbox(MediaDataBox *mdat, WavHeader* wavHeader, int totaltracks, FILE *imf, FILE *song, int numtr){

    int d, cnt, j, find = 0;
    int  dat = 0, dat1 = 0, dat2 = 0, dat3 = 0;
    u32 size = 0, swap, sizeMDAT =0;
    FILE* sensor;

    //Positonate the pointer at the end of the file to know the size of it
    fseek(song, 0, SEEK_END);
    size = ftell(song);
    //Positionate the pointer at first
    fseek(song, 0, SEEK_SET);
    d=0;
    cnt = 0;
    //Find the header of the first frame (the beginning), when find it d=1 and jump out the loop.
    // The header is 32 bytes. We find in groups of 8 bytes
    // Contemplate all possible options of headers
    while (d == 0) {
        find = 0;
        fread(&dat, sizeof(unsigned char), 1, song);
        cnt++;
        
        if (dat == 0xFF) {
            cnt++;                                              // cnt : stores the position of the pointer.  
            fread(&dat1, sizeof(unsigned char), 1, song);
            cnt++;
            fread(&dat2, sizeof(unsigned char), 1, song);
            cnt++;
            fread(&dat3, sizeof(unsigned char), 1, song);
            if (dat1 == 0xFB && dat2 == 146 && dat3 == 64 ) {
                find = 1;                                       // find: if the header is found
                d=1;                                            // d: jump out the loop
            }
            if (dat1 == 0xFB && dat2 == 146 && dat3 == 96 ) {
                d=1;
                find = 1;
            }
            if (dat1 == 0xFB && dat2 == 144 && dat3 == 64 ) {
                find = 1;
                d=1;
            }
            if (dat1 == 0xFB && dat2 == 144 && dat3 == 96 ) {
                find = 1;
                d=1;
            }     
            if (dat1 == 0xFB && dat2 == 146 && dat3 == 100 ) {
                d=1;
                find = 1;
            }
            if (dat1 == 0xFB && dat2 == 144 && dat3 == 100 ) {
                find = 1;
                d=1;
            }
            if (dat1 == 0xFA && dat2 == 146 && dat3 == 64 ) {
                find = 1;
                d=1;
            }
            if (dat1 == 0xFA && dat2 == 146 && dat3 == 96 ) {
                d=1;
                find = 1;
            }
            if (dat1 == 0xFA && dat2 == 144 && dat3 == 64 ) {
                find = 1;
                d=1;
            }
            if (dat1 == 0xFA && dat2 == 144 && dat3 == 96 ) {
                find = 1;
                d=1;
            }     
            if (dat1 == 0xFA && dat2 == 146 && dat3 == 100 ) {
                d=1;
                find = 1;
            }
            if (dat1 == 0xFA && dat2 == 144 && dat3 == 100 ) {
                find = 1;
                d=1;
            }
            if (find == 0) {
                fseek(song, -3, SEEK_CUR);
                cnt = cnt - 3;
            }
        }
        if (cnt == size) {
            d = 1;
        }
    }
    size =  size - (cnt - 4);       // Calculate the size of the samples. size = pos. end of file - pos. first header. 
    if (numtr == 0) {
        sizeMDAT = size*totaltracks + 8;    // size of the whole media box 
        //Add sensor file
        char namesensor[20];
        printf("Name of sensor data (.wav 5kHz, 16bit, monochannel): \n");
        fflush(stdin);
        scanf("%s", namesensor);
        
        //FILE *sensor = readSensorHeader(namesensor, &wavHeader);
        int ex = 0;
        while (ex == 0){
        sensor = fopen(namesensor, "r");
        if((sensor)==NULL) {
            printf("Name does not exist. Try again:\n");
            fflush(stdin);
            scanf("%s", namesensor);
        }else{
            ex = 1;
        }
        fread(wavHeader, 1, sizeof(WavHeader), sensor);
    }
     //sensor = fopen(namesensor, "rb");
        sizeMDAT += wavHeader->Chunk2_data_Size;
        swap = byterevers(sizeMDAT);
        fwrite(&swap, sizeof(u32), 1, imf);
        swap = byterevers('mdat');
        mdat->type = swap;
        fwrite(&mdat->type, sizeof(u32), 1, imf);
    }
    fseek(song, cnt - 4, SEEK_SET);
    printf("Size of track\t:%u\nSize of sensor\t:%u\n",size, wavHeader->Chunk2_data_Size);
    for (j=0; j<size; j++) {                //read all the samples of one track and writes them in the IM AF file
        fread(&mdat->data, sizeof(u8), 1, song);
        fwrite(&mdat->data, sizeof(u8), 1, imf);
    }
    for(int i = 0; i < wavHeader->Chunk2_data_Size; i++){
        fread(&mdat->data, sizeof(u8), 1, sensor);
        fwrite(&mdat->data, sizeof(u8), 1, imf);        
    }
    fclose(sensor);
    return size;
}

int samplecontainer(MovieBox *moov, int numtrack, int sizemdat, char name[20]){

    u32 sizeSTSD, sizeSTSZ, swap, num_samples, dat=0;

    //Sample Description Box//
    sizeSTSD = sampledescription(moov, numtrack); 
     
    //Sample size box//
    swap = byterevers('stsz');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleSizeBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleSizeBox.version = 0;
    //Read Track: Frame size and Decoder Times 
    num_samples = readTrack(moov, numtrack, name);
    sizeSTSZ = num_samples*4 + 20;
    swap = byterevers(sizeSTSZ);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleSizeBox.size = swap;

    //Time To Sample Box//
    u32 sizetime, sizeSTTS;
    sizetime = byterevers(moov->TrackBox[numtrack].MediaBox.MediaInformationBox.
                          SampleTableBox.TimeToSampleBox.entry_count);
    sizeSTTS = 16 + sizetime*4*2;
    swap = byterevers(sizeSTTS);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.size = swap;
    swap = byterevers('stts');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.version = 0;
        
    //Sample To Chunk//
    u32 sizeSTSC = 28;
    swap = byterevers(sizeSTSC);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.size = swap;
    swap = byterevers('stsc');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.version = 0;
    swap = byterevers(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.entry_count = swap;
    swap = byterevers(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.first_chunk = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.samples_per_chunk = moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleSizeBox.sample_count;
    swap = byterevers(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.sample_description_index = swap;
    
    //Chunk Offset Box//
    u32 sizeSTCO = 20;
    swap = byterevers(sizeSTCO);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    ChunkOffsetBox.size = swap;
    swap = byterevers('stco');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    ChunkOffsetBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    ChunkOffsetBox.version = 0;
    swap = byterevers(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    ChunkOffsetBox.entry_count = swap;
    dat = 32 + sizemdat*numtrack;
    swap = byterevers(dat);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    ChunkOffsetBox.chunk_offset[numtrack] = swap;
    
    //Sample Table Box //
    u32 sizeSTBL = 8 + sizeSTSD + sizeSTSZ + sizeSTSC + sizeSTCO + sizeSTTS;
    swap = byterevers(sizeSTBL);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.size = swap;
    swap = byterevers('stbl');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.type =swap;

    return  sizeSTBL;
}

int sampledescription(MovieBox *moov, int numtrack){
    u32 swap, sizeESD = 35;
    swap = byterevers(sizeESD);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.size = swap;
    swap = byterevers('esds');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.version = 0;
    
    //ES Descriptor//
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.tag = 3;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.length = 21;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.ES_ID = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.mix = 0;
    
    //Decoder config descriptor//
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.
    DecoderConfigDescriptor.tag = 4;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.
    DecoderConfigDescriptor.length = 13;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.
    DecoderConfigDescriptor.objectProfileInd = 0x6B;
    swap = byterevers(0x150036B0);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.
    DecoderConfigDescriptor.mix = swap;
    swap = byterevers(128);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.
    DecoderConfigDescriptor.maxBitRate = swap;
    swap = byterevers(128);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.
    DecoderConfigDescriptor.avgBitrate = swap;
    
    //SLConfig Descriptor//
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.
    SLConfigDescriptor.tag = 6;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.
    SLConfigDescriptor.length = 1;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.
    SLConfigDescriptor.predifined = 2;
    
    //Audio Sample Entry//
    u32 sizeMP4a = 36 + sizeESD;
    swap = byterevers(sizeMP4a);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.size = swap;
    swap = byterevers('mp4a');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.type =swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.reserved[0] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.reserved[1] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.reserved[2] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.reserved[3] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.reserved[4] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.reserved[5] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.data_reference_index = 256;           // TODO: Discover what is this number
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.reserved2[0] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.reserved2[1] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.channelcount = 512; 
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.samplesize = 4096; // 16 bits
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.reserved3 = 0;
    swap = 44100 << 16;
    swap = byterevers(swap);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.samplerate = swap;
    
    //Sample description box //
    u32 sizeSTSD = 16 + sizeMP4a;
    swap = byterevers(sizeSTSD);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.size = swap;
    swap = byterevers('stsd');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.version = 0; 
    swap = byterevers(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.entry_count = swap; 

    return sizeSTSD;
}

int readTrack (MovieBox *moov, int numtrack, char name[20]){
    
    FILE *song;
    int d=0, cnt = 0, i=0, j=0, cnt2 = 0, find = 0, swap, num_entr = 0;
    int  dat = 0, dat1 = 0, dat2 = 0, dat3 = 0, num_frame = 0, end =0, pos = 0;
    u32 size[9000];
    //Change path directory here
    char pathdir[60] = "";
    strcat(pathdir, name);
    //Open the audio file with the name introduced by the user
    song = fopen (pathdir,"rb"); 
    if (song == NULL) {
        printf("Error opening input file\n");
        system("pause");
        exit(1);
    }
    //Calculate the size of the track
    fseek(song, 0, SEEK_END);
    end = ftell(song);
    fseek(song, 0, SEEK_SET);
    d=0, i=0;
    //Search for each frame one by one, and extratcs the information
    while (d == 0) {
        find = 0;
        fread(&dat, sizeof(unsigned char), 1, song);
        cnt++;
        
        if (dat == 0xFF) {
            cnt++;
            fread(&dat1, sizeof(unsigned char), 1, song);
            cnt++;
            fread(&dat2, sizeof(unsigned char), 1, song);
            cnt++;
            fread(&dat3, sizeof(unsigned char), 1, song);
            if (dat1 == 0xFB && dat2 == 146 && dat3 == 64 ) {
                pos = cnt - 4;                      //Pos of the beginning of the frame
                size[num_frame] = pos - cnt2;     //Size of one frame
                cnt2 = pos;                         //Pos of the next frame
                find = 1;
                num_frame ++;                     //Number of frames
            }
            if (dat1 == 0xFB && dat2 == 146 && dat3 == 96 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (dat1 == 0xFB && dat2 == 144 && dat3 == 64 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (dat1 == 0xFB && dat2 == 144 && dat3 == 96 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (dat1 == 0xFB && dat2 == 146 && dat3 == 100 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (dat1 == 0xFB && dat2 == 144 && dat3 == 100 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (dat1 == 0xFA && dat2 == 146 && dat3 == 64 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (dat1 == 0xFA && dat2 == 146 && dat3 == 96 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (dat1 == 0xFA && dat2 == 144 && dat3 == 64 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (dat1 == 0xFA && dat2 == 144 && dat3 == 96 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (dat1 == 0xFA && dat2 == 146 && dat3 == 100 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (dat1 == 0xFA && dat2 == 144 && dat3 == 100 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (find == 0) { //In case it does not find the header.
                             //It keeps reading next data without jump any position
                fseek(song, -3, SEEK_CUR);
                cnt = cnt - 3;
            }
        }
        
        if (cnt == end) {
            pos = cnt;
            size[num_frame] = pos - cnt2;
            d = 1;
        }
    }
    //Save Samples size//
    swap = byterevers(num_frame);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleSizeBox.sample_count = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleSizeBox.sample_size = 0;
    
    for (i=0; i< num_frame; i++) {
        swap = byterevers(size[i+1]);
        moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
        SampleSizeBox.entry_size[i] = swap;
    }
    
    //Save Decoding Times//
    //Writes manually the duration of each frame. 
    //Follows the following structure: 
    //  7 frames of 26 ms
    //  1 frame  of 27 ms
    //      ...
    // And each 13 rows it writes
    //  8 frames of 26 ms
    //  1 frame  of 27 ms
    //It is done for adjusting the different durations of each frame.
    //                  as they vary between 26.125 ms and 26.075 ms
    
    swap = byterevers(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.sample_count[0] = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.sample_delta[0] =0;
    int t=0,k=1, l =0;
    num_entr = 1;
    j = 0;
    for (i = 1; i< num_frame; i++) {
        if (j == 8 && l == 0) {
            swap = byterevers(7);
            moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.sample_count[num_entr] = swap;
            swap = byterevers(26);
            moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.sample_delta[num_entr] =swap; 
            num_entr ++;
            
            swap = byterevers(1);
            moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.sample_count[num_entr] = swap;
            swap = byterevers(27);
            moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.sample_delta[num_entr] =swap;  
            num_entr++;
            j=0;
            dat = i;            
            if (k == 6 && t == 0) {
                l = 1;
                t = 1;
                k = 1;
            }
            if (k == 6 && t ==1) {
                l = 1;
                k = 1;
            }
            k++;
        }
        
        if (j == 9 && l == 1) {
            
            swap = byterevers(8);
            moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.sample_count[num_entr] = swap;
            swap = byterevers(26);
            moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.sample_delta[num_entr] =swap; 
            num_entr ++;
            
            swap = byterevers(1);
            moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.sample_count[num_entr] = swap;
            swap = byterevers(27);
            moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.sample_delta[num_entr] =swap;  
            num_entr++;
            j=0;
            dat = i;
            l = 0;
        }
        j++;
    }

    dat = num_frame - dat;
    
    swap = byterevers(dat);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.sample_count[num_entr] = swap;
    swap = byterevers(26);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.sample_delta[num_entr] =swap; 
    num_entr++;
    swap = byterevers(num_entr);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.entry_count = swap;
    
    fclose(song);
    return num_frame;

}

int trackstructure (MovieBox *moov, int numtrack, int clock, 
                    int durationTrack, int sizemdat, char name[20]){
    int swap;

    //Sample Table Box
    int sizeSTBL = 0;
    sizeSTBL = samplecontainer(moov, numtrack,sizemdat, name);
    
    //Data Entry Url Box
    u32 sizeURL = 12;
    swap = byterevers(sizeURL);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.DataEntryUrlBox.size = swap;
    swap = byterevers('url ');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.DataEntryUrlBox.type = swap;
    swap = byterevers(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.DataEntryUrlBox.flags = swap; // =1 Track in same file as movie atom.
    
    //Data Reference
    u32 sizeDREF = sizeURL+ 16;
    swap = byterevers(sizeDREF);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.size = swap;
    swap = byterevers('dref');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.flags = 0;
    swap = byterevers(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.entry_count = swap;
    
    //Data information Box//
    u32 sizeDINF = sizeDREF + 8;
    swap = byterevers(sizeDINF);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.size = swap;
    swap = byterevers('dinf');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.type = swap;
    
    //Sound Header Box //
    u32 sizeSMHD = 16;
    swap = byterevers(sizeSMHD);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SoundMediaHeaderBox.size = swap;
    swap = byterevers('smhd');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SoundMediaHeaderBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SoundMediaHeaderBox.version = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SoundMediaHeaderBox.balance = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SoundMediaHeaderBox.reserved = 0;
    
    //Media Information Box//
    u32 sizeMINF = sizeDINF + sizeSMHD + sizeSTBL + 8;
    swap = byterevers(sizeMINF);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.size = swap;
    swap = byterevers('minf');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.type = swap;
    
    //Handler Box//
    u32 sizeHDLR = 37;
    swap = byterevers(sizeHDLR); 
    moov->TrackBox[numtrack].MediaBox.HandlerBox.size = swap;
    swap = byterevers('hdlr');
    moov->TrackBox[numtrack].MediaBox.HandlerBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.version = 0;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.pre_defined = 0;
    swap = byterevers('soun');
    moov->TrackBox[numtrack].MediaBox.HandlerBox.handler_type = swap;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.reserved[0] = 0;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.reserved[1] = 0;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.reserved[2] = 0;
    //swap = byterevers('soun');
    //moov->TrackBox[numtrack].MediaBox.HandlerBox.data = swap;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.data[0] = 's';
    moov->TrackBox[numtrack].MediaBox.HandlerBox.data[1] = 'o';
    moov->TrackBox[numtrack].MediaBox.HandlerBox.data[2] = 'u';
    moov->TrackBox[numtrack].MediaBox.HandlerBox.data[3] = 'n';
    moov->TrackBox[numtrack].MediaBox.HandlerBox.data[4] = '\0';
    
    //Media Header Box//
    u32 sizeMDHD = 32;
    swap = byterevers(sizeMDHD);
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.size = swap;
    swap = byterevers('mdhd');
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.version = 0;
    swap = byterevers(clock);
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.creation_time = swap;
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.modification_time = swap;
    swap = byterevers(1000);
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.timescale = swap;
    swap = byterevers(durationTrack);
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.duration = swap;
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.language = 0xC455;
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.pre_defined = 0;
    
    //Media Box//
    u32 sizeMDIA = sizeMDHD + sizeHDLR + sizeMINF + 8;
    swap = byterevers(sizeMDIA);
    moov->TrackBox[numtrack].MediaBox.size = swap;
    swap = byterevers('mdia');
    moov->TrackBox[numtrack].MediaBox.type = swap;
    
    //Track Header//
    u32 sizeTKHD = 92;
    swap = byterevers (sizeTKHD);
    moov->TrackBox[numtrack].TrackHeaderBox.size = swap;
    swap = byterevers ('tkhd');
    moov->TrackBox[numtrack].TrackHeaderBox.type = swap ;
    swap = byterevers (0x00000006);
    moov->TrackBox[numtrack].TrackHeaderBox.version = swap;
    swap = byterevers (clock);
    moov->TrackBox[numtrack].TrackHeaderBox.creation_time = swap;
    moov->TrackBox[numtrack].TrackHeaderBox.modification_time = swap;
    swap = byterevers (numtrack+1);
    moov->TrackBox[numtrack].TrackHeaderBox.track_ID = swap; //From 0x00000001 - 0x7FFFFFFF (dec 2147483647)
    moov->TrackBox[numtrack].TrackHeaderBox.reserved = 0;
    swap = byterevers (durationTrack);
    moov->TrackBox[numtrack].TrackHeaderBox.duration = swap;
    moov->TrackBox[numtrack].TrackHeaderBox.reserved2[0] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.reserved2[1] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.layer = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.alternate_group = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.volume = 0x1;
    moov->TrackBox[numtrack].TrackHeaderBox.reserved3 = 0;
    swap = byterevers (0x00010000);
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[0] = swap;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[1] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[2] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[3] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[4] = swap;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[5] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[6] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[7] = 0;
    swap = byterevers(0x40000000);
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[8] = swap;
    moov->TrackBox[numtrack].TrackHeaderBox.width = 0; //just for video
    moov->TrackBox[numtrack].TrackHeaderBox.height = 0; //just for video
    
    //Track container
    u32 sizeTRAK = sizeTKHD + sizeMDIA + 8;
    swap = byterevers (sizeTRAK); // Size of one track
    moov->TrackBox[numtrack].size = swap; 
    swap = byterevers ('trak');
    moov->TrackBox[numtrack].type = swap;
    return sizeTRAK;

}

int presetcontainer(MovieBox *moov, int totaltracks, nametrack namet){
    
    int swap, i,j,flag, vol=0; 
    unsigned char numpres=0, prestype=0,defaultPreset;
    char namepres1[14] = "static_track "; // 13  
    u32 sizePRST = 0;

    printf("\nPresets:\n");
    printf("Static track volume preset: invariant volume related to each track \n");
    printf("          --------------------------------------\n");
    numpres = 1;
    
    //Preset Box//
    for (i=0; i<numpres; i++) {
        printf("Preset number %d: %s\n",i+1,namepres1);
        strcpy(moov->PresetContainerBox.PresetBox[i].preset_name, namepres1);
        sizePRST = 16 + 14 + 4*totaltracks + totaltracks;
        swap = byterevers(sizePRST);
        moov->PresetContainerBox.PresetBox[i].size = swap;
        prestype = 0;

        moov->PresetContainerBox.PresetBox[i].num_preset_elements = totaltracks;
        swap = byterevers('prst');
        moov->PresetContainerBox.PresetBox[i].type = swap;
    
        flag = 0x02; // Display Enable Edit Disable
        swap = byterevers(flag);
        moov->PresetContainerBox.PresetBox[i].flags = swap;
        
        moov->PresetContainerBox.PresetBox[i].preset_ID = i+1;
        
        moov->PresetContainerBox.PresetBox[i].preset_type = prestype;
        moov->PresetContainerBox.PresetBox[i].preset_global_volume = 100;
        
        for (j=0; j<totaltracks; j++) {
            swap = byterevers(j+1);
            moov->PresetContainerBox.PresetBox[i].presElemId[j].preset_element_ID = swap;
        }
        //Enter values (two options):
        // In loop
        for (j=0; j<totaltracks; j++) {
            vol = 70 - 10*(j+1);
        //    vol = 20*(j+1);
            printf("Enter volume for %s = %d\n",namet[j].title,vol*2);
        //     scanf("%d",&vol);
        //     printf("Vol: %d\n",vol);
            moov->PresetContainerBox.PresetBox[i].presVolumElem[j].preset_volume_element = vol; //*0.02 
        }

        // Or one by one
/*        moov->PresetContainerBox.PresetBox[i].presVolumElem[0].preset_volume_element = 20;
        printf("Enter volume for %s = %d\n",namet[j].title,moov->PresetContainerBox.
               PresetBox[i].presVolumElem[0].preset_volume_element*2);
        moov->PresetContainerBox.PresetBox[i].presVolumElem[1].preset_volume_element = 40;
        printf("Enter volume for %s = %d\n",namet[j].title,moov->PresetContainerBox.
               PresetBox[i].presVolumElem[1].preset_volume_element*2);
        moov->PresetContainerBox.PresetBox[i].presVolumElem[2].preset_volume_element = 20;
        printf("Enter volume for %s = %d\n",namet[j].title,moov->PresetContainerBox.
               PresetBox[i].presVolumElem[2].preset_volume_element*2);
        moov->PresetContainerBox.PresetBox[i].presVolumElem[3].preset_volume_element = 50;
        printf("Enter volume for %s = %d\n",namet[j].title,moov->PresetContainerBox.
               PresetBox[i].presVolumElem[3].preset_volume_element*2);
        moov->PresetContainerBox.PresetBox[i].presVolumElem[4].preset_volume_element = 52;
        printf("Enter volume for %s = %d\n",namet[j].title,moov->PresetContainerBox.
               PresetBox[i].presVolumElem[4].preset_volume_element*2);
 */   }
    
    //Preset Container//
    u32 sizePRCO = sizePRST + 10;
    swap = byterevers(sizePRCO);
    moov->PresetContainerBox.size = swap; 
    swap = byterevers('prco');
    moov->PresetContainerBox.type = swap;
    defaultPreset = 1;
    moov->PresetContainerBox.default_preset_ID = defaultPreset; // Indicates initial preset activated.
    moov->PresetContainerBox.num_preset = numpres;
    
    return sizePRCO;
}

int rulescontainer(MovieBox *moov){
    int swap;
    u32 sizeRUSC, elementID, key_elem, sizeRUMX;
    
    moov->RulesContainer.num_selection_rules = 256; //u16 invert
 //   moov->RulesContainer.num_selection_rules = 0; 
    moov->RulesContainer.num_mixing_rules = 256; //u16 invert
    
    //Selection Rules
    sizeRUSC = 19 + 14;
 //   sizeRUSC = 0;
    swap = byterevers(sizeRUSC);
    moov->RulesContainer.SelectionRules.size = swap; 
    swap = byterevers('rusc');
    moov->RulesContainer.SelectionRules.type = swap;
    moov->RulesContainer.SelectionRules.version = 0;
    moov->RulesContainer.SelectionRules.selection_rule_ID = 256;
    moov->RulesContainer.SelectionRules.selection_rule_type = 2; 
    elementID = 3;
    swap = byterevers(elementID);
    moov->RulesContainer.SelectionRules.element_ID = swap;  
    strcpy(moov->RulesContainer.SelectionRules.rule_description,"Not mute rule");
    printf("\nRules:\n");
    printf("Rule 1: Not mute for channel %d\n",elementID);
    
    //Mixing Rule
    sizeRUMX = 23 + 17;
    swap = byterevers(sizeRUMX);
    moov->RulesContainer.MixingRules.size = swap;
    swap = byterevers('rumx');
    moov->RulesContainer.MixingRules.type = swap;
    moov->RulesContainer.MixingRules.version = 0;
    moov->RulesContainer.MixingRules.mixing_rule_ID = 512; 
 //  moov->RulesContainer.MixingRules.mixing_type = 0; // Equivalence rule
    moov->RulesContainer.MixingRules.mixing_type = 2; // Upper rule
    elementID = 1;
    swap = byterevers(elementID);
    moov->RulesContainer.MixingRules.element_ID = swap;
    key_elem = 2;
    swap = byterevers(key_elem);
    moov->RulesContainer.MixingRules.key_elem_ID = swap;
    strcpy(moov->RulesContainer.MixingRules.mix_description, "Lower rule");
    printf("Rule 2: Lower rule between channel %d and %d\n",elementID,key_elem);
 //   strcpy(moov->RulesContainer.MixingRules.mix_description, "Equivalence rule");
  //  printf("Rule 2: Equivalence rule\n");
    
    //Rule container
    u32 sizeRUCO = 12 + sizeRUSC + sizeRUMX;
    swap = byterevers(sizeRUCO);
    moov->RulesContainer.size = swap;
    swap = byterevers('ruco');
    moov->RulesContainer.type = swap;
    
    return sizeRUCO;
}

void moovheaderbox (MovieBox *moov,int clock, int sizeTRAK, int sizePRCO, int totaltracks, 
                    int durationTrack, int sizeRUCO, int totalSensors){
    int swap;

    //MovieHeader
    u32 sizeMVHD = 108;
    swap = byterevers (sizeMVHD);
    moov->MovieHeaderBox.size = swap;
    swap = byterevers ('mvhd');
    moov->MovieHeaderBox.type = swap;
    moov->MovieHeaderBox.version = 0;
    swap = byterevers (clock);
    moov->MovieHeaderBox.creation_time = swap;
    moov->MovieHeaderBox.modification_time = swap;
    swap = byterevers (1000);
    moov->MovieHeaderBox.timescale = swap;
    swap = byterevers (durationTrack);
    moov->MovieHeaderBox.duration = swap;
    swap = byterevers (0x00010000);
    moov->MovieHeaderBox.rate = swap;
    swap = byterevers (1);
    moov->MovieHeaderBox.volume = 1;
    moov->MovieHeaderBox.reserved=0;
    moov->MovieHeaderBox.reserved2[0] = 0;
    moov->MovieHeaderBox.reserved2[1] = 0;
    swap = byterevers (0x00010000);
    moov->MovieHeaderBox.matrix[0] = swap;
    moov->MovieHeaderBox.matrix[1] = 0;
    moov->MovieHeaderBox.matrix[2] = 0;    
    moov->MovieHeaderBox.matrix[3] = 0;    
    moov->MovieHeaderBox.matrix[4] = swap;    
    moov->MovieHeaderBox.matrix[5] = 0;    
    moov->MovieHeaderBox.matrix[6] = 0;    
    moov->MovieHeaderBox.matrix[7] = 0;
    swap = byterevers (0x40000000);
    moov->MovieHeaderBox.matrix[8] = 0x40000000;    
    moov->MovieHeaderBox.pre_defined[0] = 0;
    moov->MovieHeaderBox.pre_defined[1] = 0;
    moov->MovieHeaderBox.pre_defined[2] = 0;
    moov->MovieHeaderBox.pre_defined[3] = 0;
    moov->MovieHeaderBox.pre_defined[4] = 0;
    moov->MovieHeaderBox.pre_defined[5] = 0;
    swap = byterevers (totaltracks + totalSensors + 1);
    moov->MovieHeaderBox.next_track_ID = swap;
    
    //MovieBox
    u32 sizeMOOV = sizeMVHD + sizeTRAK + sizePRCO + sizeRUCO + 8;
    swap = byterevers (sizeMOOV); //Size movie: Taking into account number tracks
    moov->size = swap;
    swap = byterevers ('moov');
    moov->type = swap;
}

void writemoovbox(MovieBox moov, int numtrack,int totaltracks, FILE *imf){
    //Write movie box//
    fwrite(&moov.size, sizeof(u32), 1, imf);
    fwrite(&moov.type, sizeof(u32), 1, imf);
    //Movie header//
    fwrite(&moov.MovieHeaderBox, sizeof(moov.MovieHeaderBox), 1, imf);
    //Track container//
    for (numtrack = 0; numtrack < totaltracks; numtrack++) {
        fwrite(&moov.TrackBox[numtrack].size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].type, sizeof(u32), 1, imf);
        //Trck header//
        fwrite(&moov.TrackBox[numtrack].TrackHeaderBox,
               sizeof(moov.TrackBox[numtrack].TrackHeaderBox), 1, imf);
        //Media Box//
        fwrite(&moov.TrackBox[numtrack].MediaBox.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.type, sizeof(u32), 1, imf);
        //Media Header//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaHeaderBox,
               sizeof(moov.TrackBox[numtrack].MediaBox.MediaHeaderBox), 1, imf);
        //Handler Box//
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.type, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.version, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.pre_defined, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.handler_type, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.reserved[0], sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.reserved[1], sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.reserved[2], sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.data[0], sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.data[1], sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.data[2], sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.data[3], sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.data[4], sizeof(unsigned char), 1, imf);
        //Media inforamtion box//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.type, sizeof(u32), 1, imf);
        //Sound media header//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SoundMediaHeaderBox, 
               sizeof(moov.TrackBox[numtrack].MediaBox.
                      MediaInformationBox.SoundMediaHeaderBox), 1, imf);
        //Data reference//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox,
               sizeof(moov.TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox), 1, imf);
        //Sample table box//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               type, sizeof(u32), 1, imf);
        
        int i, swap, pos;
        //Time to sample box//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               TimeToSampleBox.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               TimeToSampleBox.type, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               TimeToSampleBox.version, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               TimeToSampleBox.entry_count, sizeof(u32), 1, imf);
        
        swap = byterevers(moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                          TimeToSampleBox.entry_count);
        pos = swap;
        
        for (i=0; i<pos; i++) {
            
            fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                   TimeToSampleBox.sample_count[i], sizeof(u32), 1, imf);
            fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                   TimeToSampleBox.sample_delta[i], sizeof(u32), 1, imf);
        }
        
        //Sample description box//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.type, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                SampleDescriptionBox.version, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                SampleDescriptionBox.entry_count, sizeof(u32), 1, imf);
        //Audio Sample entry//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                SampleDescriptionBox.AudioSampleEntry.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.type, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.reserved[0], sizeof(unsigned char), 6, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.data_reference_index, sizeof(u16), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.reserved2[0], sizeof(u32), 2, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.channelcount, sizeof(u16), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.samplesize, sizeof(u16), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.reserved3, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.samplerate, sizeof(u32), 1, imf);        
        //ESDBox//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.type, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.version, sizeof(u32), 1, imf);
        //ES Descriptor//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.tag
               , sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.length
               , sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.ES_ID
               , sizeof(u16), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.mix
               , sizeof(unsigned char), 1, imf);
        //Decoder Config//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.DecoderConfigDescriptor.
               tag, sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.DecoderConfigDescriptor.
               length, sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.DecoderConfigDescriptor.
               objectProfileInd, sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.DecoderConfigDescriptor.
               mix, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.DecoderConfigDescriptor.
               maxBitRate, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.DecoderConfigDescriptor.
               avgBitrate, sizeof(u32), 1, imf);
/*        //DecoderSpecificInfo//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.DecoderConfigDescriptor.
               DecoderSpecificInfo.tag, sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.DecoderConfigDescriptor.
               DecoderSpecificInfo.length, sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.DecoderConfigDescriptor.
               DecoderSpecificInfo.decSpecificInfoData[0], sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.DecoderConfigDescriptor.
               DecoderSpecificInfo.decSpecificInfoData[1], sizeof(unsigned char), 1, imf);
  */      //SLConfig//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.SLConfigDescriptor.
               tag, sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.SLConfigDescriptor.
               length, sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.SLConfigDescriptor.
               predifined, sizeof(unsigned char), 1, imf);
  

        //Sample Size box//       
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleSizeBox.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleSizeBox.type, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleSizeBox.version, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleSizeBox.sample_size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleSizeBox.sample_count, sizeof(u32), 1, imf);
        swap = byterevers(moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                          SampleSizeBox.sample_count);
        for(i=0; i<swap; i++){
            fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                   SampleSizeBox.entry_size[i], sizeof(u32), 1, imf);
        }
        
        //Sample to chunk box// 
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleToChunk.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleToChunk.type, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleToChunk.version, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleToChunk.entry_count, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleToChunk.first_chunk, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleToChunk.samples_per_chunk, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleToChunk.sample_description_index, sizeof(u32), 1, imf);

        //Chunk offset//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               ChunkOffsetBox.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               ChunkOffsetBox.type, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               ChunkOffsetBox.version, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               ChunkOffsetBox.entry_count, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               ChunkOffsetBox.chunk_offset[numtrack], sizeof(u32), 1, imf);
    }
    writeSensorTrack(moov, numtrack, totaltracks, imf);
    //Preset Container//
    fwrite(&moov.PresetContainerBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.PresetContainerBox.type, sizeof(u32), 1, imf);
    fwrite(&moov.PresetContainerBox.num_preset, sizeof(unsigned char), 1, imf);
    fwrite(&moov.PresetContainerBox.default_preset_ID, sizeof(unsigned char), 1, imf);
    //Preset Box
    int j,i;
    for (i=0; i<moov.PresetContainerBox.num_preset; i++) {
        fwrite(&moov.PresetContainerBox.PresetBox[i].size, sizeof(u32), 1, imf);
        fwrite(&moov.PresetContainerBox.PresetBox[i].type, sizeof(u32), 1, imf);
        fwrite(&moov.PresetContainerBox.PresetBox[i].flags, sizeof(u32), 1, imf);
        fwrite(&moov.PresetContainerBox.PresetBox[i].preset_ID, sizeof(unsigned char), 1, imf);
        fwrite(&moov.PresetContainerBox.PresetBox[i].num_preset_elements,
               sizeof(unsigned char), 1, imf);
        for (j=0; j< moov.PresetContainerBox.PresetBox[i].num_preset_elements; j++) {
            fwrite(&moov.PresetContainerBox.PresetBox[i].presElemId[j].
                   preset_element_ID, sizeof(u32), 1, imf);
        }
        fwrite(&moov.PresetContainerBox.PresetBox[i].preset_type , sizeof(unsigned char), 1, imf);
        fwrite(&moov.PresetContainerBox.PresetBox[i].preset_global_volume,
               sizeof(unsigned char), 1, imf);
        for (j=0; j< moov.PresetContainerBox.PresetBox[i].num_preset_elements; j++) {
            fwrite(&moov.PresetContainerBox.PresetBox[i].presVolumElem[j].
                    preset_volume_element,sizeof(unsigned char), 1, imf);
        }
        for (j=0; j<14; j++) {
            fwrite(&moov.PresetContainerBox.PresetBox[i].preset_name[j], sizeof(char), 1, imf);
        }
    }
    
    //Rules Container//
    fwrite(&moov.RulesContainer.size, sizeof(u32), 1, imf);
    fwrite(&moov.RulesContainer.type, sizeof(u32), 1, imf);
    fwrite(&moov.RulesContainer.num_selection_rules, sizeof(u16), 1, imf);
    fwrite(&moov.RulesContainer.num_mixing_rules, sizeof(u16), 1, imf);
    //Selection Rules//
    fwrite(&moov.RulesContainer.SelectionRules.size, sizeof(u32), 1, imf);
    fwrite(&moov.RulesContainer.SelectionRules.type, sizeof(u32), 1, imf);
    fwrite(&moov.RulesContainer.SelectionRules.version, sizeof(u32), 1, imf);
    fwrite(&moov.RulesContainer.SelectionRules.selection_rule_ID, sizeof(u16), 1, imf);
    fwrite(&moov.RulesContainer.SelectionRules.selection_rule_type,
            sizeof(unsigned char), 1, imf);
    fwrite(&moov.RulesContainer.SelectionRules.element_ID, sizeof(u32), 1, imf);
    for(i=0; i<14; i++){
        fwrite(&moov.RulesContainer.SelectionRules.rule_description[i],
                sizeof(char), 1, imf);
    }
    //Mixing Rules//
    fwrite(&moov.RulesContainer.MixingRules.size, sizeof(u32), 1, imf);
    fwrite(&moov.RulesContainer.MixingRules.type, sizeof(u32), 1, imf);
    fwrite(&moov.RulesContainer.MixingRules.version, sizeof(u32), 1, imf);
    fwrite(&moov.RulesContainer.MixingRules.mixing_rule_ID, sizeof(u16), 1, imf);
    fwrite(&moov.RulesContainer.MixingRules.mixing_type,sizeof(unsigned char), 1, imf);
    fwrite(&moov.RulesContainer.MixingRules.element_ID, sizeof(u32), 1, imf);
    fwrite(&moov.RulesContainer.MixingRules.key_elem_ID, sizeof(u32), 1, imf);
    for(i=0; i<17; i++){
        fwrite(&moov.RulesContainer.MixingRules.mix_description[i],
               sizeof(char), 1, imf);
    }
    
}

FILE* readSensorHeader(char* namesensor[], WavHeader* header){
    FILE* sensor;
    int ex = 0;
    while (ex == 0){
        sensor = fopen(namesensor, "r");
        if((sensor)==NULL) {
            printf("Name does not exist. Try again:\n");
            fflush(stdin);
            scanf("%s", namesensor);
        }else{
            ex = 1;
        }
    }
    fread(header, 1, sizeof(header), sensor);
    return sensor;
}

int trakSensor (MovieBox *moov, WavHeader header, int numtrack, int clock, 
                    int durationTrack, int sizemdat, char name[20], int referenceTrackID){
    int swap;

    //Sample Table Box
    int sizeSTBL = 0;
    sizeSTBL = sampleSensor(moov, header, numtrack,sizemdat, name);
     
    //Base Media Header Box //
    u32 sizeGMHD = 32;
    swap = byterevers(sizeGMHD);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.size = swap;
    swap = byterevers('gmhd');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.type = swap;
    //Base Media Information Box//
    swap = byterevers(24);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.BaseMediaInformationBox.size = swap;
    swap = byterevers('gmin');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.BaseMediaInformationBox.type = swap;
    swap = byterevers(0);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.BaseMediaInformationBox.versionAndFlag = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.BaseMediaInformationBox.graphicsMode = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.BaseMediaInformationBox.opColor[0] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.BaseMediaInformationBox.opColor[1] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.BaseMediaInformationBox.opColor[2] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.BaseMediaInformationBox.size = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.BaseMediaInformationBox.size = 0;
        //Data Entry Url Box
    u32 sizeURL = 12;
    swap = byterevers(sizeURL);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.DataEntryUrlBox.size = swap;
    swap = byterevers('url ');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.DataEntryUrlBox.type = swap;
    swap = byterevers(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.DataEntryUrlBox.flags = swap; // =1 Track in same file as movie atom.
    
    //Data Reference
    u32 sizeDREF = sizeURL+ 16;
    swap = byterevers(sizeDREF);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.size = swap;
    swap = byterevers('dref');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.flags = 0;
    swap = byterevers(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.entry_count = swap;
    
    //Data information Box//
    u32 sizeDINF = sizeDREF + 8;
    swap = byterevers(sizeDINF);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.size = swap;
    swap = byterevers('dinf');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.type = swap;
    
    //Media Information Box//
    u32 sizeMINF = sizeDINF + sizeGMHD + sizeSTBL + 8;
    swap = byterevers(sizeMINF);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.size = swap;
    swap = byterevers('minf');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.type = swap;
    
    //Handler Box//
    u32 sizeHDLR = 37;
    swap = byterevers(sizeHDLR); 
    moov->TrackBox[numtrack].MediaBox.HandlerBox.size = swap;
    swap = byterevers('hdlr');
    moov->TrackBox[numtrack].MediaBox.HandlerBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.version = 0;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.pre_defined = 0;
    swap = byterevers('meta');
    moov->TrackBox[numtrack].MediaBox.HandlerBox.handler_type = swap;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.reserved[0] = 0;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.reserved[1] = 0;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.reserved[2] = 0;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.data[0] = 'm';
    moov->TrackBox[numtrack].MediaBox.HandlerBox.data[1] = 'e';
    moov->TrackBox[numtrack].MediaBox.HandlerBox.data[2] = 't';
    moov->TrackBox[numtrack].MediaBox.HandlerBox.data[3] = 'a';
    moov->TrackBox[numtrack].MediaBox.HandlerBox.data[4] = '\0';
    
    //Media Header Box//
    u32 sizeMDHD = 32;
    swap = byterevers(sizeMDHD);
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.size = swap;
    swap = byterevers('mdhd');
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.version = 0;
    swap = byterevers(clock);
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.creation_time = swap;
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.modification_time = swap;
    swap = byterevers(5000);
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.timescale = swap;
    swap = byterevers(durationTrack*5);
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.duration = swap;
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.language = 0xC455;
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.pre_defined = 0;
    
    //Media Box//
    u32 sizeMDIA = sizeMDHD + sizeHDLR + sizeMINF + 8;
    swap = byterevers(sizeMDIA);
    moov->TrackBox[numtrack].MediaBox.size = swap;
    swap = byterevers('mdia');
    moov->TrackBox[numtrack].MediaBox.type = swap;
    
    //Track Header//
    u32 sizeTKHD = 92;
    swap = byterevers (sizeTKHD);
    moov->TrackBox[numtrack].TrackHeaderBox.size = swap;
    swap = byterevers ('tkhd');
    moov->TrackBox[numtrack].TrackHeaderBox.type = swap ;
    swap = byterevers (0x00000006);
    moov->TrackBox[numtrack].TrackHeaderBox.version = swap;
    swap = byterevers (clock);
    moov->TrackBox[numtrack].TrackHeaderBox.creation_time = swap;
    moov->TrackBox[numtrack].TrackHeaderBox.modification_time = swap;
    swap = byterevers (numtrack+1);
    moov->TrackBox[numtrack].TrackHeaderBox.track_ID = swap; //From 0x00000001 - 0x7FFFFFFF (dec 2147483647)
    moov->TrackBox[numtrack].TrackHeaderBox.reserved = 0;
    swap = byterevers (durationTrack);
    moov->TrackBox[numtrack].TrackHeaderBox.duration = swap;
    moov->TrackBox[numtrack].TrackHeaderBox.reserved2[0] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.reserved2[1] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.layer = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.alternate_group = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.volume = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.reserved3 = 0;
    swap = byterevers (0x00010000);
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[0] = swap;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[1] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[2] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[3] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[4] = swap;           // identity matrix
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[5] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[6] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[7] = 0;
    //swap = byterevers(1);
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[8] = swap;
    moov->TrackBox[numtrack].TrackHeaderBox.width = 0; //just for video
    moov->TrackBox[numtrack].TrackHeaderBox.height = 0; //just for video

    //Track Reference Type Box
    u32 sizeCDSC = 12;
    swap = byterevers(sizeCDSC);
    moov->TrackBox[numtrack].TrackReferenceBox.TrackReferenceTypeBox.size = swap;
    swap = byterevers('cdsc');
    moov->TrackBox[numtrack].TrackReferenceBox.TrackReferenceTypeBox.type = swap;
    swap = byterevers(referenceTrackID);                                                            // reference to the first audio track
    moov->TrackBox[numtrack].TrackReferenceBox.TrackReferenceTypeBox.trackIDReference = swap;
    //Track Reference Box
    u32 sizeTREF = sizeCDSC + 8;
    swap = byterevers(sizeTREF);
    moov->TrackBox[numtrack].TrackReferenceBox.size = swap;
    swap = byterevers('tref');
    moov->TrackBox[numtrack].TrackReferenceBox.type = swap;
    
    //Track container
    u32 sizeTRAK = sizeTKHD + sizeMDIA + sizeTREF + 8;
    swap = byterevers (sizeTRAK); // Size of one track
    moov->TrackBox[numtrack].size = swap; 
    swap = byterevers ('trak');
    moov->TrackBox[numtrack].type = swap;
    return sizeTRAK;
}

int sampleSensor(MovieBox *moov, WavHeader header, int numtrack, int sizemdat, char name[20]){

    u32 sizeSTSD, sizeSTSZ, swap, num_samples, dat=0;

    //Sample Description Box//
    sizeSTSD = sampleDescriptionSensor(moov, numtrack); 
     

    //Sample size box//
    swap = byterevers('stsz');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleSizeBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleSizeBox.version = 0;
    num_samples = header.Chunk2_data_Size/(header.bits_per_sample/8);
    sizeSTSZ = 20;
    swap = byterevers(sizeSTSZ);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleSizeBox.size = swap;
    swap = byterevers(2);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleSizeBox.sample_size = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleSizeBox.sample_count = 0;


    //Time To Sample Box//
    u32 sizetime, sizeSTTS;
    sizetime = byterevers(moov->TrackBox[numtrack].MediaBox.MediaInformationBox.
                          SampleTableBox.TimeToSampleBox.entry_count);
    sizeSTTS = 24;
    swap = byterevers(sizeSTTS);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.size = swap;
    swap = byterevers('stts');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.version = 0;
    swap = byterevers(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.entry_count = swap;
    swap = byterevers(num_samples);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.sample_count[0] = swap;
    swap = byterevers(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.sample_delta[0] = swap;
        
    //Sample To Chunk//
    u32 sizeSTSC = 28;
    swap = byterevers(sizeSTSC);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.size = swap;
    swap = byterevers('stsc');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.version = 0;
    swap = byterevers(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.entry_count = swap;
    swap = byterevers(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.first_chunk = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.samples_per_chunk = num_samples;
    swap = byterevers(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.sample_description_index = swap;
    
    //Chunk Offset Box//
    u32 sizeSTCO = 20;
    swap = byterevers(sizeSTCO);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    ChunkOffsetBox.size = swap;
    swap = byterevers('stco');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    ChunkOffsetBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    ChunkOffsetBox.version = 0;
    swap = byterevers(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    ChunkOffsetBox.entry_count = swap;
    //This approach can be used only with identical files
    //dat = 32 + sizemdat*numtrack;
    // For now, knowing that sensor data is after track data we can use 32 + sizemdat w/o sensor data
    dat = 32 + sizemdat - (header.Chunk2_data_Size);
    swap = byterevers(dat);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    ChunkOffsetBox.chunk_offset[numtrack] = swap;
    
    //Sample Table Box //
    u32 sizeSTBL = 8 + sizeSTSD + sizeSTSZ + sizeSTSC + sizeSTCO + sizeSTTS;
    swap = byterevers(sizeSTBL);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.size = swap;
    swap = byterevers('stbl');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.type =swap;

    return  sizeSTBL;
}

int sampleDescriptionSensor(MovieBox *moov, int numtrack){
    u32 swap, sizeKEYD = 16;
    //Metadata Key Declaration//
    swap = byterevers(sizeKEYD);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.
    MetadataKeyTableBox.MetadataKeyBox.MetadataKeyDeclarationBox.size = swap;
    swap = byterevers('keyd');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.
    MetadataKeyTableBox.MetadataKeyBox.MetadataKeyDeclarationBox.type = swap;
    swap = byterevers('mdta');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.
    MetadataKeyTableBox.MetadataKeyBox.MetadataKeyDeclarationBox.keyNamespace = swap;
    swap = byterevers('snsr');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.
    MetadataKeyTableBox.MetadataKeyBox.MetadataKeyDeclarationBox.keyValue = swap;
    //Metadata Datatype Definition//
    u32 sizeDTYP = 16;
    swap = byterevers(sizeDTYP);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.
    MetadataKeyTableBox.MetadataKeyBox.MetadataDatatypeDefinitionBox.size = swap;
    swap = byterevers('dtyp');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.
    MetadataKeyTableBox.MetadataKeyBox.MetadataDatatypeDefinitionBox.type = swap;
    swap = byterevers(0);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.
    MetadataKeyTableBox.MetadataKeyBox.MetadataDatatypeDefinitionBox.namespace = swap;
    swap = byterevers(76);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.
    MetadataKeyTableBox.MetadataKeyBox.MetadataDatatypeDefinitionBox.array = swap;

    //Metadata Key// Here we're giving a unique ID to the metadata key = 'snsr', can be changed in 'sns#'/'sn##' for multiple sensors
    u32 sizeSNSR = 8 + sizeDTYP + sizeKEYD;
    swap = byterevers(sizeSNSR);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.
    MetadataKeyTableBox.MetadataKeyBox.size = swap;
    swap = byterevers('snsr');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.
    MetadataKeyTableBox.MetadataKeyBox.type = swap;
    //Metadata Key Table//
    u32 sizeKEYS = 8 + sizeSNSR;
    swap = byterevers(sizeKEYS);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.
    MetadataKeyTableBox.size = swap;
    swap = byterevers('keys');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.
    MetadataKeyTableBox.type = swap;
    //Sample Description Table//
    u32 sizeMEBX = 16 + sizeKEYS;        
    swap = byterevers(sizeMEBX);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.size = swap;
    swap = byterevers('mebx');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.reserved[0] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.reserved[1] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.reserved[2] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.reserved[3] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.reserved[4] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.reserved[5] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.SampleDescriptionTableBox.data_reference_index = 256;
    

    //Sample description box //
    u32 sizeSTSD = 16 + sizeMEBX;
    swap = byterevers(sizeSTSD);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.size = swap;
    swap = byterevers('stsd');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.version = 0; 
    swap = byterevers(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.entry_count = swap; 

    return sizeSTSD;
}

int writeSensorTrack(MovieBox moov, int numtrack, int totaltracks, FILE* imf){
    fwrite(&moov.TrackBox[numtrack].size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].type, sizeof(u32), 1, imf);
    //Track header//
    fwrite(&moov.TrackBox[numtrack].TrackHeaderBox,
            sizeof(moov.TrackBox[numtrack].TrackHeaderBox), 1, imf);
    //Track Reference Box//
    fwrite(&moov.TrackBox[numtrack].TrackReferenceBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].TrackReferenceBox.type, sizeof(u32), 1, imf);
    //Track Reference Type Box//
    fwrite(&moov.TrackBox[numtrack].TrackReferenceBox.TrackReferenceTypeBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].TrackReferenceBox.TrackReferenceTypeBox.type, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].TrackReferenceBox.TrackReferenceTypeBox.trackIDReference, sizeof(u32), 1, imf);
    
    //Media Box//
    fwrite(&moov.TrackBox[numtrack].MediaBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.type, sizeof(u32), 1, imf);
    //Media Header//
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaHeaderBox,
            sizeof(moov.TrackBox[numtrack].MediaBox.MediaHeaderBox), 1, imf);
    //Handler Box//
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.type, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.version, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.pre_defined, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.handler_type, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.reserved[0], sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.reserved[1], sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.reserved[2], sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.data[0], sizeof(unsigned char), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.data[1], sizeof(unsigned char), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.data[2], sizeof(unsigned char), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.data[3], sizeof(unsigned char), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.data[4], sizeof(unsigned char), 1, imf);
    //Media inforamtion box//
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.type, sizeof(u32), 1, imf);
    //Base media header//
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.type, sizeof(u32), 1, imf);
    //Base Media Information Box//
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.BaseMediaInformationBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.BaseMediaInformationBox.type, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.BaseMediaInformationBox.versionAndFlag, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.BaseMediaInformationBox.graphicsMode, sizeof(u16), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.BaseMediaInformationBox.opColor[0], sizeof(u16), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.BaseMediaInformationBox.opColor[1], sizeof(u16), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.BaseMediaInformationBox.opColor[2], sizeof(u16), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.BaseMediaInformationBox.balance, sizeof(u16), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.BaseMediaInformationHeaderBox.BaseMediaInformationBox.reserved, sizeof(u16), 1, imf);
    //Data reference//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox,
               sizeof(moov.TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox), 1, imf);
        
    //Sample table box//
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            type, sizeof(u32), 1, imf);
    
    int i, swap, pos;
    //Time to sample box//
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.type, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.version, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.entry_count, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.sample_count[0], sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.sample_delta[0], sizeof(u32), 1, imf);
    
    //Sample description box//
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.type, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.version, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.entry_count, sizeof(u32), 1, imf);
    //Sample Description Table box//
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.SampleDescriptionTableBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.SampleDescriptionTableBox.type, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.SampleDescriptionTableBox.reserved, sizeof(moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.SampleDescriptionTableBox.reserved), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.SampleDescriptionTableBox.data_reference_index, sizeof(u16), 1, imf);

    //Metadata Key Table Box + all the metadata boxes, if something crashes its probably here//
     fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.SampleDescriptionTableBox.MetadataKeyTableBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.SampleDescriptionTableBox.MetadataKeyTableBox.type, sizeof(u32), 1, imf);
    //Metadata Key Box//
      fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.SampleDescriptionTableBox.MetadataKeyTableBox.MetadataKeyBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.SampleDescriptionTableBox.MetadataKeyTableBox.MetadataKeyBox.type, sizeof(u32), 1, imf);
    //Metadata Key Declaration Box//
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.SampleDescriptionTableBox.MetadataKeyTableBox.MetadataKeyBox.MetadataKeyDeclarationBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.SampleDescriptionTableBox.MetadataKeyTableBox.MetadataKeyBox.MetadataKeyDeclarationBox.type, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.SampleDescriptionTableBox.MetadataKeyTableBox.MetadataKeyBox.MetadataKeyDeclarationBox.keyNamespace, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.SampleDescriptionTableBox.MetadataKeyTableBox.MetadataKeyBox.MetadataKeyDeclarationBox.keyValue, sizeof(u32), 1, imf);
    //Metadata Datatype Definition Box//
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.SampleDescriptionTableBox.MetadataKeyTableBox.MetadataKeyBox.MetadataDatatypeDefinitionBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.SampleDescriptionTableBox.MetadataKeyTableBox.MetadataKeyBox.MetadataDatatypeDefinitionBox.type, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.SampleDescriptionTableBox.MetadataKeyTableBox.MetadataKeyBox.MetadataDatatypeDefinitionBox.namespace, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.SampleDescriptionTableBox.MetadataKeyTableBox.MetadataKeyBox.MetadataDatatypeDefinitionBox.array, sizeof(u32), 1, imf);
    
    //Sample Size box//       
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleSizeBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleSizeBox.type, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleSizeBox.version, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleSizeBox.sample_size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleSizeBox.sample_count, sizeof(u32), 1, imf);
    //Sample to chunk box// 
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleToChunk.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleToChunk.type, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleToChunk.version, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleToChunk.entry_count, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleToChunk.first_chunk, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleToChunk.samples_per_chunk, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleToChunk.sample_description_index, sizeof(u32), 1, imf);

    //Chunk offset//
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            ChunkOffsetBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            ChunkOffsetBox.type, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            ChunkOffsetBox.version, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            ChunkOffsetBox.entry_count, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            ChunkOffsetBox.chunk_offset[numtrack], sizeof(u32), 1, imf);
    return 1;
}


int byterevers(num){
    int swapped;
    swapped = ((num>>24)&0xff) | // move byte 3 to byte 0
    ((num<<8)&0xff0000) | // move byte 1 to byte 2
    ((num>>8)&0xff00) | // move byte 2 to byte 1
    ((num<<24)&0xff000000); // byte 0 to byte 3
    return swapped;
}



