/*
 *
 * LV2_Classes.h
 *
 *     Written by Philippe Groarke - February 2015
 *
 * Legal Terms:
 *
 *     This source file is released into the public domain.  It is
 *     distributed without any warranty; without even the implied
 *     warranty * of merchantability or fitness for a particular
 *     purpose.
 *
 * Function:
 *
 *
 */
#ifndef LV2_CLASSES_H
#define LV2_CLASSES_H

#include <lilv/lilv.h>

class LV2_Classes
{
public:
    LV2_Classes(LilvWorld* world);
    ~LV2_Classes();

    LilvNode* input_class;
    LilvNode* output_class;
    LilvNode* control_class;
    LilvNode* audio_class;
    LilvNode* event_class;
    LilvNode* midi_class;
    LilvNode* optional;
};


LV2_Classes::LV2_Classes(LilvWorld* world)
{
    input_class = lilv_new_uri(world, LILV_URI_INPUT_PORT);
    output_class = lilv_new_uri(world, LILV_URI_OUTPUT_PORT);
    control_class = lilv_new_uri(world, LILV_URI_CONTROL_PORT);
    audio_class = lilv_new_uri(world, LILV_URI_AUDIO_PORT);
    event_class = lilv_new_uri(world, LILV_URI_EVENT_PORT);
    midi_class = lilv_new_uri(world, LILV_URI_MIDI_EVENT);
    optional = lilv_new_uri(world, LILV_NS_LV2 "connectionOptional");
}

LV2_Classes::~LV2_Classes()
{
    lilv_node_free(input_class);
    lilv_node_free(output_class);
    lilv_node_free(control_class);
    lilv_node_free(audio_class);
    lilv_node_free(event_class);
    lilv_node_free(midi_class);
    lilv_node_free(optional);
}


#endif // LV2_CLASSES_H
