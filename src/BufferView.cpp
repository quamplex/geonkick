/**
 * File name: BufferView.cpp
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

#include "BufferView.h"

#include "RkPainter.h"
#include "RkEvent.h"

BufferView::BufferView(GeonkickWidget* parent, const std::vector<float> &data)
        : GeonkickWidget(parent)
        , bufferData{data}
        , updateGraph{true}
        , waveformImage{nullptr}
{
        setBackgroundColor(50, 50, 50);
}

void BufferView::setData(const std::vector<float> &data)
{
        bufferData = data;
        updateGraph = true;
        update();
}

const std::vector<float>& BufferView::getData() const
{
        return bufferData;
}

void BufferView::paintWidget(RkPaintEvent *event)
{
        RK_UNUSED(event);
        if (updateGraph)
                drawGraph();
        if (waveformImage && !waveformImage->isNull()) {
                RkPainter painter(this);
                painter.drawImage(*waveformImage.get(), 0, 0);
        }
}

void BufferView::drawGraph()
{
        waveformImage = std::make_unique<RkImage>(size());
        RkPainter painter(waveformImage.get());
        if (bufferData.empty())
                return;

        std::vector<RkRealPoint> graphPoints;
        int x = 0;
        double kY = height() / 2;
        float yScale = 1.0f;
        float max = fabs(*std::max_element(bufferData.begin(), bufferData.end(),
                                           [](float a, float b){ return fabs(a) < fabs(b); }));
        if (max > 1e-5)
                yScale = 1.0f / max;
        int graphWidth = width();
        for (const auto &val: bufferData) {
                double y =  kY * (1.0f - yScale * val);
                graphPoints.emplace_back(RkRealPoint(x / graphWidth, y));
                x++;
        }

        RkPen pen = painter.pen();

        // Simulate shadow
        pen.setColor({44, 44, 44, 200});
        pen.setWidth(2);
        painter.setPen(pen);
        painter.translate({2, 2});
        painter.drawPolyline(graphPoints);

        pen.setColor({59, 130, 4, 150});
        pen.setWidth(1);
        painter.setPen(pen);
        painter.translate({-2, -2});
        painter.drawPolyline(graphPoints);

        updateGraph = false;
}

void BufferView::mouseButtonPressEvent(RkMouseEvent *event)
{
        if (event->button() == RkMouseEvent::ButtonType::Left)
                action graphPressed();
}
