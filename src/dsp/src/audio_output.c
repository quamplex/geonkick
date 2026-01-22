/**
 * File name: audio_output.c
 * Project: Geonkick (A kick synthesizer)
 *
 * Copyright (C) 2018 Iurie Nistor
 *
 * This file is part of Geonkick.
 *
 * GeonKick is free software; you can redistribute it and/or modify
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

#include "audio_output.h"

enum geonkick_error
gkick_audio_output_create(struct gkick_audio_output **audio_output, int sample_rate)
{
        if (audio_output == NULL) {
                gkick_log_error("wrong arguments");
                return GEONKICK_ERROR;
        }

        *audio_output = (struct gkick_audio_output*)calloc(1, sizeof(struct gkick_audio_output));
        if (*audio_output == NULL) {
                gkick_log_error("can't allocate memory");
                return GEONKICK_ERROR;
        }
        (*audio_output)->play         = false;
        (*audio_output)->start_play   = false;
	(*audio_output)->enabled      = true;
        (*audio_output)->muted        = false;
        (*audio_output)->solo         = false;
        (*audio_output)->channel      = 0;
        (*audio_output)->playing_key  = GEONKICK_ANY_KEY;
        (*audio_output)->midi_channel = GEONKICK_ANY_MIDI_CHANNEL;
        (*audio_output)->sample_rate  = sample_rate;
        (*audio_output)->note_off     = false;
        (*audio_output)->choke_group  = GKICK_CHOKE_GROUP_OFF;

        gkick_humanizer_init(&(*audio_output)->velocity_humanizer);
        gkick_humanizer_init(&(*audio_output)->timing_humanizer);

        (*audio_output)->humanizer_params = (struct gkick_humanizer_params) {
                .enabled = false,
                .velocity = (*audio_output)->velocity_humanizer.velocity_percent,
                .timing = (*audio_output)->velocity_humanizer.timing };

        gkick_buffer_new(&(*audio_output)->updated_buffer,
                         (*audio_output)->sample_rate * GEONKICK_MAX_LENGTH);
        if ((*audio_output)->updated_buffer == NULL) {
                gkick_log_error("can't create updated buffer");
                gkick_audio_output_free(audio_output);
                return GEONKICK_ERROR;
        }
        gkick_buffer_set_size((*audio_output)->updated_buffer, 0);

        gkick_buffer_new(&(*audio_output)->playing_buffer,
                         (*audio_output)->sample_rate * GEONKICK_MAX_LENGTH);
        if ((*audio_output)->playing_buffer == NULL) {
                gkick_log_error("can't create playing buffer");
                gkick_audio_output_free(audio_output);
                return GEONKICK_ERROR;
        }
        gkick_buffer_set_size((*audio_output)->playing_buffer, 0);

        /**
         * Create the instrument ring buffer.
         * The ring buffer is twice bigger than the
         * maximum instrument sound length. This permits
         * to tune the instrument sound up to an octave.
         */
        enum geonkick_error err;
        err = ring_buffer_new(&(*audio_output)->ring_buffer,
                              (*audio_output)->sample_rate * 2 * GEONKICK_MAX_LENGTH,
                              sample_rate);
        if (err != GEONKICK_OK) {
                gkick_log_error("can'r create ring buffer");
                gkick_audio_output_free(audio_output);
                return GEONKICK_ERROR;
        }

        if (pthread_mutex_init(&(*audio_output)->lock, NULL) != 0) {
                gkick_log_error("error on init mutex");
                gkick_audio_output_free(audio_output);
                return GEONKICK_ERROR;
	}

        return GEONKICK_OK;
}

void gkick_audio_output_free(struct gkick_audio_output **audio_output)
{
        if (audio_output != NULL && *audio_output != NULL) {
                gkick_buffer_free(&(*audio_output)->playing_buffer);
                gkick_buffer_free(&(*audio_output)->updated_buffer);
                ring_buffer_free(&(*audio_output)->ring_buffer);
                pthread_mutex_destroy(&(*audio_output)->lock);
                free(*audio_output);
                *audio_output = NULL;
        }
}

enum geonkick_error
gkick_audio_output_key_pressed(struct gkick_audio_output *audio_output,
                               struct gkick_note_info *key)
{
        if (audio_output->muted)
                return GEONKICK_OK;

        if (key->state == GKICK_KEY_STATE_PRESSED) {
                audio_output->key = *key;
#ifndef GEONKICK_BASIC_VERSION
                gkick_instrument_humanize_key(audio_output, &audio_output->key);
#endif // GEONKICK_BASIC_VERSION
                atomic_store_explicit(&audio_output->play,
                                      true,
                                      memory_order_relaxed);
                ring_buffer_turnoff_decay(audio_output->ring_buffer);
                gkick_audio_output_swap_buffers(audio_output);
                if (!gkick_audio_output_note_off(audio_output)) {
                        // Add all the buffer.
                        gkick_audio_add_playing_buffer_to_ring(audio_output, SIZE_MAX);
                }
        } else if (key->state == GKICK_KEY_STATE_RELEASED) {
                audio_output->key.state = key->state;
                if (gkick_audio_output_note_off(audio_output)) {
                        atomic_store_explicit(&audio_output->play,
                                              false,
                                              memory_order_relaxed);
                        ring_buffer_start_decay(audio_output->ring_buffer);
                }
        }

