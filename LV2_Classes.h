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

#endif // LV2_CLASSES_H
