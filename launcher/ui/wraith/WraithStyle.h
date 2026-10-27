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

#include <QIcon>
#include <QString>

class QWidget;

/** Design tokens shared by the QML views (Theme.qml mirrors these) and the widget pages hosted in the shell. */
namespace Wraith::Color {
inline constexpr const char* bg = "#0e1016";
inline constexpr const char* sidebar = "#0a0c11";
inline constexpr const char* surface = "#161922";
inline constexpr const char* surfaceHover = "#1d2130";
inline constexpr const char* surfaceRaised = "#222738";
inline constexpr const char* border = "#262b3a";
inline constexpr const char* borderStrong = "#353b50";
inline constexpr const char* text = "#e8eaf2";
inline constexpr const char* textMuted = "#9aa1b5";
inline constexpr const char* textFaint = "#636a80";
inline constexpr const char* accent = "#7fdbea";
inline constexpr const char* accentText = "#0b1418";
inline constexpr const char* violet = "#a58cff";
inline constexpr const char* success = "#4fd18b";
inline constexpr const char* warning = "#f2c45a";
inline constexpr const char* danger = "#ff6b7d";
}  // namespace Wraith::Color

namespace Wraith {
/** Stylesheet for everything inside the shell that is a widget (embedded pages, menus, toasts, settings rows). */
QString shellStyleSheet();

/** One of the shell's line glyphs (launcher/ui/wraith/icons) as a QIcon; `checkedColor` is used for the On state. */
QIcon glyphIcon(const QString& name, const QString& color, const QString& checkedColor = QString());

/**
 * Makes an original Prism page fit the shell: its side toolbars become a horizontal action bar at the top with
 * text labels, and frames lose their sunken borders.
 */
void modernizePage(QWidget* page);
}  // namespace Wraith
