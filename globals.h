#ifndef GLOBALS
#define GLOBALS

const double sample_rate = 48000.0;
const int buf_size = 512;

/**
   Control change event, sent through ring buffers for UI updates.
*/
typedef struct {
    uint32_t index;
    uint32_t protocol;
    uint32_t size;
    uint8_t  body[];
} ControlChange;

#endif // GLOBALS

