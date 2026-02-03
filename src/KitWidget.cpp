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

KitWidget::KitWidget(GeonkickWidget *parent, KitModel *model)
	: GeonkickWidget(parent)
        , kitModel{model}
        , addButton{nullptr}
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

        addShortcut(Rk::Key::Key_Up);
        addShortcut(Rk::Key::Key_Down);
        addShortcut(Rk::Key::Key_Up, Rk::KeyModifiers::Control_Left);
        addShortcut(Rk::Key::Key_Up, Rk::KeyModifiers::Control_Right);
        addShortcut(Rk::Key::Key_Down, Rk::KeyModifiers::Control_Left);
        addShortcut(Rk::Key::Key_Down, Rk::KeyModifiers::Control_Right);

        auto kitContainer = new RkContainer(this, Rk::Orientation::Vertical);
        kitContainer->setHiddenTakesPlace();
        kitContainer->setSize(size());

        auto topContainer = new RkContainer(this);
        topContainer->setSpacing(5);
        instrumentsContainer->setHiddenTakesPlace();
        topContainer->setSize({width(), 25});

        addButton = new RkButton(this);
        addButton->setBackgroundColor(background());
        addButton->setCheckable(true);
        addButton->setSize(16, 16);
        addButton->setImage(RkImage(16, 16, RK_IMAGE_RC(add_per_button)));
        RK_ACT_BIND(addButton, toggled, RK_ACT_ARGS(bool b), kitModel, addNewPercussion());
        topContainer->addWidget(addButton);
        addButton->show();

        instrumentsContainer->setHeight(kitContainer->height() - topContainer->height());

        auto label = new RkLabel(this, "MIDI Ch.");
        label->setTextColor(textColor());
        label->setBackgroundColor(background());
        label->setSize({50, 20});
        label->show();
        topContainer->addWidget(label);
        label = new RkLabel(this, "Key");
        label->setTextColor(textColor());
        label->setBackgroundColor(background());
        label->setSize({30, 20});
        label->show();
        topContainer->addWidget(label);

        kitContainer->addSpace(5);
        kitContainer->addContainer(topContainer);
        kitContainer->addSpace(5);
        kitContainer->addContainer(instrumentsContainer);

        updateView();
        levelersTimer->start();

        show();
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

void KitWidget::keyPressEvent(RkKeyEvent *event)
{
        if (event->key() != Rk::Key::Key_Up && event->key() != Rk::Key::Key_Down)
                return;

        auto index = kitModel->selectedPercussion();
        if ((event->modifiers() & static_cast<int>(Rk::KeyModifiers::Control))) {
                kitModel->moveSelectedPercussion(event->key() == Rk::Key::Key_Down);
        } else if (event->key() == Rk::Key::Key_Up) {
                kitModel->selectPercussion(--index);
        } else if (event->key() == Rk::Key::Key_Down) {
                kitModel->selectPercussion(++index);
        }
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

