/**
 * File name: InstrumentView.cpp
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
RK_DECLARE_IMAGE_RC(kit_midi_on);
RK_DECLARE_IMAGE_RC(kit_midi_off);
RK_DECLARE_IMAGE_RC(kit_midi_hover);
RK_DECLARE_IMAGE_RC(note_off_unpressed);
RK_DECLARE_IMAGE_RC(note_off_hover);
RK_DECLARE_IMAGE_RC(note_off_pressed);
RK_DECLARE_IMAGE_RC(instr_key_up);
RK_DECLARE_IMAGE_RC(instr_key_up_hover);
RK_DECLARE_IMAGE_RC(instr_key_up_on);
RK_DECLARE_IMAGE_RC(instr_key_down);
RK_DECLARE_IMAGE_RC(instr_key_down_hover);
RK_DECLARE_IMAGE_RC(instr_key_down_on);

using namespace Geonkick;

PercussionLimiter::PercussionLimiter(GeonkickWidget *parent)
        : GeonkickSlider(parent)
        , levelerValue{0}
{
        setBackgroundColor({50, 50, 50});
}

void PercussionLimiter::setLeveler(int value)
{
        levelerValue = std::clamp(value, 0, 100);
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
                if (getOrientation() == GeonkickSlider::Orientation::Horizontal) {
                        painter.fillRect(RkRect(1, 2, value, height() - 4), color);
                } else {
                        painter.fillRect(RkRect(height() - 2 - value, 2,
                                                width() - 4, value), color);
                }
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
        , outputChannelSpinBox{nullptr}
        , keySpinBox{nullptr}
        , keyOctaveSpinBox{nullptr}
        , playButton{nullptr}
        , muteButton{nullptr}
        , soloButton{nullptr}
        , noteOffButton{nullptr}
        , chokeGroupSpinbox{nullptr}
        , instrumentLimiter{nullptr}
        , padding{8}
{
        setSize(parent->width(), 40);

        setBorderWidth(1);
        setBorderColor(38, 38, 38);

        createView();
        setModel(model);
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

        // Play button
        instrumentContainer->addSpace(padding + 5);
        playButton = new RkButton(this);
        playButton->setType(RkButton::ButtonType::ButtonPush);
        playButton->setImage(RK_RC_IMAGE(per_play), RkButton::State::Unpressed);
        playButton->setImage(RK_RC_IMAGE(per_play_hover), RkButton::State::UnpressedHover);
        playButton->setImage(RK_RC_IMAGE(per_play_on), RkButton::State::Pressed);
        playButton->show();
        instrumentContainer->addWidget(playButton);

        // Insturment name
        instrumentContainer->addSpace(3);
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
        midiChannelSpinBox->setSize(50, 30);
        midiChannelSpinBox->setTextColor({160, 160, 160});
        midiChannelSpinBox->setBackgroundColor({44, 44, 44});
        midiChannelSpinBox->label()->setTextColor({160, 160, 160});
        midiChannelSpinBox->upControl()->setImage(RK_RC_IMAGE(instr_key_up),
                                                  RkButton::State::Unpressed);
        midiChannelSpinBox->upControl()->setImage(RK_RC_IMAGE(instr_key_up_hover),
                                                  RkButton::State::UnpressedHover);
        midiChannelSpinBox->upControl()->setImage(RK_RC_IMAGE(instr_key_up_hover),
                                                  RkButton::State::PressedHover);
        midiChannelSpinBox->upControl()->setImage(RK_RC_IMAGE(instr_key_up_on),
                                                  RkButton::State::Pressed);
        midiChannelSpinBox->downControl()->setImage(RK_RC_IMAGE(instr_key_down),
                                                    RkButton::State::Unpressed);
        midiChannelSpinBox->downControl()->setImage(RK_RC_IMAGE(instr_key_down_hover),
                                                    RkButton::State::UnpressedHover);
        midiChannelSpinBox->downControl()->setImage(RK_RC_IMAGE(instr_key_down_hover),
                                                    RkButton::State::PressedHover);
        midiChannelSpinBox->downControl()->setImage(RK_RC_IMAGE(instr_key_down_on),
                                                    RkButton::State::Pressed);
        midiChannelSpinBox->setCustomControls(true);
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
        instrumentContainer->addSpace(10);

        // Midi key spinbox
        keySpinBox = new RkSpinBox(this);
        keySpinBox->setSize(38, 30);
        keySpinBox->setTextColor({160, 160, 160});
        keySpinBox->setBackgroundColor({44, 44, 44});
        keySpinBox->label()->setAlignment(Rk::Alignment::AlignRight);
        keySpinBox->label()->setTextColor({160, 160, 160});
        keySpinBox->upControl()->setImage(RK_RC_IMAGE(instr_key_up),
                                          RkButton::State::Unpressed);
        keySpinBox->upControl()->setImage(RK_RC_IMAGE(instr_key_up_hover),
                                          RkButton::State::UnpressedHover);
        keySpinBox->upControl()->setImage(RK_RC_IMAGE(instr_key_up_hover),
                                          RkButton::State::PressedHover);
        keySpinBox->upControl()->setImage(RK_RC_IMAGE(instr_key_up_on),
                                          RkButton::State::Pressed);
        keySpinBox->downControl()->setImage(RK_RC_IMAGE(instr_key_down),
                                            RkButton::State::Unpressed);
        keySpinBox->downControl()->setImage(RK_RC_IMAGE(instr_key_down_hover),
                                            RkButton::State::UnpressedHover);
        keySpinBox->downControl()->setImage(RK_RC_IMAGE(instr_key_down_hover),
                                            RkButton::State::PressedHover);
        keySpinBox->downControl()->setImage(RK_RC_IMAGE(instr_key_down_on),
                                            RkButton::State::Pressed);
        keySpinBox->setCustomControls(true);
        keySpinBox->setControlsPosition(RkSpinBox::ControlsPosition::PositionLeft);
        keySpinBox->show();
        RK_ACT_BIND(keySpinBox,
                    currentIndexChanged,
                    RK_ACT_ARGS(int index),
                    this,
                    setKey(index - 1));
        instrumentContainer->addWidget(keySpinBox);

        // Midi key octave spinbox
        keyOctaveSpinBox = new RkSpinBox(this);
        keyOctaveSpinBox->setSize(33, 30);
        keyOctaveSpinBox->setTextColor({220, 220, 220});
        keyOctaveSpinBox->setBackgroundColor({44, 44, 44});
        keyOctaveSpinBox->label()->setAlignment(Rk::Alignment::AlignLeft);
        keySpinBox->label()->setTextColor({160, 160, 160});
        keyOctaveSpinBox->upControl()->setImage(RK_RC_IMAGE(instr_key_up),
                                                RkButton::State::Unpressed);
        keyOctaveSpinBox->upControl()->setImage(RK_RC_IMAGE(instr_key_up_hover),
                                                RkButton::State::UnpressedHover);
        keyOctaveSpinBox->upControl()->setImage(RK_RC_IMAGE(instr_key_up_hover),
                                                RkButton::State::PressedHover);
        keyOctaveSpinBox->upControl()->setImage(RK_RC_IMAGE(instr_key_up_on),
                                                RkButton::State::Pressed);
        keyOctaveSpinBox->downControl()->setImage(RK_RC_IMAGE(instr_key_down),
                                                RkButton::State::Unpressed);
        keyOctaveSpinBox->downControl()->setImage(RK_RC_IMAGE(instr_key_down_hover),
                                                RkButton::State::UnpressedHover);
        keyOctaveSpinBox->downControl()->setImage(RK_RC_IMAGE(instr_key_down_hover),
                                                RkButton::State::PressedHover);
        keyOctaveSpinBox->downControl()->setImage(RK_RC_IMAGE(instr_key_down_on),
                                                RkButton::State::Pressed);
        keyOctaveSpinBox->setCustomControls(true);
        keyOctaveSpinBox->label()->setBackgroundColor({0, 111, 111});
        keyOctaveSpinBox->show();
        RK_ACT_BIND(keyOctaveSpinBox,
                    currentIndexChanged,
                    RK_ACT_ARGS(int index),
                    this,
                    setKeyOctave(index - 1));
        instrumentContainer->addWidget(keyOctaveSpinBox);


        // Note off button
        instrumentContainer->addSpace(10);
        noteOffButton = new RkButton(this);
        noteOffButton->setType(RkButton::ButtonType::ButtonCheckable);
        noteOffButton->setImage(RK_RC_IMAGE(note_off_unpressed),
                                RkButton::State::Unpressed);
        noteOffButton->setImage(RK_RC_IMAGE(note_off_hover),
                                RkButton::State::UnpressedHover);
        noteOffButton->setImage(RK_RC_IMAGE(note_off_hover),
                                RkButton::State::PressedHover);
        noteOffButton->setImage(RK_RC_IMAGE(note_off_pressed),
                                RkButton::State::Pressed);
        noteOffButton->show();
        instrumentContainer->addWidget(noteOffButton);

        createChokeGroupControl(instrumentContainer);

        // Limiter
        instrumentLimiter = new PercussionLimiter(this);
        instrumentLimiter->setSize(100, 10);
        instrumentContainer->addSpace(5);
        instrumentContainer->addWidget(instrumentLimiter);
        instrumentContainer->addSpace(15);

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
        instrumentContainer->addSpace(15);

        createOutputChannelControl(instrumentContainer);
}

void KitPercussionView::createOutputChannelControl(RkContainer *container)
{
        // Midi channel spinbox.
        outputChannelSpinBox = new RkSpinBox(this);
        outputChannelSpinBox->setSize(50, 30);
        outputChannelSpinBox->setTextColor({160, 160, 160});
        outputChannelSpinBox->setBackgroundColor({44, 44, 44});
        outputChannelSpinBox->label()->setTextColor({160, 160, 160});
        outputChannelSpinBox->upControl()->setImage(RK_RC_IMAGE(instr_key_up),
                                                  RkButton::State::Unpressed);
        outputChannelSpinBox->upControl()->setImage(RK_RC_IMAGE(instr_key_up_hover),
                                                  RkButton::State::UnpressedHover);
        outputChannelSpinBox->upControl()->setImage(RK_RC_IMAGE(instr_key_up_hover),
                                                  RkButton::State::PressedHover);
        outputChannelSpinBox->upControl()->setImage(RK_RC_IMAGE(instr_key_up_on),
                                                  RkButton::State::Pressed);
        outputChannelSpinBox->downControl()->setImage(RK_RC_IMAGE(instr_key_down),
                                                    RkButton::State::Unpressed);
        outputChannelSpinBox->downControl()->setImage(RK_RC_IMAGE(instr_key_down_hover),
                                                    RkButton::State::UnpressedHover);
        outputChannelSpinBox->downControl()->setImage(RK_RC_IMAGE(instr_key_down_hover),
                                                    RkButton::State::PressedHover);
        outputChannelSpinBox->downControl()->setImage(RK_RC_IMAGE(instr_key_down_on),
                                                    RkButton::State::Pressed);
        outputChannelSpinBox->setCustomControls(true);
        outputChannelSpinBox->show();
        RK_ACT_BIND(outputChannelSpinBox,
                    currentIndexChanged,
                    RK_ACT_ARGS(int index),
                    instrumentModel,
                    setChannel(index));
        RK_ACT_BIND(instrumentModel,
                    channelUpdated,
                    RK_ACT_ARGS(int index),
                    outputChannelSpinBox,
                    setCurrentIndex(index));
        container->addWidget(outputChannelSpinBox);
        container->addSpace(10);
}

void KitPercussionView::createChokeGroupControl(RkContainer *container)
{
        container->addSpace(10);
        chokeGroupSpinbox = new RkSpinBox(this);
        chokeGroupSpinbox->setSize(54, 30);
        chokeGroupSpinbox->setTextColor({160, 160, 160});
        chokeGroupSpinbox->setBackgroundColor({44, 44, 44});
        chokeGroupSpinbox->label()->setTextColor({160, 160, 160});
        chokeGroupSpinbox->upControl()->setImage(RK_RC_IMAGE(instr_key_up),
                                                  RkButton::State::Unpressed);
        chokeGroupSpinbox->upControl()->setImage(RK_RC_IMAGE(instr_key_up_hover),
                                                  RkButton::State::UnpressedHover);
        chokeGroupSpinbox->upControl()->setImage(RK_RC_IMAGE(instr_key_up_hover),
                                                  RkButton::State::PressedHover);
        chokeGroupSpinbox->upControl()->setImage(RK_RC_IMAGE(instr_key_up_on),
                                                  RkButton::State::Pressed);
        chokeGroupSpinbox->downControl()->setImage(RK_RC_IMAGE(instr_key_down),
                                                    RkButton::State::Unpressed);
        chokeGroupSpinbox->downControl()->setImage(RK_RC_IMAGE(instr_key_down_hover),
                                                    RkButton::State::UnpressedHover);
        chokeGroupSpinbox->downControl()->setImage(RK_RC_IMAGE(instr_key_down_hover),
                                                    RkButton::State::PressedHover);
        chokeGroupSpinbox->downControl()->setImage(RK_RC_IMAGE(instr_key_down_on),
                                                    RkButton::State::Pressed);
        chokeGroupSpinbox->setCustomControls(true);
        chokeGroupSpinbox->show();
        RK_ACT_BIND(chokeGroupSpinbox,
                    currentIndexChanged,
                    RK_ACT_ARGS(int index),
                    instrumentModel,
                    setChokeGroup(index));
        RK_ACT_BIND(instrumentModel,
                    chokeGroupUpdated,
                    RK_ACT_ARGS(int index),
                    chokeGroupSpinbox,
                    setCurrentIndex(index));
        container->addWidget(chokeGroupSpinbox);
        container->addSpace(10);
}

void KitPercussionView::updateView()
{
        auto backgorundColor = instrumentModel->isSelected() ? RkColor(55, 55, 55) : RkColor(50, 50, 50);
        setBackgroundColor(backgorundColor);

        nameLabel->setBackgroundColor(backgorundColor);
        nameLabel->setText(instrumentModel->name());

        waveformPreview->setData(instrumentModel->data());
        waveformPreview->setBackgroundColor(backgorundColor);

        instrumentLimiter->onSetValue(instrumentModel->limiter(), 55.0 * 100.0 / 75);

        muteButton->setPressed(instrumentModel->isMuted());
        soloButton->setPressed(instrumentModel->isSolo());
        noteOffButton->setPressed(instrumentModel->isNoteOffEnabled());

        // Midi channel
        auto nMidiChannels = instrumentModel->numberOfMidiChannels();
        midiChannelSpinBox->clear();
        midiChannelSpinBox->addItem("--");
        for (size_t i = 0; i < nMidiChannels; i++)
                midiChannelSpinBox->addItem(std::to_string(i + 1));
        midiChannelSpinBox->setCurrentIndex(instrumentModel->midiChannel() + 1);

        // Ouput channels
        auto nChannels = instrumentModel->numberOfChannels();
        outputChannelSpinBox->clear();
        for (size_t i = 0; i < nChannels; i++)
                outputChannelSpinBox->addItem(std::to_string(i + 1));
        outputChannelSpinBox->setCurrentIndex(instrumentModel->channel());

        // Chocke groups
        auto nChokeGroups = instrumentModel->numberOfChokeGroups();
        chokeGroupSpinbox->clear();
        for (size_t i = 0; i < nChokeGroups; i++)
                chokeGroupSpinbox->addItem( i > 0 ? std::to_string(i) : "-");
        chokeGroupSpinbox->setCurrentIndex(instrumentModel->getChokeGroup());

        // Midi key name
        keySpinBox->clear();
        keySpinBox->addItem("-");
        for (int semitone = 0; semitone < 12; semitone++)
                keySpinBox->addItem(std::string(semitoneToNote(semitone)));
        keySpinBox->setCurrentIndex(midiKeySemitone(instrumentModel->key()) + 1);

        // Midi key octave
        keyOctaveSpinBox->clear();
        keyOctaveSpinBox->addItem("-");
        for (int oct = 0; oct < 9; oct++)
                keyOctaveSpinBox->addItem(std::to_string(oct));
        keyOctaveSpinBox->setCurrentIndex(midiKeyOctave(instrumentModel->key()) + 1);

        update();
}

void KitPercussionView::setModel(PercussionModel *model)
{
        if (!model)
                return;

        instrumentModel = model;

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

void KitPercussionView::hoverEvent(RkHoverEvent *event)
{
        /*        if (!event->isHover())
                setBackgroundColor({50, 50, 50});
        else
                setBackgroundColor({55, 55, 55});
                update();*/
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

void KitPercussionView::setKey(int semitone)
{
        //        if ()
}

void KitPercussionView::setKeyOctave(int oct)
{
}
