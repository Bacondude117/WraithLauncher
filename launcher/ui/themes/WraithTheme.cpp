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
#include "WraithTheme.h"

#include <QObject>

#include "Application.h"
#include "settings/SettingsObject.h"

namespace {
// Spectral palette. Accents are kept soft so long sessions stay easy on the eyes.
const QColor s_window(27, 29, 36);      // #1b1d24 deep charcoal
const QColor s_base(21, 23, 29);        // #15171d input/list background
const QColor s_alternate(31, 34, 43);   // #1f222b
const QColor s_button(37, 40, 51);      // #252833
const QColor s_text(230, 233, 242);     // #e6e9f2
const QColor s_mutedText(122, 128, 145);  // #7a8091
const QColor s_cyan(127, 219, 234);     // #7fdbea pale cyan accent
const QColor s_violet(180, 167, 255);   // #b4a7ff soft violet accent
const QColor s_onAccent(16, 18, 26);    // #10121a
}  // namespace

bool WraithTheme::isActive()
{
    return APPLICATION && APPLICATION->settings() && APPLICATION->settings()->get("ApplicationTheme").toString() == themeId;
}

QString WraithTheme::id()
{
    return themeId;
}

QString WraithTheme::name()
{
    return QObject::tr("Wraith");
}

QString WraithTheme::tooltip()
{
    return QObject::tr("Wraith Launcher's default dark theme");
}

QPalette WraithTheme::colorScheme()
{
    QPalette palette;
    palette.setColor(QPalette::Window, s_window);
    palette.setColor(QPalette::WindowText, s_text);
    palette.setColor(QPalette::Base, s_base);
    palette.setColor(QPalette::AlternateBase, s_alternate);
    palette.setColor(QPalette::ToolTipBase, s_button);
    palette.setColor(QPalette::ToolTipText, s_text);
    palette.setColor(QPalette::Text, s_text);
    palette.setColor(QPalette::Button, s_button);
    palette.setColor(QPalette::ButtonText, s_text);
    palette.setColor(QPalette::BrightText, QColor(255, 128, 160));
    palette.setColor(QPalette::Link, s_violet);
    palette.setColor(QPalette::LinkVisited, s_violet.darker(115));
    palette.setColor(QPalette::Highlight, s_cyan);
    palette.setColor(QPalette::HighlightedText, s_onAccent);
    palette.setColor(QPalette::PlaceholderText, s_mutedText);
    palette.setColor(QPalette::Light, QColor(52, 56, 70));
    palette.setColor(QPalette::Midlight, QColor(44, 47, 59));
    palette.setColor(QPalette::Mid, QColor(34, 37, 46));
    palette.setColor(QPalette::Dark, QColor(18, 19, 24));
    palette.setColor(QPalette::Shadow, QColor(11, 12, 16));
    palette.setColor(QPalette::Disabled, QPalette::Text, s_mutedText);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, s_mutedText);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, s_mutedText);
    return fadeInactive(palette, fadeAmount(), fadeColor());
}

double WraithTheme::fadeAmount()
{
    return 0.5;
}

QColor WraithTheme::fadeColor()
{
    return s_window;
}

bool WraithTheme::hasStyleSheet()
{
    return true;
}

QString WraithTheme::appStyleSheet()
{
    // Kept deliberately small: Fusion + palette does most of the work, QSS only adds rounding and spacing.
    return QStringLiteral(R"(
QToolTip { color: #e6e9f2; background-color: #252833; border: 1px solid #b4a7ff; border-radius: 4px; padding: 4px; }
QLineEdit, QPlainTextEdit, QTextEdit, QSpinBox, QComboBox {
    border: 1px solid #343846; border-radius: 6px; padding: 1px 4px; background-color: #15171d;
}
QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus, QSpinBox:focus, QComboBox:focus { border-color: #7fdbea; }
QPushButton {
    border: 1px solid #343846; border-radius: 6px; padding: 5px 12px; background-color: #252833;
}
QPushButton:hover { border-color: #7fdbea; background-color: #2c3040; }
QPushButton:pressed { background-color: #1f222b; }
QPushButton:default { border-color: #b4a7ff; }
QPushButton:disabled { color: #7a8091; border-color: #252833; }
QListView, QTreeView, QTableView { border: 1px solid #252833; border-radius: 8px; }
QListView::item:hover, QTreeView::item:hover { background-color: rgba(127, 219, 234, 30); }
QToolBar { border: none; spacing: 4px; padding: 3px; }
QToolBar QToolButton { border-radius: 6px; padding: 4px; }
QToolBar QToolButton:hover { background-color: rgba(180, 167, 255, 40); }
QStatusBar { border-top: 1px solid #252833; }
)");
}
