//
//  IM_AF Encoder.h
//  IM_AM Encoder
//
//  Created by eugenio oñate hospital on 14/06/12.
//  Copyright (c) 2012 SAE. All rights reserved.
//

#ifndef IM_AM_Encoder_IM_AF_Encoder_h
#define IM_AM_Encoder_IM_AF_Encoder_h

/* for FILE typedef, */
#include <stdio.h>
#include <stdint.h>

#define maxtracks 8
#define maxpreset 10
#define maxrules 10

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

 typedef struct  WAV_HEADER
{
    // RIFF Chunk
    u8         Chunk_ID[4];        // RIFF
    u32        Chunk_data_Size;      // RIFF Chunk data Size
    u8         RIFF_TYPE_ID[4];        // WAVE
    // format sub-chunk
    u8         Chunk1_ID[4];         // fmt
    u32        Chunk1_data_Size;  // Size of the format chunk
    u16        Format_Tag;    //  format_Tag 1=PCM
    u16        Num_Channels;      //  1=Mono 2=Sterio
    u32        Sample_rate;  // Sampling Frequency in (44100)Hz
    u32        byte_rate;    // Byte rate
    u16        block_Align;     // 4
    u16        bits_per_sample;  // 16
    /* "data" sub-chunk */
    u8         Chunk2_ID[4]; // data
    u32        Chunk2_data_Size;
   
     // Size of the audio data
} WavHeader;

WavHeader* WavHeader_new(){
    WavHeader* h = malloc(sizeof(WavHeader));
    h->Num_Channels = 1;
    return h;
}

typedef struct nametrack { // Stores the different titles of the tracks
    char title[20];
}nametrack[maxtracks];

typedef struct FileTypeBox 
{
    u32 size;
    u32 type;          // ftyp 
    u32 major_brand;   // brand identifier
    u32 minor_version; // informative integer for the mirror version
    u32 compatible_brands[3]; //list of brands
}FileTypeBox;

