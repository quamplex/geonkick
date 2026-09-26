/**
 * File name: LayersView.cpp
 * Project: Geonkick (A percussive synthesizer)
 *
 * Copyright (C) 2019 Iurie Nistor
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

#include "LayersView.h"
#include "LayersModel.h"
#include "LayerView.h"

#include "RkContainer.h"

RK_DECLARE_IMAGE_RC(layer1_name_label);
RK_DECLARE_IMAGE_RC(layer2_name_label);
RK_DECLARE_IMAGE_RC(layer3_name_label);

LayersView::LayersView(GeonkickWidget *parent, LayersModel *model)
        : AbstractView(parent, model)
{
        setFixedSize(224, 80);
        setBackgroundColor({70, 68, 68});
        setBorderWidth(1);
        setBorderColor(55, 54, 54);
        createView();
}

void LayersView::createView()
{
        auto layersModel = static_cast<LayersModel*>(getModel());

        auto layerListLayout = new RkContainer(this, Rk::Orientation::Vertical);
        layerListLayout->setSize(size());

        const std::vector<RkImage> rcNameLabels {
                RK_RC_IMAGE(layer1_name_label),
                RK_RC_IMAGE(layer2_name_label),
                RK_RC_IMAGE(layer3_name_label)
        };

        layerListLayout->addSpace(3);
        const auto nLayers = layersModel->layers().size();
        for (size_t i = 0; i < nLayers; i++) {
                auto layerView = new LayerView(this,
                                               layersModel->layers()[i],
                                               layersModel,
                                               i,
                                               rcNameLabels[i % rcNameLabels.size()]);
                layerViews.push_back(layerView);
                layerListLayout->addSpace(3);
                layerListLayout->addWidget(layerView);
        }
}

void LayersView::updateView()
{
        for (auto layerView : layerViews)
                layerView->updateView();
}

void LayersView::bindModel()
{
}

void LayersView::unbindModel()
{
}
