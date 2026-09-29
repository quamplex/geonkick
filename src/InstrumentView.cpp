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
#include "Sidebar.h"
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
RK_DECLARE_IMAGE_RC(instr_key_up);
RK_DECLARE_IMAGE_RC(instr_key_up_hover);
RK_DECLARE_IMAGE_RC(instr_key_up_on);
RK_DECLARE_IMAGE_RC(instr_key_down);
RK_DECLARE_IMAGE_RC(instr_key_down_hover);
RK_DECLARE_IMAGE_RC(instr_key_down_on);

using namespace Geonkick;

namespace {

class InstrumentWaveformPreview : public BufferView
{
public:
        InstrumentWaveformPreview(GeonkickWidget *parent, PercussionModel *model)
                : BufferView(parent, model->data())
                , instrumentModel{model}
        {
                setGraphColor({76, 170, 92, 210});
        }

protected:
        void mouseButtonPressEvent(RkMouseEvent *event) override
        {
                if (event->button() == RkMouseEvent::ButtonType::Left)
                        instrumentModel->play();
                event->setAccepted(false);
        }

        void mouseDoubleClickEvent(RkMouseEvent *event) override
        {
                if (event->button() == RkMouseEvent::ButtonType::Left)
                        instrumentModel->play();
                event->setAccepted(false);
        }

private:
        PercussionModel *instrumentModel;
};

class InstrumentNameLabel : public RkLabel
{
public:
        InstrumentNameLabel(GeonkickWidget *parent, PercussionModel *model)
                : RkLabel(parent, model->name())
                , instrumentModel{model}
                , edit{nullptr}
                , finishingEdit{false}
        {
                setSize(125, 20);

                edit = new RkLineEdit(this);
                edit->setSize(size());
                edit->setPosition({0, 0});
                edit->hide();

                RK_ACT_BIND(edit, editingFinished, RK_ACT_ARGS(),
                            this, finishEditing());
                RK_ACT_BIND(edit,
                            escapePressed,
                            RK_ACT_ARGS(),
                            this,
                            cancelEditing());
                RK_ACT_BIND(instrumentModel,
                            nameUpdated,
                            RK_ACT_ARGS(std::string name),
                            this,
                            setText(name));
                RK_ACT_BIND(this,
                            doubleClicked,
                            RK_ACT_ARGS(),
                            this,
                            beginEditing());
        }

        RK_DECL_ACT(doubleClicked,
                    doubleClicked(),
                    RK_ARG_TYPE(),
                    RK_ARG_VAL());

protected:
        void mouseButtonPressEvent(RkMouseEvent *event) override
        {
                event->setAccepted(false);
        }

        void mouseDoubleClickEvent(RkMouseEvent *event) override
        {
                if (event->button() == RkMouseEvent::ButtonType::Left) {
                        action doubleClicked();
                        event->setAccepted();
                } else {
                        event->setAccepted(false);
                }
        }

        void hoverEvent(RkHoverEvent *event) override
        {
                setTextColor(event->isHover() ? RkColor(240, 240, 240)
                                              : RkColor(180, 180, 180));
                update();
        }

private:
        void beginEditing()
        {
                edit->setBackgroundColor({44, 44, 44});
                edit->setTextColor(textColor());
                edit->setCursorColor(textColor());
                edit->setText(instrumentModel->name());
                edit->moveCursorToEnd();
                edit->show();
                edit->setFocus();
        }

        void finishEditing()
        {
                if (finishingEdit)
                        return;

                finishingEdit = true;
                const auto name = edit->text();
                if (name.empty() || !instrumentModel->setName(name))
                        edit->setText(instrumentModel->name());
                edit->hide();
                finishingEdit = false;
        }

        void cancelEditing()
        {
                finishingEdit = true;
                edit->setText(instrumentModel->name());
                edit->hide();
                finishingEdit = false;
        }

