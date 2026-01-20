/**
 * @file qx_fader.h
 * @brief Smooth fade in/out for DSP signals.
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

#ifndef QX_FADER_H
#define QX_FADER_H

#include "qx_math.h"

#include <stdbool.h>
#include <stddef.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief Smooth fade in/out for DSP signals.
*
* This struct maintains the current fade value and step for
* smoothly ramping audio signals up or down.
*/
typedef struct qx_fader {
        float fade;      /**< Current fade value [0..1] */
        float step;      /**< Fade increment per sample */
        bool enabled;    /**< Target state: true = fade in, false = fade out */
} qx_fader;

/**
 * @brief Initialize a fader.
 *
 * @param fader Pointer to qx_fader struct.
 * @param fadeTimeSeconds Time to fade in milliseconds.
 * @param sampleRate Audio sample rate.
 *
 * Sets the initial fade value to 0 and computes per-sample
 * step based on fade time and sample rate.
 */
static inline void qx_fader_init(struct qx_fader* fader, float fadeTime, float sample_rate)
{
        fader->fade = 0.0f;
        fader->enabled = false;

        if (fadeTime <= 0.0f)
                fader->step = 1.0f; // instant fade
        else
                fader->step = 1.0f / ((fadeTime / 1000.0f) * sample_rate);
}

/**
 * @brief Enable or disable the fader.
 *
 * @param fader Pointer to qx_fader struct.
 * @param enabled True to fade in, false to fade out.
 *
 * Sets the target state. The fade value will ramp towards
 * 1.0 if enabled or 0.0 if disabled during processing.
 */
static inline void qx_fader_enable(struct qx_fader* fader, bool enabled)
{
        fader->enabled = enabled;
        fader->fade = enabled ? 0.0f : 1.0f;
}

/**
 * @brief Apply fade to a single sample.
 *
 * @param fader Pointer to qx_fader struct.
 * @param val Input sample value.
 * @return Faded sample value.
 *
 * Updates the internal fade value and multiplies the input
 * sample by it. Should be called for every audio sample.
 */
static inline float qx_fader_fade(struct qx_fader* fader, float val)
{
        fader->fade += fader->enabled ? fader->step : -fader->step;
        fader->fade = qx_clamp_float(fader->fade, 0.0f, 1.0f);
        return val * fader->fade;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // QX_FADER_H
