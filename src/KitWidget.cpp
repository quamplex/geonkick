/**
 * File name: KitWidget.cpp
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

#include "KitWidget.h"
#include "kit_model.h"
#include "geonkick_slider.h"
#include "InstrumentView.h"
#include "InstrumentModel.h"

#include "RkEvent.h"
#include "RkImage.h"
#include "RkLabel.h"
#include "RkLineEdit.h"
#include "RkButton.h"
#include "RkProgressBar.h"
#include "RkContainer.h"
#include "RkTimer.h"

RK_DECLARE_IMAGE_RC(add_per_button);
RK_DECLARE_IMAGE_RC(add_per_button_hover);
RK_DECLARE_IMAGE_RC(add_per_button_on);
RK_DECLARE_IMAGE_RC(remove_per_button);
RK_DECLARE_IMAGE_RC(remove_per_button_hover);
RK_DECLARE_IMAGE_RC(remove_per_button_on);
RK_DECLARE_IMAGE_RC(duplicate_per_button);
RK_DECLARE_IMAGE_RC(duplicate_per_button_hover);
RK_DECLARE_IMAGE_RC(duplicate_per_button_on);
RK_DECLARE_IMAGE_RC(move_up_per_button);
RK_DECLARE_IMAGE_RC(move_up_per_button_hover);
RK_DECLARE_IMAGE_RC(move_up_per_button_on);
RK_DECLARE_IMAGE_RC(move_down_per_button);
RK_DECLARE_IMAGE_RC(move_down_per_button_hover);
RK_DECLARE_IMAGE_RC(move_down_per_button_on);

KitWidget::KitWidget(GeonkickWidget *parent, KitModel *model)
	: GeonkickWidget(parent)
        , kitModel{model}
        , addButton{nullptr}
        , removeButton{nullptr}
        , duplicateButton{nullptr}
        , moveupButton{nullptr}
        , movedownButton{nullptr}
        , instrumentsContainer{new RkContainer(this, Rk::Orientation::Vertical)}
        , levelersTimer{new RkTimer(this, 30)}
{
        setSize({parent->size().width() - 5, parent->size().height()}) ;

        RK_ACT_BIND(levelersTimer, timeout, RK_ACT_ARGS(), this, onUpdateLevelers());
        instrumentsContainer->setHiddenTakesPlace();

        RK_ACT_BIND(kitModel, modelUpdated, RK_ACT_ARGS(), this, updateView());
        RK_ACT_BIND(kitModel, instrumentAdded, RK_ACT_ARGS(PercussionModel *model),
                    this, addPercussion(model));
        RK_ACT_BIND(kitModel, instrumentRemoved, RK_ACT_ARGS(PercussionIndex index),
                    this, removePercussion(index));

        auto kitContainer = new RkContainer(this, Rk::Orientation::Vertical);
        kitContainer->setHiddenTakesPlace();
        kitContainer->setSize(size());

        auto topMenu = createTopMenu();

        instrumentsContainer->setHiddenTakesPlace();
        instrumentsContainer->setHeight(kitContainer->height() - topMenu->height());

        kitContainer->addSpace(8);
        kitContainer->addWidget(topMenu);
        kitContainer->addSpace(5);
        kitContainer->addContainer(instrumentsContainer);

        updateView();
        levelersTimer->start();

        show();
}

GeonkickWidget* KitWidget::createTopMenu()
{
        auto topMenu = new GeonkickWidget(this);
        topMenu->setSize({width(), 18});

        auto topContainer = new RkContainer(topMenu);

        addButton = new RkButton(topMenu);
        addButton->setType(RkButton::ButtonType::ButtonPush);
        addButton->setBackgroundColor(background());
        addButton->setImage(RK_RC_IMAGE(add_per_button),
                            RkButton::State::Unpressed);
        addButton->setImage(RK_RC_IMAGE(add_per_button_hover),
                            RkButton::State::UnpressedHover);
        addButton->setImage(RK_RC_IMAGE(add_per_button_hover),
                            RkButton::State::PressedHover);
        addButton->setImage(RK_RC_IMAGE(add_per_button_on),
                            RkButton::State::Pressed);
        RK_ACT_BIND(addButton, pressed, RK_ACT_ARGS(), kitModel, addNewPercussion());
        topContainer->addWidget(addButton);
        addButton->show();

        topContainer->addSpace(3);
        removeButton = new RkButton(topMenu);
        removeButton->setType(RkButton::ButtonType::ButtonPush);
        removeButton->setBackgroundColor(background());
        removeButton->setImage(RK_RC_IMAGE(remove_per_button),
                            RkButton::State::Unpressed);
        removeButton->setImage(RK_RC_IMAGE(remove_per_button_hover),
                            RkButton::State::UnpressedHover);
        removeButton->setImage(RK_RC_IMAGE(remove_per_button_hover),
                            RkButton::State::PressedHover);
        removeButton->setImage(RK_RC_IMAGE(remove_per_button_on),
                            RkButton::State::Pressed);
        //RK_ACT_BIND(removeButton, pressed, RK_ACT_ARGS(), kitModel, removeNewPercussion());
        topContainer->addWidget(removeButton);
        removeButton->show();

        topContainer->addSpace(3);
        duplicateButton = new RkButton(topMenu);
        duplicateButton->setType(RkButton::ButtonType::ButtonPush);
        duplicateButton->setBackgroundColor(background());
        duplicateButton->setImage(RK_RC_IMAGE(duplicate_per_button),
                            RkButton::State::Unpressed);
        duplicateButton->setImage(RK_RC_IMAGE(duplicate_per_button_hover),
                            RkButton::State::UnpressedHover);
        duplicateButton->setImage(RK_RC_IMAGE(duplicate_per_button_hover),
                            RkButton::State::PressedHover);
        duplicateButton->setImage(RK_RC_IMAGE(duplicate_per_button_on),
                            RkButton::State::Pressed);
        //RK_ACT_BIND(duplicateButton, pressed, RK_ACT_ARGS(), kitModel, duplicateNewPercussion());
        topContainer->addWidget(duplicateButton);
        duplicateButton->show();

        topContainer->addSpace(3);
        moveupButton = new RkButton(topMenu);
        moveupButton->setType(RkButton::ButtonType::ButtonPush);
        moveupButton->setBackgroundColor(background());
        moveupButton->setImage(RK_RC_IMAGE(move_up_per_button),
                            RkButton::State::Unpressed);
        moveupButton->setImage(RK_RC_IMAGE(move_up_per_button_hover),
                            RkButton::State::UnpressedHover);
        moveupButton->setImage(RK_RC_IMAGE(move_up_per_button_hover),
                            RkButton::State::PressedHover);
        moveupButton->setImage(RK_RC_IMAGE(move_up_per_button_on),
                            RkButton::State::Pressed);
        //        RK_ACT_BIND(moveupButton, pressed, RK_ACT_ARGS(), kitModel, moveSelectedPercussion(false));
        topContainer->addWidget(moveupButton);
        moveupButton->show();

        topContainer->addSpace(3);
        movedownButton = new RkButton(topMenu);
        movedownButton->setType(RkButton::ButtonType::ButtonPush);
        movedownButton->setBackgroundColor(background());
        movedownButton->setImage(RK_RC_IMAGE(move_down_per_button),
                            RkButton::State::Unpressed);
        movedownButton->setImage(RK_RC_IMAGE(move_down_per_button_hover),
                            RkButton::State::UnpressedHover);
        movedownButton->setImage(RK_RC_IMAGE(move_down_per_button_hover),
                            RkButton::State::PressedHover);
        movedownButton->setImage(RK_RC_IMAGE(move_down_per_button_on),
                            RkButton::State::Pressed);
        //        RK_ACT_BIND(movedownButton, toggled, RK_ACT_ARGS(bool b), kitModel, moveSelectedPercussion(true));
        topContainer->addWidget(movedownButton);
        movedownButton->show();

        // Midi channel
        topContainer->addSpace(205);
        auto label = new RkLabel(topMenu, "MIDI Ch.");
        label->setTextColor(textColor());
        label->setBackgroundColor(background());
        label->setSize({50, 20});
        label->show();
        topContainer->addWidget(label);

        // Midi key
        topContainer->addSpace(30);
        label = new RkLabel(topMenu, "Key");
        label->setTextColor(textColor());
        label->setBackgroundColor(background());
        label->setSize({30, 20});
        label->show();
        topContainer->addWidget(label);

        // Choke group
        topContainer->addSpace(63);
        label = new RkLabel(topMenu, "Choke");
        label->setTextColor(textColor());
        label->setBackgroundColor(background());
        label->setSize({50, 20});
        label->show();
        topContainer->addWidget(label);

        // Output channel
        topContainer->addSpace(185);
        label = new RkLabel(topMenu, "Output ch.");
        label->setTextColor(textColor());
        label->setBackgroundColor(background());
        label->setSize({54, 20});
        label->show();
        topContainer->addWidget(label);

        return topMenu;
}

void KitWidget::updateView()
{
        instrumentsContainer->clear();
        for (auto &instrumentView: instrumentViewList)
                delete instrumentView;
        instrumentViewList.clear();

        auto &models = kitModel->instrumentModels();
        for (const auto &m: models)
                addPercussion(m);
}

void KitWidget::addPercussion(PercussionModel *model)
{
        auto instrumentView = new KitPercussionView(this, model);
        instrumentsContainer->addWidget(instrumentView, Rk::Alignment::AlignTop);
        instrumentViewList.push_back(instrumentView);
        instrumentView->show();
}

void KitWidget::updatePercussion(PercussionIndex index, PercussionModel *model)
{
        auto instrumentView = dynamic_cast<KitPercussionView*>(instrumentsContainer->at(index));
        if (instrumentView)
                instrumentView->setModel(model);
}

void KitWidget::removePercussion(PercussionIndex index)
{
        size_t containerIndex = 0;
        for (auto it = instrumentViewList.begin(); it != instrumentViewList.end(); ++it) {
                if ((*it)->getModel()->index() == index) {
                        instrumentsContainer->removeAt(containerIndex);
                        delete *it;
                        instrumentViewList.erase(it);
                        instrumentsContainer->update();
                        break;
                }
                containerIndex++;
        }
}

void KitWidget::copyPercussion(int index)
{
        kitModel->copyPercussion(index);
}

KitModel* KitWidget::getModel() const
{
        return kitModel;
}

void KitWidget::onUpdateLevelers()
{
        for (const auto &per: instrumentViewList)
                per->updateLeveler();
}

