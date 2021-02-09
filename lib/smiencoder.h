#ifndef SMIENCODER_H
#define SMIENCODER_H

#include <stdio.h>
#include <stdint.h>

#define MAX_TRACKS 4  //change this value to support more than 8 audio tracks. This value was 6 before I changed it
#define MAX_SENSORS 4 //Max sensor files number for each track
#define MAX_GROUPS 2
#define MAX_PRESETS 10
#define MAX_RULES 10
#define MAX_FILTERS 3            //Max number of Filters for an EQ preset
#define MAX_DYNAMIC 2            //Max number of Dynamic Volume changes
#define NUM_CHANNELS 2           //Number of channel outputs (STEREO)
#define TIME_TO_SAMPLE_SIZE 9000 //Max samples for a file
#define SAMPLE_SIZE 9000
#define XML_SIZE 2000 //Size of xml string in XMLBox

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef struct FileTypeBox
{
    u32 size;
    u32 type;                 // ftyp
    u32 major_brand;          // brand identifier
    u32 minor_version;        // informative integer for the mirror version
    u32 compatible_brands[2]; // list of brands
} FileTypeBox;

typedef struct MovieHeaderBox
{
    u32 size;
    u32 type;    // mvhd
    u32 version; // version + flag
    u32 creation_time;
    u32 modification_time;
    u32 timescale; // specifies the time-scale
    u32 duration;
    u32 rate;           // typically 1.0
    u16 volume;         // typically full volume
    u16 reserved;       // =0
    u32 reserved2[2];   //=0
    u32 matrix[9];      // information matrix for video (u,v,w)
    u32 pre_defined[6]; // =0
    u32 next_track_ID;  //non zero value for the next track ID
} MovieHeaderBox;

typedef struct TrackHeaderBox
{
    u32 size;
    u32 type;
    u32 version; // version + flag
    u32 creation_time;
    u32 modification_time;
    u32 track_ID;
    u32 reserved; // =0
    u32 duration;
    u32 reserved2[2];    // =0
    u16 layer;           // =0  // for video
    u16 alternate_group; // =0
    u16 volume;          // full volume is 1 = 0x0100
    u16 reserved3;       // =0
    u32 matrix[9];       // for video
    u32 width;           // video
    u32 height;          // video
} TrackHeaderBox;

typedef struct MediaHeaderBox
{
    u32 size;
    u32 type;    // mdhd
    u32 version; // version + flag
    u32 creation_time;
    u32 modification_time;
    u32 timescale;
    u32 duration;
    u16 language;    // [pad,5x3] = 16 bits and pad = 0
    u16 pre_defined; // =0
} MediaHeaderBox;

typedef struct HandlerBox
{
    u32 size;
    u32 type;         // hdlr
    u32 version;      // version = 0 + flag
    u32 pre_defined;  // =0 (TODO: probably this became component type attribute, so try to change to 4char 'mhlr' to specify media handler type)
    u32 handler_type; // = 'soun' for audio track, text or hint || 'meta' for Timed Metadata
    u32 reserved[3];  // =0
    u8 data[5];       // Does not work! only 4 bytes
} HandlerBox;

typedef struct SoundMediaHeaderBox
{
    u32 size;
    u32 type; // smhd
    u32 version;
    u16 balance;  // =0 place mono tracks in stereo. 0 is center
    u16 reserved; // =0
} SoundMediaHeaderBox;

typedef struct NullMediaHeaderBox
{
    u32 size;
    u32 type; //nmhd
    u32 flags;
} NullMediaHeaderBox;

typedef struct DataEntryUrlBox
{
    u32 size;
    u32 type; // 'url '
    u32 flags;
} DataEntryUrlBox;

typedef struct TimeToSampleBox
{
    u32 size;
    u32 type;
    u32 version;
    u32 entry_count;
    u32 sample_count;
    u32 sample_delta;
    //u32 sample_count[TIME_TO_SAMPLE_SIZE];
    //u32 sample_delta[TIME_TO_SAMPLE_SIZE];
} TimeToSampleBox;

typedef struct FontTableBox
{
    u32 size;
    u32 type;
    u16 entrycount;
    u16 fontID;
    u8 fontnamelenght;
    u8 font[5]; //Serif
} FontTableBox;

typedef struct TextSampleEntryBox
{
    u32 size;
    u32 type; //tx3g
    u32 a;
    u32 b;
    u32 displayFlags;
    u8 horizontaljustification;
    u8 verticaljustification;
    u8 backgroundcolorrgba[4];
    u16 top;
    u16 left;
    u16 bottom;
    u16 right;
    //StyleRecord
    u16 startChar;
    u16 endChar;
    u16 fontID;
    u8 facestyleflags;
    u8 fontsize;
    u8 textcolorrgba[4];
    struct FontTableBox fontTableBox;
} TextSampleEntryBox;

