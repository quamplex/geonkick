/**
 * File name: RkScroolbar.cpp
 * Project: Redkite (A small GUI toolkit)
 *
 * Copyright (C) 2025 Iurie Nistor
 *
 * This file is part of Redkite.
 *
 * Redkite is free software; you can redistribute it and/or modify
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

#include "RkScroolbar.h"

#include "RkButton.h"
#include "RkEvent.h"
#include "RkPainter.h"

class SliderButton : public RkButton {
public:
        SliderButton(RkWidget* parent)
                : RkButton(parent)
                , isSelected{false}
                , previousY{0}
        {
                setType(RkButton::ButtonType::ButtonPush);
        }
        void setPositionRange(const std::pair<int, int> &range)
        {
                positionRange = range;
        }

        RK_DECL_ACT(onPositionChanged,
                    onPositionChanged(),
                    RK_ARG_TYPE(),
                    RK_ARG_VAL());

protected:
        void mouseButtonPressEvent(RkMouseEvent *event) override
        {
                RkButton::mouseButtonPressEvent(event);
                isSelected = true;
                setFocus(true);
                auto mouseEvent = std::make_unique<RkMouseEvent>();
                *mouseEvent.get() = *event;
                eventQueue()->postEvent(parent(), std::move(mouseEvent));
                previousY = mapToGlobal(event->point()).y();
        }

        void mouseButtonReleaseEvent(RkMouseEvent *event) override
        {
                RkButton::mouseButtonReleaseEvent(event);
                isSelected = false;
        }

        void mouseMoveEvent(RkMouseEvent *event) override
        {
                RkButton::mouseMoveEvent(event);
                if (isSelected) {
                        auto currentY = mapToGlobal(event->point()).y();
                        auto delta = currentY - previousY;
                        setY(std::clamp(y() + delta,
                                        positionRange.first,
                                        positionRange.second));
                        previousY = currentY;
                        action onPositionChanged();
                }
        }

private:
        bool isSelected;
        int previousY;
        std::pair<int, int> positionRange;
};

RkScroolbar::RkScroolbar(RkWidget *parent)
        : RkWidget(parent)
        , scroolRange{0, 0}
        , pageSize{0}
        , scroolStep{1}
        , pageScroolStep{1}
        , sliderButton{new SliderButton(this)}
        , upButton{new RkButton(this)}
        , downButton{new RkButton(this)}
        , scroolOffset {0}
{
        setBackgroundColor(parent->background() + 40);
        upButton->setSize({24, 24});
        upButton->setType(RkButton::ButtonType::ButtonPush);
        downButton->setSize({24, 24});
        downButton->setType(RkButton::ButtonType::ButtonPush);
        sliderButton->setSize({24, 24});

        RK_ACT_BIND(upButton,
                    pressed,
                    RK_ACT_ARGS(),
                    this,
                    scroolUp());
        RK_ACT_BIND(downButton,
                    pressed,
                    RK_ACT_ARGS(),
                    this,
                    scroolDown());
        RK_ACT_BIND(static_cast<SliderButton*>(sliderButton),
                    onPositionChanged,
                    RK_ACT_ARGS(),
                    this,
                    onSliderPositionChanged());
}

void RkScroolbar::setContentSize(int val)
{
        contentSize = val;
}

int RkScroolbar::getContentSize() const
{
        return contentSize;
}

void RkScroolbar::setPageSize(int val)
{
        pageSize = val;
}

int RkScroolbar::getPageSize() const
{
        return pageSize;
}

void RkScroolbar::setScroolStep(int val)
{
        scroolStep = val;
}

int RkScroolbar::getScroolStep() const
{
        return scroolStep;
}

void RkScroolbar::setPageScroolStep(int val)
{
        pageScroolStep = val;
}

int RkScroolbar::getPageScroolStep() const
{
        return pageScroolStep;
}

void RkScroolbar::setScrool(int val)
{
        if (scroolOffset != val) {
                scroolOffset = std::clamp(val, 0, getContentSize() - getPageSize());
                sliderButton->setPosition({3, getSliderPosition()});
        }
}

void RkScroolbar::resizeEvent(RkResizeEvent *event)
{
        RkColor buttonsBk = background() + 15;

        upButton->setSize({event->size().width(), event->size().width()});
        upButton->setBackgroundColor(buttonsBk);

        downButton->setSize({event->size().width(), event->size().width()});
        downButton->setPosition({0, height() - downButton->height()});
        downButton->setBackgroundColor(buttonsBk);

        {
                RkImage buttonImage(downButton->size());
                buttonImage.fill(buttonsBk);
                upButton->setImage(buttonImage, RkButton::State::Unpressed);
                downButton->setImage(buttonImage, RkButton::State::Unpressed);
                buttonImage.fill(buttonsBk + 40);
                upButton->setImage(buttonImage, RkButton::State::UnpressedHover);
                downButton->setImage(buttonImage, RkButton::State::UnpressedHover);
                buttonImage.fill(buttonsBk + 60);
                upButton->setImage(buttonImage, RkButton::State::PressedHover);
                downButton->setImage(buttonImage, RkButton::State::PressedHover);
                upButton->setImage(buttonImage, RkButton::State::Pressed);
                downButton->setImage(buttonImage, RkButton::State::Pressed);
        }

        sliderButton->setWidth(event->size().width() - 6);
        sliderButton->setBackgroundColor(buttonsBk + 5);
        sliderButton->setPosition({3, getSliderPosition()});
        static_cast<SliderButton*>(sliderButton)->setPositionRange(
                                  {upButton->position().y() + downButton->height(),
                                   downButton->position().y() - sliderButton->height()});
        {
                RkImage buttonImage(sliderButton->size());
                buttonImage.fill(buttonsBk + 5);
                sliderButton->setImage(buttonImage, RkButton::State::Unpressed);
                buttonImage.fill(buttonsBk + 30);
                sliderButton->setImage(buttonImage, RkButton::State::Pressed);
                sliderButton->setImage(buttonImage, RkButton::State::UnpressedHover);
                sliderButton->setImage(buttonImage, RkButton::State::PressedHover);
        }
}

void RkScroolbar::mouseButtonPressEvent(RkMouseEvent *event)
{
        if (event->button() == RkMouseEvent::ButtonType::WheelUp
            || event->button() == RkMouseEvent::ButtonType::WheelDown) {
                auto isScrooldown = event->button() == RkMouseEvent::ButtonType::WheelDown;
                auto delta = isScrooldown ? getScroolStep() : -getScroolStep();
                setScrool(scroolOffset + delta);
                action onScrool(scroolOffset);
                return;
        }

        if (event->button() == RkMouseEvent::ButtonType::Left) {
                int posY = event->point().y();
                if ((posY > upButton->y() + upButton->height())
                    && (posY < sliderButton->y())) {
                        scroolPageUp();
                } else if ((posY > sliderButton->y() + sliderButton->height())
                           && (posY < downButton->y())) {
                        scroolPageDown();
                }
        }
}

int RkScroolbar::getSliderPosition() const
{
        int scroolSize = height() - 2 * upButton->height() - sliderButton->height();
        return upButton->height()
                + scroolSize * (static_cast<double>(scroolOffset) / (contentSize - pageSize));
}

void RkScroolbar::onSliderPositionChanged()
{
        auto pos = sliderButton->position().y() - upButton->height();
        auto range = height() - 2 * upButton->height() - sliderButton->height();
        scroolOffset = ((double)pos / range) * (contentSize - pageSize);
        action onScrool(scroolOffset);
}

void RkScroolbar::scroolUp()
{
        setScrool(scroolOffset - getScroolStep());
        action onScrool(scroolOffset);
}

void RkScroolbar::scroolDown()
{
        setScrool(scroolOffset + getScroolStep());
        action onScrool(scroolOffset);
}

void RkScroolbar::scroolPageUp()
{
        setScrool(scroolOffset - getPageSize() * getPageScroolStep());
        action onScrool(scroolOffset);
}

void RkScroolbar::scroolPageDown()
{
        setScrool(scroolOffset + getPageSize() * getPageScroolStep());
        action onScrool(scroolOffset);
}


