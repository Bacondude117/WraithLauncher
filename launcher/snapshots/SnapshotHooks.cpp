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

#include "SnapshotHooks.h"

namespace Wraith::Snapshots {

namespace {
BeforeChangeHandler& handler()
{
    static BeforeChangeHandler s_handler;
    return s_handler;
}
}  // namespace

void setBeforeChangeHandler(BeforeChangeHandler h)
{
    handler() = std::move(h);
}

void beforeChange(BaseInstance* instance, Change change)
{
    if (instance && handler())
        handler()(instance, change);
}

}  // namespace Wraith::Snapshots