typedef struct DecoderConfigDescriptorBox
{ // size: 15
    u8 tag;
    u8 length;
    u8 objectProfileInd;
    u32 mix;
    u32 maxBitRate;
    u32 avgBitrate;
} DecoderConfigDescriptorBox;

typedef struct SLConfigDescriptorBox
{ // size: 3
    u8 tag;
    u8 length;
    u8 predifined;
} SLConfigDescriptorBox;

typedef struct ESDescriptorBox
{ // size: 5
    u8 tag;
    u8 length;
    u16 ES_ID;
    u8 mix;
    struct DecoderConfigDescriptorBox decoderConfigDescriptorBox;
    struct SLConfigDescriptorBox slConfigDescriptorBox;
} ESDescriptorBox;

typedef struct ESBox
{
    u32 size; // 12
    u32 type;
    u32 version;
    struct ESDescriptorBox esDescriptorBox;
} ESBox;

typedef struct AudioSampleEntryBox
{
    u32 size; // 36
    u32 type; // testing 'sowt' for 16-bit little-endian, twos-complement, if doesn't work TODO: try 0/none
    char reserved[6];
    u16 data_reference_index; // = 1
    u16 version;              // testing version 0 that supports only uncompressed audio
    u16 revision_level;       // = 0
    u32 vendor;               // = 0
    u16 num_channels;         // 1 = mono , 2 = stereo
    u16 sample_size;          // sample size in bits (8 or 16)
    u16 compression_ID;       // = 0
    u16 packet_size;          // = 0
    u32 sample_rate;          // says its a 32-bit unsigned fixed-point number
    struct ESBox esBox;
} AudioSampleEntryBox;

typedef struct SampleSizeBox
{
    u32 size;
    u32 type;
    u32 version;
    u32 sample_size;
    u32 entry_count;
    //u32 entry_size[SAMPLE_SIZE];
} SampleSizeBox;

typedef struct SampleToChunkBox
{
    u32 size;
    u32 type;
    u32 version;
    u32 entry_count;
    u32 first_chunk;
    u32 samples_per_chunk;
    u32 sample_description_index;
} SampleToChunkBox;

typedef struct ChunkOffsetBox
{
    u32 size;
    u32 type;
    u32 version;
    u32 entry_count;
    u32 chunk_offset;
} ChunkOffsetBox;

