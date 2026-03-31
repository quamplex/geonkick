/**
 * File name: kick_waveform.cpp
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

#include "InstrumentWaveform.h"
#include "DspProxy.h"
#include "globals.h"
#include "envelope.h"

#include "RkEventQueue.h"
#include "RkAction.h"

InstrumentWaveform::InstrumentWaveform(RkObject *parent, DspProxy *dsp, const RkSize &size)
        : RkObject(parent)
        , dspProxy{dsp}
        , waveformThread{nullptr}
        , waveformSize{size}
        , isRunning{true}
        , redrawWaveform{true}
        , currentEnvelope{nullptr}
{
        RK_ACT_BIND(dspProxy, kickUpdated, RK_ACT_ARGS(), this, updateWaveformBuffer());
        waveformThread = std::make_unique<std::thread>(&InstrumentWaveform::drawInstrumentWaveform, this);
}

InstrumentWaveform::~InstrumentWaveform()
{
        isRunning = false;
        threadConditionVar.notify_one();
        waveformThread->join();
}

void InstrumentWaveform::setEnvelope(Envelope * envelope)
{
        std::unique_lock<std::mutex> lock(waveformMutex);
        currentEnvelope = envelope;
        updateWaveform(false);
}

Envelope* InstrumentWaveform::getEnvelope() const
{
        std::unique_lock<std::mutex> lock(waveformMutex);
        return currentEnvelope;
}

void InstrumentWaveform::updateWaveform(bool lock)
{
        if (lock) {
                std::unique_lock<std::mutex> lock(waveformMutex);
                redrawWaveform = true;
        } else {
                redrawWaveform = true;
        }

        threadConditionVar.notify_one();
}

void InstrumentWaveform::updateWaveformBuffer()
{
        {
                std::unique_lock<std::mutex> lock(waveformMutex);
                kickBuffer = dspProxy->getKickBuffer();
                if (kickBuffer.empty())
                        dspProxy->triggerSynthesis();
                updateWaveform(false);
        }

        threadConditionVar.notify_one();
}

void InstrumentWaveform::drawInstrumentWaveform()
{
        while (isRunning) {
                std::unique_lock<std::mutex> lock(waveformMutex);
                if (!redrawWaveform)
                        threadConditionVar.wait(lock);

                if (!isRunning)
                        break;

                if (!currentEnvelope || kickBuffer.empty()) {
                        redrawWaveform = false;
                        continue;
                }

                const auto zoomFactor = currentEnvelope->getZoom();
                const auto timeOrigin = currentEnvelope->getTimeOrigin();
                auto waveformImage = std::make_shared<RkImage>(waveformSize.width(), waveformSize.height());
                RkPainter painter(waveformImage.get());
                RkPen pen(RkColor(59, 130, 4, 255));
                painter.setPen(pen);
                std::vector<RkRealPoint> waveformPoints;
                const auto instrumentBuffer = kickBuffer;
                waveformPoints.reserve(instrumentBuffer.size());
                const auto buffSize = static_cast<double>(instrumentBuffer.size()) / zoomFactor;
                const gkick_real k = static_cast<gkick_real>(waveformSize.width()) / buffSize;
                const size_t indexOffset = (instrumentBuffer.size() / dspProxy->kickLength()) * timeOrigin;
                const auto instrumentWaveformSize = waveformSize;
                redrawWaveform = false;

                lock.unlock();

                /**
                 * In this loop there is an implementation of an
                 * antialiasing algorithm that reduces in most of
                 * the cases antialiasing, and at the same time
                 * reduces and normalizes the size of the buffer.
                 */
                RkRealPoint prev;
                painter.translate({0, instrumentWaveformSize.height()});
                for (decltype(instrumentBuffer.size()) i = indexOffset; i < instrumentBuffer.size(); i++) {
                        const double x = k * (i - indexOffset);
                        const double value = -(instrumentWaveformSize.height() / 2
                                               + instrumentWaveformSize.height() * (instrumentBuffer[i] / 2));
                        double y = value;
                        RkRealPoint p(k * (i - indexOffset), value);
                        if (p == prev)
                                continue;
                        else
                                prev = p;
                        waveformPoints.push_back(p);

                        const int i0 = i;
                        double ymin, ymax;
                        ymin = ymax = y;
                        while (++i < instrumentBuffer.size()) {
                                if (x != k * (i - indexOffset))
                                        break;
                                y = instrumentWaveformSize.height() - value;
                                ymin = std::min(ymin, y);
                                ymax = std::min(ymax, y);
                        }

                        if (i - i0 > 4) {
                                waveformPoints.emplace_back(RkRealPoint(x, ymin));
                                waveformPoints.emplace_back(RkRealPoint(x, ymax));
                                waveformPoints.emplace_back(RkRealPoint(x, y));
                        }
                }

                waveformPoints.shrink_to_fit();
                painter.drawPolyline(waveformPoints);

                if (eventQueue()) {
                        auto act = std::make_unique<RkAction>(this);
                        act->setCallback([this, waveformImage](void){ waveformUpdated(waveformImage); });
                        waveformImage.reset();
                        eventQueue()->postAction(std::move(act));
                }
        }
}
