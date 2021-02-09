//File input/output
#include <stdio.h>
//Standard library: numeric conversion, memory allocation...
#include <stdlib.h>
//Operations with strings
#include <string.h>

//Internal library
#include "lib/smiencoder.h"
#include "lib/util.h"

char *scanTrackName();
void writeTrack(int, int, int, WavHeader, FILE *);

int main()
{
    //Scan name and open file
    char trackName[20];
    strcpy(trackName, scanTrackName());
    FILE *smi = fopen(trackName, "rb");
    if (smi == NULL)
    {
        printf("Error opening output file\n");
        system("pause");
        exit(1);
    }
    u32 size;

    // read File type box
    FileTypeBox ftyp;
    fread(&ftyp, 1, sizeof(FileTypeBox), smi);
    writeReverse(ftyp.size, &size);
    printf("ftyp size: %u \n", size);
    // read Media data box
    MediaDataBox mdat;
    fread(&mdat.size, 1, sizeof(mdat.size), smi);
    fread(&mdat.type, 1, sizeof(mdat.type), smi);
    writeReverse(mdat.size, &size);
    printf("mdat size: %u \n", size);

    //skip all the media data until we know how to decode it (sizeMDAT - 8 bytes of size and type fields)
    fseek(smi, size - 8, SEEK_CUR);

    // read Movie box
    MovieBox moov;
    fread(&moov.size, 1, sizeof(moov.size), smi);
    fread(&moov.type, 1, sizeof(moov.type), smi);
    writeReverse(moov.size, &size);
    printf("moov size: %u \n", size);

    // read Movie Header box
    MovieHeaderBox mvhd;
    fread(&mvhd, 1, sizeof(MovieHeaderBox), smi);
    writeReverse(mvhd.size, &size);
    printf("mvhd size: %u \n", size);
    // calculate total tracks by next_track_id field of movie header
    u32 totalTracks;
    writeReverse(mvhd.next_track_ID, &totalTracks);
    totalTracks -= 1;
    printf("total tracks: %u \n", totalTracks);
    int trackDone = 0;
    int audioDone = 0;
    int sensorDone = 0;
    while (trackDone < totalTracks)
    {
        // read Track box
        TrackBox trak;
        fread(&trak.size, 1, sizeof(trak.size), smi);
        fread(&trak.type, 1, sizeof(trak.type), smi);
        writeReverse(trak.size, &size);
        printf("trak size: %u \n", size);

        // read Track Header Box
        TrackHeaderBox tkhd;
        fread(&tkhd, 1, sizeof(TrackHeaderBox), smi);
        writeReverse(tkhd.size, &size);
        printf("tkhd size: %u \n", size);
        // a semi hardcoded and lazy approach, but if volume is 0 I suppose track is of timed metadata type
        u16 volume;
        writeReverse16(tkhd.volume, &volume);
        printf("trak%u volume: %u \n", trackDone, volume);
        int isAudio;
        if (volume == 0)
            isAudio = 0;
        else
            isAudio = 1;

        // if audio (.wav) track read Media box
        if (isAudio)
        {
            MediaBox mdia;
            fread(&mdia.size, 1, sizeof(mdia.size), smi);
            fread(&mdia.type, 1, sizeof(mdia.type), smi);
            writeReverse(mdia.size, &size);
            printf("mdia size: %u \n", size);

            // read Media Header box
            MediaHeaderBox mdhd;
            fread(&mdhd, 1, sizeof(MediaHeaderBox), smi);
            writeReverse(mdhd.size, &size);
            printf("mdhd size: %u \n", size);

            // read Handler box
            HandlerBox hdlr;
            fread(&size, 1, sizeof(u32), smi);
            writeReverse(size, &size);
            printf("hdlr size: %u \n", size);
            fseek(smi, size - 4, SEEK_CUR);

            // read Media Information box
            MediaInformationBox minf;
            fread(&minf.size, 1, sizeof(minf.size), smi);
            fread(&minf.type, 1, sizeof(minf.type), smi);
            writeReverse(minf.size, &size);
            printf("minf size: %u \n", size);

            // read Sound Media Header box
            SoundMediaHeaderBox smhd;
            fread(&smhd, 1, sizeof(SoundMediaHeaderBox), smi);
            writeReverse(smhd.size, &size);
            printf("smhd size: %u \n", size);

            // read Data infromation box + data reference + data entry (we know data is in the file)
            DataInformationBox dinf;
            fread(&dinf, 1, sizeof(DataInformationBox), smi);
            writeReverse(dinf.size, &size);
            printf("dinf size: %u \n", size);
            writeReverse(dinf.dataReferenceBox.size, &size);
            printf("dref size: %u \n", size);

            // read Sample Table box
            SampleTableBox stbl;
            fread(&stbl.size, 1, sizeof(stbl.size), smi);
            fread(&stbl.type, 1, sizeof(stbl.type), smi);
            writeReverse(stbl.size, &size);
            printf("stbl size: %u \n", size);

            // read Sample to Time box
            TimeToSampleBox stts;
            fread(&stts, 1, sizeof(TimeToSampleBox), smi);
            writeReverse(stts.size, &size);
            printf("stts size: %u \n", size);

            // read Sample Description box
            SampleDescriptionBox stsd;
            fread(&stsd.size, 1, sizeof(stsd.size), smi);
            fread(&stsd.type, 1, sizeof(stsd.type), smi);
            fread(&stsd.version, 1, sizeof(stsd.version), smi);
            fread(&stsd.entry_count, 1, sizeof(stsd.entry_count), smi);
            writeReverse(stsd.size, &size);
            printf("stsd size: %u \n", size);

            // read Audio Sample Entry box   ( from here number of channels, sample rate and size can be found)
            AudioSampleEntryBox sowt;
            fread(&sowt.size, 1, sizeof(sowt.size), smi);
            fread(&sowt.type, 1, sizeof(sowt.type), smi);
            writeReverse(sowt.size, &size);
            printf("sowt size: %u \n", size);
            fread(&sowt.reserved[0], 6, sizeof(u8), smi);
            fread(&sowt.data_reference_index, 1, sizeof(sowt.data_reference_index), smi);
            fread(&sowt.version, 1, sizeof(sowt.version), smi);
            fread(&sowt.revision_level, 1, sizeof(sowt.revision_level), smi);
            fread(&sowt.vendor, 1, sizeof(sowt.vendor), smi);
            fread(&sowt.num_channels, 1, sizeof(sowt.num_channels), smi);
            fread(&sowt.sample_size, 1, sizeof(sowt.sample_size), smi);
            fread(&sowt.compression_ID, 1, sizeof(sowt.compression_ID), smi);
            fread(&sowt.packet_size, 1, sizeof(sowt.packet_size), smi);
            fread(&sowt.sample_rate, 1, sizeof(sowt.sample_rate), smi);
            u16 channels, sampleSize;
            u32 sampleRate;
            writeReverse16(sowt.num_channels, &channels);
            writeReverse16(sowt.sample_size, &sampleSize);
            writeReverse(sowt.sample_rate, &sampleRate);
            sampleRate = sampleRate >> 16;
            printf("[AudioTrackInfo] channels: %u sample size: %u sample rate: %u \n", channels, sampleSize, sampleRate);

            // read Sample Size box
            SampleSizeBox stsz;
            fread(&stsz, 1, sizeof(SampleSizeBox), smi);
            writeReverse(stsz.size, &size);
            printf("stsz size: %u \n", size);

            // read Sample to Chunk box ( from here sample number can be found )
            SampleToChunkBox stsc;
            fread(&stsc, 1, sizeof(SampleToChunkBox), smi);
            writeReverse(stsc.size, &size);
            printf("stsc size: %u \n", size);
            u32 sampleNumber;
            writeReverse(stsc.samples_per_chunk, &sampleNumber);

            // read Chunk offset box ( from here media data offset in the mdat box can be found )
            ChunkOffsetBox stco;
            fread(&stco, 1, sizeof(ChunkOffsetBox), smi);
            writeReverse(stco.size, &size);
            printf("stco size: %u \n", size);
            u32 dataOffset;
            writeReverse(stco.chunk_offset, &dataOffset);
            printf("[AudioTrackInfo] sample number: %u data offset: %u \n", sampleNumber, dataOffset);

            // create new .wav header for the track by calculating the needed info
            u32 byteRate = (sampleRate * sampleSize * channels) / 8;
            u16 blockAllign = (channels * sampleSize) / 8;
            u32 dataSize = (sampleNumber * channels * (sampleSize / 8));
            printf("datasize: %u\n", dataSize);
            WavHeader wavHeader;
            WavHeader_new(&wavHeader);
            wavHeader.Chunk_data_Size = dataSize + 36;
            wavHeader.Num_Channels = channels;
            wavHeader.Sample_rate = sampleRate;
            wavHeader.byte_rate = byteRate;
            wavHeader.block_Align = blockAllign;
            wavHeader.bits_per_sample = sampleSize;
            wavHeader.Chunk2_data_Size = dataSize;

            // save current pos and write new file
            long currentOffset = ftell(smi);
            fseek(smi, dataOffset, SEEK_SET);
            audioDone++;
            writeTrack(audioDone, 0, 1, wavHeader, smi);

            // return to current pos and start scanning new track
            fseek(smi, currentOffset, SEEK_SET);
            trackDone++;
        }
        // if sensor (.wav) track read Track Reference Box (which contains the id of the referenced track )
        else
        {
            TrackReferenceBox tref;
            fread(&tref, 1, sizeof(TrackReferenceBox), smi);
            writeReverse(tref.size, &size);
            printf("tref size: %u \n", size);
            writeReverse(tref.trackReferenceTypeBox.size, &size);
            printf("cdsc size: %u \n", size);

            // read Media box
            MediaBox mdia;
            fread(&mdia.size, 1, sizeof(mdia.size), smi);
            fread(&mdia.type, 1, sizeof(mdia.type), smi);
            writeReverse(mdia.size, &size);
            printf("mdia size: %u \n", size);

            // read Media Header box
            MediaHeaderBox mdhd;
            fread(&mdhd, 1, sizeof(MediaHeaderBox), smi);
            writeReverse(mdhd.size, &size);
            printf("mdhd size: %u \n", size);

            // read Handler box
            HandlerBox hdlr;
            fread(&size, 1, sizeof(u32), smi);
            writeReverse(size, &size);
            printf("hdlr size: %u \n", size);
            fseek(smi, size - 4, SEEK_CUR);

            // read Media Information box
            MediaInformationBox minf;
            fread(&minf.size, 1, sizeof(minf.size), smi);
            fread(&minf.type, 1, sizeof(minf.type), smi);
            writeReverse(minf.size, &size);
            printf("minf size: %u \n", size);

            // read Generic Media Header Box
            fread(&size, 1, sizeof(u32), smi);
            writeReverse(size, &size);
            printf("gmhd size: %u \n", size);
            fseek(smi, size - 4, SEEK_CUR);

            // read Data infromation box + data reference + data entry (we know data is in the file)
            DataInformationBox dinf;
            fread(&dinf, 1, sizeof(DataInformationBox), smi);
            writeReverse(dinf.size, &size);
            printf("dinf size: %u \n", size);
            writeReverse(dinf.dataReferenceBox.size, &size);
            printf("dref size: %u \n", size);

            // read Sample Table box
            SampleTableBox stbl;
            fread(&stbl.size, 1, sizeof(stbl.size), smi);
            fread(&stbl.type, 1, sizeof(stbl.type), smi);
            writeReverse(stbl.size, &size);
            printf("stbl size: %u \n", size);

            // read Sample to Time box
            TimeToSampleBox stts;
            fread(&stts, 1, sizeof(TimeToSampleBox), smi);
            writeReverse(stts.size, &size);
            printf("stts size: %u \n", size);

            // read Sample Description box
            SampleDescriptionBox stsd;
            fread(&stsd.size, 1, sizeof(stsd.size), smi);
            fread(&stsd.type, 1, sizeof(stsd.type), smi);
            fread(&stsd.version, 1, sizeof(stsd.version), smi);
            fread(&stsd.entry_count, 1, sizeof(stsd.entry_count), smi);
            writeReverse(stsd.size, &size);
            printf("stsd size: %u \n", size);

            // read Metadata Sample Entry box   ( here begins the series of boxes that could be extended to provide more info about sensors )
            SampleDescriptionTableBox mebx;
            fread(&mebx.size, 1, sizeof(mebx.size), smi);
            fread(&mebx.type, 1, sizeof(mebx.type), smi);
            fread(&mebx.reserved[0], 6, sizeof(mebx.reserved[0]), smi);
            fread(&mebx.data_reference_index, 1, sizeof(mebx.data_reference_index), smi);

            // read Metadata Key Table box
            MetadataKeyTableBox keys;
            fread(&keys.size, 1, sizeof(keys.size), smi);
            fread(&keys.type, 1, sizeof(keys.type), smi);

            // read Metadata Key box ( the type of this box is the sensor id generated by the encoder)
            MetadataKeyBox snsr;
            fread(&snsr.size, 1, sizeof(snsr.size), smi);
            fread(&snsr.type, 1, sizeof(snsr.type), smi);

            // read Metadata Key Declaration box ( key value could specify the type of sensor, for now its only a FourCC with value "snsr")
            MetadataKeyDeclarationBox keyd;
            fread(&keyd, 1, sizeof(MetadataKeyDeclarationBox), smi);
            writeReverse(keyd.size, &size);
            printf("keyd size: %u \n", size);

            // read Metadata Datatype Definition box ( field "array" specifies the data type of samples from a table found in Apple QuickTime documentation)
            MetadataDatatypeDefinitionBox dtyp;
            fread(&dtyp, 1, sizeof(MetadataDatatypeDefinitionBox), smi);
            writeReverse(dtyp.size, &size);
            printf("dtyp size: %u \n", size);

            // read Sample Size box
            SampleSizeBox stsz;
            fread(&stsz, 1, sizeof(SampleSizeBox), smi);
            writeReverse(stsz.size, &size);
            printf("stsz size: %u \n", size);

            // read Sample to Chunk box ( from here sample number can be found )
            SampleToChunkBox stsc;
            fread(&stsc, 1, sizeof(SampleToChunkBox), smi);
            writeReverse(stsc.size, &size);
            printf("stsc size: %u \n", size);
            u32 sampleNumber;
            writeReverse(stsc.samples_per_chunk, &sampleNumber);

            // read Chunk offset box ( from here media data offset in the mdat box can be found )
            ChunkOffsetBox stco;
            fread(&stco, 1, sizeof(ChunkOffsetBox), smi);
            writeReverse(stco.size, &size);
            printf("stco size: %u \n", size);
            u32 dataOffset;
            writeReverse(stco.chunk_offset, &dataOffset);
            printf("[SensorTrackInfo] sample number: %u data offset: %u \n", sampleNumber, dataOffset);

            // create new .wav header for the track by calculating the needed info
            u32 sampleSize;
            writeReverse(stsz.sample_size, &sampleSize);
            u32 sampleRate;
            writeReverse(mdhd.timescale, &sampleRate);
            u32 byteRate = (sampleRate * sampleSize);
            u16 blockAllign = sampleSize;
            u32 dataSize = (sampleNumber * sampleSize);
            printf("[SensorTrackInfo] sample size: %u sample rate: %u byte rate: %u \n", sampleSize, sampleRate, byteRate);
            printf("datasize: %u\n", dataSize);
            WavHeader wavHeader;
            WavHeader_new(&wavHeader);
            wavHeader.Chunk_data_Size = dataSize + 36;
            wavHeader.Num_Channels = 1;
            wavHeader.Sample_rate = sampleRate;
            wavHeader.byte_rate = byteRate;
            wavHeader.block_Align = blockAllign;
            wavHeader.bits_per_sample = sampleSize * 8;
            wavHeader.Chunk2_data_Size = dataSize;

            // save current pos and write new file
            long currentOffset = ftell(smi);
            fseek(smi, dataOffset, SEEK_SET);
            sensorDone++;
            writeTrack(sensorDone, audioDone, 0, wavHeader, smi);

            // return to current pos and start scanning new track
            fseek(smi, currentOffset, SEEK_SET);

            trackDone++;
        }
    }
    fclose(smi);
    return 0;
}

char *scanTrackName()
{
    static char name[20];
    printf("SMI decoder \n");
    printf("Name of track to decode: \n");
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

void writeTrack(int trackID, int trackRef, int isAudio, WavHeader wavHeader, FILE *smi)
{
    FILE *track;
    char trackName[20];
    if (isAudio)
    {
        snprintf(trackName, sizeof trackName, "a%d.wav", trackID);
    }
    else
    {
        snprintf(trackName, sizeof trackName, "s%d_%d.wav",trackRef, trackID);
    }

    printf("Writing file : %s \n", trackName);
    track = fopen(trackName, "wb");
    if (track == NULL)
    {
        printf("Error opening output file\n");
        system("pause");
        exit(1);
    }
    fwrite(&wavHeader, 1, sizeof(wavHeader), track);
    u8 data;
    for (int i = 0; i < wavHeader.Chunk2_data_Size; i++)
    {
        fread(&data, sizeof(u8), 1, smi);
        fwrite(&data, sizeof(u8), 1, track);
    }
    fclose(track);
}