typedef struct PresetBox
{
    u32 size;
    u32 type;
    u32 flags;
    char preset_name[50];
    u8 preset_ID;
    u8 num_preset_elements;
    u8 preset_type;
    u8 preset_global_volume;
    struct presElemId
    {
        u32 preset_element_ID;
    } presElemId[MAX_TRACKS];
    // TODO: this part is taken from the old im af encoder and im not really sure how to rewrite it.
    // if (preset_type == 0) || (preset_type == 8) - Static track volume preset
    struct StaticTrackVolume
    {
        struct presVolumElem
        {
            u8 preset_volume_element;
            struct EQ
            { // if preset_type == 8 (with EQ)
                u8 num_eq_filters;
                struct Filter
                {
                    u8 filter_type;
                    u16 filter_reference_frequency;
                    u8 filter_gain;
                    u8 filter_bandwidth;
                } Filter[MAX_FILTERS];
            } EQ;
        } presVolumElem[MAX_TRACKS];
    } StaticTrackVolume;

    // if (preset_type == 1) || (preset_type == 9) - Static object volume preset
    struct StaticObjectVolume
    {
        struct InputCH
        {
            u8 num_input_channel;
        } InputCH[MAX_TRACKS];
        u8 output_channel_type;
        struct presElVol_1
        {
            struct Input
            {
                struct Output
                {
                    u8 preset_volume_element;
                } Output[NUM_CHANNELS];
                struct EQ_1
                { // if preset_type == 9 (with EQ)
                    u8 num_eq_filters;
                    struct Filter_1
                    {
                        u8 filter_type;
                        u16 filter_reference_frequency;
                        u8 filter_gain;
                        u8 filter_bandwidth;
                    } Filter[MAX_FILTERS];
                } EQ;
            } Input[NUM_CHANNELS];
        } presElVol[MAX_TRACKS];
    } StaticObjectVolume;

    // if (preset_type == 2) || (preset_type == 10) - Dynamic track volume preset
    struct DynamicTrackVolume
    {
        u16 num_updates;
        struct DynamicChange
        {
            u16 updated_sample_number;
            struct presVolumElem_2
            {
                u8 preset_volume_element;
                struct EQ_2
                { // if preset_type == 10 (with EQ)
                    u8 num_eq_filters;
                    struct Filter_2
                    {
                        u8 filter_type;
                        u16 filter_reference_frequency;
                        u8 filter_gain;
                        u8 filter_bandwidth;
                    } Filter[MAX_FILTERS];
                } EQ;
            } presVolumElem[MAX_TRACKS];
        } DynamicChange[MAX_DYNAMIC];
    } DynamicTrackVolume;

    // if (preset_type == 3) || (preset_type == 11) - Dynamic object volume preset
    struct DynamicObjectVolume
    {
        u16 num_updates;
        struct InputCH_3
        {
            u8 num_input_channel;
        } InputCH[MAX_TRACKS];
        u8 output_channel_type;
        struct DynamicChange_3
        {
            u16 updated_sample_number;
            struct presElVol
            {
                struct Input_3
                {
                    struct Output_3
                    {
                        u8 preset_volume_element;
                    } Output[NUM_CHANNELS];
                    struct EQ_3
                    { // if preset_type == 11 (with EQ)
                        u8 num_eq_filters;
                        struct Filter_3
                        {
                            u8 filter_type;
                            u16 filter_reference_frequency;
                            u8 filter_gain;
                            u8 filter_bandwidth;
                        } Filter[MAX_FILTERS];
                    } EQ;
                } Input[NUM_CHANNELS];
            } presElVol[MAX_TRACKS];
        } DynamicChange[MAX_DYNAMIC];
    } DynamicObjectVolume;

    // if (preset_type == 4) || (preset_type == 12) - Dynamic track approximated volume preset
    struct DynamicTrackApproxVolume
    {
        u16 num_updates;
        struct DynamicChange_4
        {
            u16 start_sample_number;
            u16 duration_update;
            struct presElVol_4
            {
                u8 end_preset_volume_element;
                struct EQ_4
                { // if preset_type == 12 (with EQ)
                    u8 num_eq_filters;
                    struct Filter_4
                    {
                        u8 filter_type;
                        u16 filter_reference_frequency;
                        u8 end_filter_gain;
                        u8 filter_bandwidth;
                    } Filter[MAX_FILTERS];
                } EQ;
            } presElVol[MAX_TRACKS];
        } DynamicChange[MAX_DYNAMIC];
    } DynamicTrackApproxVolume;

    // if (preset_type == 5) || (preset_type == 13) - Dynamic object approximated volume preset
    // THIS STRUCTURE GIVES STACK OVERFLOW PROBLEMS - MORE STACK SIZE NEEDED -> Needs investigation
    struct DynamicObjectApproxVolume
    {
        u16 num_updates;
        struct InputCH_5
        {
            u8 num_input_channel;
        } InputCH[MAX_TRACKS];
        u8 output_channel_type;
        struct DynamicChange_5
        {
            u16 start_sample_number;
            u16 duration_update;
            struct presElVol_5
            {
                struct Input_5
                {
                    struct Output_5
                    {
                        u8 preset_volume_element;
                    } Output[NUM_CHANNELS];
                    struct EQ_5
                    { // if preset_type == 11 (with EQ)
                        u8 num_eq_filters;
                        struct Filter_5
                        {
                            u8 filter_type;
                            u16 filter_reference_frequency;
                            u8 end_filter_gain;
                            u8 filter_bandwidth;
                        } Filter[MAX_FILTERS];
                    } EQ;
                } Input[NUM_CHANNELS];
            } presElVol[MAX_TRACKS];
        } DynamicChange[MAX_DYNAMIC];
    } DynamicObjectApproxVolume;
} PresetBox;

typedef struct PresetContainerBox
{
    u32 size;
    u32 type; //prco
    u8 num_preset;
    u8 default_preset_ID;
    struct PresetBox presetBox;
} PresetContainerBox;

typedef struct SelectionRulesBox
{
    u32 size;
    u32 type;
    u32 version;
    u16 selection_rule_ID;
    u8 selection_rule_type;
    u32 element_ID;
    // Only for Min/Max Rule
    // if (selection_rule_type==0)
    u16 min_num_elements;
    u16 max_num_elements;
    // Only for Exclusion and Implication Rules
    // if (selection_rule_type==1 || selection_rule_type==3)
    u32 key_element_ID;
    char rule_description[20];
} SelectionRulesBox;

typedef struct MixingRulesBox
{
    u32 size;
    u32 type;
    u32 version;
    u16 mixing_rule_ID;
    u8 mixing_type;
    u32 element_ID;
    u16 min_volume;
    u16 max_volume;
    u32 key_elem_ID;
    char mix_description[17];
} MixingRulesBox;

