#ifndef __DACOUT_DECAY_H__
#define __DACOUT_DECAY_H__

#include <math.h>
#include "arm_math.h" // 假设已包含


#define LINEAR_DECAY 0.798f
#define FREQ_3DB 750000.0f

#define CURRENT_DECAY(freq) (LINEAR_DECAY * expf(-freq / FREQ_3DB))


#endif /* __DACOUT_DECAY_H__ */