        PercussionModel *instrumentModel;
        RkLineEdit *edit;
        bool finishingEdit;
};

} // namespace

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
        RK_UNUSED(event);
        RkPainter painter(this);
        painter.fillRect(rect(), RkColor(37, 37, 37));
        painter.setPen(RkPen(RkColor(32, 32, 32)));
        painter.drawRect({0, 0, width() - 1, height() - 1});

        if (getOrientation() == GeonkickSlider::Orientation::Horizontal) {
                const auto limiterWidth =
                        static_cast<double>(getValue()) / 100 * (width() - 2);
                const auto levelerWidth =
                        static_cast<double>(levelerValue) / 100 * (width() - 2);
                painter.fillRect(RkRect(1, 1, limiterWidth, height() - 2),
                                 RkColor(78, 82, 84));
                painter.fillRect(RkRect(1, 1, levelerWidth, height() - 2),
                                 RkColor(112, 151, 105));
        } else {
                const auto limiterHeight =
                        static_cast<double>(getValue()) / 100 * (height() - 2);
                const auto levelerHeight =
                        static_cast<double>(levelerValue) / 100 * (height() - 2);
                painter.fillRect(RkRect(1, height() - 1 - limiterHeight,
                                        width() - 2, limiterHeight),
                                 RkColor(78, 82, 84));
                painter.fillRect(RkRect(1, height() - 1 - levelerHeight,
                                        width() - 2, levelerHeight),
                                 RkColor(112, 151, 105));
        }
}

KitPercussionView::KitPercussionView(KitWidget *parent,
                                     PercussionModel *model)
        : GeonkickWidget(parent)
        , parentView{parent}
        , instrumentModel{model}
        , nameLabel{nullptr}
        , waveformPreview{nullptr}
        , midiChannelSpinBox{nullptr}
        , outputChannelSpinBox{nullptr}
        , keySpinBox{nullptr}
        , keyOctaveSpinBox{nullptr}
        , playButton{nullptr}
        , muteButton{nullptr}
        , soloButton{nullptr}
        , playbackModeButton{nullptr}
        , chokeGroupSpinbox{nullptr}
        , instrumentLimiter{nullptr}
        , padding{8}
        , updatingControls{false}
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
        instrumentContainer->addSpace(8);
        nameLabel = new InstrumentNameLabel(this, instrumentModel);
        auto font = nameLabel->font();
        font.setWeight(RkFont::Weight::Bold);
        nameLabel->setFont(font);
        nameLabel->setTextColor({180, 180, 180});
        nameLabel->setBackgroundColor(background());
        instrumentContainer->addWidget(nameLabel);

        // Waveform preview
        instrumentContainer->addSpace(10);
        waveformPreview = new InstrumentWaveformPreview(this, instrumentModel);
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
        keySpinBox->setSize(50, 30);
        keySpinBox->setTextColor({160, 160, 160});
        keySpinBox->setBackgroundColor({44, 44, 44});
        keySpinBox->label()->setAlignment(Rk::Alignment::AlignCenter);
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
        keySpinBox->show();
        RK_ACT_BIND(keySpinBox,
                    currentIndexChanged,
                    RK_ACT_ARGS(int index),
                    this,
                    setKey(index - 1));
        RK_ACT_BIND(keySpinBox,
                    valueAreaClicked,
                    RK_ACT_ARGS(),
                    this,
                    showMidiPopup(keySpinBox));
        instrumentContainer->addWidget(keySpinBox);

        // Midi key octave spinbox
        instrumentContainer->addSpace(5);
        keyOctaveSpinBox = new RkSpinBox(this);
        keyOctaveSpinBox->setSize(50, 30);
        keyOctaveSpinBox->setTextColor({160, 160, 160});
        keyOctaveSpinBox->setBackgroundColor({44, 44, 44});
        keyOctaveSpinBox->label()->setAlignment(Rk::Alignment::AlignCenter);
        keyOctaveSpinBox->label()->setTextColor({160, 160, 160});
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
        keyOctaveSpinBox->show();
        RK_ACT_BIND(keyOctaveSpinBox,
                    currentIndexChanged,
                    RK_ACT_ARGS(int index),
                    this,
                    setKeyOctave(index - 1));
        RK_ACT_BIND(keyOctaveSpinBox,
                    valueAreaClicked,
                    RK_ACT_ARGS(),
                    this,
                    showMidiPopup(keyOctaveSpinBox));
        instrumentContainer->addWidget(keyOctaveSpinBox);


        // Playback mode button
        instrumentContainer->addSpace(10);
        playbackModeButton = new RkButton(this);
        playbackModeButton->setType(RkButton::ButtonType::ButtonPush);
        playbackModeButton->setSize(56, 20);
        playbackModeButton->setBackgroundColor({48, 62, 62});
        playbackModeButton->setTextColor({180, 180, 180});
        playbackModeButton->show();
        instrumentContainer->addWidget(playbackModeButton);

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
        updatingControls = true;
        auto backgorundColor = instrumentModel->isSelected() ? RkColor(55, 55, 55) : RkColor(50, 50, 50);
        setBackgroundColor(backgorundColor);

        nameLabel->setBackgroundColor(backgorundColor);
        nameLabel->setText(instrumentModel->name());

        updateWaveformPreview();
        waveformPreview->setBackgroundColor(backgorundColor);

        instrumentLimiter->onSetValue(instrumentModel->limiter(), 55.0 * 100.0 / 75);

        muteButton->setPressed(instrumentModel->isMuted());
        soloButton->setPressed(instrumentModel->isSolo());
        updatePlaymodeButton();

        // Midi channel
        auto nMidiChannels = instrumentModel->numberOfMidiChannels();
        midiChannelSpinBox->clear();
        midiChannelSpinBox->addItem("Any");
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

        // Midi key octave
        keyOctaveSpinBox->clear();
        keyOctaveSpinBox->addItem("-");
        for (int oct = 0; oct < 9; oct++)
                keyOctaveSpinBox->addItem(std::to_string(oct));

        const auto key = instrumentModel->key();
        if (key == GeonkickTypes::geonkickAnyKey) {
                keySpinBox->setCurrentIndex(0);
                keyOctaveSpinBox->setCurrentIndex(0);
        } else {
                keySpinBox->setCurrentIndex(midiKeySemitone(key) + 1);
                keyOctaveSpinBox->setCurrentIndex(midiKeyOctave(key) + 1);
        }

        updatingControls = false;
        update();
}

