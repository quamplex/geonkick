/**
 * File name: LayerView.cpp
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
 * This program is free software: you can redistribute it and/or modify
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

#include "LayerView.h"
#include "LayerModel.h"
#include "LayersModel.h"
#include "Limiter.h"
#include "geonkick_button.h"

#include "RkEvent.h"
#include "RkLabel.h"

RK_DECLARE_IMAGE_RC(layer_enable_button);
RK_DECLARE_IMAGE_RC(layer_enable_button_hover);
RK_DECLARE_IMAGE_RC(layer_enable_button_on);

LayerView::LayerView(GeonkickWidget *parent,
                     LayerModel *model,
                     LayersModel *layerListModel,
                     size_t index,
                     const RkImage &layerNameLabel)
        : AbstractView(parent, model)
        , layerNameLabel{layerNameLabel}
        , nameLabel{nullptr}
        , limiter{nullptr}
        , enableButton{nullptr}
        , layersModel{layerListModel}
        , layerIndex{index}
        , hovered{false}
        , selected{layerListModel->currentLayer() == index}
{
        setFixedSize(214, 21);
        setBackgroundColor({68, 68, 70});
        setBorderWidth(1);
        setBorderColor(55, 54, 54);
        createView();
        bindModel();
        updateView();
}

void LayerView::createView()
{
        int xPos = 2;
        nameLabel = new RkLabel(this, layerNameLabel);
        nameLabel->setBackgroundColor(background());
        nameLabel->setPosition(xPos, 1 + (height() - nameLabel->height()) / 2);
        nameLabel->show();

        xPos += nameLabel->width() + 3;
        limiter = new GeonkickLimiter(this);
        constexpr int limiterWidth = 140;
        constexpr int limiterHeight = 10;
        limiter->setSize(limiterWidth, limiterHeight);
        limiter->setPosition(xPos, 1 + (height() - limiterHeight) / 2);

        enableButton = new GeonkickButton(this);
        enableButton->setType(RkButton::ButtonType::ButtonCheckable);
        enableButton->setSize(16, 16);
        enableButton->setPosition(width() - enableButton->width() - 4,
                                  1 + (height() - enableButton->height()) / 2);
        enableButton->setImage(RK_RC_IMAGE(layer_enable_button),
                               RkButton::State::Unpressed);
        enableButton->setImage(RK_RC_IMAGE(layer_enable_button_hover),
                               RkButton::State::UnpressedHover);
        enableButton->setImage(RK_RC_IMAGE(layer_enable_button_on),
                               RkButton::State::Pressed);
        enableButton->setImage(RK_RC_IMAGE(layer_enable_button_hover),
                               RkButton::State::PressedHover);
        enableButton->show();
}

void LayerView::updateView()
{
        auto model = static_cast<LayerModel*>(getModel());
        if (!model)
                return;

        selected = layersModel->currentLayer() == layerIndex;
        limiter->setValue(model->limiter());
        enableButton->setPressed(model->isEnabled());
        updateBackground();
}

void LayerView::bindModel()
{
        auto model = static_cast<LayerModel*>(getModel());
        if (!model)
                return;

        RK_ACT_BIND(limiter,
                    valueUpdated,
                    RK_ACT_ARGS(double value),
                    model,
                    setLimiter(value));
        RK_ACT_BIND(enableButton,
                    toggled,
                    RK_ACT_ARGS(bool enabled),
                    model,
                    enable(enabled));
        RK_ACT_BIND(model,
                    enbaledUpdated,
                    RK_ACT_ARGS(bool enabled),
                    enableButton,
                    setPressed(enabled));
        RK_ACT_BIND(model,
                    limiterUpdated,
                    RK_ACT_ARGS(double value),
                    limiter,
                    setValue(value));
        RK_ACT_BIND(layersModel,
                    currentLayerChanged,
                    RK_ACT_ARGS(size_t index),
                    this,
                    setSelectedLayer(index));
}

void LayerView::unbindModel()
{
        auto model = getModel();
        if (!model)
                return;

        unbindObject(model);
        limiter->unbindObject(model);
        enableButton->unbindObject(model);
        layersModel->unbindObject(this);
}

void LayerView::hoverEvent(RkHoverEvent *event)
{
        hovered = event->isHover();
        updateBackground();
}

void LayerView::mouseButtonPressEvent(RkMouseEvent *event)
{
        RK_UNUSED(event);
        layersModel->setCurrentLayer(layerIndex);
}

void LayerView::setSelectedLayer(size_t index)
{
        selected = index == layerIndex;
        updateBackground();
}

void LayerView::updateBackground()
{
        const RkColor color = selected
                ? RkColor{75, 83, 101}
                : hovered
                  ? RkColor{78, 78, 80}
                  : RkColor{68, 68, 70};
        setBackgroundColor(color);
        if (nameLabel)
                nameLabel->setBackgroundColor(color);
        update();
}
