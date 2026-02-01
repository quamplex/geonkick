/**
 * File name: CentralWidget.h
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

#ifndef GKICK_INSTRUMENT_EDITOR_H
#define GKICK_INSTRUMENT_EDITOR_H

#include "envelope.h"
#include "geonkick_widget.h"
#include "ViewState.h"

class GeonkickModel;

class CentralWidget: public GeonkickWidget
{
 public:
        explicit CentralWidget(GeonkickWidget *parent, GeonkickModel* model);
        ~CentralWidget() = default;
        void showSynthesizer();
#ifndef GEONKICK_SINGLE
        void showKit();
#endif // GEONKICK_SINGLE

 protected:
        void showWidget(ViewState::View view);

 private:
        GeonkickModel *geonkickModel;
        RkWidget* currentWidget;
};

#endif // GKICK_CENTRAL_WIDGET_H
