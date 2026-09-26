/**
 * File name: DspLayerProxy.cpp
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

#include "DspLayerProxy.h"
#include "DspProxy.h"

DspLayerProxy::DspLayerProxy(DspProxy *proxy, size_t layerIndex)
        : dspProxy{proxy}
        , layer{layerIndex}
{
}

bool DspLayerProxy::enable(bool b)
{
        return dspProxy->enableLayer(static_cast<DspProxy::Layer>(layer), b);
}

bool DspLayerProxy::isEnabled() const
{
        return dspProxy->isLayerEnabled(static_cast<DspProxy::Layer>(layer));
}

double DspLayerProxy::getLimiterValue() const
{
        return dspProxy->getLayerAmplitude(static_cast<DspProxy::Layer>(layer));
}

bool DspLayerProxy::setLimiterValue(double value)
{
        return dspProxy->setLayerAmplitude(static_cast<DspProxy::Layer>(layer), value);
}
