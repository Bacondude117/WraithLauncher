// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Wraith Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Wraith Launcher contributors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <functional>

class BaseInstance;

/**
 * One-line hooks that Prism code calls right before it changes an instance's mods, modpack or components (Minecraft
 * and loader versions). Wraith's UI installs the handler that takes an automatic snapshot (if enabled). Without a
 * handler (tests, classic code paths) the hooks do nothing.
 */
namespace Wraith::Snapshots {

enum class Change { Content, Modpack, Components };

using BeforeChangeHandler = std::function<void(BaseInstance*, Change)>;

void setBeforeChangeHandler(BeforeChangeHandler handler);
void beforeChange(BaseInstance* instance, Change change);

}  // namespace Wraith::Snapshots
