/**
 * File name: LayersModel.cpp
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

#include "LayersModel.h"

LayersModel::LayersModel(DspProxy *proxy, RkObject *parent)
        : AbstractModel(parent)
        , dspProxy{proxy}
{
        size_t nLayers = dspProxy->numberOfLayers();
        for (size_t i = 0; i < nLayers; i++)
                layersList.push_back(new LayerModel(this, dspProxy));
}
