/**
 * File name: EnvelopePointContextWidget.cpp
 * Project: Geonkick (A percussive synthesizer)
 *
 * Copyright (C) 2023 Iurie Nistor
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

#include "EnvelopePointContextWidget.h"
#include "envelope.h"

#include "RkEvent.h"
#include "RkLineEdit.h"

#include <charconv>
#include <cctype>
#include <cmath>

namespace {

std::optional<double> noteFrequency(std::string_view text)
{
        text = Geonkick::trim(text);
        if (text.size() < 2)
                return std::nullopt;

        constexpr std::array<int, 7> noteOffsets = {9, 11, 0, 2, 4, 5, 7};
        const char note = static_cast<char>(std::toupper(static_cast<unsigned char>(text.front())));
        if (note < 'A' || note > 'G')
                return std::nullopt;

        int semitone = noteOffsets[note - 'A'];
        text.remove_prefix(1);
        if (!text.empty() && text.front() == '#') {
                ++semitone;
                text.remove_prefix(1);
        }

        int octave = 0;
        const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), octave);
        if (error != std::errc{} || end != text.data() + text.size())
                return std::nullopt;

        const int midiNote = 12 * (octave + 1) + semitone;
        if (midiNote < 21 || midiNote > 128)
                return std::nullopt;

        return 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
}

std::optional<double> numericValue(std::string_view text)
{
        text = Geonkick::trim(text);
        if (text.empty())
                return std::nullopt;

        double value = 0.0;
        const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
        if (error != std::errc{} || end != text.data() + text.size() || !std::isfinite(value))
                return std::nullopt;

        return value;
}

bool acceptsNoteNames(Envelope::Type type)
{
        return type == Envelope::Type::Frequency || type == Envelope::Type::FilterCutOff;
}

} // namespace

EnvelopePointContextWidget::EnvelopePointContextWidget(Envelope* envelope,
                                                       GeonkickWidget *parent,
                                                       Rk::WidgetFlags flag)
        : GeonkickWidget(parent, flag)
        , pointEnvelope{envelope}
        , lineEdit{new RkLineEdit(this)}
{
        setFixedSize(110, 30);
        setBackgroundColor({68, 68, 70});
        setBorderColor(40, 40, 40);
        setBorderWidth(1);
        lineEdit->setSize(100, 20);
        lineEdit->setPosition(5, 5);
        lineEdit->show();
        RK_ACT_BIND(lineEdit,
                    editingFinished,
                    RK_ACT_ARGS(),
                    this,
                    onUpdateValue());
        RK_ACT_BIND(lineEdit,
                    escapePressed,
                    RK_ACT_ARGS(),
                    this,
                    close());
        pointEnvelope->setEditCurrentPoint();
        setValue(pointEnvelope->getSelectedPointValue());
}

void EnvelopePointContextWidget::setFocus()
{
        lineEdit->setFocus();
}

void EnvelopePointContextWidget::setValue(rk_real val)
{
        double roundedValue = std::round(val * 10000.0) / 10000.0;
        lineEdit->setText(Geonkick::doubleToStr(roundedValue, 4));
        lineEdit->moveCursorToEnd();
}

void EnvelopePointContextWidget::closeEvent(RkCloseEvent *event)
{
        RkWidget::closeEvent(event);
}

void EnvelopePointContextWidget::onUpdateValue()
{
        const auto text = lineEdit->text();
        auto value = acceptsNoteNames(pointEnvelope->type()) ? noteFrequency(text) : std::nullopt;
        if (!value)
                value = numericValue(text);
        if (!value) {
                lineEdit->setFocus();
                return;
        }
        pointEnvelope->updateSelectedPointValue(*value);

        close();
}
