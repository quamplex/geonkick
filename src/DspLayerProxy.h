/**
 * File name: DspLayerProxy.h
 * Project: Geonkick (A percussive synthesizer)
 *
 * Copyright (C) 2026 Iurie Nistor
 *
 * This file is part of Geonkick.
 *
 * Geonkick is free software; you can redistribute it and/or modify
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

#ifndef DSP_LAYER_PROXY_H
#define DSP_LAYER_PROXY_H

#include <cstddef>

class DspProxy;

class DspLayerProxy {
public:
        DspLayerProxy(DspProxy *dspProxy, size_t layer);
        bool enable(bool b);
        bool isEnabled() const;
        double getLimiterValue() const;
        bool setLimiterValue(double value);

private:
        DspProxy *dspProxy;
        size_t layer;
};

#endif // DSP_LAYER_PROXY_H
