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

#ifndef GEONKICK_LIMITER_H
#define GEONKICK_LIMITER_H

#include "geonkick_widget.h"
#include "geonkick_slider.h"

class GeonkickLimiter : public GeonkickWidget
{
 public:
        using Orientation = GeonkickSlider::Orientation;

        GeonkickLimiter(GeonkickWidget *parent);
        ~GeonkickLimiter() = default;

        void setRange(double min, double max);
        std::pair<double, double> getRange(double min, double max);

        void setValue(double value);
        double getValue() const;

        RK_DECL_ACT(valueUpdated, valueUpdated(double value),
                    RK_ARG_TYPE(double), RK_ARG_VAL(value));

 private:
        void onSliderUpdated(int value);
        double mapIntToDouble(int val) const;
        int mapDoubleToInt(double val) const;
        GeonkickSlider *slider;
        double rangeMin;
        double rangeMax;
};

#endif // GEONKICK_LIMITER_H
