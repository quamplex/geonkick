/**
 * File name: SynthesizerWidget.cpp
 * Project: Geonkick (A percussive synthesizer)
 *
 * Copyright (C) 2020 Iurie Nistor
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

#include "SynthesizerWidget.h"
#include "DspProxy.h"
#include "OscillatorModel.h"
#include "GeonkickModel.h"
#include "InstrumentModel.h"
#include "kit_model.h"
#include "envelope_widget.h"
#include "oscillator_group_box.h"
#include "HumanizerView.h"
#include "general_group_box.h"
#include "layers_group_box.h"
#include "AppInfoWidget.h"
#include "limiter.h"
#include "kit_model.h"
#ifndef GEONKICK_SINGLE
#include "KitTabs.h"
#endif // GEONKICK_SINGLE

SynthesizerWidget::SynthesizerWidget(GeonkickWidget *parent,
                                     GeonkickModel* model)
        : GeonkickWidget(parent)
        , geonkickModel{model}
{
        setSize(size());

        // Wavefrom widget
        auto envelopeWidget = new EnvelopeWidget(this, geonkickModel);
        envelopeWidget->show();

        // Limiter
        auto limiterWidget = new Limiter(geonkickModel->getDspProxy(), this);
        limiterWidget->setPosition(envelopeWidget->x() + envelopeWidget->width() + 8,
                                   envelopeWidget->y());
        RK_ACT_BIND(this, updateGui, RK_ACT_ARGS(), limiterWidget, onUpdateLimiter());

        auto controlsYPos = envelopeWidget->y() +  envelopeWidget->height();
        const auto& oscillators = geonkickModel->getOscillatorModels();
        setFixedSize({parent->width(), parent->height()});
        auto oscillator = oscillators[static_cast<int>(OscillatorModel::Type::Oscillator1)];
        auto widget = new OscillatorGroupBox(this, oscillator);
        widget->setPosition(0, controlsYPos);
        RK_ACT_BIND(this, updateGui, RK_ACT_ARGS(), widget, updateGui());
        widget->show();

        oscillator = oscillators[static_cast<int>(OscillatorModel::Type::Oscillator2)];
        widget = new OscillatorGroupBox(this, oscillator);
        widget->setPosition(8 + 223, controlsYPos);
        RK_ACT_BIND(this, updateGui, RK_ACT_ARGS(), widget, updateGui());
        widget->show();

        oscillator = oscillators[static_cast<int>(OscillatorModel::Type::Oscillator3)];
        widget = new OscillatorGroupBox(this, oscillator);
        widget->setPosition(2 * (8 + 223), controlsYPos);
        RK_ACT_BIND(this, updateGui, RK_ACT_ARGS(), widget, updateGui());
        widget->show();

        auto kitModel = geonkickModel->getKitModel();
        auto globalWidget = new GeneralGroupBox(this, kitModel->currentPercussion());
        globalWidget->setPosition(3 * (8 + 223), controlsYPos);
        RK_ACT_BIND(this, updateGui, RK_ACT_ARGS(), globalWidget, updateView());
        RK_ACT_BIND(kitModel,
                    instrumentSelected,
                    RK_ACT_ARGS(PercussionModel *model),
                    globalWidget,
                    setModel(model));
        RK_ACT_BIND(kitModel,
                    instrumentUpdated,
                    RK_ACT_ARGS(PercussionModel *model),
                    globalWidget,
                    setModel(model));
        RK_ACT_BIND(kitModel,
                    modelUpdated,
                    RK_ACT_ARGS(),
                    globalWidget,
                    setModel(kitModel->currentPercussion()));
        globalWidget->show();

        auto layersWidget = new LayersGroupBox(geonkickModel->getDspProxy(), this);
        layersWidget->setPosition(3 * (8 + 223), controlsYPos + 270);
        RK_ACT_BIND(this, updateGui, RK_ACT_ARGS(), layersWidget, updateGui());
        layersWidget->show();

        auto appInfoWidget = new AppInfoWidget(this, geonkickModel);
        appInfoWidget->setPosition(layersWidget->x() + layersWidget->width(),
                                   controlsYPos + layersWidget->y());

#ifndef GEONKICK_SINGLE
        auto kitTabs = new KitTabs(this, geonkickModel->getKitModel());
        kitTabs->setPosition(0, height() - kitTabs->height() - 1);
        RK_ACT_BIND(this, updateGui, RK_ACT_ARGS(), kitTabs, updateView());
#endif // GEONKICK_SINGLE

        RK_ACT_BIND(geonkickModel->getDspProxy(),
                    stateChanged,
                    RK_ACT_ARGS(),
                    this,
                    updateGui());

        show();
}
