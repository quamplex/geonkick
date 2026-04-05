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

RK_DECLARE_IMAGE_RC(layers_mixer);

LayersGroupBox::LayersGroupBox(DspProxy *dsp, GeonkickWidget *parent)
        : GeonkickGroupBox(parent)
        , dspProxy{dsp}
        , layerSliders{nullptr, nullptr, nullptr}
{
        setFixedSize(224, 83);
        setBackgroundColor({99, 0, 0});

        auto layerLayout = new RkCotainer(this);
        layerLayout->setSize(width(), 24);

        craeteLayersMenu(layerLayout);

        for (auto i = 0; i < 3; i++) {
                // Name label
                auto nameLabel = new RkLabel(this, RK_RC_IMAGE(layer1_name_label));
                layerLayout->addWidget(nameLabel);

                // Layer 1
                layerLayout->addSpace(5);
                auto layerSlider = new GeonkickSlider(this);
                layerSlider->setFixedSize(100, 10);
                layerLayout->addWidget(layerSlider);
                RK_ACT_BIND(layerSlider,
                            valueUpdated,
                            RK_ACT_ARGS(int val),
                            this,
                            setLayerAmplitude(1, val));

                // Enable button
                auto enableButton = new RkButton(this);
                muteButton->setType(RkButton::ButtonType::ButtonCheckable);
                muteButton->setSize(16, 16);
                muteButton->setImage(RkImage(muteButton->size(), RK_IMAGE_RC(mute)),
                                     RkButton::State::Unpressed);
                muteButton->setImage(RkImage(muteButton->size(), RK_IMAGE_RC(mute_hover)),
                                     RkButton::State::UnpressedHover);
                muteButton->setImage(RkImage(muteButton->size(), RK_IMAGE_RC(mute_on)),
                                     RkButton::State::Pressed);
                muteButton->setImage(RkImage(muteButton->size(), RK_IMAGE_RC(mute_hover)),
                                     RkButton::State::PressedHover);
                muteButton->show();
        }

        createLayerMixer();

        show();
        updateGui();
}

void LayersGroupBox::createLayerMixer()
{
        int y = 23;
        for (auto i = 0; i < 3; i++) {
                layerSliders[i] = new GeonkickSlider(this);
                layerSliders[i]->setFixedSize(width() - 38, 10);
                layerSliders[i]->setPosition(18, y);
                y += layerSliders[i]->height() + 6;
                layerSliders[i]->show();
                RK_ACT_BIND(layerSliders[i],
                            valueUpdated,
                            RK_ACT_ARGS(int val),
                            this,
                            setLayerAmplitude(i, val));
        }
}

void LayersGroupBox::setLayerAmplitude(int layer, int val)
{
        double logVal = -60 * (1.0 - (static_cast<double>(val) / 100));
        double amplitude = pow(10, logVal / 20);
        dspProxy->setLayerAmplitude(static_cast<DspProxy::Layer>(layer), amplitude);
}

void LayersGroupBox::updateGui()
{
        for (auto i = 0; i < 3; i++) {
                double amplitude = dspProxy->getLayerAmplitude(static_cast<DspProxy::Layer>(i));
                double logVal;
                if (amplitude > 0)
                        logVal = 20 * log10(amplitude);
                else
                        logVal = 60;
                layerSliders[i]->onSetValue(100 * (60 - fabs(logVal)) / 60, 100);
        }
}
