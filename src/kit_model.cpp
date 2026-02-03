/**
 * File name: kit.cpp
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

#include "kit_model.h"
#include "DspProxy.h"
#include "InstrumentState.h"
#include "InstrumentModel.h"
#include "kit_state.h"
#include "ExportToSfz.h"
#include "ExportSoundData.h"
#include "GeonkickModel.h"
#include "preset.h"

#include "RkAction.h"
#include "RkEventQueue.h"

KitModel::KitModel(GeonkickModel *parent)
        : RkObject(parent)
        , geonkickModel{parent}
        , dspProxy{geonkickModel->getDspProxy()}
        , midiKeys {"A4", "A#4", "B4", "C5",
                   "C#5", "D5", "D#5", "E5",
                   "F5", "F#5", "G5", "G#5",
                   "A5", "A#5", "B5", "C6", "Any"}
{
        loadModelData();
        RK_ACT_BIND(dspProxy, kitUpdated, RK_ACT_ARGS(), this, loadModelData());
        RK_ACT_BIND(dspProxy, instrumentUpdated, RK_ACT_ARGS(size_t id), this, updatePercussion(getIndex(id)));
        RK_ACT_BIND(dspProxy, kickUpdated,
                    RK_ACT_ARGS(),
                    this,
                    updatePercussion(getIndex(dspProxy->currentPercussion())));
}

void KitModel::updatePercussion(PercussionIndex index)
{
        if (isValidIndex(index)) {
                action instrumentUpdated(instrumentsList[index]);
                action instrumentsList[index]->modelUpdated();
        }
}

bool KitModel::isValidIndex(PercussionIndex index)
{
        return index > -1 && static_cast<size_t>(index) < instrumentsList.size();
}

bool KitModel::enableInstrument(PercussionIndex index, bool b)
{
        if (isValidIndex(index) && dspProxy->enablePercussion(instrumentId(index))) {
                action instrumentEnabled(instrumentsList[getIndex(index)]);
                return true;
        }
        return false;
}

bool KitModel::isInstrumentEnabled(PercussionIndex index) const
{
        return dspProxy->isPercussionEnabled(instrumentId(index));
}

void KitModel::selectPercussion(PercussionIndex index)
{
        if (isValidIndex(index) && dspProxy->setCurrentPercussion(instrumentId(index))) {
                dspProxy->notifyUpdateGui();
                action instrumentSelected(instrumentsList[index]);
        }
}

bool KitModel::isPercussionSelected(PercussionIndex index) const
{
        return static_cast<size_t>(instrumentId(index)) == dspProxy->currentPercussion();
}

KitModel::PercussionIndex KitModel::selectedPercussion() const
{
        return getIndex(dspProxy->currentPercussion());
}

PercussionModel* KitModel::currentPercussion() const
{
        return instrumentsList[getIndex(dspProxy->currentPercussion())];
}

size_t KitModel::numberOfChannels() const
{
        return dspProxy->numberOfInstruments();
}

int KitModel::instrumentChannel(PercussionIndex index) const
{
        return dspProxy->getPercussionChannel(instrumentId(index));
}

bool KitModel::setPercussionChannel(PercussionIndex index, int channel)
{
        return dspProxy->setPercussionChannel(instrumentId(index), channel);
}

size_t KitModel::numberOfMidiChannels() const
{
        return dspProxy->numberOfMidiChannels();
}

int KitModel::instrumentMidiChannel(PercussionIndex index) const
{
        return dspProxy->getPercussionMidiChannel(instrumentId(index));
}

bool KitModel::setPercussionMidiChannel(PercussionIndex index, int channel)
{
        return dspProxy->setPercussionMidiChannel(instrumentId(index), channel);
}

bool KitModel::setPercussionName(PercussionIndex index, const std::string &name)
{
        if (dspProxy->setPercussionName(instrumentId(index), name)) {
                dspProxy->notifyUpdateGui();
                return true;
        }
        return false;
}

std::string KitModel::instrumentName(PercussionIndex index) const
{
        return dspProxy->getPercussionName(instrumentId(index));
}

size_t KitModel::keysNumber() const
{
        return midiKeys.size();
}

std::string KitModel::keyName(KeyIndex index) const
{
        if (index >= 0 && static_cast<size_t>(index) < midiKeys.size())
                return midiKeys[index];
        return std::string();
}

bool KitModel::setPercussionKey(PercussionIndex index, KeyIndex keyIndex)
{
        if (!isValidIndex(index))
                return false;
        if (dspProxy->setPercussionPlayingKey(instrumentId(index), keyIndex)) {
                action instrumentUpdated(instrumentsList[index]);
                return true;
        }
        return false;
}

KitModel::KeyIndex KitModel::instrumentKey(PercussionIndex index) const
{
        return dspProxy->getPercussionPlayingKey(instrumentId(index));
}

double KitModel::getInstrumentMaxLength([[maybe_unused]] PercussionIndex index) const
{
        return dspProxy->kickMaxLength();
}

bool KitModel::setInstrumentLength([[maybe_unused]] PercussionIndex index, double val)
{
        return dspProxy->setKickLength(val);
}

double KitModel::getInstrumentLength([[maybe_unused]] PercussionIndex index) const
{
        return dspProxy->kickLength();
}

bool KitModel::setInstrumentAmplitude([[maybe_unused]] PercussionIndex index, double val)
{
        return dspProxy->setKickAmplitude(val);
}

double KitModel::getInstrumentAmplitude([[maybe_unused]] PercussionIndex index) const
{
        return dspProxy->kickAmplitude();
}

void KitModel::playPercussion(PercussionIndex index)
{
        dspProxy->playKick(instrumentId(index));
}

bool KitModel::setPercussionLimiter(PercussionIndex index, int value)
{
        double logVal = (75.0 / 100) * value - 55;
        auto realVal = pow(10, logVal / 20);
        if (dspProxy->setPercussionLimiter(instrumentId(index), realVal)) {
                action limiterUpdated(index);
                return true;
        }
        return false;
}

int KitModel::instrumentLimiter(PercussionIndex index) const
{
        auto realVal = dspProxy->instrumentLimiter(instrumentId(index));
        double logVal = 20 * log10(realVal);
        int val = (logVal + 55.0) * 100.0 / 75;
        return  val;
}

int KitModel::instrumentLeveler(PercussionIndex index) const
{
        auto realVal = dspProxy->getLimiterLevelerValue(instrumentId(index));
        // add small delta to avoid -inf for zero value
        double logVal = 20 * log10(realVal + 0.000000001);
        int val = (logVal + 55.0) * 100.0 / 75;
        if (val < 0)
                val = 0;
        return val;
}

bool KitModel::mutePercussion(PercussionIndex index, bool b)
{
        return dspProxy->mutePercussion(instrumentId(index), b);
}

bool KitModel::isPercussionMuted(PercussionIndex index) const
{
        return dspProxy->isPercussionMuted(instrumentId(index));
}

bool KitModel::soloPercussion(PercussionIndex index, bool b)
{
        return dspProxy->soloPercussion(instrumentId(index), b);
}

bool KitModel::isPercussionSolo(PercussionIndex index) const
{
        return dspProxy->isPercussionSolo(instrumentId(index));
}

void KitModel::loadModelData()
{
        for (auto &per: instrumentsList)
                delete per;
        instrumentsList.clear();
        for (const auto &id : dspProxy->ordredPercussionIds()) {
                auto model = new PercussionModel(this, id);
                instrumentsList.push_back(model);
        }
        action modelUpdated();
}

bool KitModel::open(const std::string &file)
{
        auto kit = std::make_unique<KitState>();
        if (!kit->open(file)) {
                GEONKICK_LOG_ERROR("can't open kit, the preset might be wrong or corrupted");
                return false;
        }

        auto filePath = std::filesystem::path(file);
        auto path = filePath.has_parent_path() ? filePath.parent_path() : filePath;
        if (!dspProxy->setKitState(kit)) {
                GEONKICK_LOG_ERROR("can't set kit state");
                return false;
        } else {
                dspProxy->setCurrentWorkingPath("OpenKit", path);
                loadModelData();
                dspProxy->notifyUpdateGui();
                action modelUpdated();
        }
        return true;
}

bool KitModel::save(const std::string &file)
{
        auto state = dspProxy->getKitState();
        if (!state || !state->save(file)) {
                GEONKICK_LOG_ERROR("can't save kit state");
                return false;
        }
        auto filePath = std::filesystem::path(file);
        auto path = filePath.has_parent_path() ? filePath.parent_path() : filePath;
        dspProxy->setCurrentWorkingPath("SaveKit", path);
        return true;
}

void KitModel::addNewPercussion()
{
        int newId = dspProxy->getUnusedPercussion();
        if (newId < 0)
                return;

        auto state = dspProxy->getDefaultPercussionState();
        state->setId(newId);
        state->enable(true);
        dspProxy->setPercussionState(state);
        dspProxy->addOrderedPercussionId(newId);
        auto model = new PercussionModel(this, newId);
        instrumentsList.push_back(model);
        action instrumentAdded(model);
}

void KitModel::copyPercussion(PercussionIndex index)
{
        if (!isValidIndex(index))
                return;

        auto newId = dspProxy->getUnusedPercussion();
        if (newId < 0)
                return;

        auto state = dspProxy->getPercussionState(instrumentId(index));
        if (state) {
                state->setId(newId);
                state->enable(true);
                dspProxy->setPercussionState(state);
                dspProxy->addOrderedPercussionId(newId);
                auto model = new PercussionModel(this, newId);
                instrumentsList.push_back(model);
                action instrumentAdded(model);
        }
}

void KitModel::removePercussion(PercussionIndex index)
{
        if (!isValidIndex(index) || instrumentsList.size() < 2)
                return;

        for (auto it = instrumentsList.begin(); it != instrumentsList.end(); ++it) {
                if ((*it)->index() == index && dspProxy->enablePercussion(instrumentId(index), false)) {
                        action instrumentRemoved(index);
                        bool notify = (*it)->isSelected();
                        delete *it;
                        instrumentsList.erase(it);
                        dspProxy->removeOrderedPercussionId(instrumentId(index));
                        if (notify) {
                                dspProxy->setCurrentPercussion(instrumentId(0));
                                action selectPercussion(0);
                        }
                        break;
                }
        }

        for (const auto & per: instrumentsList)
                action per->modelUpdated();
}

void KitModel::moveSelectedPercussion(bool down)
{
        auto currentIndex = getIndex(dspProxy->currentPercussion());
        auto nextIndex = currentIndex + (down ? 1 : -1);
        if (isValidIndex(currentIndex) && isValidIndex(nextIndex)) {
                bool res = dspProxy->moveOrdrepedPercussionId(dspProxy->currentPercussion(), down ? 1 : -1);
                if (res) {
                        instrumentsList[currentIndex]->setId(instrumentId(currentIndex));
                        instrumentsList[nextIndex]->setId(instrumentId(nextIndex));
                        selectPercussion(nextIndex);
                }
        }
}

size_t KitModel::instrumentNumber() const
{
        return dspProxy->ordredPercussionIds().size();
}

size_t KitModel::maxPercussionNumber() const
{
        return dspProxy->numberOfInstruments();
}

int KitModel::instrumentId(int index) const
{
        const auto &ids = dspProxy->ordredPercussionIds();
        if (index < 0 || index >= static_cast<decltype(index)>(ids.size()))
                return -1;
        return ids[index];
}

KitModel::PercussionIndex KitModel::getIndex(int id) const
{
        const auto &ids = dspProxy->ordredPercussionIds();
        auto it = std::find(ids.begin(), ids.end(), id);
        if (it != ids.end())
                return it - ids.begin();
        return -1;
}

std::filesystem::path
KitModel::workingPath(const std::string &key) const
{
        return dspProxy->currentWorkingPath(key);
}

std::filesystem::path
KitModel::getHomePath() const
{
        return dspProxy->getSettings("GEONKICK_CONFIG/HOME_PATH");
}

const std::vector<PercussionModel*>& KitModel::instrumentModels() const
{
        return instrumentsList;
}

DspProxy* KitModel::getDspProxy() const
{
        return dspProxy;
}

bool KitModel::doExport(const std::string &file, const ExportInfo &info) const
{
        using ExportFormat = ExportAbstract::ExportFormat;
        switch (info.format) {
        case ExportFormat::Sfz:
        {
                 ExportToSfz toSfz(this, file);
                 return toSfz.doExport();
        }
        case ExportFormat::Flac:
        case ExportFormat::Wav:
        case ExportFormat::Ogg:
        {
                auto currentIndex = getIndex(dspProxy->currentPercussion());
                ExportSoundData exportToAudioFile(file,
                                                  instrumentData(currentIndex),
                                                  info.format);
                exportToAudioFile.setBitDepth(info.bitDepth);
                exportToAudioFile.setNumberOfChannels(info.channels);
                exportToAudioFile.setSampleRate(getDspProxy()->getSampleRate());
                return exportToAudioFile.doExport();
        }
        default:
                return false;
        }
}

RkString KitModel::name() const
{
        return RkString("Unknown");
}

RkString KitModel::author() const
{
        return RkString("Unknown");
}

RkString KitModel::license() const
{
        return RkString("Unknown");
}

std::vector<float> KitModel::instrumentData(PercussionIndex index) const
{
        return dspProxy->getInstrumentBuffer(instrumentId(index));
}

bool KitModel::enableNoteOff(PercussionIndex index, bool b)
{
        return dspProxy->enableNoteOff(instrumentId(index), b);
}

bool KitModel::isNoteOffEnabled(PercussionIndex index) const
{
        return dspProxy->isNoteOffEnabled(instrumentId(index));
}

bool KitModel::setChokeGroup(PercussionIndex index, KitModel::ChokeGroup group)
{
        return dspProxy->setChokeGroup(instrumentId(index), group);
}

KitModel::ChokeGroup KitModel::getChokeGroup(PercussionIndex index) const
{
        return dspProxy->getChokeGroup(instrumentId(index));
}

OscillatorModel* KitModel::getCurrentLayerOscillator(OscillatorModel::Type type) const
{
        return geonkickModel->getOscillatorModels()[static_cast<int>(type)];
}

bool KitModel::loadPreset(const Preset &preset, PercussionIndex index)
{
        if (!isValidIndex(index))
                return false;

        auto state = dspProxy->getDefaultPercussionState();
        if (!state->loadFile(preset.path().string())) {
                GEONKICK_LOG_ERROR("can't open preset");
                return false;
        } else {
                state->setId(dspProxy->currentPercussion());
                dspProxy->setPercussionState(state);
                dspProxy->notifyUpdateGui();
                dspProxy->notifyPercussionUpdated(state->getId());
                return true;
        }
}

bool KitModel::loadPreset(const Preset &preset)
{
        return open(preset.path().string());
}
