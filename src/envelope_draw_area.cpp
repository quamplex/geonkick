/**
 * File name: envelope_draw_area.cpp
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

#include "envelope_draw_area.h"
#include "envelope.h"
#include "InstrumentWaveform.h"
#include "EnvelopePointContextWidget.h"

#include "RkPainter.h"
#include "RkEvent.h"
#include "RkAction.h"

EnvelopeWidgetDrawingArea::EnvelopeWidgetDrawingArea(GeonkickWidget *parent, DspProxy *dsp)
          : GeonkickWidget(parent)
          , dspProxy{dsp}
          , currentEnvelope{nullptr}
          , hideEnvelope{false}
          , instrumentWaveformImage{nullptr}
          , instrumentWaveform{nullptr}
          , pointEditingMode{false}
          , addAsControlPoint{false}
          , bezierMode {false}
{
        setFixedSize(850, 300);
        int padding = 50;
        drawingArea = RkRect(1.1 * padding, padding / 2, width() - 1.5 * padding, height() - 1.2 * padding);
        setBackgroundColor(40, 40, 40);
        instrumentWaveform = new InstrumentWaveform(this, dspProxy, drawingArea.size());
        RK_ACT_BIND(instrumentWaveform,
                    waveformUpdated,
                    RK_ACT_ARGS(std::shared_ptr<RkImage> waveformImage),
                    this, updateInstrumentWaveform(waveformImage));
        addShortcut(Rk::Key::Key_Control_Left, Rk::KeyModifiers::Control_Left);
        addShortcut(Rk::Key::Key_Control_Right, Rk::KeyModifiers::Control_Right);
}

void EnvelopeWidgetDrawingArea::setEnvelope(Envelope* envelope)
{
        if (envelope) {
                currentEnvelope = envelope;
                if (currentEnvelope) {
                        instrumentWaveform->setEnvelope(currentEnvelope);
                        action zoomUpdated(Geonkick::doubleToStr(currentEnvelope->getZoom(), 0));
                }
                envelopeUpdated();
        }
}

void EnvelopeWidgetDrawingArea::paintWidget([[maybe_unused]] RkPaintEvent *event)
{
        if (width() != envelopeImage.width() || height() != envelopeImage.height()) {
                RkImage im(size());
                envelopeImage = im;
        }

        RkPainter painter(&envelopeImage);
        painter.fillRect(rect(), background());

        if (instrumentWaveformImage && !instrumentWaveformImage->isNull())
                painter.drawImage(*instrumentWaveformImage.get(), drawingArea.topLeft().x(), drawingArea.topLeft().y());
        else
                instrumentWaveform->updateWaveformBuffer();

        if (currentEnvelope)
                currentEnvelope->draw(painter, Envelope::DrawLayer::Axies);

        if (currentEnvelope && !isHideEnvelope())
                currentEnvelope->draw(painter, Envelope::DrawLayer::Envelope);

        auto pen = painter.pen();
        pen.setColor({180, 180, 180, 200});
        pen.setWidth(1);
        painter.setPen(pen);
#ifndef GEONKICK_BASIC_VERSION
        painter.drawText(90, height() - 12, getEnvStateText());
#else
        painter.drawText(0, height() - 12, getEnvStateText());
#endif // GEONKICK_BASIC_VERSION
        pen.setColor({20, 20, 20, 255});
        painter.setPen(pen);
        painter.drawRect({0, 0, width() - 1, height() - 1});

        RkPainter paint(this);
        paint.drawImage(envelopeImage, 0, 0);
}

std::string EnvelopeWidgetDrawingArea::getEnvStateText() const
{
        std::string str;
        std::string layer;
#ifndef GEONKICK_BASIC_VERSION
        layer = "L" + std::to_string(static_cast<int>(dspProxy->layer()) + 1) + " / ";
#endif // GEONKICK_SINGLE_VERSION
        switch(currentEnvelope->category()) {
        case Envelope::Category::Oscillator1:
                str += layer + "OSC1";
                break;
        case Envelope::Category::Oscillator2:
                str += layer + "OSC2";
                break;
        case Envelope::Category::Oscillator3:
                str += layer + "OSC3";
                break;
        case Envelope::Category::InstrumentGlobal:
                str += "GLOB";
                break;
        default:
                break;
        }

        str += " / ";

        switch (currentEnvelope->type()) {
        case Envelope::Type::Amplitude:
                str += "AENV";
                break;
        case Envelope::Type::Frequency:
                str += "FENV";
                break;
        case Envelope::Type::DistortionDrive:
                str += "DIST / DRIVE";
                break;
        case Envelope::Type::DistortionVolume:
                str += "DIST / VOL";
                break;
        case Envelope::Type::FilterCutOff:
                str += "FILT / CFENV";
                break;
        case Envelope::Type::FilterQFactor:
                str += "FILT /QENV";
                break;
        case Envelope::Type::PitchShift:
                str += "PSHIFT";
                break;
        case Envelope::Type::NoiseDensity:
                str += "NDENSITY";
                break;
        default:
                break;
        }

        return str;
}

void EnvelopeWidgetDrawingArea::mouseButtonPressEvent(RkMouseEvent *event)
{
        if (event->button() != RkMouseEvent::ButtonType::Right
            && event->button() != RkMouseEvent::ButtonType::Left)
                return;

        RkPoint point(event->x() - drawingArea.left(),
                      drawingArea.bottom() - event->y());
        if (event->button() == RkMouseEvent::ButtonType::Right) {
                if (currentEnvelope) {
                        currentEnvelope->removePoint(point);
                        envelopeUpdated();
                }
        } else {
                mousePoint.setX(event->x());
                mousePoint.setY(event->y());
                if (currentEnvelope) {
                        currentEnvelope->selectPoint(point);
                        if (currentEnvelope->hasSelected())
                                envelopeUpdated();
                        else {
                                currentEnvelope->setScrollState(true);
                                mousePoint.setX(event->x());
                                mousePoint.setY(event->y());
                        }
                }
        }
        setFocus(true);
}

void EnvelopeWidgetDrawingArea::mouseButtonReleaseEvent(RkMouseEvent *event)
{
        if (!currentEnvelope)
                return;

        if (currentEnvelope->isScrollState()) {
                currentEnvelope->setScrollState(false);
                return;
        }

        auto toUpdate = false;
        if (currentEnvelope->hasSelected()) {
                currentEnvelope->unselectPoint();
                toUpdate = true;
        }

        if (currentEnvelope->hasOverPoint())
                toUpdate = true;

        if (toUpdate)
                envelopeUpdated();
}

void EnvelopeWidgetDrawingArea::mouseDoubleClickEvent(RkMouseEvent *event)
{
        if (event->button() == RkMouseEvent::ButtonType::Left) {
                if (!currentEnvelope)
                        return;
                RkPoint point(event->x() - drawingArea.left(), drawingArea.bottom() - event->y());
                if (pointEditingMode && currentEnvelope->hasOverPoint()) {
                        auto act = std::make_unique<RkAction>();
                        int x = event->x();
                        int y = event->y();
                        auto topWidget = dynamic_cast<GeonkickWidget*>(RkWidget::getTopWidget());
                        act->setCallback([&, x, y, topWidget](void){
                                auto widget = new EnvelopePointContextWidget(currentEnvelope,
                                                                             topWidget);
                                widget->setPosition({x, y + 40});
                                widget->setFocus();
                                widget->show();
                        });
                        eventQueue()->postAction(std::move(act));
                } else {
                        currentEnvelope->addPoint(point, addAsControlPoint);
                        currentEnvelope->selectPoint(point);
                        envelopeUpdated();
                }
        }
}

void EnvelopeWidgetDrawingArea::mouseMoveEvent(RkMouseEvent *event)
{
        if (!currentEnvelope)
                return;

        if (currentEnvelope->isScrollState()) {
                auto zoomedLengthX = currentEnvelope->envelopeLength() / currentEnvelope->getZoom();
                auto zoomedLengthY = currentEnvelope->envelopeAmplitude() / currentEnvelope->getZoom();
                auto pointDiff = mousePoint - event->point();
                auto timeOrg = pointDiff.x() * (zoomedLengthX / currentEnvelope->W());
                auto valueOrg = -pointDiff.y() * (zoomedLengthY / currentEnvelope->H());
                currentEnvelope->setTimeOrigin(currentEnvelope->getTimeOrigin() + timeOrg);
                currentEnvelope->setValueOrigin(currentEnvelope->getValueOrigin() + valueOrg);
                instrumentWaveform->updateWaveform();
                mousePoint.setX(event->x());
                mousePoint.setY(event->y());
                envelopeUpdated();
                return;
        }

        RkPoint point(event->x() - drawingArea.left(), drawingArea.bottom() - event->y());
        if (currentEnvelope->hasSelected()) {
                currentEnvelope->moveSelectedPoint(point.x(), point.y());
                mousePoint.setX(event->x());
                mousePoint.setY(event->y());
                envelopeUpdated();
		return;
        }

	auto overPoint = currentEnvelope->hasOverPoint();
        currentEnvelope->overPoint(point);
	if (overPoint != currentEnvelope->hasOverPoint())
                envelopeUpdated();
        mousePoint.setX(event->x());
        mousePoint.setY(event->y());
}

void EnvelopeWidgetDrawingArea::shortcutEvent(RkKeyEvent *event)
{
        pointEditingMode = event->modifiers()
                & static_cast<int>(Rk::KeyModifiers::Control);

        if (bezierMode)
                return;

        addAsControlPoint = pointEditingMode;
}

void EnvelopeWidgetDrawingArea::wheelEvent(RkWheelEvent *event)
{
        event->direction() == RkWheelEvent::WheelDirection::DirectionUp ? zoomIn() : zoomOut();
}

void EnvelopeWidgetDrawingArea::zoomIn()
{
#ifndef GEONKICK_BASIC_VERSION
        if (currentEnvelope && (static_cast<int>(currentEnvelope->getZoom()) < 32)) {
                currentEnvelope->zoomIn();
                auto zoomedLengthX = currentEnvelope->envelopeLength() / currentEnvelope->getZoom();
                auto zoomedLengthY = currentEnvelope->envelopeAmplitude() / currentEnvelope->getZoom();
                auto pointDiff = mousePoint - drawingArea.bottomLeft();
                auto timeOrg = pointDiff.x() * (zoomedLengthX / currentEnvelope->W());
                auto valueOrg = pointDiff.y() * (zoomedLengthY / currentEnvelope->H());
                currentEnvelope->setTimeOrigin(currentEnvelope->getTimeOrigin() + timeOrg);
                currentEnvelope->setValueOrigin(currentEnvelope->getValueOrigin() - valueOrg);
                instrumentWaveform->updateWaveform();
                action zoomUpdated(Geonkick::doubleToStr(currentEnvelope->getZoom(), 0));
        }
        update();
#endif // GEONKICK_BASIC_VERSION
}

void EnvelopeWidgetDrawingArea::zoomOut()
{
#ifndef GEONKICK_BASIC_VERSION
        if (currentEnvelope && (static_cast<int>(currentEnvelope->getZoom()) / 2 > 0)) {
                auto zoomedLengthX = currentEnvelope->envelopeLength() / currentEnvelope->getZoom();
                auto zoomedLengthY = currentEnvelope->envelopeAmplitude() / currentEnvelope->getZoom();
                auto pointDiff = mousePoint - drawingArea.bottomLeft();
                auto timeOrg = pointDiff.x() * (zoomedLengthX / currentEnvelope->W());
                auto valueOrg = pointDiff.y() * (zoomedLengthY / currentEnvelope->H());
                currentEnvelope->setTimeOrigin(currentEnvelope->getTimeOrigin() - timeOrg);
                currentEnvelope->setValueOrigin(currentEnvelope->getValueOrigin() + valueOrg);
                currentEnvelope->zoomOut();
                instrumentWaveform->updateWaveform();
                action zoomUpdated(Geonkick::doubleToStr(currentEnvelope->getZoom(), 0));
        }
        update();
#endif // GEONKICK_BASIC_VERSION
}

void EnvelopeWidgetDrawingArea::setBezierMode(bool b)
{
        bezierMode = b;
        addAsControlPoint = b;
}

bool EnvelopeWidgetDrawingArea::isBezierMode() const
{
        return bezierMode;
}

void EnvelopeWidgetDrawingArea::envelopeUpdated()
{
        action isOverPoint(currentEnvelope->getCurrentPointInfo());
        update();
}

Envelope* EnvelopeWidgetDrawingArea::getEnvelope() const
{
        return currentEnvelope;
}

const RkRect EnvelopeWidgetDrawingArea::getDrawingArea()
{
        return drawingArea;
}

void EnvelopeWidgetDrawingArea::updateInstrumentWaveform(const std::shared_ptr<RkImage> &waveformImage)
{
        instrumentWaveformImage = waveformImage;
        update();
}

bool EnvelopeWidgetDrawingArea::isHideEnvelope() const
{
        return hideEnvelope;
}

void EnvelopeWidgetDrawingArea::setHideEnvelope(bool b)
{
        if (hideEnvelope != b) {
                hideEnvelope = b;
                update();
        }
}

void EnvelopeWidgetDrawingArea::setPointEditingMode(bool b)
{
        pointEditingMode = b;
}
