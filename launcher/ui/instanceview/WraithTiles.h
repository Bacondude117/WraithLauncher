// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Wraith Launcher - Minecraft Launcher (fork of Prism Launcher)
 *  Copyright (C) 2026 Wraith Launcher Contributors
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

#include <QModelIndex>
#include <QStyleOptionViewItem>

class QPainter;

/** Library tile painting for the instance grid when the Wraith theme is active (Modrinth/CurseForge-style cards). */
namespace WraithTiles {

constexpr int s_tileWidth = 156;

QSize tileSize(const QStyleOptionViewItem& option);
void paintTile(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index);
/** Where the inline rename editor goes. */
QRect nameRect(const QStyleOptionViewItem& option);

}  // namespace WraithTiles
