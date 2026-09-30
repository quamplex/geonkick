/**
 * File name: oscillator.cpp
 * Project: Geonkick (A percussive synthesizer)
 *
 * Copyright (C) 2017 Iurie Nistor
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

#include "OscillatorModel.h"
#include "OscillatorDistortionModel.h"
#include "OscillatorFilterModel.h"
#include "GeonkickModel.h"

OscillatorModel::OscillatorModel(GeonkickModel *model, OscillatorModel::Type type)
        : dspProxy{model->getDspProxy()}
        , oscillatorType{type}
        , filterModel{new OscillatorFilterModel(this)}
        , distortionModel{new OscillatorDistortionModel(this)}
{
        RK_ACT_BIND(dspProxy,
                    kickLengthUpdated,
                    RK_ACT_ARGS(double val),
                    this, kickLengthUpdated(val));
}

void OscillatorModel::setAsFm(bool b)
{
        dspProxy->setOscillatorAsFm(index(), b);
}

bool OscillatorModel::isFm() const
{
        return dspProxy->isOscillatorAsFm(index());
}

bool OscillatorModel::setFunction(FunctionType func)
{
        auto functionType = static_cast<DspProxy::FunctionType>(func);
        if (functionType == dspProxy->oscillatorFunction(index()))
                return false;

        dspProxy->setOscillatorFunction(index(), functionType);
        action functionUpdated(func);
        return true;
}

void OscillatorModel::setPhase(gkick_real phase)
{
        dspProxy->setOscillatorPhase(index(), phase);
}

gkick_real OscillatorModel::getPhase() const
{
        return dspProxy->oscillatorPhase(index());
}

void OscillatorModel::setSeed(int seed)
{
        dspProxy->setOscillatorSeed(index(), seed);
}

int OscillatorModel::getSeed() const
{
        return dspProxy->oscillatorSeed(index());
}

OscillatorModel::FunctionType OscillatorModel::function() const
{
        return static_cast<FunctionType>(dspProxy->oscillatorFunction(index()));
}

std::vector<EnvelopePoint>
OscillatorModel::envelopePoints(EnvelopeType envelope) const
{
        const auto env = dspProxy->oscillatorEvelopePoints(index(), static_cast<DspProxy::EnvelopeType>(envelope));
        return dspProxy->oscillatorEvelopePoints(index(), static_cast<DspProxy::EnvelopeType>(envelope));
}

void OscillatorModel::addEnvelopePoint(EnvelopeType envelope, const EnvelopePoint &point)
{
        dspProxy->addOscillatorEnvelopePoint(index(),
                                                static_cast<DspProxy::EnvelopeType>(envelope),
                                                point);
}

void OscillatorModel::removeEnvelopePoint(EnvelopeType envelope, int point_index)
{
        dspProxy->removeOscillatorEvelopePoint(index(),
                                                  static_cast<DspProxy::EnvelopeType>(envelope),
                                                  point_index);
}

void OscillatorModel::updateEnvelopePoint(EnvelopeType envelope,
                                     int point_index,
                                     const EnvelopePoint &point)
{
        dspProxy->updateOscillatorEvelopePoint(index(),
                                                  static_cast<DspProxy::EnvelopeType>(envelope),
                                                  point_index,
                                                  point);
}

void OscillatorModel::setType(OscillatorModel::Type type)
{
        oscillatorType = type;
}

OscillatorModel::Type OscillatorModel::type() const
{
        return oscillatorType;
}

void OscillatorModel::setAmplitude(double amp)
{
        if (dspProxy->setOscillatorAmplitude(index(), amp))
                action amplitudeUpdated(amp);
}

double OscillatorModel::amplitude(void) const
{
	return dspProxy->oscillatorAmplitude(index());
}

void OscillatorModel::setFrequency(double freq)
{
	if (dspProxy->setOscillatorFrequency(index(), freq))
                action frequencyUpdated(freq);
}

void OscillatorModel::setPitchShift(double semitones)
{
	if (dspProxy->setOscillatorPitchShift(index(), semitones))
                action pitchShiftUpdated(semitones);
}

void OscillatorModel::setNoiseDensity(double density)
{
	if (dspProxy->setOscillatorNoiseDensity(index(), density))
                action noiseDensityUpdated(density);
}

double OscillatorModel::frequency(void) const
{
        return dspProxy->oscillatorFrequency(index());
}

double OscillatorModel::pitchShift(void) const
{
        return dspProxy->oscillatorPitchShift(index());
}

double OscillatorModel::noiseDensity(void) const
{
        return dspProxy->oscillatorNoiseDensity(index());
}

int OscillatorModel::index() const
{
        return static_cast<int>(oscillatorType);
}

void OscillatorModel::enable(bool b)
{
        dspProxy->enableOscillator(index(), b);
}

bool OscillatorModel::isEnabled() const
{
        return dspProxy->isOscillatorEnabled(index());
}

double OscillatorModel::envelopeLength() const
{
        return dspProxy->kickLength();
}

bool OscillatorModel::setSample(const std::string &file)
{
        if (auto res = dspProxy->setOscillatorSample(file, index()); !res)
                return false;
        auto path = std::filesystem::path(file);
        dspProxy->setCurrentWorkingPath("Samples", path.has_parent_path() ? path.parent_path().string() : path.string());
        return true;
}

std::string OscillatorModel::samplesPath() const
{
        return dspProxy->currentWorkingPath("Samples").string();
}

void OscillatorModel::setEnvelopeApplyType(OscillatorModel::EnvelopeType envelope,
				      OscillatorModel::EnvelopeApplyType apply)
{
	dspProxy->setOscillatorEnvelopeApplyType(index(), envelope, apply);
}

OscillatorModel::EnvelopeApplyType
OscillatorModel::envelopeApplyType(OscillatorModel::EnvelopeType envelope) const
{
	return dspProxy->getOscillatorEnvelopeApplyType(index(), envelope);
}

FilterModel* OscillatorModel::getFilter() const
{
        return filterModel;
}

DistortionModel* OscillatorModel::getDistortion() const
{
        return distortionModel;
}

DspProxy* OscillatorModel::getDspProxy() const
{
        return dspProxy;
}
