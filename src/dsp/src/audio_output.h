/**
 * File name: audio_output.h
 * Project: Geonkick (A percussive synthesizer)
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

#ifndef GKICK_AUDO_OUTPUT_H
#define GKICK_AUDO_OUTPUT_H

#include "geonkick_internal.h"
#include "ring_buffer.h"
#include "humanizer.h"

#include <pthread.h>
#include <stdatomic.h>

/* Decay time measured in number of audio frames. */
#define GEKICK_KEY_RELESE_DECAY_TIME 1000

struct gkick_note_info {
        enum gkick_key_state state;
        signed char channel;
        signed char note_number;
        signed char velocity;
        float timing;
};

struct gkick_humanizer_params
{
        atomic_bool  enabled;
        _Atomic float velocity;
        _Atomic float timing;
};

enum gkick_instrument_param {
        GKICK_INSTR_PARAM_HUM_ENABLE,
        GKICK_INSTR_PARAM_HUM_VEL,
        GKICK_INSTR_PARAM_HUM_TIME
};

struct gkick_audio_output
{
        int sample_rate;

	/* Specifies if this audio output is active. */
        _Atomic bool enabled;
        struct gkick_buffer *updated_buffer;
        struct gkick_buffer* playing_buffer;
        struct ring_buffer* ring_buffer;
        _Atomic bool buffer_updated;

        /* Note info is changed only by the audio thread. */
        struct gkick_note_info key;

        /* The key number that triggres playing. */
        _Atomic signed char playing_key;

        /* MIDI channel for playing. */
        _Atomic signed char midi_channel;

        /**
         * Specifies if the audio output is in the
         * playing state (the instrument is playing)
         */
        _Atomic bool play;

        /**
         * Triggers the play, usualy changed from UI call.
         */
        _Atomic bool start_play;

        /**
         * Specifies if to tune the output in accordance with
         * the note (central note is A4).
         */
        _Atomic bool tune;

        /* Output channel. */
      	atomic_size_t channel;

        /* The output is muted or not. */
        _Atomic bool muted;

        /* The output is muted or not. */
        _Atomic bool solo;

        /* Output audio limiter value. */
        atomic_int limiter;

        /* Enable/disable note off */
        _Atomic bool note_off;

        /* Current instrument choke group. 0 value is none (off) */
        _Atomic signed char choke_group;

        /* Contains the paramteres for humanizer. */
        struct gkick_humanizer_params humanizer_params;

        pthread_mutex_t lock;

        /* Velocity humanizer */
        struct gkick_humanizer velocity_humanizer;

        /* Timing humanizer */
        struct gkick_humanizer timing_humanizer;
};

enum geonkick_error
gkick_audio_output_create(struct gkick_audio_output **audio_output, int sample_rate);

void gkick_audio_output_free(struct gkick_audio_output **audio_output);

struct gkick_buffer*
gkick_audio_output_get_buffer(struct gkick_audio_output  *audio_output);

/**
 * This function must be called
 * only from the audio thread.
 */
enum geonkick_error
gkick_audio_output_key_pressed(struct gkick_audio_output *audio_output,
                               struct gkick_note_info *key);

void
gkick_audio_add_playing_buffer_to_ring(struct gkick_audio_output *audio_output, size_t size);

enum geonkick_error
gkick_audio_output_play(struct gkick_audio_output *audio_output);

/* This funciton is called from the audio thread. */
void
gkick_audio_set_play(struct gkick_audio_output *audio_output);

gkick_real
gkick_audio_output_tune_factor(int note_number);

void gkick_audio_output_swap_buffers(struct gkick_audio_output *audio_output);

enum geonkick_error
gkick_audio_output_set_playing_key(struct gkick_audio_output *audio_output, signed char key);

enum geonkick_error
gkick_audio_output_set_midi_channel(struct gkick_audio_output *audio_output, signed char channel);

enum geonkick_error
gkick_audio_output_get_midi_channel(struct gkick_audio_output *audio_output, signed char *channel);

enum geonkick_error
gkick_audio_output_get_playing_key(struct gkick_audio_output *audio_output, signed char *key);

void gkick_audio_output_tune_output(struct gkick_audio_output *audio_output, bool tune);

bool gkick_audio_output_is_tune_output(struct gkick_audio_output *audio_output);

enum geonkick_error
gkick_audio_output_set_channel(struct gkick_audio_output *audio_output,
                               size_t channel);

enum geonkick_error
gkick_audio_output_get_channel(struct gkick_audio_output *audio_output,
                               size_t *channel);

void gkick_audio_output_get_data(struct gkick_audio_output *audio_output,
                                 gkick_real **data,
                                 gkick_real *leveler,
                                 size_t size);
void gkick_audio_output_enable_note_off(struct gkick_audio_output *audio_output,
                                 bool enable);

bool gkick_audio_output_note_off(struct gkick_audio_output *audio_output);

void gkick_instrument_set_param(struct gkick_audio_output *audio_output,
                                enum gkick_instrument_param param,
                                const void *value);
void gkick_instrument_get_param(const struct gkick_audio_output *audio_output,
                                enum gkick_instrument_param param,
                                void *value);
struct gkick_note_info*
gkick_instrument_humanize_key(struct gkick_audio_output *audio_output,
                              struct gkick_note_info* key);
void gkick_audio_output_lock(struct gkick_audio_output *audio_output);
void gkick_audio_output_unlock(struct gkick_audio_output *audio_output);

#endif // GKICK_AUDO_OUTPUT_H