void KitPercussionView::setModel(PercussionModel *model)
{
        if (!model)
                return;

        instrumentModel = model;

        RK_ACT_BIND(playButton, pressed, RK_ACT_ARGS(), instrumentModel, play());
        RK_ACT_BIND(playbackModeButton,
                    pressed,
                    RK_ACT_ARGS(),
                    this,
                    onPlaybackModePressed());
        RK_ACT_BIND(muteButton, toggled, RK_ACT_ARGS(bool toggled), instrumentModel, mute(toggled));
        RK_ACT_BIND(soloButton, toggled, RK_ACT_ARGS(bool toggled), instrumentModel, solo(toggled));
        RK_ACT_BIND(instrumentLimiter, valueUpdated, RK_ACT_ARGS(int val), instrumentModel, setLimiter(val));

        RK_ACT_BIND(instrumentModel, keyUpdated, RK_ACT_ARGS(KeyIndex index), this, updateView());
        RK_ACT_BIND(instrumentModel, channelUpdated, RK_ACT_ARGS(int val), this, update());
        RK_ACT_BIND(instrumentModel, limiterUpdated, RK_ACT_ARGS(int val),
                    instrumentLimiter, onSetValue(val, 55.0 * 100.0 / 75));
        RK_ACT_BIND(instrumentModel, muteUpdated, RK_ACT_ARGS(bool b), muteButton, setPressed(b));
        RK_ACT_BIND(instrumentModel, soloUpdated, RK_ACT_ARGS(bool b), soloButton, setPressed(b));
        RK_ACT_BIND(instrumentModel, selected, RK_ACT_ARGS(), this, updateView());
        RK_ACT_BIND(instrumentModel, modelUpdated, RK_ACT_ARGS(), this, updateView());
        RK_ACT_BIND(instrumentModel, midiChannelUpdated, RK_ACT_ARGS(int val), this, update());
        RK_ACT_BIND(instrumentModel,
                    playbackModeUpdated,
                    RK_ACT_ARGS(DspProxy::PlaybackMode mode),
                    this,
                    updateView());
        RK_ACT_BIND(instrumentModel,
                    waveformUpdated,
                    RK_ACT_ARGS(),
                    this,
                    updateWaveformPreview());

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

void KitPercussionView::showMidiPopup(RkWidget *anchor)
{
        auto topWidget = dynamic_cast<GeonkickWidget*>(getTopWidget());
        if (!topWidget || !anchor)
                return;

        auto popup = new MidiKeyWidget(topWidget, instrumentModel);
        int usableWidth = topWidget->width();
        for (auto child : topWidget->children()) {
                auto sidebar = dynamic_cast<Sidebar*>(child);
                if (sidebar && sidebar->isVisible())
                        usableWidth = std::min(usableWidth, sidebar->x());
        }

        const auto anchorPosition = topWidget->mapToLocal(anchor->mapToGlobal({0, 0}));
        int x = anchorPosition.x();
        int y = anchorPosition.y() + anchor->height();
        if (x + popup->width() > usableWidth)
                x = usableWidth - popup->width() - 8;
        if (y + popup->height() > topWidget->height())
                y = anchorPosition.y() - popup->height();

        x = std::clamp(x, 0, std::max(0, usableWidth - popup->width() - 8));
        y = std::clamp(y, 0, std::max(0, topWidget->height() - popup->height()));
        popup->setPosition(x, y);
        popup->show();
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

        setFocus(true);
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
        if (updatingControls)
                return;
        if (semitone < 0) {
                instrumentModel->setKey(GeonkickTypes::geonkickAnyKey);
                return;
        }

        const auto currentKey = instrumentModel->key();
        const auto octave = (currentKey == GeonkickTypes::geonkickAnyKey)
                ? 4 : midiKeyOctave(currentKey);
        const auto key = (octave + 1) * 12 + semitone;
        if (semitone > 11 || key < 21 || key > 108) {
                updateView();
                return;
        }

        instrumentModel->setKey(static_cast<GeonkickTypes::MidiKey>(key));
}

void KitPercussionView::setKeyOctave(int oct)
{
        if (updatingControls)
                return;

        if (oct < 0) {
                instrumentModel->setKey(GeonkickTypes::geonkickAnyKey);
                return;
        }

        const auto currentKey = instrumentModel->key();
        const auto semitone = currentKey == GeonkickTypes::geonkickAnyKey
                ? 0 : midiKeySemitone(currentKey);
        const auto key = (oct + 1) * 12 + semitone;
        if (oct > 8 || key < 21 || key > 108) {
                updateView();
                return;
        }

        instrumentModel->setKey(static_cast<GeonkickTypes::MidiKey>(key));
}

void KitPercussionView::updatePlaymodeButton()
{
        switch (instrumentModel->playbackMode()) {
        case DspProxy::PlaybackMode::FullLength:
                playbackModeButton->setText("FULL");
                break;
        case DspProxy::PlaybackMode::NoteOff:
                playbackModeButton->setText("NOFF");
                break;
        case DspProxy::PlaybackMode::Cut:
                playbackModeButton->setText("CUT");
                break;
        }
}

void KitPercussionView::onPlaybackModePressed()
{
        const auto mode = instrumentModel->playbackMode();
        const auto nextMode = (static_cast<int>(mode) + 1) % 3;
        instrumentModel->setPlaybackMode(static_cast<DspProxy::PlaybackMode>(nextMode));
        updatePlaymodeButton();
}

void KitPercussionView::updateWaveformPreview()
{
        waveformPreview->setData(instrumentModel->data());
}
