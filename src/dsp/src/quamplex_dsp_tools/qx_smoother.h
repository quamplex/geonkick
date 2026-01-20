/**
 * @file qx_smoother.h
 * @brief A smoother - smoothly interpolates a float value from current to target.
 *
 * Project: Quamplex DSP Tools (A small C library of tools for audio DSP processing)
 * Website: https://quamplex.com
 *
 * Copyright (C) 2025 Iurie Nistor
 *
 * This file is part of Quamplex DSP Tools.
 *
 * Quamplex DSP Tools is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef QX_SMOOTHER_H
#define QX_SMOOTHER_H

#include "qx_math.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic float smoother.
 *
 * Smoothly interpolates a float value from current to target
 * over a fixed number of frames (samples or blocks).
 */
typedef struct qx_smoother {
    float current;    /**< Current value */
    float target;     /**< Target value */
    float step;       /**< Increment per frame */
    size_t frames;    /**< Number of frames to reach target */
} qx_smoother;

/**
 * @brief Initialize the smoother.
 *
 * @param s Pointer to qx_smoother
 * @param initial Initial value
 * @param frames Number of frames (samples or blocks) over which to smooth
 */
static inline void qx_smoother_init(qx_smoother* s, float initial, size_t frames)
{
    s->current = initial;
    s->target = initial;
    s->frames = frames > 0 ? frames : 1;
    s->step = 0.0f;
}

/**
 * @brief Set a new target value.
 *
 * @param s Pointer to qx_smoother
 * @param target New target value
 */
static inline void qx_smoother_set_target(qx_smoother* s, float target)
{
    s->target = target;
    s->step = (s->target - s->current) / (float)s->frames;
}

/**
 * @brief Advance the smoother by one frame.
 *
 * @param s Pointer to qx_smoother
 * @return Smoothed value
 */
static inline float qx_smoother_next(qx_smoother* s)
{
    if (s->current == s->target)
        return s->current;

    s->current += s->step;

    // Clamp to target to avoid overshoot
    if ((s->step > 0.0f && s->current > s->target) ||
        (s->step < 0.0f && s->current < s->target)) {
        s->current = s->target;
    }

    return s->current;
}

/**
 * @brief Get current value without advancing.
 *
 * @param s Pointer to qx_smoother
 * @return Current value
 */
static inline float qx_smoother_get(const qx_smoother* s)
{
    return s->current;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // QX_SMOOTHER_H
