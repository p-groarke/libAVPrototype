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

#endif // LV2_CLASSES_H
