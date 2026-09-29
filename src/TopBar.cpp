/**
 * File name: TopBar.cpp
 * Project: Geonkick (A percussive synthesizer)
 *
 * Copyright (C) 2018 Iurie Nistor
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

#include "TopBar.h"
#include "geonkick_button.h"
#include "preset_browser_model.h"
#include "preset_browser_view.h"
#include "ViewState.h"
#include "GeonkickModel.h"
#include "kit_model.h"
#include "InstrumentModel.h"
#include "MidiKeyWidget.h"
#include "PresetNavigator.h"
#include "SettingsWidget.h"

#include "RkLabel.h"
#include "RkButton.h"
#include "RkContainer.h"
#include "RkSpinBox.h"
#include "RkLineEdit.h"

RK_DECLARE_IMAGE_RC(separator);
RK_DECLARE_IMAGE_RC(logo);
RK_DECLARE_IMAGE_RC(play);
RK_DECLARE_IMAGE_RC(play_pressed);
RK_DECLARE_IMAGE_RC(play_hover);
RK_DECLARE_IMAGE_RC(tune_checkbox_on);
RK_DECLARE_IMAGE_RC(tune_checkbox_off);
RK_DECLARE_IMAGE_RC(tune_checkbox_hover);
RK_DECLARE_IMAGE_RC(topbar_synth_tab);
RK_DECLARE_IMAGE_RC(topbar_synth_tab_hover);
RK_DECLARE_IMAGE_RC(topbar_synth_tab_on);
RK_DECLARE_IMAGE_RC(topmenu_midi_off);
RK_DECLARE_IMAGE_RC(topmenu_midi_active);
RK_DECLARE_IMAGE_RC(topmenu_midi_hover);
#ifndef GEONKICK_SINGLE
RK_DECLARE_IMAGE_RC(topmenu_kit_active);
RK_DECLARE_IMAGE_RC(topmenu_kit_hover);
RK_DECLARE_IMAGE_RC(topmenu_kit_off);
#endif // GEONKICK_SINGLE
RK_DECLARE_IMAGE_RC(topmenu_settings_active);
RK_DECLARE_IMAGE_RC(topmenu_settings_hover);
RK_DECLARE_IMAGE_RC(topmenu_settings_off);
RK_DECLARE_IMAGE_RC(control_arrow_up);
RK_DECLARE_IMAGE_RC(control_arrow_up_hover);
RK_DECLARE_IMAGE_RC(control_arrow_up_pressed);
RK_DECLARE_IMAGE_RC(control_arrow_down);
RK_DECLARE_IMAGE_RC(control_arrow_down_hover);
RK_DECLARE_IMAGE_RC(control_arrow_down_pressed);

namespace {
void configureChannelSpinBox(RkSpinBox *spinBox)
{
        spinBox->setCustomControls();
        spinBox->upControl()->setImage(RK_RC_IMAGE(control_arrow_up),
                                       RkButton::State::Unpressed);
        spinBox->upControl()->setImage(RK_RC_IMAGE(control_arrow_up_hover),
                                       RkButton::State::UnpressedHover);
        spinBox->upControl()->setImage(RK_RC_IMAGE(control_arrow_up_hover),
                                       RkButton::State::PressedHover);
        spinBox->upControl()->setImage(RK_RC_IMAGE(control_arrow_up_pressed),
                                       RkButton::State::Pressed);
        spinBox->downControl()->setImage(RK_RC_IMAGE(control_arrow_down),
                                         RkButton::State::Unpressed);
        spinBox->downControl()->setImage(RK_RC_IMAGE(control_arrow_down_hover),
                                         RkButton::State::UnpressedHover);
        spinBox->downControl()->setImage(RK_RC_IMAGE(control_arrow_down_hover),
                                         RkButton::State::PressedHover);
        spinBox->downControl()->setImage(RK_RC_IMAGE(control_arrow_down_pressed),
        RkButton::State::Pressed);
}
}

TopBar::TopBar(GeonkickWidget *parent, GeonkickModel *model)
        : GeonkickWidget(parent)
        , geonkickModel{model}
        , kitModel{geonkickModel->getKitModel()}
        , presetNavigator{nullptr}
        , instrumentName {nullptr}
        , synthButton{nullptr}
        , midiKeyButton{nullptr}
        , midiChannelSpinBox{nullptr}
        , outputChannelSpinBox{nullptr}
        , playbackModeButton{nullptr}
#ifndef GEONKICK_SINGLE
        , kitButton{nullptr}
#endif // GEONKICK_SINGLE
        , presetsButton{nullptr}
        , samplesButton{nullptr}
        , settingsButton{nullptr}
{
        setName("TopBar");
        setFixedSize({940 - 10, 30});
        auto mainLayout = new RkContainer(this);
        mainLayout->setSize(size());

        auto logo = new RkLabel(this);
        logo->setBackgroundColor(background());
        RkImage image(22, 22, RK_IMAGE_RC(logo));
        image.grayscaleImage();
        logo->setSize(image.width(), image.height());
        logo->setImage(image);
        logo->show();
        mainLayout->addWidget(logo);

        // Setting button
        addSeparator(mainLayout);
        settingsButton = new GeonkickButton(this);
        settingsButton->setType(RkButton::ButtonType::ButtonUncheckable);
        settingsButton->setFixedSize(16, 16);
        settingsButton->setImage(RkImage(settingsButton->size(),
                                         RK_IMAGE_RC(topmenu_settings_off)),
                                 RkButton::State::Unpressed);
        settingsButton->setImage(RkImage(settingsButton->size(),
                                         RK_IMAGE_RC(topmenu_settings_active)),
                                 RkButton::State::Pressed);
        settingsButton->setImage(RkImage(settingsButton->size(),
                                         RK_IMAGE_RC(topmenu_settings_hover)),
                                 RkButton::State::UnpressedHover);
        settingsButton->show();
        RK_ACT_BIND(settingsButton, pressed, RK_ACT_ARGS(), this, showSettings());
        mainLayout->addWidget(settingsButton);

        addSeparator(mainLayout);
	auto playButton = new RkButton(this);
        playButton->setType(RkButton::ButtonType::ButtonPush);
        playButton->setSize(21, 18);
        playButton->setImage(RkImage(playButton->size(), RK_IMAGE_RC(play)),
                             RkButton::State::Unpressed);
        playButton->setImage(RkImage(playButton->size(), RK_IMAGE_RC(play_hover)),
                             RkButton::State::UnpressedHover);
        playButton->setImage(RkImage(playButton->size(), RK_IMAGE_RC(play_pressed)),
                             RkButton::State::Pressed);
        RK_ACT_BIND(playButton, pressed, RK_ACT_ARGS(), geonkickModel->getDspProxy(), playKick());
	playButton->show();
        mainLayout->addWidget(playButton);

        // Main menu
        createMainMenu(mainLayout);

        // Preset Navigator
        addSeparator(mainLayout);
        presetNavigator = new PresetNavigator(this, geonkickModel->getPresetsModel());
        mainLayout->addWidget(presetNavigator);

        // Instrument name
        addSeparator(mainLayout);
        mainLayout->addWidget(createInstrumentNameLabel());
        addSeparator(mainLayout, 4);

        auto labelFont = font();
        labelFont.setSize(10);

        // MIDI key
        mainLayout->addSpace(3);
        auto midiKeyLabel = new RkLabel(this, "Key:");
        midiKeyLabel->setTextColor({150, 150, 150});
        midiKeyLabel->setBackgroundColor(background());
        midiKeyLabel->setFont(labelFont);
        midiKeyLabel->setAlignment(Rk::Alignment::AlignRight);
        midiKeyLabel->setSize(22, 20);
        midiKeyLabel->show();
        mainLayout->addWidget(midiKeyLabel);
        mainLayout->addSpace(4);
        midiKeyButton = new GeonkickButton(this);
        midiKeyButton->setTextColor({200, 200, 200});
        midiKeyButton->setType(RkButton::ButtonType::ButtonUncheckable);
        midiKeyButton->setSize(36, 20);
        midiKeyButton->setImage(RkImage(midiKeyButton->size(), RK_IMAGE_RC(topmenu_midi_off)),
                                RkButton::State::Unpressed);
        midiKeyButton->setImage(RkImage(midiKeyButton->size(), RK_IMAGE_RC(topmenu_midi_active)),
                                RkButton::State::Pressed);
        midiKeyButton->setImage(RkImage(midiKeyButton->size(), RK_IMAGE_RC(topmenu_midi_hover)),
                                RkButton::State::UnpressedHover);
        RK_ACT_BIND(midiKeyButton, toggled,
                    RK_ACT_ARGS(bool pressed),
                    this,
                    showMidiPopup());
        mainLayout->addWidget(midiKeyButton);

        // Output channel
        auto outputChannelLabel = new RkLabel(this, "Out:");
        outputChannelLabel->setTextColor({150, 150, 150});
        outputChannelLabel->setBackgroundColor(background());
        outputChannelLabel->setFont(labelFont);
        outputChannelLabel->setAlignment(Rk::Alignment::AlignRight);
        outputChannelLabel->setSize(30, 20);
        outputChannelLabel->show();
        mainLayout->addWidget(outputChannelLabel);
        mainLayout->addSpace(4);
        outputChannelSpinBox = new RkSpinBox(this);
        outputChannelSpinBox->setBackgroundColor({44, 44, 44});
        outputChannelSpinBox->setTextColor({180, 180, 180});
        outputChannelSpinBox->upControl()->setBackgroundColor({50, 47, 47});
        outputChannelSpinBox->upControl()->setTextColor({100, 100, 100});
        outputChannelSpinBox->downControl()->setBackgroundColor({50, 47, 47});
        outputChannelSpinBox->downControl()->setTextColor({100, 100, 100});
        outputChannelSpinBox->setSize(54, 23);
        configureChannelSpinBox(outputChannelSpinBox);
        outputChannelSpinBox->show();
        RK_ACT_BINDL(outputChannelSpinBox,
                     currentIndexChanged,
                     RK_ARG_ARGS(int index),
                     [=, this](int index) {
                             kitModel->currentPercussion()->setChannel(index);
                     });
        mainLayout->addWidget(outputChannelSpinBox);

        // MIDI channel
        auto midiChannelLabel = new RkLabel(this, "MIDI:");
        midiChannelLabel->setTextColor({150, 150, 150});
        midiChannelLabel->setBackgroundColor(background());
        midiChannelLabel->setFont(labelFont);
        midiChannelLabel->setAlignment(Rk::Alignment::AlignRight);
        midiChannelLabel->setSize(34, 20);
        midiChannelLabel->show();
        mainLayout->addWidget(midiChannelLabel);
        mainLayout->addSpace(4);
        midiChannelSpinBox = new RkSpinBox(this);
        midiChannelSpinBox->setBackgroundColor({44, 44, 44});
        midiChannelSpinBox->setTextColor({180, 180, 180});
        midiChannelSpinBox->upControl()->setBackgroundColor({50, 47, 47});
        midiChannelSpinBox->upControl()->setTextColor({100, 100, 100});
        midiChannelSpinBox->downControl()->setBackgroundColor({50, 47, 47});
        midiChannelSpinBox->downControl()->setTextColor({100, 100, 100});
        midiChannelSpinBox->setSize(54, 23);
        configureChannelSpinBox(midiChannelSpinBox);
        midiChannelSpinBox->show();
        mainLayout->addWidget(midiChannelSpinBox);
        RK_ACT_BINDL(midiChannelSpinBox,
                    currentIndexChanged,
                    RK_ACT_ARGS(int index),
                     [=, this](int index) {
                             auto *currentIntrument = kitModel->currentPercussion();
                             currentIntrument->setMidiChannel(index - 1);
                     });

        // Playback mode button
        mainLayout->addSpace(6);
        playbackModeButton = new GeonkickButton(this);
        playbackModeButton->setType(RkButton::ButtonType::ButtonPush);
        playbackModeButton->setSize(56, 20);
        playbackModeButton->setBackgroundColor({42, 42, 42});
        playbackModeButton->setTextColor({200, 200, 200});
        mainLayout->addWidget(playbackModeButton);
        RK_ACT_BINDL(playbackModeButton,
                     pressed,
                     RK_ACT_ARGS(),
                     [=, this]() {
                             auto instrument = kitModel->currentPercussion();
                             const auto mode = (static_cast<int>(instrument->playbackMode()) + 1) % 3;
                             instrument->setPlaybackMode(static_cast<DspProxy::PlaybackMode>(mode));
                             updatePlaymodeButton();
                     } );

        // Tune instrument
        addSeparator(mainLayout);
        tuneCheckbox = new GeonkickButton(this);
        tuneCheckbox->setCheckable(true);
        tuneCheckbox->setFixedSize(33, 18);
        tuneCheckbox->setImage(RkImage(tuneCheckbox->size(), RK_IMAGE_RC(tune_checkbox_off)),
                               RkButton::State::Unpressed);
        tuneCheckbox->setImage(RkImage(tuneCheckbox->size(), RK_IMAGE_RC(tune_checkbox_on)),
                               RkButton::State::Pressed);
        tuneCheckbox->setImage(RkImage(tuneCheckbox->size(), RK_IMAGE_RC(tune_checkbox_hover)),
                               RkButton::State::PressedHover);
        tuneCheckbox->setImage(RkImage(tuneCheckbox->size(), RK_IMAGE_RC(tune_checkbox_hover)),
                               RkButton::State::UnpressedHover);
        tuneCheckbox->show();
        RK_ACT_BIND(tuneCheckbox, toggled, RK_ACT_ARGS(bool b), geonkickModel->getDspProxy(),
		    tuneAudioOutput(geonkickModel->getDspProxy()->currentPercussion(), b));
        mainLayout->addWidget(tuneCheckbox);

        RK_ACT_BIND(kitModel,
                    modelUpdated,
                    RK_ACT_ARGS(),
                    this, updateGui());
        RK_ACT_BINDL(kitModel,
                     instrumentUpdated,
                     RK_ACT_ARGS(PercussionModel* model),
                     [=, this](PercussionModel* model) {
                             if (model->isSelected())
                                     updateGui();
                     } );

        RK_ACT_BIND(kitModel,
                    instrumentAdded,
                    RK_ACT_ARGS(PercussionModel *model),
                    this,
                    bindInstrumentChannelUpdates(model));
        for (auto *model: kitModel->instrumentModels())
                bindInstrumentChannelUpdates(model);
        updateGui();
}

void TopBar::addSeparator(RkContainer *mainLayout, int width)
{
        mainLayout->addSpace(width);
        auto separator = new RkLabel(this);
        separator->setSize(2, 21);
        separator->setBackgroundColor(68, 68, 70);
        separator->setImage(RkImage(separator->size(), RK_IMAGE_RC(separator)));
        separator->show();
        mainLayout->addWidget(separator);
        mainLayout->addSpace(width);
}

void TopBar::createMainMenu(RkContainer *layout)
{
        // Synth button
        addSeparator(layout);
        synthButton = new GeonkickButton(this);
        synthButton->setPressed(viewState()->getMainView() == ViewState::View::Synth);
        synthButton->setImage(RK_RC_IMAGE(topbar_synth_tab),
                                 RkButton::State::Unpressed);
        synthButton->setImage(RK_RC_IMAGE(topbar_synth_tab_hover),
                                 RkButton::State::UnpressedHover);
        synthButton->setImage(RK_RC_IMAGE(topbar_synth_tab_on),
                                 RkButton::State::Pressed);

        synthButton->show();
        layout->addWidget(synthButton);
        RK_ACT_BIND(synthButton, pressed, RK_ACT_ARGS(),
                    viewState(), setMainView(ViewState::View::Synth));
        RK_ACT_BIND(viewState(), mainViewChanged, RK_ACT_ARGS(ViewState::View view),
        synthButton, setPressed(view == ViewState::View::Synth));

#ifndef GEONKICK_SINGLE
        // Kit button
        addSeparator(layout);
        kitButton = new GeonkickButton(this);
        kitButton->setPressed(viewState()->getMainView() == ViewState::View::Kit);
        kitButton->setFixedSize(25, 20);
        kitButton->setImage(RkImage(kitButton->size(), RK_IMAGE_RC(topmenu_kit_off)),
                               RkButton::State::Unpressed);
        kitButton->setImage(RkImage(kitButton->size(), RK_IMAGE_RC(topmenu_kit_active)),
                               RkButton::State::Pressed);
        kitButton->setImage(RkImage(kitButton->size(), RK_IMAGE_RC(topmenu_kit_hover)),
                               RkButton::State::UnpressedHover);
        kitButton->show();
        RK_ACT_BIND(kitButton, pressed, RK_ACT_ARGS(),
                    viewState(), setMainView(ViewState::View::Kit));
        RK_ACT_BIND(viewState(), mainViewChanged, RK_ACT_ARGS(ViewState::View view),
                    kitButton, setPressed(view == ViewState::View::Kit));
                    layout->addWidget(kitButton);
#endif // GEONKICK_SINGLE
}

RkWidget* TopBar::createInstrumentNameLabel()
{
        instrumentName = new RkLineEdit(this);
        instrumentName->setBackgroundColor({44, 44, 44});
        instrumentName->setTextColor({180, 180, 180});
        instrumentName->setCursorColor({180, 180, 180});
        instrumentName->setSize(100, 20);
        instrumentName->show();
        RK_ACT_BINDL(instrumentName, editingFinished, RK_ACT_ARGS(),
                     [=, this]() {
                             auto currentInstrument = kitModel->currentPercussion();
                             if (!currentInstrument->setName(instrumentName->text()))
                                     instrumentName->setText(currentInstrument->name());
                     });
        RK_ACT_BINDL(instrumentName, escapePressed, RK_ACT_ARGS(),
                     [=, this]() {
                             auto currentInstrument = kitModel->currentPercussion();
                             instrumentName->setText(currentInstrument->name());
                     });

        return instrumentName;
}

void TopBar::setPresetName(const std::string &name)
{
        if (name.size() > 20) {
                std::string preset = name;
                preset.resize(15);
                preset += "...";
                instrumentName->setText(preset);
        } else {
                instrumentName->setText(name);
        }
        instrumentName->moveCursorToEnd();
}

void TopBar::updateGui()
{
        auto dsp = geonkickModel->getDspProxy();
        tuneCheckbox->setPressed(dsp->isAudioOutputTuned(dsp->currentPercussion()));
        setPresetName(kitModel->currentPercussion()->name());
        midiKeyButton->setText(MidiKeyWidget::midiKeyToNote(kitModel->currentPercussion()->key()));
        auto instrumentModel = kitModel->currentPercussion();
        auto nMidiChannels = instrumentModel->numberOfMidiChannels();
        midiChannelSpinBox->clear();
        midiChannelSpinBox->addItem("Any");
        for (size_t i = 0; i < nMidiChannels; i++)
                midiChannelSpinBox->addItem(std::to_string(i + 1));
        midiChannelSpinBox->setCurrentIndex(instrumentModel->midiChannel() + 1);
        outputChannelSpinBox->clear();
        for (size_t i = 0; i < instrumentModel->numberOfChannels(); i++)
                outputChannelSpinBox->addItem(std::to_string(i + 1));
        outputChannelSpinBox->setCurrentIndex(instrumentModel->channel());

        updatePlaymodeButton();
}

void TopBar::bindInstrumentChannelUpdates(PercussionModel *model)
{
        RK_ACT_BINDL(model,
                     channelUpdated,
                     RK_ARG_TYPE(int),
                     [=, this](int index) {
                             if (model->isSelected()
                                 && outputChannelSpinBox->currentIndex() != index)
                                     outputChannelSpinBox->setCurrentIndex(index);
                     });
        RK_ACT_BINDL(model,
                     midiChannelUpdated,
                     RK_ARG_TYPE(int),
                     [=, this](int index) {
                             const auto spinBoxIndex = index + 1;
                             if (model->isSelected()
                                 && midiChannelSpinBox->currentIndex() != spinBoxIndex)
                                     midiChannelSpinBox->setCurrentIndex(spinBoxIndex);
                     });
}

void TopBar::updatePlaymodeButton()
{
        auto instrumentModel = kitModel->currentPercussion();
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

void TopBar::showMidiPopup()
{
        auto midiPopup = new MidiKeyWidget(dynamic_cast<GeonkickWidget*>(getTopWidget()),
                                           kitModel->currentPercussion());
        midiPopup->setPosition(150, y() + 35);
        RK_ACT_BIND(midiPopup,
                    isAboutToClose,
                    RK_ACT_ARGS(),
                    midiKeyButton,
                    setPressed(false));
        midiPopup->show();
}

void TopBar::showSettings()
{
        settingsButton->setPressed(false);
        auto settingsPopup = new SettingsWidget(dynamic_cast<GeonkickWidget*>(getTopWidget()),
                                                geonkickModel->getDspProxy());
        settingsPopup->setPosition((getTopWidget()->width()
                                    - settingsPopup->width()) / 2 - 120,
                                   50);
        settingsPopup->show();
}
