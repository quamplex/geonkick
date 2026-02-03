/**
 * File name: CentralWidget.cpp
 * Project: Geonkick (A percussive synthesizer)
 *
 * Copyright (C) 2017 Iurie Nistor
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

#include "CentralWidget.h"
#include "GeonkickModel.h"
#include "SynthesizerWidget.h"
#ifndef GEONKICK_SINGLE
#include "KitWidget.h"
#endif // GEONKICK_SINGLE

CentralWidget::CentralWidget(GeonkickWidget *parent, GeonkickModel* model)
        : GeonkickWidget(parent)
        , geonkickModel{model}
        , currentWidget{nullptr}
{
        setSize(930, parent->height() - 30);
        RK_ACT_BIND(viewState(),
                    mainViewChanged,
                    RK_ACT_ARGS(ViewState::View view),
                    this,
                    showWidget(view));

        showWidget(viewState()->getMainView());
}

void CentralWidget::showWidget(ViewState::View view)
{
        switch (view) {
        case ViewState::View::Controls:
                showSynthesizer();
                break;
#ifndef GEONKICK_SINGLE
        case ViewState::View::Kit:
                showKit();
                break;
#endif // GEONKICK_SINGLE
        default:
                showSynthesizer();
        }
}

void CentralWidget::showSynthesizer()
{
        if (currentWidget)
                delete currentWidget;

        currentWidget = new SynthesizerWidget(this, geonkickModel);
}

#ifndef GEONKICK_SINGLE
void CentralWidget::showKit()
{
        if (currentWidget)
                delete currentWidget;

        currentWidget = new KitWidget(this, geonkickModel->getKitModel());
}
#endif // GEONKICK_SINGLE
