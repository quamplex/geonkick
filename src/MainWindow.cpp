
/**
 * File name: MainWindow.cpp
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

#include "MainWindow.h"
#include "ViewState.h"
#include "UiSettings.h"
#include "GeonkickConfig.h"
#include "InstrumentState.h"
#include "DspProxy.h"
#include "GeonkickModel.h"
#include "TopBar.h"
#include "Sidebar.h"
#include "InstrumentEditor.h"

#include "RkEvent.h"

constexpr int MAIN_WINDOW_WIDTH  = 940;
#ifdef GEONKICK_SINGLE
constexpr int MAIN_WINDOW_HEIGHT = 690;
#else
constexpr int MAIN_WINDOW_HEIGHT = 715;
#endif // GEONKICK_SINGLE

MainWindow::MainWindow(RkMain& app, DspProxy *dsp, const std::string &preset)
        : GeonkickWidget(app)
        , dspProxy{dsp}
        , topBar{nullptr}
        , instrumentEditor{nullptr}
        , presetName{preset}
        , geonkickModel{new GeonkickModel(this, dspProxy)}
{
        setTitle(Geonkick::applicationName);
        setScaleFactor(dspProxy->getScaleFactor());
        createViewState();
        setFixedSize(MAIN_WINDOW_WIDTH + (GeonkickConfig().isShowSidebar() ? 313 : 0),
                     MAIN_WINDOW_HEIGHT);
        dspProxy->registerCallbacks(true);
        RK_ACT_BIND(dspProxy, stateChanged, RK_ACT_ARGS(), this, updateGui());
        createShortcuts();
}

MainWindow::MainWindow(RkMain& app, DspProxy *dsp, const RkNativeWindowInfo &info)
        : GeonkickWidget(app, info)
        , dspProxy{dsp}
        , topBar{nullptr}
        , instrumentEditor{nullptr}
        , presetName{std::string()}
        , geonkickModel{new GeonkickModel(this, dspProxy)}
{
        setTitle(Geonkick::applicationName);
        setScaleFactor(dspProxy->getScaleFactor());
        createViewState();
        setFixedSize(MAIN_WINDOW_WIDTH + (GeonkickConfig().isShowSidebar() ? 313 : 0),
                     MAIN_WINDOW_HEIGHT);
        dspProxy->registerCallbacks(true);
        RK_ACT_BIND(dspProxy, stateChanged, RK_ACT_ARGS(), this, updateGui());
        createShortcuts();
}

MainWindow::~MainWindow()
{
        if (dspProxy) {
                dspProxy->registerCallbacks(false);
                dspProxy->setEventQueue(nullptr);
                if (dspProxy->isStandalone())
                        delete dspProxy;
        }
}

void MainWindow::createViewState()
{
        auto viewState = new ViewState(this);
        viewState->setName("ViewState");
        UiSettings *uiSettings = dspProxy->getUiSettings();
        viewState->setMainView(uiSettings->getMainView());
        viewState->setSamplesBrowserPath(uiSettings->samplesBrowserPath());
        RK_ACT_BIND(viewState, mainViewChanged,
                    RK_ACT_ARGS(ViewState::View view),
                    dspProxy, getUiSettings()->setMainView(view));
        RK_ACT_BIND(viewState, samplesBrowserPathChanged,
                    RK_ACT_ARGS(const std::string &path),
                    dspProxy, getUiSettings()->setSamplesBrowserPath(path));
        setViewState(viewState);
}

void MainWindow::createShortcuts()
{
        addShortcut(Rk::Key::Key_K, Rk::KeyModifiers::Control_Left);
        addShortcut(Rk::Key::Key_K, Rk::KeyModifiers::Control_Right);
        addShortcut(Rk::Key::Key_k, Rk::KeyModifiers::Control_Left);
        addShortcut(Rk::Key::Key_k, Rk::KeyModifiers::Control_Right);

        addShortcut(Rk::Key::Key_H, Rk::KeyModifiers::Control_Left);
        addShortcut(Rk::Key::Key_H, Rk::KeyModifiers::Control_Right);
        addShortcut(Rk::Key::Key_h, Rk::KeyModifiers::Control_Left);
        addShortcut(Rk::Key::Key_h, Rk::KeyModifiers::Control_Right);

        addShortcut(Rk::Key::Key_C, Rk::KeyModifiers::Control_Left);
        addShortcut(Rk::Key::Key_C, Rk::KeyModifiers::Control_Right);
        addShortcut(Rk::Key::Key_c, Rk::KeyModifiers::Control_Left);
        addShortcut(Rk::Key::Key_c, Rk::KeyModifiers::Control_Right);

        addShortcut(Rk::Key::Key_V, Rk::KeyModifiers::Control_Left);
        addShortcut(Rk::Key::Key_V, Rk::KeyModifiers::Control_Right);
        addShortcut(Rk::Key::Key_v, Rk::KeyModifiers::Control_Left);
        addShortcut(Rk::Key::Key_v, Rk::KeyModifiers::Control_Right);

        addShortcut(Rk::Key::Key_R, Rk::KeyModifiers::Control_Left);
        addShortcut(Rk::Key::Key_R, Rk::KeyModifiers::Control_Right);
        addShortcut(Rk::Key::Key_r, Rk::KeyModifiers::Control_Left);
        addShortcut(Rk::Key::Key_r, Rk::KeyModifiers::Control_Right);

        addShortcut(Rk::Key::Key_f, Rk::KeyModifiers::Control_Left);
        addShortcut(Rk::Key::Key_F, Rk::KeyModifiers::Control_Left);

        addShortcut(Rk::Key::Key_Control_Left, Rk::KeyModifiers::Control_Left);
}

bool MainWindow::init(void)
{
        if (dspProxy->isStandalone() && !dspProxy->isJackEnabled()) {
                GEONKICK_LOG_INFO("Jack is not installed or not running. "
                                  << "There is a need for jack server running "
                                  << "in order to have audio output.");
        }
        topBar = new TopBar(this, geonkickModel);
        topBar->setX(10);
        topBar->show();
        RK_ACT_BIND(this, updateGui, RK_ACT_ARGS(), topBar, updateGui());
        RK_ACT_BIND(topBar, resetToDefault, RK_ACT_ARGS(),
                    this, resetToDefault());
        RK_ACT_BIND(topBar, layerSelected,
                    RK_ACT_ARGS(DspProxy::Layer layer, bool b),
                    dspProxy, enbaleLayer(layer, b));

        // Create Sidebar
        if (GeonkickConfig().isShowSidebar()) {
                auto sidebar = new Sidebar(this, geonkickModel);
                sidebar->setPosition({MAIN_WINDOW_WIDTH, 4});
        }

        instrumentEditor = new InstrumentEditor(this, geonkickModel);
        instrumentEditor->setPosition(10, topBar->y() + topBar->height());
        instrumentEditor->show();
        RK_ACT_BIND(this, updateGui, RK_ACT_ARGS(), instrumentEditor, updateGui());

        RK_ACT_BIND(this, updateGui, RK_ACT_ARGS(), instrumentEditor, updateGui());
        if (dspProxy->isStandalone() && !presetName.empty())
                openPreset(presetName);
        topBar->setPresetName(dspProxy->getPercussionName(dspProxy->currentPercussion()));
        updateGui();
        show();
        return true;
}

void MainWindow::openPreset(const std::string &fileName)
{
        if (fileName.size() < 7) {
                RK_LOG_ERROR("Open Preset: "
                             << "Can't open preset. File name "
                             << "empty or wrong format. Format example: 'mykick.gkick'");
                return;
        }

        std::filesystem::path filePath(fileName);
        if (filePath.extension().empty()
            || !std::filesystem::is_regular_file(filePath)
            || (filePath.extension() != ".gkick"
            && filePath.extension() != ".GKICK")) {
                RK_LOG_ERROR("Open Preset: " << "Can't open preset. Wrong file format.");
                return;
        }

        std::ifstream file;
        file.open(std::filesystem::absolute(filePath));
        if (!file.is_open()) {
                RK_LOG_ERROR("Open Preset" + std::string(" - ") + std::string(GEONKICK_NAME)
                             << ". Can't open preset.");
                return;
        }

        std::string fileData((std::istreambuf_iterator<char>(file)),
                             (std::istreambuf_iterator<char>()));
        auto state = dspProxy->getDefaultPercussionState();
        state->loadData(fileData);
        if (state->getName().empty() || state->getName() == "Default")
                state->setName(filePath.stem().string());
        state->setId(dspProxy->currentPercussion());
        dspProxy->setPercussionState(state);
        action dspProxy->instrumentUpdated(state->getId());
        file.close();
        dspProxy->setCurrentWorkingPath("OpenPreset",
                                           filePath.has_parent_path() ? filePath.parent_path().string() : filePath.string());
        updateGui();
}

void MainWindow::shortcutEvent(RkKeyEvent *event)
{
        if (event->type() == RkEvent::Type::KeyPressed) {
                if (event->modifiers() & static_cast<int>(Rk::KeyModifiers::Control)
                    && (event->key() == Rk::Key::Key_k || event->key() == Rk::Key::Key_K)) {
                        dspProxy->playKick();
                } else if (event->modifiers() & static_cast<int>(Rk::KeyModifiers::Control)
                           && (event->key() == Rk::Key::Key_r || event->key() == Rk::Key::Key_R)) {
                        resetToDefault();
                } else if ((event->modifiers() & static_cast<int>(Rk::KeyModifiers::Control))
                           && (event->key() == Rk::Key::Key_c || event->key() == Rk::Key::Key_C)) {
                        dspProxy->copyToClipboard();
                } else if ((event->modifiers() & static_cast<int>(Rk::KeyModifiers::Control))
                           && (event->key() == Rk::Key::Key_v || event->key() == Rk::Key::Key_V)) {
                        dspProxy->pasteFromClipboard();
                        dspProxy->notifyPercussionUpdated(dspProxy->currentPercussion());
                        updateGui();
                } else if ((event->modifiers() & static_cast<int>(Rk::KeyModifiers::Control))
                           && (event->key() == Rk::Key::Key_F || event->key() == Rk::Key::Key_f)) {
                        dspProxy->setScaleFactor((dspProxy->getScaleFactor() + 0.5 > 2.1) ? 1 : scaleFactor() + 0.5);
                        setScaleFactor(dspProxy->getScaleFactor());
                        setFixedSize(MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
                        updateGui();
                        action onScaleFactor(dspProxy->getScaleFactor());
                }
        }
}

void MainWindow::resetToDefault()
{
        auto currId = dspProxy->currentPercussion();
        auto state = dspProxy->getDefaultPercussionState();
        state->setId(currId);
        state->setName(dspProxy->getPercussionName(currId));
        state->setPlayingKey(dspProxy->getPercussionPlayingKey(currId));
        state->setChannel(dspProxy->getPercussionChannel(currId));
        dspProxy->setPercussionState(state);
        dspProxy->notifyPercussionUpdated(dspProxy->currentPercussion());
        updateGui();
}

void MainWindow::dropEvent(RkDropEvent *event)
{
        std::string fileExtention;
        try {
                std::filesystem::path path(event->getFilePath());
                fileExtention = Geonkick::toLower(path.extension().string());
        } catch (const std::exception& e) {
                GEONKICK_LOG_ERROR("can't create path " << e.what());
                return;
        }

        std::string file = event->getFilePath();
        if (fileExtention == ".gkit") {
                geonkickModel->getKitModel()->open(file);
        } else if  (fileExtention == ".gkick") {
                openPreset(file);
        } else if (fileExtention == ".wav"
                 || fileExtention == ".flac"
                 || fileExtention == ".ogg") {
                setSample(file);
        }
}

void MainWindow::setSample(const std::string &file)
{
        /*auto osc = envelopeWidget->getCurrentOscillator();
        if (osc) {
                osc->setFunction(OscillatorModel::FunctionType::Sample);
                dspProxy->setOscillatorSample(file, osc->index());
                dspProxy->notifyPercussionUpdated(dspProxy->currentPercussion());
                updateGui();
                }*/
}

RkSize MainWindow::getWindowSize()
{
        return {MAIN_WINDOW_WIDTH + (GeonkickConfig().isShowSidebar() ? 313 : 0),
                MAIN_WINDOW_HEIGHT};
}
