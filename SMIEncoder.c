//File input/output
#include <stdio.h>
//Standard library: numeric conversion, memory allocation...
#include <stdlib.h>
//Operations with strings
#include <string.h>
//Get the creation time: clock
#include <time.h>

//Internal library
#include "lib/smiencoder.h"
#include "lib/util.h"
#include "lib/boxes/ftyp.h"
#include "lib/boxes/mdat.h"
#include "lib/boxes/moov.h"
#include "lib/boxes/mdia.h"
#include "lib/boxes/minf.h"
#include "lib/boxes/trak.h"

void greetings();
int scanTotalTracks();
int scanTotalSensors();
char *scanTrackName(int);
int scanSensorCount(int, char[]);
char *scanSensorName(int);

int main()
{
    Track tracks[MAX_TRACKS];
    for (int i = 0; i < MAX_TRACKS; i++)
    {
        tracks[i].sensorCount = 0;
    }

    // Greet and scan file info (total audio&sensor track number)
    greetings();
    int totalTracks = scanTotalTracks();
    int totalSensors = scanTotalSensors();
    int remainingSensors = totalSensors;

    //Create output file
    FILE *smi = fopen("smi.mp4", "wb");
    if (smi == NULL)
    {
        printf("Error opening output file\n");
        system("pause");
        exit(1);
    }

    //Read & Write tracks and sensors names
    for (int numtr = 0; numtr < totalTracks; numtr++)
    {
        // Save tracks names
        strcpy(tracks[numtr].title, scanTrackName(numtr));
        if (remainingSensors > 0)
        {
            tracks[numtr].sensorCount = scanSensorCount(remainingSensors, tracks[numtr].title);
            if (tracks[numtr].sensorCount > 0)
            {
                remainingSensors -= tracks[numtr].sensorCount;
                for (int numsn = 0; numsn < tracks[numtr].sensorCount; numsn++)
                {
                    // Save sensors names
                    strcpy(tracks[numtr].sensor[numsn].title, scanSensorName(numsn));
                }
            }
        }
    }

    //Define & Write the File Type Box
    FileTypeBox ftyp;
    FileTypeBox_new(&ftyp);
    FileTypeBox_write(ftyp, smi);

    //Extract media from the audio and sensor files & Calculate duration/size
    MediaDataBox mdat;
    u32 duration = 0;
    int sizeMDAT = MediaDataBox_new(&mdat, tracks, totalTracks, totalSensors, &duration);
    //printf("sizemdat\t:%d\n", sizeMDAT);
    printf("duration\t:%u\n", duration);
    // Write media
    MediaDataBox_write(mdat, smi, tracks, totalTracks, totalSensors);
    // Create  and write the movie box with all the tracks
    MovieBox moov;
    int sizeMOOV = MovieBox_new(&moov, tracks, duration, totalTracks, totalSensors);
    int writtenMOOV = MovieBox_write(moov, smi, tracks, totalTracks, totalSensors);
    fclose(smi);
    return 0;
}

void greetings()
{
    printf("\n\tSMI encoder\n");
    printf("This program will allow you to create a SMI file.\n");
}

int scanTotalTracks()
{
    int totalTracks = 0;
    printf("Insert the number of audio tracks:\n");
    fflush(stdin);
    scanf("%d", &totalTracks);
    while (totalTracks > MAX_TRACKS || totalTracks < 1)
    {
        printf("Error: max track number: %d\n", MAX_TRACKS);
        printf("Insert the number of audio tracks:\n");
        scanf("%d", &totalTracks);
    }
    return totalTracks;
}

int scanTotalSensors()
{
    int totalSensors = 0;
    printf("Insert the number of sensor tracks:\n");
    fflush(stdin);
    scanf("%d", &totalSensors);
    while (totalSensors > MAX_SENSORS || totalSensors < 0)
    {
        printf("Error: max sensor number: %d\n", MAX_SENSORS);
        printf("Insert the number of sensor tracks:\n");
        scanf("%d", &totalSensors);
    }
    return totalSensors;
}

char *scanTrackName(int numTrack)
{
    static char name[20];
    printf("Name of track number %d:\n", numTrack + 1);
    fflush(stdin);
    scanf("%s", name);

    int isNameValid = 0;
    //Check if the track exist and then open it.
    while (!isNameValid)
    {
        FILE *file = fopen(name, "rb");
        if ((file) == NULL)
        {
            printf("Name does not exist. Try again:\n");
            fflush(stdin);
            scanf("%s", name);
        }
        else
        {
            fclose(file);
            isNameValid = 1;
            return name;
        }
    }
}

int scanSensorCount(int remaining, char name[])
{
    int count = 0;
    printf("Number of sensor tracks for %s? (0 to skip, 4 max total sensors): \n", name);
    fflush(stdin);
    scanf("%d", &count);
    while (count > remaining && count != 0)
    {
        printf("Too many sensors. (0 to skip, %d remaining sensors): \n", remaining);
        fflush(stdin);
        scanf("%d", &count);
    }
    return count;
}

char *scanSensorName(int numSensor)
{
    static char name[20];
    printf("Name of sensor file %d:\n", numSensor + 1);
    fflush(stdin);
    scanf("%s", name);
    int isNameValid = 0;
    //Check if the track exist and then open it.
    while (!isNameValid)
    {
        FILE *file = fopen(name, "rb");
        if ((file) == NULL)
        {
            printf("Name does not exist. Try again:\n");
            fflush(stdin);
            scanf("%s", name);
        }
        else
        {
            fclose(file);
            isNameValid = 1;
            return name;
        }
    }
}