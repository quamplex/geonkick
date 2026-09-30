/**
 * File name: MainWindow.h
 * Project: Geonkick (A percussive synthesizer)
 *
 * Copyright (C) 2017 Iurie Nistor
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

#ifndef GEONGKICK_MAINWINDOW_H
#define GEONGKICK_MAINWINDOW_H

#include "geonkick_widget.h"

class DspProxy;
class TopBar;
class GeonkickModel;

class MainWindow : public GeonkickWidget
{
public:
        explicit MainWindow(RkMain& app,
                            DspProxy *dsp,
                            const std::string &preset = std::string());
        explicit MainWindow(RkMain& app,
                            DspProxy *dsp,
                            const RkNativeWindowInfo &info);
        ~MainWindow() = default;
        static RkSize getWindowSize();
        RK_DECL_ACT(onScaleFactor,
                    onScaleFactor(double factor),
                    RK_ARG_TYPE(double),
                    RK_ARG_VAL(factor));

protected:
        void shortcutEvent(RkKeyEvent *event) override;
        void dropEvent(RkDropEvent *event) override;
        void openPreset(const std::string &fileName);
        void setPreset(const std::string &fileName);
        void openPreset();
        void resetToDefault();
        void setSample(const std::string &file);

private:
        void createViewState();
        void createShortcuts();
        void createUi();

        DspProxy *dspProxy;
        TopBar *topBar;
        std::string presetName;
        std::string currentWorkingPath;
        GeonkickModel *geonkickModel;
};
#endif // GEONKICK_MAINWINDOW_H