        return GEONKICK_OK;
}

/* This funciton is called from the audio thread. */
void
gkick_audio_set_play(struct gkick_audio_output *audio_output)
{
        if (audio_output->muted)
                return;

        struct gkick_note_info key;
        key.channel        = GEONKICK_ANY_MIDI_CHANNEL;
        key.note_number    = GEONKICK_ANY_KEY;
        key.velocity       = 127;
        key.state          = GKICK_KEY_STATE_PRESSED;
        gkick_audio_output_key_pressed(audio_output, &key);
}

void
gkick_audio_add_playing_buffer_to_ring(struct gkick_audio_output *audio_output,
                                       size_t size)
{
        size_t i = 0;
        gkick_real factor = gkick_audio_output_tune_factor(audio_output->key.note_number);
        while (i < size) {
                if (audio_output->key.timing > 0.0f) {
                        float time_ms = 1000.0f / audio_output->sample_rate;
                        audio_output->key.timing -= time_ms;
                        if (audio_output->key.timing > 0) {
                                i++;
                                continue;
                        } else {
                                audio_output->key.timing = 0.0f;
                        }
                }

                /**
                 * When NOTE OFF is ignored, the entire buffer needs to be added.
                 * Therefore, the loop will terminate because `size` is set to `SIZE_MAX`.
                 * When tuning is enabled, and the buffer is stretched, and its final
                 * actual size is bigger than the original and we need to count only on
                 * gkick_buffer_is_end.
                 */
                if (gkick_buffer_is_end(audio_output->playing_buffer))
                        break;

                gkick_real val;
                if (audio_output->tune) {
                        val = gkick_buffer_stretch_get_next(audio_output->playing_buffer,
                                                            factor);
                } else {
                        val = gkick_buffer_get_next(audio_output->playing_buffer);
                }

                val *= ((float)audio_output->key.velocity / 127);
                ring_buffer_add_value(audio_output->ring_buffer, i++, val);
        }
}

enum geonkick_error
gkick_audio_output_play(struct gkick_audio_output *audio_output)
{
        audio_output->start_play = true;
        return GEONKICK_OK;
}

gkick_real
gkick_audio_output_tune_factor(int note_number)
{
        gkick_real factor = exp2f((gkick_real)(note_number - 69) / 12.0f);
        return GKICK_CLAMP(factor, 0.25f, 3.0f);
}

struct gkick_buffer*
gkick_audio_output_get_buffer(struct gkick_audio_output  *audio_output)
{
        return audio_output->playing_buffer;
}

void gkick_audio_output_swap_buffers(struct gkick_audio_output *audio_output)
{
        gkick_buffer_reset(audio_output->playing_buffer);

        /**
         * Try lock. If successful, swap buffers. If not, continue with
         * the current one to play until the next press of the key.
         */
        if (pthread_mutex_trylock(&audio_output->lock) == 0) {
                /* Test if the updated buffer is full. Otherwise it means that it was not updated. */
                if (gkick_buffer_size(audio_output->updated_buffer) > 0
                    && gkick_buffer_is_end(audio_output->updated_buffer)) {
                        struct gkick_buffer *buff = audio_output->updated_buffer;
                        audio_output->updated_buffer = audio_output->playing_buffer;
                        audio_output->playing_buffer = buff;
                }
                gkick_buffer_reset(audio_output->playing_buffer);
                gkick_audio_output_unlock(audio_output);
        }
}

enum geonkick_error
gkick_audio_output_set_playing_key(struct gkick_audio_output *audio_output, signed char key)
{
        if (key < 21 || key > 108)
                key = GEONKICK_ANY_KEY;
        audio_output->playing_key = key;
        return GEONKICK_OK;
}

enum geonkick_error
gkick_audio_output_get_playing_key(struct gkick_audio_output *audio_output, signed char *key)
{
        *key = audio_output->playing_key;
        return GEONKICK_OK;
}


enum geonkick_error
gkick_audio_output_set_midi_channel(struct gkick_audio_output *audio_output, signed char channel)
{
        if (channel < -1 && channel > 15) {
                gkick_log_error("wrong channel value: %d", channel);
                return GEONKICK_ERROR;
        }

        audio_output->midi_channel = channel;
        return GEONKICK_OK;
}

enum geonkick_error
gkick_audio_output_get_midi_channel(struct gkick_audio_output *audio_output, signed char *channel)
{
        *channel = audio_output->midi_channel;
        return GEONKICK_OK;
}

void gkick_audio_output_tune_output(struct gkick_audio_output *audio_output, bool tune)
{
        audio_output->tune = tune;
}

bool gkick_audio_output_is_tune_output(struct gkick_audio_output *audio_output)
{
        return audio_output->tune;
}

enum geonkick_error
gkick_audio_output_set_channel(struct gkick_audio_output *audio_output,
                               size_t channel)
{
        audio_output->channel = channel;
        return GEONKICK_OK;
}

