/**
 * File name: layers_group_box.cpp
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

#include "layers_group_box.h"
#include "geonkick_slider.h"
#include "geonkick_button.h"
#include "DspProxy.h"

#include "RkLabel.h"
#include "RkContainer.h"

RK_DECLARE_IMAGE_RC(layer1_name_label);
RK_DECLARE_IMAGE_RC(layer2_name_label);
RK_DECLARE_IMAGE_RC(layer3_name_label);
RK_DECLARE_IMAGE_RC(layer_enable_button);
RK_DECLARE_IMAGE_RC(layer_enable_button_hover);
RK_DECLARE_IMAGE_RC(layer_enable_button_on);

LayersView::LayersView(LayersModel *model, GeonkickWidget *parent)
        : AbstractView(parent, model)
{
        setFixedSize(224, 83);
        setBackgroundColor({99, 0, 0});
        show();
}

void LayerView::createView()
{
        auto layerLayout = new RkContainer(this);
        layerLayout->setSize(width(), 24);

        std::vector<RkImage> rcNameLables {
                RK_RC_IMAGE(layer1_name_label),
                RK_RC_IMAGE(layer2_name_label),
                RK_RC_IMAGE(layer3_name_label)
        };

        for (size_t i = 0; i < layersModel->numberOfLayers(); i++) {
                auto layerModel = layersModel->layer(i);

                // Name label
                auto nameLabel = new RkLabel(this, rcNameLables[i]);
                layerLayout->addWidget(nameLabel);

                // Slider
                layerLayout->addSpace(5);
                auto limiterSlider = new GeonkickSlider(this);
                limiterSlider->setFixedSize(100, 10);
                limiterSlider->addWidget(limiterSlider);
                RK_ACT_BIND(limiterSlider,
                            valueUpdated,
                            RK_ACT_ARGS(int val),
                            layerModel,
                            setLimiter(val));

                // Enable button
                auto enableButton = new RkButton(this);
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
                RK_ACT_BIND(enableButton,
                            toggled,
                            RK_ACT_ARGS(bool b),
                            layerModel,
                            enable(b));
                RK_ACT_BIND(layerModel,
                            enable,
                            RK_ACT_ARGS(bool b),
                            enableButton,
                            enableButton->setPressed(b));
        }
}

void LayerView::updateView()
{
}

void LayerView::bindModel()
{
}

void LayerView::unbindModel()
{
}
