/**
 * File name: InstrumentGlobalEffects.cpp
 * Project: Geonkick (A percussive synthesizer)
 *
 * Copyright (C) 2024 Iurie Nistor
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

#include "InstrumentGlobalEffects.h"
#include "InstrumentModel.h"
#include "FilterModel.h"
#include "FilterView.h"
#include "DistortionModel.h"
#include "DistortionView.h"
#include "HumanizerModel.h"
#include "HumanizerView.h"
#include "EffectTabButton.h"

#include "RkContainer.h"

RK_DECLARE_IMAGE_RC(osc_effects_bk);
RK_DECLARE_IMAGE_RC(effects_tab_filter_button);
RK_DECLARE_IMAGE_RC(effects_tab_filter_button_active);
RK_DECLARE_IMAGE_RC(effects_tab_filter_button_hover);
RK_DECLARE_IMAGE_RC(effects_tab_distortion_button);
RK_DECLARE_IMAGE_RC(effects_tab_distortion_button_active);
RK_DECLARE_IMAGE_RC(effects_tab_distortion_button_hover);
RK_DECLARE_IMAGE_RC(effects_tab_humanizer_button);
RK_DECLARE_IMAGE_RC(effects_tab_humanizer_button_active);
RK_DECLARE_IMAGE_RC(effects_tab_humanizer_button_hover);


InstrumentGlobalEffects::InstrumentGlobalEffects(GeonkickWidget *parent, PercussionModel* model)
        : AbstractView(parent, model)
        , instrumentModel{model}
        , filterTabButton{nullptr}
        , distortionTabButton{nullptr}
#ifndef GEONKICK_BASIC_VERSION
        , humanizerTabButton {nullptr}
#endif // GEONKICK_BASIC_VERSION
        , currentTabView{nullptr}
{
        setName("InstrumentGlobalEffects");
        setSize(224, 131);
        setBackgroundImage(RkImage(224, 115, RK_IMAGE_RC(osc_effects_bk)));
        createView();
        show();
}

void InstrumentGlobalEffects::bindModel()
{
        instrumentModel = static_cast<PercussionModel*>(getModel());
        if (!instrumentModel)
                return;

        if (currentTabView) {
                if (dynamic_cast<FilterView*>(currentTabView))
                        currentTabView->setModel(instrumentModel->getFilter());
                else if (dynamic_cast<DistortionView*>(currentTabView))
                        currentTabView->setModel(instrumentModel->getDistortion());
                else
                        currentTabView->setModel(instrumentModel->getHumanizer());
        }
}

void InstrumentGlobalEffects::unbindModel()
{
}

void InstrumentGlobalEffects::createView()
{
        auto tabButtonsLayout = new RkContainer(this);
        tabButtonsLayout->setPosition({0, 4});
        tabButtonsLayout->setSize({width(), 16});
        tabButtonsLayout->addSpace(4);

        // Filter tab
        filterTabButton = new EffectTabButton(this);
        filterTabButton->setPressed(true);
        filterTabButton->setImage(RkImage(filterTabButton->size(), RK_IMAGE_RC(effects_tab_filter_button)),
                                  RkButton::State::Unpressed);
        filterTabButton->setImage(RkImage(filterTabButton->size(), RK_IMAGE_RC(effects_tab_filter_button_active)),
                                  RkButton::State::Pressed);
        filterTabButton->setImage(RkImage(filterTabButton->size(), RK_IMAGE_RC(effects_tab_filter_button_hover)),
                                  RkButton::State::UnpressedHover);
        filterTabButton->setImage(RkImage(filterTabButton->size(), RK_IMAGE_RC(effects_tab_filter_button_hover)),
                                  RkButton::State::PressedHover);
        tabButtonsLayout->addWidget(filterTabButton);
        RK_ACT_BINDL(filterTabButton,
                     enabled,
                     RK_ACT_ARGS(bool b),
                     [=,this](bool b){instrumentModel->getFilter()->enable(b);});

        // Distortion tab
        tabButtonsLayout->addSpace(2);
        distortionTabButton = new EffectTabButton(this);
        distortionTabButton->setImage(RkImage(distortionTabButton->size(), RK_IMAGE_RC(effects_tab_distortion_button)),
                                  RkButton::State::Unpressed);
        distortionTabButton->setImage(RkImage(distortionTabButton->size(), RK_IMAGE_RC(effects_tab_distortion_button_active)),
                                  RkButton::State::Pressed);
        distortionTabButton->setImage(RkImage(distortionTabButton->size(), RK_IMAGE_RC(effects_tab_distortion_button_hover)),
                                  RkButton::State::UnpressedHover);
        distortionTabButton->setImage(RkImage(distortionTabButton->size(), RK_IMAGE_RC(effects_tab_distortion_button_hover)),
                                  RkButton::State::PressedHover);

        tabButtonsLayout->addWidget(distortionTabButton);
        RK_ACT_BINDL(distortionTabButton,
                    enabled,
                    RK_ACT_ARGS(bool b),
                     [=,this](bool b){
                             if (instrumentModel)
                                     instrumentModel->getDistortion()->enable(b);
                     });
#ifndef GEONKICK_BASIC_VERSION
        // Humanizer tab
        tabButtonsLayout->addSpace(2);
        humanizerTabButton = new EffectTabButton(this);
        humanizerTabButton->setImage(RkImage(humanizerTabButton->size(), RK_IMAGE_RC(effects_tab_humanizer_button)),
                                  RkButton::State::Unpressed);
        humanizerTabButton->setImage(RkImage(humanizerTabButton->size(), RK_IMAGE_RC(effects_tab_humanizer_button_active)),
                                  RkButton::State::Pressed);
        humanizerTabButton->setImage(RkImage(humanizerTabButton->size(), RK_IMAGE_RC(effects_tab_humanizer_button_hover)),
                                  RkButton::State::UnpressedHover);
        humanizerTabButton->setImage(RkImage(humanizerTabButton->size(), RK_IMAGE_RC(effects_tab_humanizer_button_hover)),
                                  RkButton::State::PressedHover);

        tabButtonsLayout->addWidget(humanizerTabButton);
        RK_ACT_BINDL(humanizerTabButton,
                    enabled,
                    RK_ACT_ARGS(bool b),
                     [=,this](bool b){
                             if (instrumentModel)
                                     instrumentModel->getHumanizer()->enable(b);
                     });
#endif // GEONKICK_BASIC_VERSION

        RK_ACT_BIND(filterTabButton,
                     pressed,
                     RK_ACT_ARGS(),
                     this,
                     showFilter());
        RK_ACT_BIND(distortionTabButton,
                     pressed,
                     RK_ACT_ARGS(),
                     this,
                     showDistortion());
#ifndef GEONKICK_BASIC_VERSION
        RK_ACT_BIND(humanizerTabButton,
                     pressed,
                     RK_ACT_ARGS(),
                     this,
                     showHumanizer());
#endif // GEONKICK_BASIC_VERSION

        if (filterTabButton->isPressed())
                showFilter();
        else if (distortionTabButton->isPressed())
                showDistortion();
        else
                showHumanizer();

        updateView();
}

void InstrumentGlobalEffects::showFilter()
{
        if (currentTabView)
                delete currentTabView;
        distortionTabButton->setPressed(false);
#ifndef GEONKICK_BASIC_VERSION
        humanizerTabButton->setPressed(false);
#endif //GEONKICK_BASIC_VERSION
        currentTabView = new FilterView(this, instrumentModel->getFilter());
        currentTabView->setPosition(0, 22);
}

void InstrumentGlobalEffects::showDistortion()
{
        if (currentTabView)
                delete currentTabView;
        filterTabButton->setPressed(false);
#ifndef GEONKICK_BASIC_VERSION
        humanizerTabButton->setPressed(false);
#endif // GEONKICK_BASIC_VERSION
        currentTabView = new DistortionView(this, instrumentModel->getDistortion());
        currentTabView->setPosition(0, 22);
}

void InstrumentGlobalEffects::showHumanizer()
{
        if (currentTabView)
                delete currentTabView;
        filterTabButton->setPressed(false);
        distortionTabButton->setPressed(false);
        currentTabView = new HumanizerView(this, instrumentModel->getHumanizer());
        currentTabView->setPosition(0, 22);
}

void InstrumentGlobalEffects::updateView()
{
        if (!getModel())
                return;
        filterTabButton->enable(instrumentModel->getFilter()->isEnabled());
        distortionTabButton->enable(instrumentModel->getDistortion()->isEnabled());
#ifndef GEONKICK_BASIC_VERSION
        humanizerTabButton->enable(instrumentModel->getHumanizer()->isEnabled());
#endif // GEONKICK_BASIC_VERSION
        currentTabView->updateView();
}
