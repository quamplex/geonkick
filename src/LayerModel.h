/**
 * File name: LayerModel.h
 * Project: Geonkick (A percussive synthesizer)
 *
 * Copyright (C) 2026 Iurie Nistor
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

#ifndef GKICK_LAYER_MODEL_H
#define GKICK_LAYER_MODEL_H

#include "AbstractModel.h"

class DspLayerProxy;

class LayerModel: public AbstractModel
{
 public:
        explicit LayersModel(DspLayerProxy *proxy, RkObject *parent);
        ~LayersModel() = default;
        bool isEnabled() const;
        bool enable() const;
        double limiter() const;
        void setLimiter(double value);

        RK_DECL_ACT(enbaledUpdated,
                    enbaledUpdated(bool b),
                    RK_ARG_TYPE(bool),
                    RK_ARG_VAL(b));
        RK_DECL_ACT(limiterUpdated,
                    limiterUpdated(double value),
                    RK_ARG_TYPE(double),
                    RK_ARG_VAL(value));

 private:
        DspLayerProxy *dspProxy;
};

#endif // GKICK_LAYER_MODEL_H