typedef struct MoiveBox //extends Box('moov') 
{
    u32 size;
    u32 type;       // moov
    
    struct MovieHeaderBox 
    {
        u32 size;
        u32 type;                                   // mvhd
        u32 version;                                // version + flag
        u32 creation_time; 
        u32 modification_time;
        u32 timescale;                              // specifies the time scale for the entire file (every track has its own time scale)
        u32 duration;
        u32 rate;                                   // typically 1.0
        u16 volume;                                 // typically full volume 1.0
        u16 reserved;                               // =0
        u32 reserved2[2];                           //=0
        u32 matrix[9];                              // information matrix for video (u,v,w)
        u32 pre_defined[6];                         // =0
        u32 next_track_ID;                          //non zero value for the next track ID
    }MovieHeaderBox; 
    
   struct TrackBox                                  // trak
    {
        u32 size;
        u32 type;
        struct TrackHeaderBox 
        {
            u32 size;
            u32 type;                               // tkhd
            u32 version;                            // version + flag
            u32 creation_time;
            u32 modification_time;
            u32 track_ID;  
            u32 reserved;                           // =0
            u32 duration;
            u32 reserved2[2];                       // =0
            u16 layer;                              // =0  // for video
            u16 alternate_group;                    // =0
            u16 volume;                             // full volume is 1 = 0x0100
            u16 reserved3;                          // =0
            u32 matrix[9];                          // for video
            u32 width;                              // video
            u32 height;                             // video
        }TrackHeaderBox;

        struct TrackReferenceBox                    // extends atom ("tref")
        {
            u32 size;
            u32 type;                           //tref
            struct TrackReferenceTypeBox{       // extends atom ("cdsc") <-- timed metadata reference type
                u32 size;
                u32 type;                       // 'cdsc'
                u32 trackIDReference;           // track ID reference for the metadata (Many sensors will reference a single track that matches a SMI)
            }TrackReferenceTypeBox;
        }TrackReferenceBox;
 
        struct MediaBox // extends Box('mdia')
        {
            u32 size;
            u32 type;
            struct MediaHeaderBox // extends FullBox('mdhd', version,0)
            {
                u32 size;
                u32 type;
                u32 version; // version + flag
                u32 creation_time;
                u32 modification_time;
                u32 timescale;
                u32 duration;
                u16 language; // [pad,5x3] = 16 bits and pad = 0
                u16 pre_defined; // =0
            }MediaHeaderBox;
           struct HandlerBox  
            {
                u32 size;
                u32 type;                      // 'hdlr'
                u32 version;                   // version = 0 + flag
                u32 pre_defined;               // =0 (TODO: probably this became component type attribute, so try to change to 4char 'mhlr' to specify media handler type)
                u32 handler_type;              // = 'soun' for audio track, text or hint  'meta' for Timed Metadata used for sensor data
                u32 reserved[3];               // =0
               unsigned char data[5];          // Does not work! only 4 bytes
                
            }HandlerBox;    
             struct MediaInformationBox        //extends Box('minf')
            {
                u32 size;
                u32 type;
                // smhd in sound track only!!
                struct SoundMediaHeaderBox 
                {
                    u32 size;
                    u32 type;                   // 'smhd'
                    u32 version;
                    u16 balance;                // =0 place mono tracks in stereo. 0 is center
                    u16 reserved;               // =0
                }SoundMediaHeaderBox;
                // gmhd only in meta track
                struct BaseMediaInformationHeaderBox
                {
                    u32 size;                   // 8 + 24
                    u32 type;                   // 'gmhd' also GenericMediaHeaderBox
                    struct BaseMediaInformationBox
                    {
                        u32 size;               // 24
                        u32 type;               // 'gmin'
                        u32 versionAndFlag;     // version is 1 byte, flag 3 bytes. no  3byte variable exist, so this is the way I suppose 
                        u16 graphicsMode;       // testing 0x0
                        u16 opColor[3];         // testing 0
                        u16 balance;            // 0
                        u16 reserved;           // 0
                    }BaseMediaInformationBox;
                }BaseMediaInformationHeaderBox;

               struct DataInformationBox        //extends Box('dinf')
                {
                    u32 size;
                    u32 type;
                    struct DataReferenceBox 
                    {
                        u32 size;
                        u32 type;
                        u32 flags;
                        u32 entry_count; // counts the actual entries. 
                        struct DataEntryUrlBox //extends FullBox('url', version=0, flags)
                        {
                            u32 size;
                            u32 type;
                            u32 flags;
                        }DataEntryUrlBox;
                    }DataReferenceBox;
                }DataInformationBox;
                struct SampleTableBox // extends Box('stbl')
                {
                    u32 size;
                    u32 type;
                    struct TimeToSampleBox{
                        u32 size;
                        u32 type;                               // 'stts'
                        u32 version;
                        u32 entry_count;                        // if sample time is costant (in .wav uncompressed format it should be), count can be set to 1
                        u32 sample_count[3000];                 // total number of uncompressed samples as one number
                        u32 sample_delta[3000];                 // if we set rate to sampleRate of .wav, delta will be always set to 1
                    }TimeToSampleBox;
                    struct SampleDescriptionBox // stsd
                    {
                        u32 size;
                        u32 type;
                        u32 version;
                        u32 entry_count;                                // = 1 number of entries
                        struct SampleDescriptionTableBox                // This box can be extended to provide more data about sensors
                        {                                               // For now it has a structure of a generic Timed Metadata Description Table atom
                            u32 size;
                            u32 type;                                   // 'mebx' for general Timed Metadata
                            u8 reserved[6];
                            u16 data_reference_index;                   // = 1
                            struct MetadataKeyTableBox
                            {
                                u32 size;
                                u32 type;                               // 'keys'
                                struct MetadataKeyBox                   // This box can be an array of keys
                                {
                                    u32 size;                           // 8 + sKeyd + sDtyp
                                    u32 type;                           // 'local_key_id' (FourCC 32-bit integer, can be 'snsr' for sensor? Or 'sns#'/'sn##' so 10 or 100 unique sensor ids can be created)
                                    struct MetadataKeyDeclarationBox    // This box could specify the sensor type?
                                    {
                                        u32 size;                       // 16
                                        u32 type;                       // 'keyd'
                                        u32 keyNamespace;               // 'mdta' for generic reverse utf-8 string, can't find other types (if we register a new metadata key for sensors, we can specify all kind of stuff in here)
                                        u32 keyValue;                   // 'snsr' new value to specify sensor data(?) *** we can use an exisestent metadata key but they are not what we need ***
                                    }MetadataKeyDeclarationBox;
                                    struct MetadataDatatypeDefinitionBox // This box can specify our sensor data
                                    {
                                        u32 size;                       // 16
                                        u32 type;                       // 'dtyp'
                                        u32 namespace;                  // 0 for a numbered list of datatypes, 1 for a reverse utf-8 string
                                        u32 array;                      // 76 for a 16-bit big-endian unsigned int / 23 for a 32-bit big-endian floating point value (IEEE754)
                                    }MetadataDatatypeDefinitionBox;
                                }MetadataKeyBox;
                            }MetadataKeyTableBox;
                            struct BitRateBox                           // optional, but could be useful for sensor data streaming
                            {
                                u32 size;                               // 20
                                u32 type;                               //  'btrt'
                                u32 bufferSize;                         // 
                                u32 maxBitRate;
                                u32 averageBitRate;
                            }BitRateBox;

                        }SampleDescriptionTableBox;
                        struct AudioSampleEntry{
                            u32 size;
                            u32 type;   //mp4a
                            char reserved[6];
                            u16 data_reference_index; // = 1
                            u32 reserved2[2];
                            u16 channelcount; // = 2
                            u16 samplesize; // = 16
                            u32 reserved3;
                            u32 samplerate; // 44100 << 16

                            struct ESbox{
                                u32 size;
                                u32 type;
                                u32 version;
                                struct ES_Descriptor{
                                    unsigned char tag;
                                    unsigned char length;
                                    u16 ES_ID;
                                    unsigned char mix;
                                    struct DecoderConfigDescriptor{
                                        unsigned char tag;
                                        unsigned char length;
                                        unsigned char objectProfileInd;
                                        u32 mix; 
                                        u32 maxBitRate;
                                        u32 avgBitrate;
                                     /*   struct DecoderSpecificInfo{
                                            unsigned char tag;
                                            unsigned length;
                                           // unsigned char decSpecificInfosize;
                                            unsigned char decSpecificInfoData[2];
                                        }DecoderSpecificInfo;
                                   */ }DecoderConfigDescriptor;
                                    struct SLConfigDescriptor{
                                        unsigned char tag;
                                        unsigned char length;
                                        unsigned char predifined;
                                    }SLConfigDescriptor;
                                  }ES_Descriptor;
                            }ESbox;
                        }AudioSampleEntry;
                    }SampleDescriptionBox;
                    struct SampleSizeBox{
                        u32 size;
                        u32 type;
                        u32 version;
                        u32 sample_size;                                // 0 if sample size is not costant (compressed data), x bytes if costant
                        u32 sample_count;                               // if sample size != 0, count = 0  
                        u32 entry_size[9000];                           // and no table follows!
                    }SampleSizeBox;
                    struct SampleToChunk{
                        u32 size;
                        u32 type;
                        u32 version;
                        u32 entry_count;
                        u32 first_chunk;
                        u32 samples_per_chunk;
                        u32 sample_description_index;
                    }SampleToChunk;
                    struct ChunkOffsetBox{
                        u32 size;
                        u32 type;
                        u32 version;
                        u32 entry_count;
                        u32 chunk_offset[maxtracks];
                    }ChunkOffsetBox;
                }SampleTableBox;
            }MediaInformationBox;
        }MediaBox;
    }TrackBox[maxtracks]; // max 10 tracks
    struct PresetContainerBox // extends Box('prco')
    {
        u32 size;
        u32 type;
        unsigned char num_preset;
        unsigned char default_preset_ID; 
        struct PresetBox //extends FullBox('prst',version=0,flags)
        {
            u32 size;
            u32 type;
            u32 flags;
            unsigned char preset_ID;
            unsigned char num_preset_elements;
            struct presElemId{
                u32 preset_element_ID;
            }presElemId[maxtracks];
            unsigned char preset_type;
            unsigned char preset_global_volume;
            //IF preset_type == 1
            struct presVolumElem{
                unsigned char preset_volume_element;
            }presVolumElem[maxtracks];
           char preset_name[14];
        }PresetBox[maxpreset];
    }PresetContainerBox;
    
    struct RulesContainer{
        u32 size;
        u32 type;
        u16 num_selection_rules;
        u16 num_mixing_rules;
        struct SelectionRules{
            u32 size;
            u32 type;
            u32 version;
            u16 selection_rule_ID;
            unsigned char selection_rule_type;
            u32 element_ID;
            char rule_description[14];
        }SelectionRules;
        struct MixingRules{
            u32 size;
            u32 type;
            u32 version;
            u16 mixing_rule_ID;
            unsigned char mixing_type;
            u32 element_ID;
            u32 key_elem_ID;
            char mix_description[17];
        }MixingRules;
    }RulesContainer;
}MovieBox; 

typedef struct MediaDataBox // extends Box('mdat')
{
    u32 size;
    u32 type;
    unsigned char data;  
}MediaDataBox;

#endif




















