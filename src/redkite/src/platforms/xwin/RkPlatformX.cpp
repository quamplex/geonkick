/**
 * File name: RkPlatformX.cpp
 * Project: Redkite (A small GUI toolkit)
 *
 * Copyright (C) 2019 Iurie Nistor
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

#include "RkPlatform.h"

RkNativeWindowInfo
rk_from_native_x11(Display* display, int screenNumber,
                   Window window,
                   std::string_view name,
                   std::string_view className)
{
        RkNativeWindowInfo info;
        info.display = display;
        info.screenNumber = screenNumber;
        info.window = window;
	info.name = name;
        info.className = className;
        return info;
}

RkWindowId rk_id_from_x11(Window window)
{
        RkWindowId id;
        id.id = window;
        return id;
}
