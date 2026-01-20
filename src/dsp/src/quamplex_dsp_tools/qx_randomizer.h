/**
 * @file qx_randomizer.h
 * @brief Basic pseudo-random number generator using lightweight PRNG.
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

#ifndef QX_RANDOMIZER_H
#define QX_RANDOMIZER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief Basic pseudo-random number generator configuration and state.
*
* This randomizer is designed with audio DSP in mind:
* - Uses a lightweight PRNG for efficiency in real-time audio processing.
* - Provides sufficient randomness quality for typical audio purposes,
*   prioritizing speed and repeatability over cryptographic-grade randomness.
* - Given the same seed, the generated pattern is reproducible.
* - No global state; multiple instances can coexist without interference.
*
* @note Not thread-safe.
*/

struct qx_randomizer {
    uint32_t seed;         /**< Current seed used for pseudo-random generation. */
    float min;             /**< Minimum value (inclusive) of the output range. */
    float max;             /**< Maximum value (inclusive) of the output range. */
    float resolution;      /**< Step size for quantization (e.g., 0.01f for hundredths). */

    // Internal precomputed values for speed (not meant for manual editing)
    float range;           /**< Cached: max - min. */
    float inv_max_uint;    /**< Cached: 1.0f / UINT32_MAX, for normalization. */
    int max_steps;         /**< Cached: number of quantization steps. */
};

/**
 * @brief Global atomic seed used for generating unique seeds via SplitMix32.
 *
 * Atomic ensures thread-safe updates without locking,
 * suitable for real-time audio.
 */
static atomic_uint qx_global_seed = 1u;

/**
 * @brief SplitMix32 generator for producing high-quality 32-bit seeds.
 *
 * Each call advances the internal global seed and returns a new pseudo-random value.
 * This is used only to initialize qx_randomizer instances with unique seeds.
 *
 * @return A 32-bit pseudo-random seed value.
 */
static inline uint32_t qx_splitmix32()
{
        uint32_t z = atomic_fetch_add(&qx_global_seed, 0x9e3779b9u);
        z = (z ^ (z >> 16)) * 0x85ebca6bu;
        z = (z ^ (z >> 13)) * 0xc2b2ae35u;
        return z ^ (z >> 16);
}

/**
 * @brief Initializes a `qx_randomizer` instance.
 *
 * @param rand Pointer to the randomizer structure to initialize.
 * @param seed Initial seed for the pseudo-random generator.
 * @param min Minimum float value that can be generated (inclusive).
 * @param max Maximum float value that can be generated (inclusive).
 * @param resolution Step size for quantized output values.
 *
 * This function precomputes internal values for fast usage in loops.
 */
static inline void qx_randomizer_init(struct qx_randomizer* rand,
                                      float min,
                                      float max,
                                      float resolution)
{
        rand->seed = 856382025u + qx_splitmix32();
        rand->min = min;
        rand->max = max;
        rand->resolution = resolution;
        rand->range = max - min;
        rand->inv_max_uint = 1.0f / (float)UINT32_MAX;
        rand->max_steps = (int)(rand->range / resolution + 0.5f);
}

/**
 * @brief Sets the seed for the randomizer.
 *
 * This allows you to manually re-seed the randomizer for reproducible results
 * or to randomize it with a new seed.
 *
 * @param rand Pointer to the qx_randomizer structure.
 * @param seed The new seed to use.
 */
static inline void qx_randomizer_set_seed(struct qx_randomizer* rand, uint32_t seed)
{
    rand->seed = seed;
}

/**
 * @brief Sets the minimum, maximum, and resolution range for the randomizer.
 *
 * Updates the output range of the randomizer and recalculates internal
 * parameters accordingly.
 *
 * @param rand Pointer to the qx_randomizer structure.
 * @param min The minimum value the randomizer should output.
 * @param max The maximum value the randomizer should output.
 * @param resolution The step size between random values.
 */
static inline void qx_randomizer_set_range(struct qx_randomizer* rand,
                                           float min,
                                           float max)
{
    rand->min = min;
    rand->max = max;
    rand->range = max - min;
    rand->max_steps = (int)(rand->range / rand->resolution + 0.5f);
}

/**
 * @brief Sets the resolution for the randomizer.
 *
 * Updates the resolution (step size) used to generate random values
 * and recalculates the number of steps accordingly. The range must
 * already be defined using `min` and `max`.
 *
 * @param rand Pointer to the qx_randomizer structure.
 * @param resolution The new resolution value.
 */
static inline void qx_randomizer_set_resolution(struct qx_randomizer* rand, float resolution)
{
    rand->resolution = resolution;
    rand->max_steps = (int)(rand->range / resolution + 0.5f);
}

/**
 * @brief Generates a random quantized float within the configured range.
 *
 * @param rand Pointer to an initialized `qx_randomizer`.
 * @return A float value in [min, max], snapped to the nearest multiple of `resolution`.
 *
 * This function is fast and designed for tight loops. It uses an internal linear congruential
 * generator to update the seed and maps the result to quantized float values.
 */
static inline float qx_randomizer_get_float(struct qx_randomizer* rand)
{
    rand->seed = rand->seed * 1664525u + 1013904223u;

    float normalized = rand->seed * rand->inv_max_uint;
    int step = (int)(normalized * (rand->max_steps + 1));

    if (step > rand->max_steps)
            step = rand->max_steps;

    return rand->min + step * rand->resolution;
}

#ifdef __cplusplus
}
#endif

#endif // QX_RANDOMIZER_H