enum geonkick_error
gkick_audio_output_get_channel(struct gkick_audio_output *audio_output,
                               size_t *channel)
{
        *channel = audio_output->channel;
        return GEONKICK_OK;
}

void gkick_audio_output_get_data(struct gkick_audio_output *audio_output,
                                 gkick_real **data,
                                 gkick_real *leveler,
                                 size_t size)
{
        if (gkick_audio_output_note_off(audio_output))
                gkick_audio_add_playing_buffer_to_ring(audio_output, size);

        *leveler = ring_buffer_get_cur_data(audio_output->ring_buffer);
        float limiter = (float)audio_output->limiter / 1000000;

        ring_buffer_get_data(audio_output->ring_buffer,
                             data[0],
                             limiter,
                             size);
        for (size_t i = 0; i < size; i++)
                data[1][i] = data[0][i];
        ring_buffer_next(audio_output->ring_buffer, size);
}

void gkick_audio_output_enable_note_off(struct gkick_audio_output *audio_output,
                                 bool enable)
{
        audio_output->note_off = enable;
}

bool gkick_audio_output_note_off(struct gkick_audio_output *audio_output)
{
        return audio_output->note_off;
}

void gkick_instrument_set_param(struct gkick_audio_output *audio_output,
                                enum gkick_instrument_param param,
                                const void *value)
{
        switch (param) {
        case GKICK_INSTR_PARAM_HUM_ENABLE:
                atomic_store_explicit(&audio_output->humanizer_params.enabled,
                                      *(const bool *)value,
                                      memory_order_relaxed);
                break;
        case GKICK_INSTR_PARAM_HUM_VEL:
                atomic_store_explicit(&audio_output->humanizer_params.velocity,
                                      *(const float *)value,
                                      memory_order_relaxed);
                break;
        case GKICK_INSTR_PARAM_HUM_TIME:
                atomic_store_explicit(&audio_output->humanizer_params.timing,
                                      *(const float *)value,
                                      memory_order_relaxed);
                break;
        case GKICK_INSTR_PARAM_CHOKE_GROUP:
                atomic_store_explicit(&audio_output->choke_group,
                                      *(const unsigned char *)value,
                                      memory_order_relaxed);
                break;
        default:
                break;
        }
}

void gkick_instrument_get_param(const struct gkick_audio_output *audio_output,
                                enum gkick_instrument_param param,
                                void *value)
{
        switch (param) {
        case GKICK_INSTR_PARAM_HUM_ENABLE:
        {
                bool v = atomic_load_explicit(&audio_output->humanizer_params.enabled,
                                              memory_order_relaxed);
                *(bool*)value = v;
                break;
        }
        case GKICK_INSTR_PARAM_HUM_VEL:
        {
                float v = atomic_load_explicit(&audio_output->humanizer_params.velocity,
                                               memory_order_relaxed);
                *(float *)value = v;
                break;
        }
        case GKICK_INSTR_PARAM_HUM_TIME:
        {
                float v = atomic_load_explicit(&audio_output->humanizer_params.timing,
                                               memory_order_relaxed);
                *(float *)value = v;
                break;
        }
        case GKICK_INSTR_PARAM_CHOKE_GROUP:
        {
                float v = atomic_load_explicit(&audio_output->choke_group,
                                               memory_order_relaxed);
                *(unsigned char *)value = v;
                break;
        }
        default:
                break;
        }
}

struct gkick_note_info*
gkick_instrument_humanize_key(struct gkick_audio_output *audio_output,
                              struct gkick_note_info* key)
{
        bool enabled = false;
        float velocity_percent = 0.0f;
        float timing = 0.0f;

        gkick_instrument_get_param(audio_output,
                                   GKICK_INSTR_PARAM_HUM_ENABLE,
                                   &enabled);
        gkick_instrument_get_param(audio_output, GKICK_INSTR_PARAM_HUM_VEL,
                                   &velocity_percent);
        gkick_instrument_get_param(audio_output, GKICK_INSTR_PARAM_HUM_TIME,
                                   &timing);

        // Velocity
        struct gkick_humanizer *velocity_humanizer = &audio_output->velocity_humanizer;
        gkick_humanizer_enable(velocity_humanizer, enabled);
        gkick_humanizer_set_velocity_percent(velocity_humanizer, velocity_percent);
        gkick_humanizer_velocity(velocity_humanizer, &key->velocity);

        // Timing
        struct gkick_humanizer *timing_humanizer = &audio_output->timing_humanizer;
        gkick_humanizer_enable(timing_humanizer, enabled);
        gkick_humanizer_set_timing_percent(timing_humanizer, timing);
        gkick_humanizer_timing(timing_humanizer, &key->timing);

        return key;
}

void gkick_audio_output_lock(struct gkick_audio_output *audio_output)
{
        if (audio_output != NULL)
                pthread_mutex_lock(&audio_output->lock);
}

void gkick_audio_output_unlock(struct gkick_audio_output *audio_output)
{
        if (audio_output != NULL)
                pthread_mutex_unlock(&audio_output->lock);
}
