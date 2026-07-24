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
#include "Limiter.h"
#include "geonkick_button.h"
#include "LayersModel.h"

#include "RkLabel.h"
#include "RkContainer.h"

#include <algorithm>

RK_DECLARE_IMAGE_RC(layer1_name_label);
RK_DECLARE_IMAGE_RC(layer2_name_label);
RK_DECLARE_IMAGE_RC(layer3_name_label);
RK_DECLARE_IMAGE_RC(layer_enable_button);
RK_DECLARE_IMAGE_RC(layer_enable_button_hover);
RK_DECLARE_IMAGE_RC(layer_enable_button_on);

LayersView::LayersView(GeonkickWidget *parent, LayersModel *model)
        : AbstractView(parent, model)
{
        setFixedSize(224, 83);
        setBackgroundColor({99, 0, 0});

        createView();
        bindModel();

        show();
}

void LayersView::createView()
{
        auto layersModel = static_cast<LayersModel*>(getModel());

        auto layerLayout = new RkContainer(this);
        layerLayout->setSize(width(), 83);

        const std::vector<RkImage> rcNameLabels {
                RK_RC_IMAGE(layer1_name_label),
                RK_RC_IMAGE(layer2_name_label),
                RK_RC_IMAGE(layer3_name_label)
        };

        const auto nLayers = layersModel->layers().size();
        for (size_t i = 0; i < nLayers; i++) {
                // Name label
                auto nameLabel = new RkLabel(this, rcNameLabels[i % rcNameLabels.size()]);
                layerLayout->addWidget(nameLabel);

                // Limiter / Slider
                layerLayout->addSpace(5);
                auto limiter = new GeonkickLimiter(this);
                limiter->setFixedSize(100, 10);
                layerLayout->addWidget(limiter);

                // Enable button
                auto enableButton = new GeonkickButton(this);
                enableButton->setType(RkButton::ButtonType::ButtonCheckable);
                enableButton->setSize(16, 16);
                enableButton->setImage(RK_RC_IMAGE(layer_enable_button),
                                       RkButton::State::Unpressed);
                enableButton->setImage(RK_RC_IMAGE(layer_enable_button_hover),
                                       RkButton::State::UnpressedHover);
                enableButton->setImage(RK_RC_IMAGE(layer_enable_button_on),
                                       RkButton::State::Pressed);
                enableButton->setImage(RK_RC_IMAGE(layer_enable_button_hover),
                                       RkButton::State::PressedHover);
                enableButton->show();
                layerLayout->addWidget(enableButton);

                layerControls.empalce_back(nameLabel, limiter, enableButton);
        }
}

void LayersView::updateView()
{
        auto layersModel = static_cast<LayersModel*>(getModel());
        auto& layers = layersModel->layers();

        size_t n = std::min(layers.size(), layerControls.size());
        for (size_t i = 0; i < n; i++) {
                layerControls[i].limiter->setValue(layers[i]->getValue());
                layerControls[i].enableButton->setPressed(layers[i]->isEnabled());
        }
}

void LayersView::bindModel()
{
        auto layersModel = static_cast<LayersModel*>(getModel());
        auto nLayers = layersModel->layers().size();

        for (size_t i = 0; i < nLayers; i++) {
                auto layer = layersModel->layer(i);

                // UI to Model: Limiter
                RK_ACT_BIND(layerControls[i].limiter,
                            valueUpdated,
                            RK_ACT_ARGS(double val),
                            layer,
                            setLimiter(val));

                // UI to Model: Enable Toggled
                RK_ACT_BIND(layerControls[i].enableButton,
                            toggled,
                            RK_ACT_ARGS(bool b),
                            layer,
                            enable(b));

                // Model to UI: Enable State
                RK_ACT_BIND(layer,
                            enableChanged, // Assuming signal name
                            RK_ACT_ARGS(bool b),
                            layerControls[i].enableButton,
                            setPressed(b));

                // Model to UI: Limiter Value
                RK_ACT_BIND(layer,
                            limiterUpdated,
                            RK_ACT_ARGS(double val),
                            layerControls[i],
                            setValue(val));
        }
}

void LayersView::unbindModel()
{
        auto model = getModel();

        unbindObject(model);
        for (auto controls& : layerControls) {
                controls.limiter->unbinObject(model);
                controls.enableButton->unbinObject(model);
        }
}