typedef struct GroupBox
{
    u32 size; // = 21 + 22 + 32 (+2 if group_activation_mode = 2)
    u32 type; // grup
    u32 version;
    u32 group_ID;
    u16 num_elements;
    struct groupElemId
    {
        u32 element_ID;
    } groupElemId[MAX_TRACKS];
    u8 group_activation_mode;
    u16 group_activation_elements_number;
    u16 group_reference_volume;
    char group_name[22];
    char group_description[32];
} GroupBox;

typedef struct GroupContainerBox
{
    u32 size; // = 10 + sizeGRUP
    u32 type; // grco
    u16 num_groups;
    struct GroupBox groupBox;
} GroupContainerBox;

typedef struct ItemLocationBox
{
    u32 size;
    u32 type;
    u32 version;              // version = 0 + flags
    u8 offset_size;           // = 4 bytes
    u8 lenght_size;           // = 4 bytes
    u8 base_offset_size;      // = 4 bytes
    u8 reserved;              // = 0
    u16 item_count;           // = 1
    u16 item_ID;              // = 1
    u16 data_reference_index; // = 0 (this file)
    u32 base_offset;          // size=(base_offset_size*8)=4*8
    u16 extent_count;         // = 1
    u32 extent_offset;        // size=(offset_size*8)=4*8
    u32 extent_length;        // size=(lenght_size*8)=4*8
} ItemLocationBox;

typedef struct ItemInfoEntryBox
{
    u32 size;
    u32 type;                  // infe
    u32 version;               // = 0
    u16 item_ID;               // = 1
    u16 item_protection_index; // = 0 for "unprotected"
    char item_name[6];         // name with max 5 characters
    char content_type[18];     // = 'application/other' -> 17 characters
    char content_encoding[4];  // = 'jpg' for JPEG image -> 3 characters
} ItemInfoEntryBox;

typedef struct ItemInfoBox
{
    u32 size;
    u32 type;        // iinf
    u32 version;     // version = 0 + flag
    u16 entry_count; // = 1
    struct ItemInfoEntryBox itemInfoEntryBox;
} ItemInfoBox;

typedef struct XMLBox
{
    u32 size;
    u32 type; // 'xml '
    u32 version;
    char string[XML_SIZE];
} XMLBox;

typedef struct MediaDataBox
{
    u32 size;
    u32 type; // mdat
    u8 data;
} MediaDataBox;

typedef struct BaseMediaInformationBox
{
    u32 size;           // 24
    u32 type;           // 'gmin'
    u32 versionAndFlag; // version is 1 byte, flag 3 bytes. no  3byte variable exist, so this is the way I suppose
    u16 graphicsMode;   // testing 0x0
    u16 opColor[3];     // testing 0
    u16 balance;        // 0
    u16 reserved;       // 0
} BaseMediaInformationBox;
// gmhd only in meta track
typedef struct BaseMediaInformationHeaderBox
{
    u32 size; // 8 + 24
    u32 type; // 'gmhd' also GenericMediaHeaderBox
    BaseMediaInformationBox baseMediaInformationBox;
} BaseMediaInformationHeaderBox;

typedef struct BitRateBox
{                   // optional, but could be useful for sensor data streaming
    u32 size;       // 20
    u32 type;       //  'btrt'
    u32 bufferSize; //
    u32 maxBitRate;
    u32 averageBitRate;
} BitRateBox;

typedef struct MetadataKeyDeclarationBox
{
    // This box could specify the sensor type?
    u32 size;         // 16
    u32 type;         // 'keyd'
    u32 keyNamespace; // 'mdta' for generic reverse utf-8 string, can't find other types (if we register a new metadata key for sensors, we can specify all kind of stuff in here)
    u32 keyValue;     // 'snsr' new value to specify sensor data(?) *** we can use an exisestent metadata key but they are not what we need ***
} MetadataKeyDeclarationBox;

typedef struct MetadataDatatypeDefinitionBox
{
    // This box can specify our sensor data
    u32 size;      // 16
    u32 type;      // 'dtyp'
    u32 nameSpace; // 0 for a numbered list of datatypes, 1 for a reverse utf-8 string
    u32 array;     // 76 for a 16-bit big-endian unsigned int / 23 for a 32-bit big-endian floating point value (IEEE754)
} MetadataDatatypeDefinitionBox;

