/**
 * @file qx_math.h
 * @brief Math utilities for Quamplex DSP Tools.
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

#ifndef QX_MATH_H
#define QX_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * @brief Returns the number of elements in a static array.
 *
 * @param arr The array.
 * @return Number of elements in the array.
 *
 * @note Only works for statically allocated arrays, not pointers.
 */
#define QX_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/**
 * @brief Clamp a value between a minimum and maximum.
 *
 * Works for integer and floating-point types.
 *
 * @param value Input value.
 * @param min Minimum allowed value.
 * @param max Maximum allowed value.
 * @return Clamped value.
 */
#define QX_CLAMP(value, min, max) ((value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))

/**
 * @brief Normalize a float value to the range [0.0, 1.0].
 *
 * @param value Input value.
 * @param min Minimum of the original range.
 * @param max Maximum of the original range.
 * @return Normalized value.
 */
static inline float qx_normalize_float(float value, float min, float max)
{
        return (value - min) / (max - min);
}

/**
 * @brief Normalize a double value to the range [0.0, 1.0].
 *
 * @param value Input value.
 * @param min Minimum of the original range.
 * @param max Maximum of the original range.
 * @return Normalized value.
 */
 static inline double qx_normalize_double(double value, double min, double max)
 {
         return (value - min) / (max - min);
 }

/**
 * @brief Denormalize a float value from [0.0, 1.0] to a given range.
 *
 * @param normalized Normalized value.
 * @param min Minimum of the target range.
 * @param max Maximum of the target range.
 * @return Denormalized value.
 */
static inline float qx_denormalize_float(float normalized, float min, float max)
{
        return min + normalized * (max - min);
}

/**
 * @brief Denormalize a double value from [0.0, 1.0] to a given range.
 *
 * @param normalized Normalized value.
 * @param min Minimum of the target range.
 * @param max Maximum of the target range.
 * @return Denormalized value.
 */
 static inline double qx_denormalize_double(double normalized, double min, double max)
 {
         return min + normalized * (max - min);
 }

/**
 * @brief Clamp a float value between min and max.
 *
 * @param value Input value.
 * @param min Minimum allowed value.
 * @param max Maximum allowed value.
 * @return Clamped value.
 */
static inline float qx_clamp_float(float value, float min, float max)
{
        return QX_CLAMP(value, min, max);
}

/**
 * @brief Clamp a double value between min and max.
 *
 * @param value Input value.
 * @param min Minimum allowed value.
 * @param max Maximum allowed value.
 * @return Clamped value.
 */
static inline double qx_clamp_double(double value, double min, double max)
{
        return QX_CLAMP(value, min, max);
}

/**
 * @brief Convert decibels (dB) to linear amplitude.
 *
 * Uses the standard formula:
 *   linear = 10^(dB / 20)
 *
 * @param db Value in decibels.
 * @return Linear amplitude value.
 */
static inline float qx_db_to_val(float db)
{
        return powf(10.0f, db * 0.05f);
}

/**
 * @brief Convert linear amplitude to decibels (dB).
 *
 * If the input value is less than or equal to zero,
 * negative infinity is returned.
 *
 * @param val Linear amplitude value.
 * @return Value in decibels.
 */
 static inline float qx_val_to_db(float val)
 {
         return (val > 0.0f) ? (20.0f * log10f(val)) : -INFINITY;
 }

/**
 * @brief Linearly interpolate a value from a circular (ring) buffer.
 *
 * This function performs linear interpolation between two neighboring
 * samples in a ring buffer using a floating-point index.
 *
 * @param buf Pointer to the buffer.
 * @param index Floating-point read index.
 * @param size Size of the buffer.
 * @return Interpolated sample value.
 *
 * @note The buffer is assumed to be continuous and wrapped.
 */
static inline float qx_ring_interp_linear(const float *buf,
                                          float index,
                                          int size)
{
        int i1 = (int)index;
        int i2 = i1 + 1;

        if (i1 >= size)
                i1 -= size;

        if (i2 >= size)
                i2 -= size;

        float k = index - (float)i1;

        return buf[i1] + k * (buf[i2] - buf[i1]);
}

/**
 * @brief Wrap a floating-point value into the range [0, max).
 *
 * @param x Input value.
 * @param max Upper bound (exclusive).
 * @return Wrapped value.
 */
 static inline float qx_wrapf(float x, float max)
 {
         while (x < 0.0f) x += max;
         while (x >= max) x -= max;
         return x;
 }

#ifdef __cplusplus
}
#endif

#endif // QX_MATH_H
