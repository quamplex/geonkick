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
#include "LayerModel.h"
#include "DspProxy.h"

LayersModel::LayersModel(DspProxy *proxy, RkObject *parent)
        : AbstractModel(parent)
        , dspProxy{proxy}
{
        size_t nLayers = dspProxy->numberOfLayers();
        layersList.reserve(nLayers);
        for (size_t i = 0; i < nLayers; i++) {
                auto layer = new LayerModel(dspProxy->layer(i), this);
                layersList.push_back(layer);
                RK_ACT_BIND(this,
                            modelUpdated,
                            RK_ACT_ARGS(),
                            layer,
                            modelUpdated());
        }
}

const std::vector<LayerModel*>& LayersModel::layers() const
{
        return layersList;
}

size_t LayersModel::currentLayer() const
{
        return static_cast<size_t>(dspProxy->layer());
}

void LayersModel::setCurrentLayer(size_t index)
{
        if (index >= layersList.size())
                return;

        if (currentLayer() == index)
                return;

        dspProxy->setLayer(static_cast<DspProxy::Layer>(index));
        action currentLayerChanged(index);
}
