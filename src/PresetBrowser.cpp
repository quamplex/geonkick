/**
 * File name: PresetBrowser.cpp
 * Project: Geonkick (A percussive synthesizer)
 *
 * Copyright (C) 2025 Iurie Nistor
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

#include "PresetBrowser.h"
#include "GeonkickConfig.h"
#include "FileBrowser.h"
#include "kit_model.h"
#include "InstrumentModel.h"
#include "preset.h"
#include "DesktopPaths.h"
#include "PathBookmarksModel.h"
#include "FilesView.h"
#include "geonkick_button.h"

#include "RkContainer.h"

RK_DECLARE_IMAGE_RC(lock16x16);
RK_DECLARE_IMAGE_RC(lock16x16_hover);
RK_DECLARE_IMAGE_RC(lock16x16_on);

PresetBrowser::PresetBrowser(GeonkickWidget *parent, KitModel* model)
        : GeonkickWidget(parent)
        , kitModel{model}
        , fileBrowser{nullptr}
        , lockSettings{nullptr}
{
        setSize(306, parent->height() - 30);

        auto mainLayout = new RkContainer(this, Rk::Orientation::Vertical);
        mainLayout->setSize(size());

        // Preset Folders
        fileBrowser = new FileBrowser(this, "Presets Folders", true);
        lockSettings = new GeonkickButton(fileBrowser);
        lockSettings->setCheckable(true);
        lockSettings->setSize(16, 16);
        lockSettings->setImage(RK_RC_IMAGE(lock16x16), RkButton::State::Unpressed);
        lockSettings->setImage(RK_RC_IMAGE(lock16x16_hover), RkButton::State::UnpressedHover);
        lockSettings->setImage(RK_RC_IMAGE(lock16x16_on), RkButton::State::Pressed);
        lockSettings->setImage(RK_RC_IMAGE(lock16x16_hover), RkButton::State::PressedHover);
        lockSettings->setPressed(GeonkickConfig(false).keepInstrumentSettings());
        RK_ACT_BIND(lockSettings,
                    toggled,
                    RK_ACT_ARGS(bool enabled),
                    this,
                    setKeepInstrumentSettings(enabled));
        fileBrowser->getMenu()->addSpace(3);
        fileBrowser->getMenu()->addWidget(lockSettings);
        fileBrowser->setSize({width(), height() / 2 + 17});
        fileBrowser->setCurrentDirectoy(GeonkickConfig().getPresetCurrentPath());
        fileBrowser->getBookmarks()->addPath(DesktopPaths().getFactoryPresetsPath());
        fileBrowser->getBookmarks()->addPath(DesktopPaths().getUserPresetsPath());
        mainLayout->addWidget(fileBrowser);
        mainLayout->addSpace(5);

        // Presets
        auto filesView = new FilesView(this);
        filesView->showFolders(false);
        filesView->setSize({width(), height() / 2});
        filesView->setFilters({".gkit", ".gkick"});
        filesView->setCurrentPath(GeonkickConfig().getPresetCurrentPath());
        filesView->show();
        mainLayout->addWidget(filesView);

        RK_ACT_BIND(fileBrowser,
                    currentPathChanged,
                    RK_ACT_ARGS(const fs::path &path),
                    this,
                    setPresetCurrentPath(path));
        RK_ACT_BIND(fileBrowser,
                    currentPathChanged,
                    RK_ACT_ARGS(const fs::path &path),
                    filesView,
                    setCurrentPath(path));
        RK_ACT_BIND(fileBrowser,
                    folderSelected,
                    RK_ACT_ARGS(const fs::path &path),
                    filesView,
                    setCurrentPath(path));
        RK_ACT_BIND(fileBrowser,
                    fileSelected,
                    RK_ACT_ARGS(const fs::path &path),
                    filesView,
                    setCurrentPath(path));
        RK_ACT_BIND(fileBrowser,
                    createFile,
                    RK_ACT_ARGS(),
                    filesView,
                    createFile());
        RK_ACT_BIND(filesView,
                    fileSelected,
                    RK_ACT_ARGS(const fs::path &file),
                    this,
                    loadPresetFile(file));
        RK_ACT_BIND(filesView,
                    onCreateFile,
                    RK_ACT_ARGS(const fs::path &filePath),
                    this,
                    savePresetFile(filePath));

        show();
}

void PresetBrowser::savePresetFile(const fs::path &filePath)
{
        if (Geonkick::toLower(filePath.extension().string()) == ".gkick")
                kitModel->savePreset(filePath.string());
        else
                kitModel->save(filePath.string());
}

void PresetBrowser::loadPresetFile(const fs::path &filePath)
{
        kitModel->loadPreset(Preset(filePath),
                             kitModel->selectedPercussion(),
                             lockSettings->isPressed());
}

void PresetBrowser::setKeepInstrumentSettings(bool enabled)
{
        GeonkickConfig config;
        config.setKeepInstrumentSettings(enabled);
}

void PresetBrowser::setPresetCurrentPath(const fs::path &path)
{
        GeonkickConfig().setPresetCurrentPath(path);
}