typedef struct MetadataKeyBox
{               // This box can be an array of keys
    u32 size;   // 8 + sKeyd + sDtyp
    u8 type[4]; // 'local_key_id' (FourCC 32-bit integer, can be 'snsr' for sensor? Or 'sns#'/'sn##' so 10 or 100 unique sensor ids can be created)
    MetadataKeyDeclarationBox metadataKeyDeclarationBox;
    MetadataDatatypeDefinitionBox metadataDatatypeDefinitionBox;
} MetadataKeyBox;

typedef struct MetadataKeyTableBox
{
    u32 size;
    u32 type; // 'keys'
    MetadataKeyBox metadataKeyBox;
} MetadataKeyTableBox;

typedef struct SampleDescriptionTableBox
{
    // This box can be extended to provide more data about sensors
    // For now it has a structure of a generic Timed Metadata Description Table atom
    u32 size;
    u32 type; // 'mebx' for general Timed Metadata
    u8 reserved[6];
    u16 data_reference_index; // = 1
    MetadataKeyTableBox metadataKeyTableBox;
    //BitRateBox bitRateBox;
} SampleDescriptionTableBox;

typedef struct TrackReferenceTypeBox
{
    // extends atom ("cdsc") <-- timed metadata reference type
    u32 size;
    u32 type;             // 'cdsc'
    u32 trackIDReference; // track ID reference for the metadata (Many sensors will reference a single track that matches a SMI)
} TrackReferenceTypeBox;

typedef struct TrackReferenceBox
// extends atom ("tref")
{
    u32 size;
    u32 type; //tref
    TrackReferenceTypeBox trackReferenceTypeBox;
} TrackReferenceBox;

typedef struct DataReferenceBox
{
    u32 size;
    u32 type;
    u32 flags;
    u32 entry_count; // counts the actual entries.
    struct DataEntryUrlBox dataEntryUrlBox;
} DataReferenceBox;

typedef struct DataInformationBox
{
    u32 size;
    u32 type; //dinf
    struct DataReferenceBox dataReferenceBox;
} DataInformationBox;

typedef struct MetaBox
{
    u32 size;
    u32 type;
    u32 version;
    struct HandlerBox handlerBox;
    struct DataInformationBox dataInformationBox;
    struct ItemLocationBox itemLocationBox;
    struct ItemInfoBox itemInfoBox;
    struct XMLBox xmlBox;
} MetaBox;

typedef struct RulesContainerBox
{
    u32 size;
    u32 type;
    u16 num_selection_rules;
    u16 num_mixing_rules;
    struct SelectionRulesBox selectionRulesBox;
    struct MixingRulesBox mixingRulesBox;
} RulesContainerBox;

typedef struct SampleDescriptionBox
{
    u32 size;
    u32 type; //stsd
    u32 version;
    u32 entry_count;                                            // = 1 number of entries
    struct TextSampleEntryBox textSampleEntryBox;               // table for text hints
    struct AudioSampleEntryBox audioSampleEntryBox;             // table fot Audio
    struct SampleDescriptionTableBox sampleDescriptionTableBox; // table for Timed Metadata
} SampleDescriptionBox;

typedef struct SampleTableBox
{
    u32 size;
    u32 type; // stbl
    struct TimeToSampleBox timeToSampleBox;
    struct SampleDescriptionBox sampleDescriptionBox;
    struct SampleSizeBox sampleSizeBox;
    struct SampleToChunkBox sampleToChunkBox;
    struct ChunkOffsetBox chunkOffsetBox;
} SampleTableBox;

typedef struct MediaInformationBox
{
    u32 size;
    u32 type; // minf
    SoundMediaHeaderBox soundMediaHeaderBox;
    BaseMediaInformationHeaderBox baseMediaInformationHeaderBox;
    DataInformationBox dataInformationBox;
    SampleTableBox sampleTableBox;
} MediaInformationBox;

typedef struct MediaBox
{
    u32 size;
    u32 type; // mdia
    MediaHeaderBox mediaHeaderBox;
    HandlerBox handlerBox;
    MediaInformationBox mediaInformationBox;
} MediaBox;

typedef struct TrackBox
{
    u32 size;
    u32 type; //trak
    TrackHeaderBox trackHeaderBox;
    TrackReferenceBox trackReferenceBox; //optional box that specifies if a track is a reference to another one
    MediaBox mediaBox;
} TrackBox;

typedef struct MovieBox
{
    u32 size;
    u32 type; // moov
    MovieHeaderBox movieHeaderBox;
    TrackBox trackBox[MAX_TRACKS + MAX_SENSORS];
    PresetContainerBox presetContainerBox;
    RulesContainerBox rulesContainerBox;
    GroupContainerBox groupContainerBox;
} MovieBox;

#endif
