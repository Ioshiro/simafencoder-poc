#include <stdio.h>
//Standard library: numeric conversion, memory allocation...
#include <stdlib.h>
//Operations with strings
#include <string.h>
//Get the creation time: clock
#include <time.h>
#include <stdint.h>

#define PI 3.14

float sineWave(float, float, float, int);


int main()
{
    typedef struct  WAV_HEADER
{
    // RIFF Chunk
    uint8_t         Chunk_ID[4];        // RIFF
    uint32_t        Chunk_data_Size;      // RIFF Chunk data Size
    uint8_t         RIFF_TYPE_ID[4];        // WAVE
    // format sub-chunk
    uint8_t         Chunk1_ID[4];         // fmt
    uint32_t        Chunk1_data_Size;  // Size of the format chunk
    uint16_t        Format_Tag;    //  format_Tag 1=PCM
    uint16_t        Num_Channels;      //  1=Mono 2=Sterio
    uint32_t        Sample_rate;  // Sampling Frequency in (44100)Hz
    uint32_t        byte_rate;    // Byte rate
    uint16_t        block_Align;     // 4
    uint16_t        bits_per_sample;  // 16
    /* "data" sub-chunk */
    uint8_t         Chunk2_ID[4]; // data
    uint32_t        Chunk2_data_Size;
   
     // Size of the audio data
} obj;

obj header;

    const char* filePath;

   char input[20];

    {
        printf( "Enter the wave file name: ");
       fflush(stdin);
            scanf("%s", input);
    }

    FILE* fp = fopen(input, "r");
    if (fp == NULL)
    {
        fprintf(stderr, " file cannot be open %s \n", input);

    }
    {
        // Read RIFF Chunk
         fread(&header, 1, sizeof(header), fp);
         fclose(fp);
       // format subchunk
   // calculate exact duration
    float duration = ((float)(header.Chunk2_data_Size*8)/(header.Num_Channels*header.bits_per_sample*header.Sample_rate));
 obj newHeader;
    uint32_t newSampleRate = 5000;
    uint16_t newBitsPerSample = sizeof(uint16_t)*8;
    uint32_t newSize =  ((duration)*newSampleRate*newBitsPerSample)/8;
    printf("Old duration\t:%f\n", duration);
    printf("New size\t:%u\n", newSize);
    printf("New size\t:%u\n", newSize);
        //copy info to newHeader
    newHeader.Chunk_ID[0] = header.Chunk_ID[0];
    newHeader.Chunk_ID[1] = header.Chunk_ID[1];
    newHeader.Chunk_ID[2] = header.Chunk_ID[2];
    newHeader.Chunk_ID[3] = header.Chunk_ID[3];

    newHeader.Chunk_data_Size = newSize+36;

    newHeader.RIFF_TYPE_ID[0] = header.RIFF_TYPE_ID[0];
    newHeader.RIFF_TYPE_ID[1] = header.RIFF_TYPE_ID[1];
    newHeader.RIFF_TYPE_ID[2] = header.RIFF_TYPE_ID[2];
    newHeader.RIFF_TYPE_ID[3] = header.RIFF_TYPE_ID[3];

    newHeader.Chunk1_ID[0] = header.Chunk1_ID[0];
    newHeader.Chunk1_ID[1] = header.Chunk1_ID[1];
    newHeader.Chunk1_ID[2] = header.Chunk1_ID[2];
    newHeader.Chunk1_ID[3] = header.Chunk1_ID[3];

    newHeader.Chunk1_data_Size = header.Chunk1_data_Size;
    newHeader.Format_Tag = header.Format_Tag;
    newHeader.Num_Channels = 1;
    newHeader.Sample_rate = newSampleRate;
    newHeader.byte_rate = (newHeader.Num_Channels*newSampleRate*newBitsPerSample)/8;
    newHeader.block_Align = (newHeader.Num_Channels*newBitsPerSample)/8;
    newHeader.bits_per_sample = newBitsPerSample;

    newHeader.Chunk2_ID[0] = header.Chunk2_ID[0];
    newHeader.Chunk2_ID[1] = header.Chunk2_ID[1];
    newHeader.Chunk2_ID[2] = header.Chunk2_ID[2];
    newHeader.Chunk2_ID[3] = header.Chunk2_ID[3];

    newHeader.Chunk2_data_Size = newSize;

    float newDuration = ((float)(newHeader.Chunk2_data_Size*8)/(newHeader.Num_Channels*newHeader.bits_per_sample*newHeader.Sample_rate));
    printf("New duration\t:%f\n", newDuration);

        //calculate sample number and create random sensor data
    int sampleNumber = newSampleRate*newDuration;
  //  short sensorData[sampleNumber];

        //write everything to new file
    fp = fopen("snsr.wav", "wb");
    fwrite(&newHeader, 1, sizeof(newHeader), fp);
    srand(time(NULL));
    uint16_t dat;
    for(int i= 0; i <sampleNumber; i++){
        dat = ((uint16_t)(__UINT16_MAX__*((rand() % 10000)/10000.0)));
        fwrite(&dat, sizeof(uint16_t), 1, fp);
        //cout << sensorData[index] << endl;
    }
    //cout << "Size of data: " << sizeof(sensorData) << endl;
    fclose(fp);
   return 0;
    }
}
