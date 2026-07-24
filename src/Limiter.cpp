/**
 * File name: Limiter.h
 * Project: Geonkick (A percussive synthesizer)
 *
 * Copyright (C) 2024 Iurie Nistor
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

#include "Limiter.h"

GeonkickLimiter::GeonkickLimiter(GeonkickWidget *parent)
        : GeonkickWidget(parent),
          rangeMin{0.0}
        , rangeMax{1.0}
{
        slider = new GeonkickSlider(this, GeonkickSlider::Orientation::Horizontal);

        RK_ACT_BIND(slider, valueUpdated, RK_ACT_ARGS(int val),
                    this, onSliderUpdated(val));
}

void GeonkickLimiter::setRange(double min, double max)
{
        rangeMin = min;
        rangeMax = max;
}

void GeonkickLimiter::onSetValue(double value)
{
        slider->onSetValue(mapDoubleToInt(value));
}

double GeonkickLimiter::getValue() const
{
        return mapIntToDouble(slider->getValue());
}

void GeonkickLimiter::setSize(int w, int h)
{
        GeonkickWidget::setSize(w, h);
        slider->setSize(w, h);
}

void GeonkickLimiter::onSliderUpdated(int value)
{
        double dVal = mapIntToDouble(value);
        RK_ACT_EMIT(valueUpdated, dVal);
}

double GeonkickLimiter::mapIntToDouble(int val) const
{
        double norm = static_cast<double>(val) / 100.0;
        return rangeMin + norm * (rangeMax - rangeMin);
}

int GeonkickLimiter::mapDoubleToInt(double val) const
{
        if (rangeMax == rangeMin)
                return 0;

        val = std::clamp(val, rangeMin, rangeMax);
        double norm = (clamped - rangeMin) / (rangeMax - rangeMin);

        return static_cast<int>(norm * 100.0);
}
