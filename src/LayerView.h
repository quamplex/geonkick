/**
 * File name: LayerView.h
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

#ifndef GKICK_LAYER_VIEW_H
#define GKICK_LAYER_VIEW_H

#include "AbstractView.h"

class GeonkickButton;
class GeonkickLimiter;
class LayerModel;
class LayersModel;
class RkLabel;
class RkHoverEvent;
class RkMouseEvent;

class LayerView : public AbstractView
{
public:
        LayerView(GeonkickWidget *parent,
                  LayerModel *model,
                  LayersModel *layersModel,
                  size_t index,
                  const RkImage &nameLabel);
        ~LayerView() override = default;

        void createView() override;
        void updateView() override;

protected:
        void bindModel() override;
        void unbindModel() override;
        void hoverEvent(RkHoverEvent *event) override;
        void mouseButtonPressEvent(RkMouseEvent *event) override;

private:
        void updateBackground();
        void setSelectedLayer(size_t index);
        void showValue(double value);

        RkImage layerNameLabel;
        RkLabel *nameLabel;
        GeonkickLimiter *limiter;
        RkLabel *limiterValueLabel;
        GeonkickButton *enableButton;
        LayersModel *layersModel;
        size_t layerIndex;
        bool hovered;
        bool selected;
};

#endif // GKICK_LAYER_VIEW_H
