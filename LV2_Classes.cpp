#include "LV2_Classes.h"

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

