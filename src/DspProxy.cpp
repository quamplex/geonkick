/**
 * File name: DspProxy.cpp
 * Project: Geonkick (A percussive synthesizer)
 *
 * Copyright (C) 2017 Iurie Nistor
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

#include "DspProxy.h"
#include "DspProxyHumanizer.h"
#include "DesktopPaths.h"
#include "OscillatorModel.h"
#include "globals.h"
#include "InstrumentState.h"
#include "kit_state.h"
#include "preset.h"
#include "preset_folder.h"
#include "UiSettings.h"
#include "GeonkickConfig.h"

#include "RkEventQueue.h"
#include "RkAction.h"

#include <sndfile.h>

DspProxy::DspProxy(int sample_rate, InstanceType instance, geonkick *dsp)
        : geonkickDsp{dsp}
        , instanceType{instance}
        , limiterLevelers{}
        , jackEnabled{false}
        , standaloneInstance{false}
        , eventQueue{nullptr}
        , currentLayer{Layer::Layer1}
        , kitName{"Unknown"}
        , kitAuthor{"Author"}
        , clipboardPercussion{nullptr}
        , uiSettings{std::make_unique<UiSettings>()}
	, sampleRate{sample_rate}
        , scaleFactor{1.0}
{
        setupPaths();
        uiSettings->setSamplesBrowserPath(getSettings("GEONKICK_CONFIG/HOME_PATH"));
        GeonkickConfig cfg;
        scaleFactor = cfg.getScaleFactor();
}

DspProxy::~DspProxy()
{
  	if (geonkickDsp)
                geonkick_free(&geonkickDsp);
}

void DspProxy::setInstanceType(InstanceType type)
{
        instanceType = type;
}

DspProxy::InstanceType DspProxy::getInstanceType() const
{
        return instanceType;
}

unsigned int DspProxy::getVersion()
{
        return GEONKICK_VERSION;
}

void DspProxy::setEventQueue(RkEventQueue *queue)
{
        std::lock_guard<std::mutex> lock(dspMutex);
        eventQueue = queue;
}

bool DspProxy::initDSP()
{
        if (!geonkickDsp) {
                if (geonkick_create(&geonkickDsp, sampleRate) != GEONKICK_OK) {
                        GEONKICK_LOG_ERROR("can't create geonkick DSP");
                        return false;
                }
        }
        return true;
}

bool DspProxy::init()
{
        if (!initDSP())
	        return false;

        GeonkickConfig cfg;
        forceMidiChannel(cfg.getMidiChannel(), cfg.isMidiChannelForced());
        dspProxyHumanizer = std::make_unique<DspProxyHumanizer>(geonkickDsp);
	loadPresets();

        jackEnabled = geonkick_is_module_enabed(geonkickDsp, GEONKICK_MODULE_JACK);
	geonkick_enable_synthesis(geonkickDsp, false);

	auto nInstruments = numberOfInstruments();
        auto nChannels = numberOfChannels();
        kickBuffers = std::vector<std::vector<gkick_real>>(nInstruments);
	for (decltype(nInstruments) i = 0; i < nInstruments; i++) {
                auto state = getDefaultPercussionState();
                state->setId(i);
                state->setChannel(i % nChannels);
		setPercussionState(state);
        }

        setKitState(getDefaultKitState());
        enablePercussion(0, true);
        addOrderedPercussionId(0);

        // Set the first the instrument by default to be controllable.
	geonkick_set_current_instrument(geonkickDsp, 0);
	geonkick_enable_synthesis(geonkickDsp, true);
        return true;
}

size_t DspProxy::numberOfInstruments()
{
        return geonkick_instruments_number();
}

size_t DspProxy::numberOfChannels()
{
        return geonkick_channels_number();
}

size_t DspProxy::numberOfMidiChannels()
{
        return geonkick_midi_channels_number();
}

size_t DspProxy::numberOfLayers()
{
        return geonkick_layers_number();
}

std::unique_ptr<KitState> DspProxy::getDefaultKitState()
{
        return std::make_unique<KitState>();
}

std::unique_ptr<PercussionState> DspProxy::getDefaultPercussionState()
{
        auto state = std::make_unique<PercussionState>();
        state->setName("Default");
        state->setId(0);
        state->setPlayingKey(-1);
        state->setChannel(0);
        state->setLimiterValue(1.0);
        state->tuneOutput(false);
        state->setKickLength(300);
        state->setKickAmplitude(0.8);
        state->enableKickFilter(false);
        state->setKickFilterFrequency(200);
        state->setKickFilterQFactor(10);
        state->setKickFilterType(DspProxy::FilterType::LowPass);
        std::vector<EnvelopePoint> envelope;
        envelope.push_back({0, 1});
        envelope.push_back({1, 1});
        state->setKickEnvelopePoints(DspProxy::EnvelopeType::Amplitude, envelope);
        state->setKickEnvelopeApplyType(DspProxy::EnvelopeType::FilterCutOff,
					EnvelopeApplyType::Logarithmic);
	state->setKickEnvelopePoints(DspProxy::EnvelopeType::FilterCutOff, envelope);
	state->setKickEnvelopePoints(DspProxy::EnvelopeType::FilterQFactor, envelope);
	state->setKickEnvelopePoints(DspProxy::EnvelopeType::DistortionDrive, envelope);
        state->setKickEnvelopePoints(DspProxy::EnvelopeType::DistortionVolume, envelope);
        state->enableDistortion(false);
        state->setDistortionType(DistortionType::SoftClippingTan);
        state->setDistortionInLimiter(1.0);
        state->setDistortionOutLimiter(0.1);
        state->setDistortionDrive(1.0);
        state->setDistortionDrive(1.0);
        state->humanizerEnable(false);
        state->humanizerSetVelocity(10.0);
        state->humanizerSetTiming(5.0);

        std::vector<DspProxy::OscillatorType> oscillators = {
                DspProxy::OscillatorType::Oscillator1,
                DspProxy::OscillatorType::Oscillator2,
                DspProxy::OscillatorType::Oscillator3
        };

        auto nLayers = numberOfLayers();
        for (size_t l = 0; l < nLayers; l++) {
                auto layer = static_cast<Layer>(l);
                state->setLayerEnabled(layer, layer == Layer::Layer1);
                state->setLayerAmplitude(layer, 1.0);
                for (auto const &osc: oscillators) {
                        int index = static_cast<int>(osc) + GKICK_OSC_GROUP_SIZE * static_cast<int>(layer);
                        state->setOscillatorEnabled(index, osc == DspProxy::OscillatorType::Oscillator1);
                        state->setOscillatorFunction(index, DspProxy::FunctionType::Sine);
                        state->setOscillatorPhase(index, 0);
                        state->setOscillatorAmplitue(index, 0.26);
                        state->setOscillatorFrequency(index, 800);
                        state->setOscillatorPitchShift(index, 12);
                        state->setOscillatorFilterEnabled(index, false);
                        state->setOscillatorFilterType(index, DspProxy::FilterType::LowPass);
                        state->setOscillatorFilterCutOffFreq(index, 800);
                        state->setOscillatorFilterFactor(index, 10);
                        state->setOscillatorEnvelopePoints(index, envelope, DspProxy::EnvelopeType::Amplitude);
                        state->setOscillatorEnvelopeApplyType(index,
                                                              DspProxy::EnvelopeType::Frequency,
                                                              DspProxy::EnvelopeApplyType::Logarithmic);
                        state->setOscillatorEnvelopePoints(index, envelope, DspProxy::EnvelopeType::Frequency);
                        state->setOscillatorEnvelopePoints(index, envelope, DspProxy::EnvelopeType::PitchShift);
                        state->setOscillatorEnvelopePoints(index, envelope, DspProxy::EnvelopeType::DistortionDrive);
                        std::vector<EnvelopePoint> env = envelope;
                        env[0].setY(0.5);
                        env[1].setY(0.5);
                        state->setOscillatorEnvelopePoints(index, env, DspProxy::EnvelopeType::PitchShift);
                        state->setOscillatorEnvelopePoints(index,
							   envelope,
							   DspProxy::EnvelopeType::FilterCutOff);
			state->setOscillatorEnvelopePoints(index,
							   envelope,
							   DspProxy::EnvelopeType::FilterQFactor);
			state->setOscillatorEnvelopeApplyType(index,
							      DspProxy::EnvelopeType::FilterCutOff,
							      DspProxy::EnvelopeApplyType::Logarithmic);
                        state->setOscillatorEnvelopePoints(index, envelope, EnvelopeType::DistortionDrive);
                        state->setOscDistortionEnabled(index, false);
                        state->setOscDistortionType(index, DistortionType::SoftClippingTan);
                        state->setOscDistortionInLimiter(index, 1.0);
                        state->setOscDistortionOutLimiter(index, 1.0);
                        state->setOscDistortionDrive(index, 1.0);
                }
        }

        return state;
}

void DspProxy::setPercussionState(const std::unique_ptr<PercussionState> &state)
{
        if (!state)
                return;

        geonkick_enable_synthesis(geonkickDsp, false);
        geonkick_enable_instrument(geonkickDsp, state->getId(), state->isEnabled());
        auto currentId = currentPercussion();
        geonkick_set_current_instrument(geonkickDsp, state->getId());
        setPercussionName(state->getId(), state->getName());
        setPercussionPlayingKey(state->getId(), state->getPlayingKey());
        setPercussionChannel(state->getId(), state->getChannel());
        setPercussionMidiChannel(state->getId(), state->getMidiChannel());
        enableNoteOff(state->getId(), state->isNoteOffEnabled());
        setChokeGroup(state->getId(), state->getChokeGroup());
        mutePercussion(state->getId(), state->isMuted());
        soloPercussion(state->getId(), state->isSolo());
        auto nLayers = numberOfLayers();
        for (size_t i = 0; i < nLayers; i++) {
                enbaleLayer(static_cast<Layer>(i), state->isLayerEnabled(static_cast<Layer>(i)));
                setLayerAmplitude(static_cast<Layer>(i), state->getLayerAmplitude(static_cast<Layer>(i)));
        }
        setLimiterValue(state->getLimiterValue());
        tuneAudioOutput(state->getId(), state->isOutputTuned());
        setKickLength(state->getKickLength());
        setKickAmplitude(state->getKickAmplitude());
        enableKickFilter(state->isKickFilterEnabled());
        setKickFilterFrequency(state->getKickFilterFrequency());
        setKickFilterQFactor(state->getKickFilterQFactor());
        setKickFilterType(state->getKickFilterType());
        setKickEnvelopePoints(DspProxy::EnvelopeType::Amplitude,
                              state->getKickEnvelopePoints(DspProxy::EnvelopeType::Amplitude));
        setKickEnvelopeApplyType(DspProxy::EnvelopeType::FilterCutOff,
				 state->getKickEnvelopeApplyType(DspProxy::EnvelopeType::FilterCutOff));
	setKickEnvelopePoints(DspProxy::EnvelopeType::FilterCutOff,
                              state->getKickEnvelopePoints(DspProxy::EnvelopeType::FilterCutOff));
	setKickEnvelopePoints(DspProxy::EnvelopeType::FilterQFactor,
                              state->getKickEnvelopePoints(DspProxy::EnvelopeType::FilterQFactor));
	setKickEnvelopePoints(DspProxy::EnvelopeType::DistortionDrive,
                              state->getKickEnvelopePoints(DspProxy::EnvelopeType::DistortionDrive));
        setKickEnvelopePoints(DspProxy::EnvelopeType::DistortionVolume,
                              state->getKickEnvelopePoints(DspProxy::EnvelopeType::DistortionVolume));

        for (size_t i = 0; i < nLayers; i++) {
                setOscillatorState(static_cast<Layer>(i), OscillatorType::Oscillator1, state);
                setOscillatorState(static_cast<Layer>(i), OscillatorType::Oscillator2, state);
                setOscillatorState(static_cast<Layer>(i), OscillatorType::Oscillator3, state);
        }
        enableDistortion(state->isDistortionEnabled());
        setDistortionType(state->getDistortionType());
        setDistortionInLimiter(state->getDistortionInLimiter());
        setDistortionOutLimiter(state->getDistortionOutLimiter());
        setDistortionDrive(state->getDistortionDrive());
        getHumanizer()->enable(state->humanizerIsEnabled());
        getHumanizer()->setVelocityPercent(state->humanizerGetVelocity());
        getHumanizer()->setTiming(state->humanizerGetTiming());

        geonkick_set_current_instrument(geonkickDsp, currentId);
        geonkick_enable_synthesis(geonkickDsp, true);
}

void DspProxy::setPercussionState(const std::string &data)
{
        auto state = getDefaultPercussionState();
        state->loadData(data);
        setPercussionState(state);
}

std::unique_ptr<PercussionState> DspProxy::getPercussionState(size_t id) const
{
        if (id == currentPercussion()) {
                return getPercussionState();
        } else {
                auto tmpId = currentPercussion();
                auto res = geonkick_set_current_instrument(geonkickDsp, id);
                if (res != GEONKICK_OK) {
                        geonkick_set_current_instrument(geonkickDsp, tmpId);
                        return getPercussionState();
                }
                auto state = getPercussionState();
                geonkick_set_current_instrument(geonkickDsp, tmpId);
                return state;
        }
}

std::unique_ptr<PercussionState> DspProxy::getPercussionState() const
{
        auto state = std::make_unique<PercussionState>();
        state->setId(currentPercussion());
        state->setName(getPercussionName(state->getId()));
        state->setLimiterValue(limiterValue());
        state->tuneOutput(isAudioOutputTuned(state->getId()));
        state->setPlayingKey(getPercussionPlayingKey(state->getId()));
        state->setChannel(getPercussionChannel(state->getId()));
        state->setMidiChannel(getPercussionMidiChannel(state->getId()));
        state->setNoteOffEnabled(isNoteOffEnabled(state->getId()));
        state->setChokeGroup(getChokeGroup(state->getId()));
        state->setMute(isPercussionMuted(state->getId()));
        state->setSolo(isPercussionSolo(state->getId()));
        auto nLayers = numberOfLayers();
        for (size_t i = 0; i < nLayers; i++) {
                state->setLayerEnabled(static_cast<Layer>(i), isLayerEnabled(static_cast<Layer>(i)));
                state->setLayerAmplitude(static_cast<Layer>(i), getLayerAmplitude(static_cast<Layer>(i)));
        }
        state->setKickLength(kickLength());
        state->setKickAmplitude(kickAmplitude());
        state->enableKickFilter(isKickFilterEnabled());
        state->setKickFilterFrequency(kickFilterFrequency());
        state->setKickFilterQFactor(kickFilterQFactor());
        state->setKickFilterType(kickFilterType());
        state->setKickEnvelopePoints(DspProxy::EnvelopeType::Amplitude,
                                     getKickEnvelopePoints(DspProxy::EnvelopeType::Amplitude));
	state->setKickEnvelopeApplyType(DspProxy::EnvelopeType::FilterCutOff,
                                     getKickEnvelopeApplyType(DspProxy::EnvelopeType::FilterCutOff));
        state->setKickEnvelopePoints(DspProxy::EnvelopeType::FilterCutOff,
                                     getKickEnvelopePoints(DspProxy::EnvelopeType::FilterCutOff));
	state->setKickEnvelopePoints(DspProxy::EnvelopeType::FilterQFactor,
                                     getKickEnvelopePoints(DspProxy::EnvelopeType::FilterQFactor));
	state->setKickEnvelopePoints(DspProxy::EnvelopeType::DistortionDrive,
                                     getKickEnvelopePoints(DspProxy::EnvelopeType::DistortionDrive));
        state->setKickEnvelopePoints(DspProxy::EnvelopeType::DistortionVolume,
                                     getKickEnvelopePoints(DspProxy::EnvelopeType::DistortionVolume));


        for (size_t i = 0; i < nLayers; i++) {
                getOscillatorState(static_cast<Layer>(i), OscillatorType::Oscillator1, state);
                getOscillatorState(static_cast<Layer>(i), OscillatorType::Oscillator2, state);
                getOscillatorState(static_cast<Layer>(i), OscillatorType::Oscillator3, state);
        }
        state->enableDistortion(isDistortionEnabled());
        state->setDistortionType(getDistortionType());
        state->setDistortionInLimiter(getDistortionInLimiter());
        state->setDistortionOutLimiter(getDistortionOutLimiter());
        state->setDistortionDrive(getDistortionDrive());
        state->humanizerEnable(getHumanizer()->isEnabled());
        state->humanizerSetVelocity(getHumanizer()->getVelocityPercent());
        state->humanizerSetTiming(getHumanizer()->getTiming());

        return state;
}

void DspProxy::getOscillatorState(DspProxy::Layer layer,
                                     OscillatorType osc,
                                     const std::unique_ptr<PercussionState> &state) const
{
        auto temp = currentLayer;
        currentLayer = layer;
        auto index = static_cast<int>(osc);
        state->setCurrentLayer(layer);
        state->setOscillatorEnabled(index, isOscillatorEnabled(index));
        state->setOscillatorFunction(index, oscillatorFunction(index));
        state->setOscillatorSample(index, getOscillatorSample(index));
        state->setOscillatorPhase(index, oscillatorPhase(index));
        state->setOscillatorSeed(index, oscillatorSeed(index));
        state->setOscillatorAmplitue(index, oscillatorAmplitude(index));
        state->setOscillatorFrequency(index, oscillatorFrequency(index));
        state->setOscillatorPitchShift(index, oscillatorPitchShift(index));
        state->setOscillatorNoiseDensity(index, oscillatorNoiseDensity(index));
        state->setOscillatorFilterEnabled(index, isOscillatorFilterEnabled(index));
        state->setOscillatorFilterType(index, getOscillatorFilterType(index));
        state->setOscillatorFilterCutOffFreq(index, getOscillatorFilterCutOffFreq(index));
        state->setOscillatorFilterFactor(index, getOscillatorFilterFactor(index));
        auto points = oscillatorEvelopePoints(index, DspProxy::EnvelopeType::Amplitude);
        state->setOscillatorEnvelopePoints(index, points, DspProxy::EnvelopeType::Amplitude);
        auto applyType = getOscillatorEnvelopeApplyType(index, DspProxy::EnvelopeType::Frequency);
        state->setOscillatorEnvelopeApplyType(index, DspProxy::EnvelopeType::Frequency, applyType);
        points = oscillatorEvelopePoints(index, DspProxy::EnvelopeType::Frequency);
        state->setOscillatorEnvelopePoints(index, points, DspProxy::EnvelopeType::Frequency);
        points = oscillatorEvelopePoints(index, DspProxy::EnvelopeType::PitchShift);
        state->setOscillatorEnvelopePoints(index, points, DspProxy::EnvelopeType::PitchShift);
        points = oscillatorEvelopePoints(index, DspProxy::EnvelopeType::NoiseDensity);
        state->setOscillatorEnvelopePoints(index, points, DspProxy::EnvelopeType::NoiseDensity);
        applyType = getOscillatorEnvelopeApplyType(index, DspProxy::EnvelopeType::FilterCutOff);
        state->setOscillatorEnvelopeApplyType(index, DspProxy::EnvelopeType::FilterCutOff, applyType);
        points = oscillatorEvelopePoints(index, DspProxy::EnvelopeType::FilterCutOff);
        state->setOscillatorEnvelopePoints(index, points, DspProxy::EnvelopeType::FilterCutOff);
	points = oscillatorEvelopePoints(index, DspProxy::EnvelopeType::FilterQFactor);
        state->setOscillatorEnvelopePoints(index, points, DspProxy::EnvelopeType::FilterQFactor);
        state->setOscillatorAsFm(index, isOscillatorAsFm(index));

        // Distortion
        points = oscillatorEvelopePoints(index, EnvelopeType::DistortionDrive);
        state->setOscillatorEnvelopePoints(index, points, EnvelopeType::DistortionDrive);
        state->setOscDistortionType(index, getOscDistortionType(index));
        state->setOscDistortionEnabled(index, isOscDistortionEnabled(index));
        state->setOscDistortionInLimiter(index, getOscDistortionInLimiter(index));
        state->setOscDistortionOutLimiter(index, getOscDistortionOutLimiter(index));
        state->setOscDistortionDrive(index, getOscDistortionDrive(index));

        currentLayer = temp;
}

void DspProxy::setOscillatorState(DspProxy::Layer layer,
                                     OscillatorType oscillator,
                                     const std::unique_ptr<PercussionState> &state)
{
        auto temp = currentLayer;
        currentLayer = layer;
        auto osc = static_cast<int>(oscillator);
        state->setCurrentLayer(layer);
        enableOscillator(osc, state->isOscillatorEnabled(osc));
        setOscillatorFunction(osc, state->oscillatorFunction(osc));
        setOscillatorSample(state->getOscillatorSample(osc), osc);
        setOscillatorPhase(osc, state->oscillatorPhase(osc));
        setOscillatorSeed(osc, state->oscillatorSeed(osc));
        setOscillatorAmplitude(osc, state->oscillatorAmplitue(osc));
        setOscillatorFrequency(osc, state->oscillatorFrequency(osc));
        setOscillatorPitchShift(osc, state->oscillatorPitchShift(osc));
        setOscillatorNoiseDensity(osc, state->oscillatorNoiseDensity(osc));
        enableOscillatorFilter(osc, state->isOscillatorFilterEnabled(osc));
        setOscillatorFilterType(osc, state->oscillatorFilterType(osc));
        setOscillatorFilterCutOffFreq(osc, state->oscillatorFilterCutOffFreq(osc));
        setOscillatorFilterFactor(osc, state->oscillatorFilterFactor(osc));
        setOscillatorEvelopePoints(osc, EnvelopeType::Amplitude,
                                   state->oscillatorEnvelopePoints(osc, EnvelopeType::Amplitude));
        setOscillatorEnvelopeApplyType(osc, EnvelopeType::Frequency,
                                       state->getOscillatorEnvelopeApplyType(osc,
                                                                             EnvelopeType::Frequency));
        setOscillatorEvelopePoints(osc, EnvelopeType::Frequency,
                                   state->oscillatorEnvelopePoints(osc,
                                                                   EnvelopeType::Frequency));
        setOscillatorEvelopePoints(osc, EnvelopeType::PitchShift,
                                   state->oscillatorEnvelopePoints(osc,
                                                                   EnvelopeType::PitchShift));
        setOscillatorEvelopePoints(osc, EnvelopeType::NoiseDensity,
                                   state->oscillatorEnvelopePoints(osc,
                                                                   EnvelopeType::NoiseDensity));
	setOscillatorEnvelopeApplyType(osc, EnvelopeType::FilterCutOff,
				       state->getOscillatorEnvelopeApplyType(osc,
									     EnvelopeType::FilterCutOff));
        setOscillatorEvelopePoints(osc, EnvelopeType::FilterCutOff,
                                   state->oscillatorEnvelopePoints(osc, EnvelopeType::FilterCutOff));
	setOscillatorEvelopePoints(osc, EnvelopeType::FilterQFactor,
                                   state->oscillatorEnvelopePoints(osc, EnvelopeType::FilterQFactor));
        setOscillatorEvelopePoints(osc, EnvelopeType::DistortionDrive,
                                   state->oscillatorEnvelopePoints(osc, EnvelopeType::DistortionDrive));
        oscEnableDistortion(osc, state->isOscDistortionEnabled(osc));
        setOscDistortionType(osc, state->getOscDistortionType(osc));
        setOscDistortionInLimiter(osc, state->getOscDistortionInLimiter(osc));
        setOscDistortionOutLimiter(osc, state->getOscDistortionOutLimiter(osc));
        setOscDistortionDrive(osc, state->getOscDistortionDrive(osc));
        setOscillatorAsFm(osc, state->isOscillatorAsFm(osc));
        setOscillatorAsFm(osc, state->isOscillatorAsFm(osc));
        setOscillatorFmK(osc, state->getOscillatorFmK(osc));
        currentLayer = temp;
}

std::unique_ptr<KitState> DspProxy::getKitState() const
{
        auto kit = std::make_unique<KitState>();
        kit->setName(getKitName());
        kit->setAuthor(getKitAuthor());
        kit->setUrl(getKitUrl());
        size_t i = 0;
        for (const auto &id : ordredPercussionIds()) {
                auto state = getPercussionState(id);
                state->setId(i);
		GEONKICK_LOG_DEBUG("PER: " << state->getName() << ": id = " << state->getId());
                kit->addPercussion(std::move(state));
                i++;
        }
        return kit;
}

bool DspProxy::setKitState(const std::string &data)
{
        auto state = std::make_unique<KitState>();
        state->fromJson(data);
        return setKitState(state);
}

bool DspProxy::setKitState(const std::unique_ptr<KitState> &state)
{
        if (state->instruments().empty()) {
                GEONKICK_LOG_ERROR("wrong kit state");
                return false;
        }

        auto n = numberOfInstruments();
        for (decltype(n) i = 0; i < n; i++)
                enablePercussion(i, false);
        setKitName(state->getName());
        setKitAuthor(state->getAuthor());
        setKitUrl(state->getUrl());
        clearOrderedPercussionIds();
        for (const auto &per: state->instruments()) {
                setPercussionState(per);
                addOrderedPercussionId(per->getId());
        }

        if (!instrumentIdList.empty())
                setCurrentPercussion(instrumentIdList.front());
        else
                 setCurrentPercussion(0);
        return true;
}

size_t DspProxy::oscillatorsPerLayer(void) const
{
        return GKICK_OSC_GROUP_SIZE;
}

std::vector<EnvelopePoint> DspProxy::oscillatorEvelopePoints(int oscillatorIndex,  EnvelopeType envelope) const
{
        struct gkick_envelope_point_info *buf = NULL;
        std::vector<EnvelopePoint> points;
        size_t npoints = 0;
        geonkick_osc_envelope_get_points(geonkickDsp,
                                         getOscIndex(oscillatorIndex),
                                         static_cast<int>(envelope),
                                         &buf,
                                         &npoints);
        for (decltype(npoints) i = 0; i < npoints; i++) {
                EnvelopePoint p;
                p.setX(buf[i].x);
                p.setY(buf[i].y);
                p.setAsControlPoint(buf[i].control_point);
                points.emplace_back(std::move(p));
        }
        free(buf);
        return points;
}

DspProxy::EnvelopeApplyType DspProxy::getOscillatorEnvelopeApplyType(int index,
									   EnvelopeType envelope) const
{
	enum gkick_envelope_apply_type applyType;
	geonkick_osc_envelope_get_apply_type(geonkickDsp,
					     static_cast<size_t>(getOscIndex(index)),
					     static_cast<size_t>(envelope),
					     &applyType);
	return static_cast<EnvelopeApplyType>(applyType);
}

void DspProxy::setOscillatorEvelopePoints(int index,
                                             EnvelopeType envelope,
                                             const std::vector<EnvelopePoint> &points)
{
        if (points.empty())
                return;

        std::vector<struct gkick_envelope_point_info> data(points.size(), {0});
        for (decltype(points.size()) i = 0; i < points.size(); i++) {
                data[i].x = points[i].x();
                data[i].y = points[i].y();
                data[i].control_point = points[i].isControlPoint();
        }

        geonkick_osc_envelope_set_points(geonkickDsp,
                                         getOscIndex(index),
                                         static_cast<int>(envelope),
                                         reinterpret_cast<const struct gkick_envelope_point_info *>(data.data()),
                                         points.size());
}

void DspProxy::setOscillatorEnvelopeApplyType(int index,
						 EnvelopeType envelope,
						 EnvelopeApplyType applyType)
{
	geonkick_osc_envelope_set_apply_type(geonkickDsp,
					     getOscIndex(index),
					     static_cast<int>(envelope),
					     static_cast<enum gkick_envelope_apply_type>(applyType));
}

void DspProxy::addOscillatorEnvelopePoint(int oscillatorIndex,
                                             EnvelopeType envelope,
                                             const EnvelopePoint &point)
{
        struct gkick_envelope_point_info info{static_cast<float>(point.x()),
                                              static_cast<float>(point.y()),
                                              point.isControlPoint()};
        geonkick_osc_envelope_add_point(geonkickDsp,
                                        getOscIndex(oscillatorIndex),
                                        static_cast<int>(envelope),
                                        &info);
}

void DspProxy::removeOscillatorEvelopePoint(int oscillatorIndex, EnvelopeType envelope, int pointIndex)
{
        geonkick_osc_envelope_remove_point(geonkickDsp,
                                           getOscIndex(oscillatorIndex),
                                           static_cast<int>(envelope),
                                           pointIndex);
}

void DspProxy::updateOscillatorEvelopePoint(int oscillatorIndex,
                                               EnvelopeType envelope,
                                               int pointIndex,
                                               const EnvelopePoint &point)
{
        struct gkick_envelope_point_info info{static_cast<float>(point.x()),
                                              static_cast<float>(point.y()),
                                              point.isControlPoint()};
        geonkick_osc_envelope_update_point(geonkickDsp, getOscIndex(oscillatorIndex),
                                           static_cast<int>(envelope),
                                           pointIndex,
                                           &info);
}


void DspProxy::setOscillatorFunction(int oscillatorIndex, FunctionType function)
{
        geonkick_set_osc_function(geonkickDsp,
                                  getOscIndex(oscillatorIndex),
                                  static_cast<enum geonkick_osc_func_type>(function));
}

void DspProxy::setOscillatorPhase(int oscillatorIndex, gkick_real phase)
{
        geonkick_set_osc_phase(geonkickDsp,
                               getOscIndex(oscillatorIndex),
                               phase);
}

gkick_real DspProxy::oscillatorPhase(int oscillatorIndex) const
{
        gkick_real phase = 0;
        geonkick_get_osc_phase(geonkickDsp,
                               getOscIndex(oscillatorIndex),
                               &phase);
        return phase;
}

void DspProxy::setOscillatorSeed(int oscillatorIndex, int seed)
{
        geonkick_set_osc_seed(geonkickDsp,
                              getOscIndex(oscillatorIndex),
                              seed);
}

int DspProxy::oscillatorSeed(int oscillatorIndex) const
{
        unsigned int seed = 0;
        geonkick_get_osc_seed(geonkickDsp,
                              getOscIndex(oscillatorIndex),
                              &seed);
        return seed;
}

DspProxy::FunctionType DspProxy::oscillatorFunction(int oscillatorIndex) const
{

        enum geonkick_osc_func_type function;
        geonkick_get_osc_function(geonkickDsp,
                                  getOscIndex(oscillatorIndex),
                                  &function);
        return static_cast<FunctionType>(function);
}

bool DspProxy::setKickLength(double length)
{
        auto res = geonkick_set_length(geonkickDsp, length / 1000);
        return res == GEONKICK_OK;
}

double DspProxy::kickLength(void) const
{
        gkick_real length = 0;
        geonkick_get_length(geonkickDsp, &length);
        return 1000 * length;
}

bool DspProxy::setKickAmplitude(double amplitude)
{
        auto res = geonkick_kick_set_amplitude(geonkickDsp, amplitude);
        return res == GEONKICK_OK;
}

double DspProxy::kickAmplitude() const
{
        gkick_real amplitude = 0;
        geonkick_kick_get_amplitude(geonkickDsp, &amplitude);
        return static_cast<double>(amplitude);
}

std::vector<EnvelopePoint> DspProxy::getKickEnvelopePoints(EnvelopeType envelope) const
{
        struct gkick_envelope_point_info *buf;
        std::vector<EnvelopePoint> points;
        size_t npoints = 0;
        geonkick_kick_envelope_get_points(geonkickDsp,
                                          static_cast<enum geonkick_envelope_type>(envelope),
                                          &buf,
                                          &npoints);
        for (decltype(npoints) i = 0; i < npoints; i++) {
                EnvelopePoint p;
                p.setX(buf[i].x);
                p.setY(buf[i].y);
                p.setAsControlPoint(buf[i].control_point);
                points.push_back(std::move(p));
        }

        if (buf)
                free(buf);
        return points;
}

DspProxy::EnvelopeApplyType DspProxy::getKickEnvelopeApplyType(EnvelopeType envelope) const
{
	enum gkick_envelope_apply_type applyType;
	geonkick_kick_env_get_apply_type(geonkickDsp,
					 static_cast<enum geonkick_envelope_type>(envelope),
					 &applyType);
	return static_cast<EnvelopeApplyType>(applyType);
}

void DspProxy::setKickEnvelopePoints(EnvelopeType envelope,
                                        const std::vector<EnvelopePoint> &points)
{
        std::vector<struct gkick_envelope_point_info> data(points.size(), {0});
        for (decltype(points.size()) i = 0; i < points.size(); i++) {
                data[i].x             = points[i].x();
                data[i].y             = points[i].y();
                data[i].control_point = points[i].isControlPoint();
        }

        geonkick_kick_envelope_set_points(geonkickDsp,
                                          static_cast<enum geonkick_envelope_type>(envelope),
                                          reinterpret_cast<const struct gkick_envelope_point_info*>(data.data()),
                                          points.size());
}

void DspProxy::setKickEnvelopeApplyType(EnvelopeType envelope, EnvelopeApplyType applyType)
{
        geonkick_kick_env_set_apply_type(geonkickDsp,
					 static_cast<enum geonkick_envelope_type>(envelope),
					 static_cast<enum gkick_envelope_apply_type>(applyType));
}

bool DspProxy::enableKickFilter(bool b)
{
        auto res = geonkick_kick_filter_enable(geonkickDsp, b);
        return res == GEONKICK_OK;
}

bool DspProxy::isKickFilterEnabled() const
{
        int enabled = 0;
        geonkick_kick_filter_is_enabled(geonkickDsp, &enabled);
        return enabled;
}

bool DspProxy::setKickFilterType(FilterType type)
{
        auto res = geonkick_set_kick_filter_type(geonkickDsp,
                                                 static_cast<enum gkick_filter_type>(type));
        return res == GEONKICK_OK;
}

DspProxy::FilterType DspProxy::kickFilterType() const
{
        enum gkick_filter_type type;
        geonkick_get_kick_filter_type(geonkickDsp, &type);
        return static_cast<FilterType>(type);
}

bool DspProxy::setKickFilterFrequency(double frequency)
{
        auto res = geonkick_kick_set_filter_frequency(geonkickDsp, frequency);
        return res == GEONKICK_OK;
}

double DspProxy::kickFilterFrequency(void) const
{
        gkick_real frequency;
        geonkick_kick_get_filter_frequency(geonkickDsp, &frequency);
        return static_cast<double>(frequency);
}

bool DspProxy::setKickFilterQFactor(double factor)
{
        auto res = geonkick_kick_set_filter_factor(geonkickDsp, factor);
        return res == GEONKICK_OK;
}

double DspProxy::kickFilterQFactor() const
{
        gkick_real factor = 0;
        geonkick_kick_get_filter_factor(geonkickDsp, &factor);
        return static_cast<double>(factor);
}

void DspProxy::addKickEnvelopePoint(EnvelopeType envelope, const EnvelopePoint &point)
{
        struct gkick_envelope_point_info info{static_cast<float>(point.x()),
                                              static_cast<float>(point.y()),
                                              point.isControlPoint()};
        geonkick_kick_add_env_point(geonkickDsp,
                                    static_cast<enum geonkick_envelope_type>(envelope),
                                    &info);
}

void DspProxy::updateKickEnvelopePoint(EnvelopeType envelope,
                                          int index,
                                          const EnvelopePoint &point)
{
        struct gkick_envelope_point_info info{static_cast<float>(point.x()),
                                              static_cast<float>(point.y()),
                                              point.isControlPoint()};
        geonkick_kick_update_env_point(geonkickDsp,
                                       static_cast<enum geonkick_envelope_type>(envelope),
                                       index,
                                       &info);
}

 void DspProxy::removeKickEnvelopePoint(EnvelopeType envelope, int pointIndex)
{
        geonkick_kick_remove_env_point(geonkickDsp,
                                       static_cast<enum geonkick_envelope_type>(envelope),
                                       pointIndex);
}

bool DspProxy::setOscillatorAmplitude(int oscillatorIndex, double amplitude)
{
        auto res = geonkick_set_osc_amplitude(geonkickDsp,
                                              getOscIndex(oscillatorIndex),
                                              amplitude);
	if (res != GEONKICK_OK)
		return false;

	return true;
}

void DspProxy::setOscillatorAsFm(int oscillatorIndex, bool b)
{
        geonkick_osc_set_fm(geonkickDsp,
                            getOscIndex(oscillatorIndex),
                            b);
}

void DspProxy::setOscillatorFmK(int oscillatorIndex, double k)
{
        geonkick_osc_set_fm_k(geonkickDsp,
                              getOscIndex(oscillatorIndex),
                              k);
}

bool DspProxy::isOscillatorAsFm(int oscillatorIndex) const
{
        bool fm = false;
        geonkick_osc_is_fm(geonkickDsp,
                           getOscIndex(oscillatorIndex),
                           &fm);
        return fm;
}

void DspProxy::enableOscillator(int oscillatorIndex, bool enable)
{
        if (enable)
                geonkick_enable_oscillator(geonkickDsp, getOscIndex(oscillatorIndex));
        else
                geonkick_disable_oscillator(geonkickDsp, getOscIndex(oscillatorIndex));
}

double DspProxy::oscillatorAmplitude(int oscillatorIndex) const
{
	gkick_real value = 0;
	if (geonkick_get_osc_amplitude(geonkickDsp,
                                       getOscIndex(oscillatorIndex),
                                       &value) != GEONKICK_OK)
		return 0;
	return value;
}

bool DspProxy::setOscillatorFrequency(int oscillatorIndex, double frequency)
{
	return geonkick_set_osc_frequency(geonkickDsp,
                                          getOscIndex(oscillatorIndex),
                                          frequency) != GEONKICK_OK;
}

double DspProxy::oscillatorFrequency(int oscillatorIndex) const
{
	gkick_real value = 0;
	if (geonkick_get_osc_frequency(geonkickDsp,
                                       getOscIndex(oscillatorIndex),
                                       &value) != GEONKICK_OK) {
                return 0;
        }
	return value;
}

bool DspProxy::setOscillatorPitchShift(int oscillatorIndex, double semitones)
{
	return geonkick_set_osc_pitch_shift(geonkickDsp,
                                            getOscIndex(oscillatorIndex),
                                            semitones) == GEONKICK_OK;
}

double DspProxy::oscillatorPitchShift(int oscillatorIndex) const
{
	gkick_real semitones = 0;
	if (geonkick_get_osc_pitch_shift(geonkickDsp,
                                         getOscIndex(oscillatorIndex),
                                         &semitones) != GEONKICK_OK) {
                return 0;
        }
	return semitones;
}

bool DspProxy::setOscillatorNoiseDensity(int oscillatorIndex, double density)
{
	return geonkick_set_osc_noise_density(geonkickDsp,
                                              getOscIndex(oscillatorIndex),
                                              density) == GEONKICK_OK;
}

double DspProxy::oscillatorNoiseDensity(int oscillatorIndex) const
{
	gkick_real density = 0;
	if (geonkick_get_osc_noise_density(geonkickDsp,
                                           getOscIndex(oscillatorIndex),
                                           &density) != GEONKICK_OK) {
                return 0;
        }
	return density;
}

bool DspProxy::isOscillatorEnabled(int oscillatorIndex) const
{
        int enabled = 0;
        geonkick_is_oscillator_enabled(geonkickDsp,
                                       getOscIndex(oscillatorIndex),
                                       &enabled);
        return enabled;
}

double DspProxy::kickMaxLength(void) const
{
        gkick_real len = 0;
        geonkick_get_max_length(geonkickDsp, &len);
        return len * 1000;
}

bool DspProxy::enableOscillatorFilter(int oscillatorIndex, bool enable)
{
        auto res = geonkick_enbale_osc_filter(geonkickDsp,
                                              getOscIndex(oscillatorIndex),
                                              enable);
        return res == GEONKICK_OK;
}

bool DspProxy::isOscillatorFilterEnabled(int oscillatorIndex) const
{
        int enabled = false;
        geonkick_osc_filter_is_enabled(geonkickDsp,
                                       getOscIndex(oscillatorIndex),
                                       &enabled);
        return enabled;
}

bool DspProxy::setOscillatorFilterType(int oscillatorIndex, FilterType filter)
{
        auto res = geonkick_set_osc_filter_type(geonkickDsp,
                                                getOscIndex(oscillatorIndex),
                                                static_cast<enum gkick_filter_type>(filter));
        return res == GEONKICK_OK;
}

DspProxy::FilterType DspProxy::getOscillatorFilterType(int oscillatorIndex) const
{
        enum gkick_filter_type type;
        geonkick_get_osc_filter_type(geonkickDsp,
                                     getOscIndex(oscillatorIndex),
                                     &type);
        return static_cast<FilterType>(type);
}

bool DspProxy::setOscillatorFilterCutOffFreq(int oscillatorIndex, double frequency)
{
        auto res = geonkick_set_osc_filter_cutoff_freq(geonkickDsp,
                                                       getOscIndex(oscillatorIndex),
                                                       frequency);
        return res == GEONKICK_OK;
}

double DspProxy::getOscillatorFilterCutOffFreq(int oscillatorIndex) const
{
        gkick_real frequency = 0;
        geonkick_get_osc_filter_cutoff_freq(geonkickDsp,
                                            getOscIndex(oscillatorIndex),
                                            &frequency);
        return frequency;
}

bool DspProxy::setOscillatorFilterFactor(int oscillatorIndex, double factor)
{
        auto res = geonkick_set_osc_filter_factor(geonkickDsp,
                                                  getOscIndex(oscillatorIndex),
                                                  factor);
        return res == GEONKICK_OK;
}

double DspProxy::getOscillatorFilterFactor(int oscillatorIndex) const
{
        gkick_real factor = 0;
        geonkick_get_osc_filter_factor(geonkickDsp,
                                       getOscIndex(oscillatorIndex),
                                       &factor);
        return factor;
}

bool DspProxy::isOscDistortionEnabled(int oscillatorIndex) const
{
        bool enabled = false;
        geonkick_osc_distortion_is_enabled(geonkickDsp,
                                           getOscIndex(oscillatorIndex),
                                           &enabled);
        return enabled;
}

bool DspProxy::oscEnableDistortion(int oscillatorIndex, bool b)
{
        auto res = geonkick_osc_distortion_enable(geonkickDsp,
                                                  getOscIndex(oscillatorIndex),
                                                  b);
        return res == GEONKICK_OK;
}

DspProxy::DistortionType DspProxy::getOscDistortionType(int oscillatorIndex) const
{
        enum gkick_distortion_type distortionType = GEONKICK_DISTORTION_SOFT_CLIPPING_TANH;
        geonkick_osc_distortion_get_type(geonkickDsp,
                                         getOscIndex(oscillatorIndex),
                                         &distortionType);
        return static_cast<DspProxy::DistortionType>(distortionType);
}

bool DspProxy::setOscDistortionType(int oscillatorIndex, DistortionType type)
{
        auto res = geonkick_osc_distortion_set_type(geonkickDsp,
                                                    getOscIndex(oscillatorIndex),
                                                    static_cast<enum gkick_distortion_type>(type));
        return res == GEONKICK_OK;
}

bool DspProxy::setOscDistortionInLimiter(int oscillatorIndex, double val)
{
        auto res = geonkick_osc_distortion_set_in_limiter(geonkickDsp,
                                                          getOscIndex(oscillatorIndex),
                                                          val);
        return res == GEONKICK_OK;
}

double DspProxy::getOscDistortionInLimiter(int oscillatorIndex) const
{
        float value = 0.0f;
        geonkick_osc_distortion_get_in_limiter(geonkickDsp,
                                               getOscIndex(oscillatorIndex),
                                               &value);
        return value;
}

bool DspProxy::setOscDistortionOutLimiter(int oscillatorIndex, double val)
{
        auto res = geonkick_osc_distortion_set_out_limiter(geonkickDsp,
                                                           getOscIndex(oscillatorIndex),
                                                           val);
        return res == GEONKICK_OK;
}

double DspProxy::getOscDistortionOutLimiter(int oscillatorIndex) const
{
        float value = 0.0f;
        geonkick_osc_distortion_get_out_limiter(geonkickDsp,
                                                getOscIndex(oscillatorIndex),
                                                &value);
        return value;
}

double DspProxy::getOscDistortionDrive(int oscillatorIndex) const
{
        float value = 0.0;
        geonkick_osc_distortion_get_drive(geonkickDsp,
                                          getOscIndex(oscillatorIndex),
                                          &value);
        return value;
}

bool DspProxy::setOscDistortionDrive(int oscillatorIndex, double val)
{
        auto res = geonkick_osc_distortion_set_drive(geonkickDsp,
                                                     getOscIndex(oscillatorIndex),
                                                     val);
        return res == GEONKICK_OK;
}

double DspProxy::limiterValue() const
{
        gkick_real val = 0;
        geonkick_get_limiter_value(geonkickDsp, &val);
        return val;
}

void DspProxy::setLimiterValue(double value)
{
        geonkick_set_limiter_value(geonkickDsp, value);
}

void DspProxy::kickUpdatedCallback(void *arg,
                                      gkick_real *buff,
                                      size_t size,
                                      size_t id)
{
        GEONKICK_LOG_DEBUG("size[" << id << "] : " << size);
        std::vector<gkick_real> buffer(size, 0);
        std::memcpy(buffer.data(), buff, size * sizeof(gkick_real));
        DspProxy *obj = static_cast<DspProxy*>(arg);
        if (obj)
                obj->updateKickBuffer(std::move(buffer), id);
}

void DspProxy::limiterCallback(void *arg, size_t index, gkick_real val)
{
        if (index >= numberOfInstruments())
                return;

        auto obj = static_cast<DspProxy*>(arg);
        if (obj)
                obj->setLimiterLevelerValue(index, val);
}

void DspProxy::setLimiterLevelerValue(size_t index, double val)
{
        if (index < limiterLevelers.size())
                limiterLevelers[index] = val;
}

double DspProxy::getLimiterLevelerValue(size_t index) const
{
        if (index == static_cast<size_t>(-1))
                index = currentPercussion();
        if (index < limiterLevelers.size())
                return limiterLevelers[index];
        return 0;
}

void DspProxy::updateKickBuffer(const std::vector<gkick_real> &&buffer,
                                size_t id)
{
        GEONKICK_LOG_DEBUG("id: " << id);
        std::lock_guard<std::mutex> lock(dspMutex);
        if (id < numberOfInstruments()) {
                GEONKICK_LOG_DEBUG("kickBuffers[id] = buffer" << id);
                kickBuffers[id] = buffer;
        }
        if (eventQueue && id == currentPercussion()) {
                auto act = std::make_unique<RkAction>();
                act->setCallback([&](void){ kickUpdated(); });
                eventQueue->postAction(std::move(act));
                GEONKICK_LOG_DEBUG("eventQueue->postAction / kickUpdated()");
        }
}

std::vector<gkick_real> DspProxy::getInstrumentBuffer(int id) const
{
        {
                std::lock_guard<std::mutex> lock(dspMutex);
                if (static_cast<decltype(kickBuffers.size())>(id) < kickBuffers.size())
                        return kickBuffers[id];
        }
        return std::vector<gkick_real>();
}

std::vector<gkick_real> DspProxy::getKickBuffer() const
{
        std::lock_guard<std::mutex> lock(dspMutex);
        return kickBuffers[currentPercussion()];
}

int DspProxy::getSampleRate() const
{
        int sample_rate;
        if (geonkick_get_sample_rate(geonkickDsp, &sample_rate) != GEONKICK_OK)
                sample_rate = 0;

        return sample_rate;
}

// This function is called only from the audio thread.
void DspProxy::setKeyPressed(bool b, int note, int velocity)
{
        geonkick_key_pressed(geonkickDsp, b, note, velocity);
}

void DspProxy::playKick(int id)
{
        if (id < 0)
                id = currentPercussion();
        geonkick_play(geonkickDsp, id);
}

void DspProxy::playSamplePreview()
{
        geonkick_play_sample_preview(geonkickDsp);
}

void DspProxy::process(float** out, size_t offset, size_t size)
{
        geonkick_audio_process(geonkickDsp, out, offset, size);
}

bool DspProxy::enableDistortion(bool enable)
{
        auto res = geonkick_distortion_enable(geonkickDsp, enable);
        return res == GEONKICK_OK;
}

bool DspProxy::isDistortionEnabled() const
{
        bool enabled = false;
        geonkick_distortion_is_enabled(geonkickDsp, &enabled);
        return enabled;
}

bool DspProxy::setDistortionType(DspProxy::DistortionType type)
{
        auto res = geonkick_distortion_set_type(geonkickDsp, static_cast<enum gkick_distortion_type>(type));
        return res == GEONKICK_OK;
}

DspProxy::DistortionType DspProxy::getDistortionType() const
{
        enum gkick_distortion_type type = GEONKICK_DISTORTION_SOFT_CLIPPING_TANH;
        geonkick_distortion_get_type(geonkickDsp, &type);
        return static_cast<DspProxy::DistortionType>(type);
}

bool DspProxy::setDistortionInLimiter(double value)
{
        auto res = geonkick_distortion_set_in_limiter(geonkickDsp, value);
        return res == GEONKICK_OK;
}

double DspProxy::getDistortionInLimiter() const
{
        float value = 0.0f;
        geonkick_distortion_get_in_limiter(geonkickDsp, &value);
        return value;
}

bool DspProxy::setDistortionOutLimiter(double value)
{
        auto res = geonkick_distortion_set_out_limiter(geonkickDsp, value);
        return res == GEONKICK_OK;
}

double DspProxy::getDistortionOutLimiter(void) const
{
        gkick_real value = 0;
        geonkick_distortion_get_out_limiter(geonkickDsp, &value);
        return value;
}

bool DspProxy::setDistortionDrive(double drive)
{
        auto res = geonkick_distortion_set_drive(geonkickDsp, drive);
        return res == GEONKICK_OK;
}

double DspProxy::getDistortionDrive(void) const
{
        gkick_real drive = 0;
        geonkick_distortion_get_drive(geonkickDsp, &drive);
        return drive;
}

void DspProxy::registerCallbacks(bool b)
{
        if (b) {
                geonkick_set_kick_buffer_callback(geonkickDsp,
                                                  &DspProxy::kickUpdatedCallback,
                                                  this);
                geonkick_set_kick_limiter_callback(geonkickDsp,
                                                   &DspProxy::limiterCallback,
                                                   this);
        } else {
                geonkick_set_kick_buffer_callback(geonkickDsp, NULL, NULL);
                geonkick_set_kick_limiter_callback(geonkickDsp, NULL, NULL);
        }
}

bool DspProxy::isJackEnabled() const
{
        return jackEnabled;
}

void DspProxy::setStandalone(bool b)
{
        standaloneInstance = b;
}

bool DspProxy::isStandalone() const
{
        return standaloneInstance;
}

void DspProxy::triggerSynthesis()
{
        geonkick_enable_synthesis(geonkickDsp, true);
}

void DspProxy::setLayer(Layer layer)
{
        currentLayer = layer;
}

DspProxy::Layer DspProxy::layer() const
{
        return currentLayer;
}

void DspProxy::setLayerAmplitude(Layer layer, double amplitude)
{
        geonkick_group_set_amplitude(geonkickDsp,
                                     static_cast<size_t>(layer),
                                     amplitude);
}

double DspProxy::getLayerAmplitude(Layer layer) const
{
        gkick_real amplitude = 0;
        geonkick_group_get_amplitude(geonkickDsp,
                                     static_cast<size_t>(layer),
                                     &amplitude);
        return amplitude;
}

void DspProxy::enbaleLayer(Layer layer, bool enable)
{
        geonkick_enable_group(geonkickDsp,
                              static_cast<int>(layer),
                              enable);
}

bool DspProxy::isLayerEnabled(Layer layer) const
{
        bool enabled = false;
        geonkick_group_enabled(geonkickDsp,
                               static_cast<int>(layer),
                               &enabled);
        return enabled;
}

int DspProxy::getOscIndex(int index) const
{
        return index + GKICK_OSC_GROUP_SIZE * static_cast<int>(currentLayer);
}

std::filesystem::path
DspProxy::currentWorkingPath(const std::string &key) const
{
        auto it = workingPaths.find(key);
        if (it != workingPaths.end())
                return it->second;
        return getSettings("GEONKICK_CONFIG/HOME_PATH");
}

void DspProxy::setCurrentWorkingPath(const std::string &key,
                                        const std::filesystem::path &path)
{
        workingPaths[key] = path;
}

bool DspProxy::setPercussionLimiter(size_t id, double val)
{
	return geonkick_instrument_set_limiter(geonkickDsp, id, val) == GEONKICK_OK;
}

double DspProxy::instrumentLimiter(size_t id) const
{
        gkick_real val = 0.0f;
        geonkick_instrument_get_limiter(geonkickDsp, id, &val);
        return val;
}

bool DspProxy::mutePercussion(size_t id, bool b)
{
        return geonkick_instrument_mute(geonkickDsp, id, b) == GEONKICK_OK;
}

bool DspProxy::isPercussionMuted(size_t id) const
{
        bool muted = false;
        geonkick_instrument_is_muted(geonkickDsp, id, &muted);
        return muted;
}

bool DspProxy::soloPercussion(size_t id, bool b)
{
        return geonkick_instrument_solo(geonkickDsp, id, b) == GEONKICK_OK;
}

bool DspProxy::isPercussionSolo(size_t id) const
{
        bool solo = false;
        geonkick_instrument_is_solo(geonkickDsp, id, &solo);
        return solo;
}

bool DspProxy::enableNoteOff(size_t id, bool b)
{
        return geonkick_instrument_enable_note_off(geonkickDsp, id, b) == GEONKICK_OK;
}

bool DspProxy::isNoteOffEnabled(size_t id) const
{
        bool enabled = false;
        geonkick_instrument_note_off_enabled(geonkickDsp, id, &enabled);
        return enabled;
}

unsigned int DspProxy::numberOfChokeGroups([[maybe_unused]] size_t id) const
{
        return geonkick_number_of_choke_groups(geonkickDsp);
}

bool DspProxy::setChokeGroup(size_t id, int group)
{
        auto res = geonkick_set_choke_group(geonkickDsp,
                                            id,
                                            static_cast<enum gkick_choke_group>(group));
        return res == GEONKICK_OK;
}

int DspProxy::getChokeGroup(size_t id) const
{
        return static_cast<int>(geonkick_get_choke_group(geonkickDsp, id));
}

int DspProxy::getUnusedPercussion() const
{
        int index;
        geonkick_unused_instrument(geonkickDsp, &index);
        return index;
}

bool DspProxy::enablePercussion(int index, bool enable)
{
        auto res = geonkick_enable_instrument(geonkickDsp, index, enable);
        return res == GEONKICK_OK;
}

bool DspProxy::isPercussionEnabled(int index) const
{
        bool enabled = false;
        geonkick_is_instrument_enabled(geonkickDsp, index, &enabled);
        return enabled;
}

size_t DspProxy::enabledPercussions() const
{
        auto n = numberOfInstruments();
        size_t enabled  = 0;
        for (decltype(n) i = 0; i < n; i++) {
                if (isPercussionEnabled(i))
                        enabled++;
        }
        return enabled;
}

bool DspProxy::setPercussionPlayingKey(int index, int key)
{
        auto res = geonkick_set_playing_key(geonkickDsp,
                                            index,
                                            key);
        GEONKICK_LOG_DEBUG("index " << index << " key:" << key);
        return res == GEONKICK_OK;
}

int DspProxy::getPercussionPlayingKey(int index) const
{
        signed char key = -1;
        geonkick_get_playing_key(geonkickDsp,
                                 index,
                                 &key);
        return key;
}

int DspProxy::instrumentsReferenceKey() const
{
        // MIDI key A4
        return 69;
}

bool DspProxy::setPercussionChannel(int index, size_t channel)
{
        auto res = geonkick_set_instrument_channel(geonkickDsp,
                                                   index,
                                                   channel);
        return res == GEONKICK_OK;
}

int DspProxy::getPercussionChannel(int index) const
{
        size_t channel;
        auto res = geonkick_get_instrument_channel(geonkickDsp,
                                                   index,
                                                   &channel);
        if (res != GEONKICK_OK)
                return -1;
        return channel;
}

bool DspProxy::setPercussionMidiChannel(int index, size_t channel)
{
        auto res = geonkick_set_midi_channel(geonkickDsp,
                                             index,
                                             channel);
        return res == GEONKICK_OK;
}

int DspProxy::getPercussionMidiChannel(int index) const
{
        signed char channel;
        auto res = geonkick_get_midi_channel(geonkickDsp,
                                             index,
                                             &channel);
        if (res != GEONKICK_OK)
                return -1;
        return channel;
}

bool DspProxy::forceMidiChannel(size_t channel, bool force)
{
        auto res = geonkick_force_midi_channel(geonkickDsp, channel, force);
        return res == GEONKICK_OK;
}

bool DspProxy::isMidiChannelForced() const
{
        bool forced = false;
        geonkick_ged_forced_midi_channel(geonkickDsp, nullptr, &forced);
        return forced;
}

bool DspProxy::setPercussionName(int index, const std::string &name)
{
        if (name.empty())
                return false;

        auto res = geonkick_set_instrument_name(geonkickDsp,
                                                index,
                                                name.c_str(),
                                                name.size());
        return res == GEONKICK_OK;
}

std::string DspProxy::getPercussionName(int index) const
{
        auto n = numberOfInstruments();
        if (index > -1 && index < static_cast<decltype(index)>(n)) {
                char name[30];
                geonkick_get_instrument_name(geonkickDsp,
                                             index,
                                             name,
                                             sizeof(name));
                return name;
        }
        return "";
}

void DspProxy::setSettings(const std::string &key, const std::string &value)
{
        uiSettings->setSettings(key, value);
}

std::string DspProxy::getSettings(const std::string &key) const
{
        return uiSettings->getSettings(key);
}

void DspProxy::tuneAudioOutput(int id, bool tune)
{
        geonkick_tune_audio_output(geonkickDsp,
                                   id,
                                   tune);
}

bool DspProxy::isAudioOutputTuned(int id) const
{
        bool tune = false;
        geonkick_is_audio_output_tuned(geonkickDsp,
                                       id,
                                       &tune);
        return tune;
}

bool DspProxy::setCurrentPercussion(int index)
{
        auto res = geonkick_set_current_instrument(geonkickDsp, index);
        return res == GEONKICK_OK;
}

size_t DspProxy::currentPercussion() const
{
        size_t index = 0;
        geonkick_get_current_instrument(geonkickDsp, &index);
        return index;
}

bool DspProxy::setOscillatorSample(const std::string &file,
                                      int oscillatorIndex)
{
        int sRate = Geonkick::defaultSampleRate;
        geonkick_get_sample_rate(geonkickDsp, &sRate);
        auto sampleData = loadSample(file,
                                     kickMaxLength() / 1000,
                                     sRate,
                                     1);
        if (!sampleData.empty()) {
                setOscillatorSample(sampleData, oscillatorIndex);
                return true;
        }
        return false;
}

void DspProxy::setOscillatorSample(const std::vector<float> &sample,
                                      int oscillatorIndex)
{
        geonkick_set_osc_sample(geonkickDsp,
                                getOscIndex(oscillatorIndex),
                                sample.data(),
                                sample.size());
}

std::vector<float>
DspProxy::getOscillatorSample(int oscillatorIndex) const
{
        gkick_real *data = nullptr;
        size_t size = 0;
        geonkick_get_osc_sample(geonkickDsp,
                                getOscIndex(oscillatorIndex),
                                &data,
                                &size);
        if (data)
                return std::vector<float>(data, data + size);
        return {};
}

std::vector<gkick_real> DspProxy::loadSample(const std::string &file,
                                                double length,
                                                int sampleRate,
                                                int channels)
{
        GEONKICK_UNUSED(channels);

        SF_INFO sndinfo;
        memset(&sndinfo, 0, sizeof(sndinfo));
        SNDFILE *sndFile = sf_open(file.c_str(), SFM_READ, &sndinfo);
        if (!sndFile) {
                GEONKICK_LOG_ERROR("can't open sample file");
                return std::vector<gkick_real>();
        }

        auto formatType = sndinfo.format & SF_FORMAT_TYPEMASK;
        if (formatType != SF_FORMAT_FLAC
            && formatType != SF_FORMAT_WAV
            && formatType != SF_FORMAT_FLAC
            && formatType != SF_FORMAT_WAVEX
            && formatType != SF_FORMAT_OGG) {
                GEONKICK_LOG_ERROR(std::hex << "unsupported audio format");
                sf_close(sndFile);
                return std::vector<gkick_real>();
        }

        std::vector<float> data(sndinfo.samplerate * length * sndinfo.channels, 0.0f);
        auto n = sf_read_float(sndFile, data.data(), data.size());
        sf_close(sndFile);
        if (static_cast<decltype(data.size())>(n) < 1) {
                GEONKICK_LOG_ERROR("error on reading samples");
                return std::vector<gkick_real>();
        } else if (static_cast<decltype(data.size())>(n) < data.size()) {
                GEONKICK_LOG_DEBUG("read less then data, resize to " << n);
                data.resize(n);
        }

        if (sndinfo.channels > 1) {
                GEONKICK_LOG_DEBUG("multichannel file, get only the first channel");
                for (decltype(data.size()) i = 0; i < data.size(); i += sndinfo.channels)
                        data[i / sndinfo.channels] = data[i];
                data.resize(data.size() / sndinfo.channels);
        }

        float max = std::fabs(*std::max_element(data.begin(), data.end(),
                                                [](float a, float b){ return fabs(a) < fabs(b); }));
        if (max > std::numeric_limits<float>::min()) {
                float k = 1.0f / max;
                for (auto &v: data)
                        v *= k;
        }

        if (sampleRate != sndinfo.samplerate) {
                GEONKICK_LOG_DEBUG("different sample rate " << sndinfo.samplerate
                                   << ", resample to " << sampleRate);
                float f = static_cast<float>(sndinfo.samplerate) / sampleRate;
                float x = 0.0f;
                std::vector<float> out_data;
                for (decltype(data.size()) i = 0; i < data.size() - 1;) {
                        float d = x - i;
                        float val = data[i] * (1.0f - d) + data[i + 1] * d;
                        x += f;
                        i = x;
                        out_data.push_back(val);
                }
                return out_data;
        }

        return data;
}

void DspProxy::setKitName(const std::string &name)
{
        kitName = name;
}

std::string DspProxy::getKitName() const
{
        return kitName;
}

void DspProxy::setKitAuthor(const std::string &author)
{
        kitAuthor = author;
}

std::string DspProxy::getKitAuthor() const
{
        return kitAuthor;
}

void DspProxy::setKitUrl(const std::string &url)
{
        kitUrl = url;
}

std::string DspProxy::getKitUrl() const
{
        return kitUrl;
}

void DspProxy::copyToClipboard()
{
        clipboardPercussion = getPercussionState();
}

void DspProxy::pasteFromClipboard()
{
        if (clipboardPercussion) {
                auto state = std::make_unique<PercussionState>(*clipboardPercussion);
                auto currId = currentPercussion();
                state->setId(currId);
                state->setName(getPercussionName(currId));
                state->setPlayingKey(getPercussionPlayingKey(currId));
                state->setChannel(getPercussionChannel(currId));
                state->setMidiChannel(getPercussionMidiChannel(currId));
                state->setMute(isPercussionMuted(currId));
                state->setSolo(isPercussionSolo(currId));
                setPercussionState(state);
        }
}

void DspProxy::notifyUpdateGraph()
{
        if (eventQueue) {
                auto act = std::make_unique<RkAction>();
                act->setCallback([&](void){ action kickUpdated(); });
                eventQueue->postAction(std::move(act));
        }
}

void DspProxy::notifyUpdateParameters()
{
        if (eventQueue) {
                auto act = std::make_unique<RkAction>();
                act->setCallback([&](void){ action stateChanged(); });
                eventQueue->postAction(std::move(act));
        }
}

void DspProxy::notifyUpdateGui()
{
        if (eventQueue) {
                auto act = std::make_unique<RkAction>();
                act->setCallback([&](void){
                                action kickUpdated();
                                action stateChanged();
                        });
                eventQueue->postAction(std::move(act));
        }
}

void DspProxy::notifyPercussionUpdated(int id)
{
        if (eventQueue) {
                auto act = std::make_unique<RkAction>();
                act->setCallback([this, id](void){
                                GEONKICK_LOG_DEBUG("update instrument :" << id);
                                action instrumentUpdated(id);
                        });
                eventQueue->postAction(std::move(act));
        }
}

void DspProxy::notifyKitUpdated()
{
        if (eventQueue) {
                auto act = std::make_unique<RkAction>();
                act->setCallback([&](void){
                                action kitUpdated();
                        });
                eventQueue->postAction(std::move(act));
        }
}

std::vector<int> DspProxy::ordredPercussionIds() const
{
        return instrumentIdList;
}

void DspProxy::removeOrderedPercussionId(int id)
{
        for (auto it = instrumentIdList.begin(); it != instrumentIdList.end(); ++it) {
                if (*it == id) {
                        instrumentIdList.erase(it);
                        break;
                }
        }
}

void DspProxy::addOrderedPercussionId(int id)
{
        removeOrderedPercussionId(id);
        instrumentIdList.push_back(id);
}

void DspProxy::clearOrderedPercussionIds()
{
        instrumentIdList.clear();
}

bool DspProxy::moveOrdrepedPercussionId(int index, int n)
{
        if (index < 0)
                return false;

        auto size = instrumentIdList.size();
        for (decltype(instrumentIdList.size()) i = 0; i < size; i++) {
                if (instrumentIdList[i] == index) {
                        auto newId = static_cast<decltype(n)>(i) + n;
                        if (newId > -1 && static_cast<decltype(size)>(newId) < size) {
                                std::swap(instrumentIdList[i], instrumentIdList[newId]);
                                return true;
                        }
                }
        }
        return false;
}

void DspProxy::loadPresets()
{
        std::unordered_set<std::string> prestsPaths;
        auto presetsPathSufix = std::filesystem::path(GEONKICK_APP_NAME) / "presets";
        std::filesystem::path presetsPath = getSettings("GEONKICK_CONFIG/USER_PRESETS_PATH");
        prestsPaths.insert(presetsPath.string());

#ifdef GEONKICK_DATA_DIR
        prestsPaths.insert((std::filesystem::path(GEONKICK_DATA_DIR) / presetsPathSufix).string());
#endif // GEONKICK_DATA_DIR

        const char *dataDirs = std::getenv("XDG_DATA_DIRS");
        if (dataDirs == nullptr || *dataDirs == '\0') {
                prestsPaths.insert((std::filesystem::path("/usr/share") / presetsPathSufix).string());
                prestsPaths.insert((std::filesystem::path("/usr/local/share") / presetsPathSufix).string());
        } else {
                std::stringstream ss(dataDirs);
                std::string path;
                while (std::getline(ss, path, ':'))
                        prestsPaths.insert((std::filesystem::path(path) / presetsPathSufix).string());
        }

        for (const auto &path: prestsPaths) {
                try {
                        if (std::filesystem::exists(path))
                                loadPresetsFolders(path);
                } catch(const std::exception& e) {
                        GEONKICK_LOG_ERROR("error on reading path: " << path << ": " << e.what());
                }
        }
}

void DspProxy::loadPresetsFolders(const std::filesystem::path &path)
{
        try {
                for (const auto &entry : std::filesystem::directory_iterator(path)) {
                        if (!entry.path().empty() && std::filesystem::is_directory(entry.path())) {
                                auto presetFolder = std::make_unique<PresetFolder>(entry.path());
                                if (presetFolder->numberOfPresets() > 0)
                                        presetsFoldersList.push_back(std::move(presetFolder));
                        }
                }
        } catch(...) {
                GEONKICK_LOG_ERROR("error on reading path: " << path);
        }
}

void DspProxy::setupPaths()
{
	DesktopPaths desktopPaths;
	setSettings("GEONKICK_CONFIG/HOME_PATH", desktopPaths.getHomePath().string());
	setSettings("GEONKICK_CONFIG/USER_PRESETS_PATH", desktopPaths.getUserPresetsPath().string());
	setSettings("GEONKICK_CONFIG/USER_DATA_PATH", desktopPaths.getDataPath().string());

	try {
                if (!std::filesystem::exists(desktopPaths.getDataPath())) {
                        if (!std::filesystem::create_directories(desktopPaths.getDataPath())) {
                                GEONKICK_LOG_ERROR("can't create path " << desktopPaths.getDataPath());
                                return;
                        }
                }

                if (!std::filesystem::exists(desktopPaths.getUserPresetsPath())) {
                        if (!std::filesystem::create_directories(desktopPaths.getUserPresetsPath())) {
                                GEONKICK_LOG_ERROR("can't create path " << desktopPaths.getUserPresetsPath());
                                return;
                        }
                }
	} catch(const std::exception& e) {
                GEONKICK_LOG_ERROR("error on setup user data paths: " << e.what());
        }
}

PresetFolder* DspProxy::getPresetFolder(size_t index) const
{
        if (index < presetsFoldersList.size())
                return presetsFoldersList[index].get();
        return nullptr;
}

size_t DspProxy::numberOfPresetFolders() const
{
        return presetsFoldersList.size();
}

UiSettings* DspProxy::getUiSettings() const
{
        return uiSettings.get();
}

void DspProxy::setState(const std::string &data)
{
        rapidjson::Document document;
        document.Parse(data.c_str());
        if (!document.IsObject())
                return;

        for (const auto &m: document.GetObject()) {
                if (m.name == "UiSettings" && m.value.IsObject())
                        uiSettings->fromJsonObject(m.value);
                if (m.name == "KitState" && m.value.IsObject()) {
                        auto kitState = std::make_unique<KitState>();
                        kitState->fromJsonObject(m.value);
                        setKitState(kitState);
                }
        }
}

std::string DspProxy::getState() const
{
        std::ostringstream jsonStream;
        jsonStream << "{\"UiSettings\": " << std::endl;
        jsonStream << uiSettings->toJson() << ", " << std::endl;
        jsonStream << "\"KitState\": " << std::endl;
        jsonStream << getKitState()->toJson() << std::endl;
        jsonStream << "}" << std::endl;
        return jsonStream.str();
}

std::vector<gkick_real> DspProxy::setPreviewSample(const std::string &file)
{
        int sRate = Geonkick::defaultSampleRate;
        geonkick_get_sample_rate(geonkickDsp, &sRate);
        std::vector<gkick_real> sampleData = loadSample(file,
                                                        kickMaxLength() / 1000,
                                                        sRate,
                                                        1);
        if (!sampleData.empty()) {
                geonkick_set_preview_sample(geonkickDsp, sampleData.data(), sampleData.size());
                return sampleData;
        }
        return std::vector<float>();
}

void DspProxy::setSamplePreviewLimiter(double val)
{
        geonkick_set_sample_preview_limiter(geonkickDsp, val);
}

double DspProxy::samplePreviewLimiter() const
{
        gkick_real val = 0;
        geonkick_get_sample_preview_limiter(geonkickDsp, &val);
        return val;
}

double DspProxy::getScaleFactor() const
{
        return scaleFactor;
}

void DspProxy::setScaleFactor(double factor)
{
        scaleFactor = factor;
        GeonkickConfig config;
        config.setScaleFactor(scaleFactor);
        config.save();
}

DspProxyHumanizer* DspProxy::getHumanizer() const
{
        return dspProxyHumanizer.get();
}

void DspProxy::waitPlayingReady()
{
        return geonkick_wait_playing_ready();
}
