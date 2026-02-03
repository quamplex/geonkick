/**
 * File name: instrument_view.cpp
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
#include "InstrumentView.h"
#include "InstrumentModel.h"
#include "geonkick_slider.h"
#include "MidiKeyWidget.h"
#include "geonkick_button.h"

#include "RkEvent.h"
#include "RkPainter.h"
#include "RkLineEdit.h"
#include "RkLabel.h"
#include "RkButton.h"
#include "RkContainer.h"
#include "RkProgressBar.h"
#include "RkSpinBox.h"
#include "BufferView.h"

RK_DECLARE_IMAGE_RC(mute);
RK_DECLARE_IMAGE_RC(mute_hover);
RK_DECLARE_IMAGE_RC(mute_on);
RK_DECLARE_IMAGE_RC(solo);
RK_DECLARE_IMAGE_RC(solo_hover);
RK_DECLARE_IMAGE_RC(solo_on);
RK_DECLARE_IMAGE_RC(per_play);
RK_DECLARE_IMAGE_RC(per_play_hover);
RK_DECLARE_IMAGE_RC(per_play_on);
RK_DECLARE_IMAGE_RC(remove_per_button);
RK_DECLARE_IMAGE_RC(remove_per_button_hover);
RK_DECLARE_IMAGE_RC(remove_per_button_on);
RK_DECLARE_IMAGE_RC(copy_per_button);
RK_DECLARE_IMAGE_RC(copy_per_button_hover);
RK_DECLARE_IMAGE_RC(copy_per_button_on);
RK_DECLARE_IMAGE_RC(kit_midi_on);
RK_DECLARE_IMAGE_RC(kit_midi_off);
RK_DECLARE_IMAGE_RC(kit_midi_hover);
RK_DECLARE_IMAGE_RC(note_off_unpressed);
RK_DECLARE_IMAGE_RC(note_off_hover);
RK_DECLARE_IMAGE_RC(note_off_pressed);

PercussionLimiter::PercussionLimiter(GeonkickWidget *parent)
        : GeonkickSlider(parent)
        , levelerValue{0}
{
}

void PercussionLimiter::setLeveler(int value)
{
        levelerValue = value;
        if (value > 100)
                levelerValue = 100;
        else if (value < 0)
                levelerValue = 0;
        else
                levelerValue = value;
        update();
}

int PercussionLimiter::getLeveler() const
{
        return levelerValue;
}

void PercussionLimiter::paintWidget(RkPaintEvent *event)
{
        GeonkickSlider::paintWidget(event);
        RkPainter painter(this);
        double value = (static_cast<double>(levelerValue) / 100) * (width() - 2);
        RkColor color(40, 200, 40);
        if (levelerValue > 0) {
                if (getOrientation() == GeonkickSlider::Orientation::Horizontal)
                        painter.fillRect(RkRect(1, 2,
                                                value, height() - 4), color);
                else
                        painter.fillRect(RkRect(height() - 2 - value, 2,
                                                width() - 4, value), color);
        }
}

KitPercussionView::KitPercussionView(KitWidget *parent,
                                     PercussionModel *model)
        : GeonkickWidget(parent)
        , parentView{parent}
        , instrumentModel{model}
        , nameLabel{nullptr}
        , waveformPreview{nullptr}
        , editPercussion{nullptr}
        , midiChannelSpinBox{nullptr}
        , keyButton{nullptr}
        , copyButton{nullptr}
        , removeButton{nullptr}
        , playButton{nullptr}
        , muteButton{nullptr}
        , soloButton{nullptr}
        , noteOffButton{nullptr}
        , instrumentLimiter{nullptr}
        , padding{8}
{
        setSize(parent->width(), 40);
        createView();
        setModel(model);
        setBorderWidth(1);
        setBorderColor(22, 22, 22);
}

KitPercussionView::PercussionIndex KitPercussionView::index() const
{
        if (instrumentModel)
                return instrumentModel->index();
        return -1;
}

void KitPercussionView::createView()
{
        auto instrumentContainer = new RkContainer(this);
        instrumentContainer->setSize(width(), height() - 2 * padding);
        instrumentContainer->setY(padding);
        instrumentContainer->setHiddenTakesPlace();

        // Insturment name
        instrumentContainer->addSpace(padding);
        nameLabel = new RkLabel(this, instrumentModel->name());
        auto font = nameLabel->font();
        font.setWeight(RkFont::Weight::Bold);
        nameLabel->setFont(font);
        nameLabel->setTextColor({180, 180, 180});
        nameLabel->setSize(140, 20);
        nameLabel->setBackgroundColor(background());
        instrumentContainer->addWidget(nameLabel);

        // Waveform preview
        instrumentContainer->addSpace(10);
        waveformPreview = new BufferView(this, instrumentModel->data());
        waveformPreview->setSize(140, height() - 10);
        instrumentContainer->addWidget(waveformPreview);
        instrumentContainer->addSpace(20);

        // Midi channel spinbox.
        midiChannelSpinBox = new RkSpinBox(this);
        midiChannelSpinBox->setTextColor({250, 250, 250});
        midiChannelSpinBox->setBackgroundColor({60, 57, 57});
        midiChannelSpinBox->upControl()->setBackgroundColor({50, 47, 47});
        midiChannelSpinBox->upControl()->setTextColor({100, 100, 100});
        midiChannelSpinBox->downControl()->setBackgroundColor({50, 47, 47});
        midiChannelSpinBox->downControl()->setTextColor({100, 100, 100});
        midiChannelSpinBox->setSize(50, 20);
        midiChannelSpinBox->show();
        RK_ACT_BIND(midiChannelSpinBox,
                    currentIndexChanged,
                    RK_ACT_ARGS(int index),
                    instrumentModel,
                    setMidiChannel(index - 1));
        RK_ACT_BIND(instrumentModel,
                    midiChannelUpdated,
                    RK_ACT_ARGS(int index),
                    midiChannelSpinBox,
                    setCurrentIndex(index + 1));
        instrumentContainer->addWidget(midiChannelSpinBox);
        instrumentContainer->addSpace(5);

        // Midi key button.
        keyButton = new GeonkickButton(this);
        keyButton->setTextColor({250, 250, 250});
        keyButton->setType(RkButton::ButtonType::ButtonUncheckable);
        keyButton->setSize(30, 20);
        keyButton->setImage(RkImage(keyButton->size(), RK_IMAGE_RC(kit_midi_off)),
                            RkButton::State::Unpressed);
        keyButton->setImage(RkImage(keyButton->size(), RK_IMAGE_RC(kit_midi_on)),
                                 RkButton::State::Pressed);
        keyButton->setImage(RkImage(keyButton->size(), RK_IMAGE_RC(kit_midi_hover)),
                            RkButton::State::UnpressedHover);
        RK_ACT_BIND(keyButton, toggled, RK_ACT_ARGS(bool pressed), this, showMidiPopup());
        instrumentContainer->addWidget(keyButton);
        instrumentContainer->addSpace(5);

        // Note off button
        noteOffButton = new RkButton(this);
        noteOffButton->setType(RkButton::ButtonType::ButtonCheckable);
        noteOffButton->setSize(23, 16);
        noteOffButton->setImage(RkImage(noteOffButton->size(), RK_IMAGE_RC(note_off_unpressed)),
                                RkButton::State::Unpressed);
        noteOffButton->setImage(RkImage(noteOffButton->size(), RK_IMAGE_RC(note_off_hover)),
                                RkButton::State::UnpressedHover);
        noteOffButton->setImage(RkImage(noteOffButton->size(), RK_IMAGE_RC(note_off_pressed)),
                                RkButton::State::Pressed);
        noteOffButton->setImage(RkImage(noteOffButton->size(), RK_IMAGE_RC(note_off_hover)),
                                RkButton::State::PressedHover);
        noteOffButton->show();
        instrumentContainer->addWidget(noteOffButton);
        instrumentContainer->addSpace(5);

        // Remove button
        removeButton = new RkButton(this);
        removeButton->setType(RkButton::ButtonType::ButtonPush);
        removeButton->setSize(16, 16);
        removeButton->setImage(RkImage(removeButton->size(), RK_IMAGE_RC(remove_per_button)),
                               RkButton::State::Unpressed);
        removeButton->setImage(RkImage(removeButton->size(), RK_IMAGE_RC(remove_per_button_hover)),
                               RkButton::State::UnpressedHover);
        removeButton->setImage(RkImage(removeButton->size(), RK_IMAGE_RC(remove_per_button_on)),
                               RkButton::State::Pressed);
        removeButton->setImage(RkImage(removeButton->size(), RK_IMAGE_RC(remove_per_button_hover)),
                               RkButton::State::PressedHover);
        removeButton->show();
        instrumentContainer->addWidget(removeButton);
        instrumentContainer->addSpace(3);

        // Copy button
        copyButton = new RkButton(this);
        copyButton->setType(RkButton::ButtonType::ButtonPush);
        copyButton->setSize(16, 16);
        copyButton->setImage(RkImage(copyButton->size(), RK_IMAGE_RC(copy_per_button)),
                             RkButton::State::Unpressed);
        copyButton->setImage(RkImage(copyButton->size(), RK_IMAGE_RC(copy_per_button_hover)),
                             RkButton::State::UnpressedHover);
        copyButton->setImage(RkImage(copyButton->size(), RK_IMAGE_RC(copy_per_button_on)),
                             RkButton::State::Pressed);
        copyButton->setImage(RkImage(copyButton->size(), RK_IMAGE_RC(copy_per_button_hover)),
                             RkButton::State::PressedHover);
        copyButton->show();
        instrumentContainer->addWidget(copyButton);
        instrumentContainer->addSpace(5);

        // Limiter
        instrumentLimiter = new PercussionLimiter(this);
        instrumentLimiter->setSize(100, 10);
        auto limiterBox = new RkContainer(this, Rk::Orientation::Vertical);
        limiterBox->setHiddenTakesPlace();
        limiterBox->setSize({instrumentLimiter->width(), instrumentContainer->height()});
        limiterBox->addSpace((height() - instrumentLimiter->height()) / 2);
        limiterBox->addWidget(instrumentLimiter);
        instrumentContainer->addSpace(5);
        instrumentContainer->addContainer(limiterBox);
        instrumentContainer->addSpace(10);

        // Mute button
        muteButton = new RkButton(this);
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
        instrumentContainer->addWidget(muteButton);
        instrumentContainer->addSpace(3);

        // Solo button
        soloButton = new RkButton(this);
        soloButton->setType(RkButton::ButtonType::ButtonCheckable);
        soloButton->setSize(16, 16);
        soloButton->setImage(RkImage(soloButton->size(), RK_IMAGE_RC(solo)),
                             RkButton::State::Unpressed);
        soloButton->setImage(RkImage(soloButton->size(), RK_IMAGE_RC(solo_hover)),
                             RkButton::State::UnpressedHover);
        soloButton->setImage(RkImage(soloButton->size(), RK_IMAGE_RC(solo_on)),
                             RkButton::State::Pressed);
        soloButton->setImage(RkImage(soloButton->size(), RK_IMAGE_RC(solo_hover)),
                             RkButton::State::PressedHover);
        soloButton->show();
        instrumentContainer->addWidget(soloButton);
        instrumentContainer->addSpace(3);

        // Play button
        playButton = new RkButton(this);
        playButton->setType(RkButton::ButtonType::ButtonPush);
        playButton->setSize(16, 16);
        playButton->setImage(RkImage(playButton->size(), RK_IMAGE_RC(per_play)),
                         RkButton::State::Unpressed);
        playButton->setImage(RkImage(playButton->size(), RK_IMAGE_RC(per_play_hover)),
                         RkButton::State::UnpressedHover);
        playButton->setImage(RkImage(playButton->size(), RK_IMAGE_RC(per_play_on)),
                         RkButton::State::Pressed);
        playButton->show();
        instrumentContainer->addWidget(playButton);
}

void KitPercussionView::updateView()
{
        auto backgorundColor = instrumentModel->isSelected() ? RkColor(60, 60, 60) : RkColor(50, 50, 50);
        setBackgroundColor(backgorundColor);
        nameLabel->setBackgroundColor(backgorundColor);
        nameLabel->setText(instrumentModel->name());

        waveformPreview->setData(instrumentModel->data());
        waveformPreview->setBackgroundColor(backgorundColor);

        instrumentLimiter->onSetValue(instrumentModel->limiter(), 55.0 * 100.0 / 75);
        instrumentLimiter->setBackgroundColor(backgorundColor);
        muteButton->setPressed(instrumentModel->isMuted());
        soloButton->setPressed(instrumentModel->isSolo());
        noteOffButton->setPressed(instrumentModel->isNoteOffEnabled());
        size_t nMidiChannels = instrumentModel->numberOfMidiChannels();
        midiChannelSpinBox->clear();
        midiChannelSpinBox->addItem("Any");
        for (size_t i = 0; i < nMidiChannels; i++)
                midiChannelSpinBox->addItem(std::to_string(i + 1));
        midiChannelSpinBox->setCurrentIndex(instrumentModel->midiChannel() + 1);
        keyButton->setText(MidiKeyWidget::midiKeyToNote(instrumentModel->key()));
        update();
}

void KitPercussionView::setModel(PercussionModel *model)
{
        if (!model)
                return;

        instrumentModel = model;

        RK_ACT_BIND(removeButton, released, RK_ACT_ARGS(), this, remove());
        RK_ACT_BIND(copyButton, released, RK_ACT_ARGS(), instrumentModel, copy());
        RK_ACT_BIND(playButton, pressed, RK_ACT_ARGS(), instrumentModel, play());
        RK_ACT_BIND(noteOffButton, toggled, RK_ACT_ARGS(bool toggled), instrumentModel, enableNoteOff(toggled));
        RK_ACT_BIND(muteButton, toggled, RK_ACT_ARGS(bool toggled), instrumentModel, mute(toggled));
        RK_ACT_BIND(soloButton, toggled, RK_ACT_ARGS(bool toggled), instrumentModel, solo(toggled));
        RK_ACT_BIND(instrumentLimiter, valueUpdated, RK_ACT_ARGS(int val), instrumentModel, setLimiter(val));

        RK_ACT_BIND(instrumentModel, nameUpdated, RK_ACT_ARGS(std::string name), this, update());
        RK_ACT_BIND(instrumentModel, keyUpdated, RK_ACT_ARGS(KeyIndex index), this, updateView());
        RK_ACT_BIND(instrumentModel, channelUpdated, RK_ACT_ARGS(int val), this, update());
        RK_ACT_BIND(instrumentModel, limiterUpdated, RK_ACT_ARGS(int val),
                    instrumentLimiter, onSetValue(val, 55.0 * 100.0 / 75));
        RK_ACT_BIND(instrumentModel, muteUpdated, RK_ACT_ARGS(bool b), muteButton, setPressed(b));
        RK_ACT_BIND(instrumentModel, soloUpdated, RK_ACT_ARGS(bool b), soloButton, setPressed(b));
        RK_ACT_BIND(instrumentModel, selected, RK_ACT_ARGS(), this, updateView());
        RK_ACT_BIND(instrumentModel, modelUpdated, RK_ACT_ARGS(), this, updateView());
        RK_ACT_BIND(instrumentModel, midiChannelUpdated, RK_ACT_ARGS(int val), this, update());
        RK_ACT_BIND(instrumentModel, noteOffUpdated, RK_ACT_ARGS(bool b), this, update());
        RK_ACT_BIND(instrumentModel, waveformUpdated, RK_ACT_ARGS(), this, updateView());

        updateView();
}

PercussionModel* KitPercussionView::getModel()
{
        return instrumentModel;
}

void KitPercussionView::remove()
{
        if (getModel())
                getModel()->remove();
}

void KitPercussionView::mouseButtonPressEvent(RkMouseEvent *event)
{
        if (event->button() == RkMouseEvent::ButtonType::Left) {
                instrumentModel->select();
                updateView();
        }

        if (event->button() != RkMouseEvent::ButtonType::Left
            && event->button() != RkMouseEvent::ButtonType::WheelUp
            && event->button() != RkMouseEvent::ButtonType::WheelDown)
                return;

        updatePercussionName();
        setFocus(true);
}

void KitPercussionView::mouseDoubleClickEvent(RkMouseEvent *event)
{
        /*        if (event->button() == RkMouseEvent::ButtonType::WheelUp
            || event->button() == RkMouseEvent::ButtonType::WheelDown) {
                mouseButtonPressEvent(event);
                return;
        }

        if (event->button() == RkMouseEvent::ButtonType::Left && event->x() < nameWidth) {
                if (editPercussion == nullptr) {
                        editPercussion = new RkLineEdit(this);
                        editPercussion->setSize({nameWidth, height()});
                        RK_ACT_BIND(editPercussion, editingFinished, RK_ACT_ARGS(),
                                    this, updatePercussionName());
                }
                editPercussion->setText(instrumentModel->name());
                editPercussion->moveCursorToFront();
                editPercussion->show();
                editPercussion->setFocus();
                }*/
}

void KitPercussionView::updatePercussionName()
{
        if (editPercussion) {
		auto name = editPercussion->text();
		if (!name.empty()) {
			instrumentModel->setName(name);
			editPercussion->close();
                        editPercussion = nullptr;
		}
	}
}

void KitPercussionView::updateLeveler()
{
        if (instrumentModel->leveler() > instrumentLimiter->getLeveler())
                instrumentLimiter->setLeveler(instrumentModel->leveler());
        else if (instrumentLimiter->getLeveler() > 0)
                instrumentLimiter->setLeveler(instrumentLimiter->getLeveler() - 2);
}

void KitPercussionView::showMidiPopup()
{
        auto midiPopup = new MidiKeyWidget(dynamic_cast<GeonkickWidget*>(getTopWidget()),
                                           instrumentModel);
        midiPopup->setPosition(keyButton->x() - midiPopup->width() - 5,
                               getTopWidget()->height() - 2 * midiPopup->height()
                               + height() * (index() - 3));
        RK_ACT_BIND(midiPopup,
                    isAboutToClose,
                    RK_ACT_ARGS(),
                    keyButton,
                    setPressed(false));
        midiPopup->show();
}
