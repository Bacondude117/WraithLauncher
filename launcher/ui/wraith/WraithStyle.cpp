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
#include "WraithStyle.h"

#include <QBuffer>
#include <QFile>
#include <QFrame>
#include <QImageReader>
#include <QMainWindow>
#include <QPixmap>
#include <QToolBar>
#include <QToolButton>

namespace Wraith {

QString shellStyleSheet()
{
    using namespace Color;
    QString css = R"(
#wraithHost, #wraithHost QWidget { background-color: %bg%; color: %text%; font-size: 10pt; }
#wraithHost QLabel { background: transparent; }
#wraithHost QLabel#wraithSectionTitle { font-size: 13pt; font-weight: 600; padding: 4px 0px; }
#wraithHost QLabel#wraithMuted { color: %textMuted%; }
#wraithHost QFrame#wraithCard { background-color: %surface%; border: 1px solid %border%; border-radius: 14px; }
#wraithHost QFrame#wraithCard QWidget { background: transparent; }
#wraithHost QFrame#wraithRow { background: transparent; border: none; border-bottom: 1px solid %border%; }

#wraithHost QPushButton, #wraithHost QToolButton {
    background-color: %surfaceRaised%; color: %text%; border: 1px solid %border%; border-radius: 9px; padding: 6px 14px; min-height: 18px;
}
#wraithHost QPushButton:hover, #wraithHost QToolButton:hover { background-color: #2a3044; border-color: %borderStrong%; }
#wraithHost QPushButton:pressed, #wraithHost QToolButton:pressed { background-color: #1a1e2b; }
#wraithHost QPushButton:disabled, #wraithHost QToolButton:disabled { color: %textFaint%; background-color: %surface%; }
#wraithHost QPushButton:checked, #wraithHost QToolButton:checked { background-color: #23404a; border-color: %accent%; color: %accent%; }
#wraithHost QPushButton#wraithPrimary { background-color: %accent%; color: %accentText%; border: none; font-weight: 600; }
#wraithHost QPushButton#wraithPrimary:hover { background-color: #9de6f1; }
#wraithHost QPushButton#wraithDanger { color: %danger%; }
#wraithHost QPushButton#wraithPill { border-radius: 15px; padding: 5px 14px; background-color: transparent; border: 1px solid %border%; color: %textMuted%; }
#wraithHost QPushButton#wraithPill:hover { color: %text%; background-color: %surface%; }
#wraithHost QPushButton#wraithPill:checked { background-color: %accent%; color: %accentText%; border-color: %accent%; font-weight: 600; }
#wraithHost QPushButton#wraithNav { background: transparent; border: none; border-radius: 10px; text-align: left; padding: 10px 14px; color: %textMuted%; font-size: 10.5pt; }
#wraithHost QPushButton#wraithNav:hover { background-color: %surface%; color: %text%; }
#wraithHost QPushButton#wraithNav:checked { background-color: rgba(127, 219, 234, 30); color: %text%; font-weight: 600; }
#wraithHost QPushButton#wraithLink { background: transparent; border: none; color: %violet%; padding: 2px 0px; text-align: left; }
#wraithHost QPushButton#wraithLink:hover { color: #c3b3ff; }
#wraithHost QFrame#wraithCard QFrame#wraithDisclosure { background: transparent; border: none; border-radius: 10px; }
#wraithHost QFrame#wraithCard QFrame#wraithDisclosure:hover { background-color: %surfaceHover%; }
#wraithHost QPushButton::menu-indicator, #wraithHost QToolButton::menu-indicator { width: 0px; image: none; }
#wraithHost QToolButton::menu-button { border: none; width: 16px; }

#wraithHost QToolBar { background: transparent; border: none; spacing: 6px; padding: 0px 0px 10px 0px; }
#wraithHost QToolBar::separator { width: 1px; background: %border%; margin: 4px 6px; }
#wraithHost QToolBar QToolButton { padding: 5px 12px; }

#wraithHost QLineEdit, #wraithHost QAbstractSpinBox, #wraithHost QComboBox, #wraithHost QPlainTextEdit, #wraithHost QTextEdit, #wraithHost QTextBrowser {
    background-color: %surface%; color: %text%; border: 1px solid %border%; border-radius: 8px; padding: 5px 8px;
    selection-background-color: #2d5560; selection-color: %text%;
}
#wraithHost QAbstractSpinBox QLineEdit { border: none; padding: 0px; background: transparent; }
#wraithHost QLineEdit:focus, #wraithHost QAbstractSpinBox:focus, #wraithHost QComboBox:focus, #wraithHost QPlainTextEdit:focus, #wraithHost QTextEdit:focus { border-color: %accent%; }
#wraithHost QLineEdit:disabled, #wraithHost QAbstractSpinBox:disabled, #wraithHost QComboBox:disabled { color: %textFaint%; }
#wraithHost QComboBox::drop-down { border: none; width: 22px; }
#wraithHost QComboBox QAbstractItemView { background-color: %surfaceRaised%; border: 1px solid %borderStrong%; border-radius: 8px; selection-background-color: #2d5560; outline: none; }

#wraithHost QCheckBox, #wraithHost QRadioButton { background: transparent; spacing: 8px; }
#wraithHost QCheckBox::indicator { width: 16px; height: 16px; border-radius: 5px; border: 1px solid %borderStrong%; background: %surface%; }
#wraithHost QCheckBox::indicator:checked { background: %accent%; border-color: %accent%; }
#wraithHost QRadioButton::indicator { width: 14px; height: 14px; border-radius: 8px; border: 1px solid %borderStrong%; background: %surface%; }
#wraithHost QRadioButton::indicator:checked { background: %accent%; border: 4px solid %surface%; outline: 1px solid %accent%; }

#wraithHost QGroupBox {
    background-color: %surface%; border: 1px solid %border%; border-radius: 14px; margin-top: 26px; padding: 14px 12px 10px 12px; font-weight: 600;
}
#wraithHost QGroupBox QWidget { background: transparent; font-weight: normal; }
#wraithHost QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 4px; top: 2px; padding: 0px 4px; color: %text%; }
#wraithHost QGroupBox::indicator { width: 16px; height: 16px; border-radius: 5px; border: 1px solid %borderStrong%; background: %surface%; }
#wraithHost QGroupBox::indicator:checked { background: %accent%; border-color: %accent%; }

#wraithHost QAbstractItemView {
    background-color: %surface%; alternate-background-color: #191c26; border: 1px solid %border%; border-radius: 12px;
    selection-background-color: #23404a; selection-color: %text%; outline: none;
}
#wraithHost QAbstractItemView::item { padding: 4px 2px; }
#wraithHost QAbstractItemView::item:hover { background-color: %surfaceHover%; }
#wraithHost QAbstractItemView::item:selected { background-color: #23404a; color: %text%; }
#wraithHost QHeaderView { background: transparent; border: none; }
#wraithHost QHeaderView::section { background-color: %surface%; color: %textMuted%; border: none; border-bottom: 1px solid %border%; padding: 6px 8px; font-weight: 600; }
#wraithHost QTableCornerButton::section { background: %surface%; border: none; }

#wraithHost QTabWidget::pane { border: none; border-top: 1px solid %border%; top: -1px; }
#wraithHost QTabBar { background: transparent; }
#wraithHost QTabBar::tab { background: transparent; color: %textMuted%; border: none; border-bottom: 2px solid transparent; padding: 8px 14px; margin-right: 4px; }
#wraithHost QTabBar::tab:hover { color: %text%; }
#wraithHost QTabBar::tab:selected { color: %text%; border-bottom-color: %accent%; }

#wraithHost QScrollArea { border: none; }
#wraithHost QSplitter::handle { background: %border%; }
#wraithHost QProgressBar { background: %surface%; border: 1px solid %border%; border-radius: 6px; text-align: center; color: %text%; }
#wraithHost QProgressBar::chunk { background-color: %accent%; border-radius: 5px; }
#wraithHost QSlider::groove:horizontal { height: 4px; background: %surfaceRaised%; border-radius: 2px; }
#wraithHost QSlider::sub-page:horizontal { background: %accent%; border-radius: 2px; }
#wraithHost QSlider::handle:horizontal { width: 16px; height: 16px; margin: -6px 0px; border-radius: 8px; background: %text%; }

#wraithShell QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
#wraithShell QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }
#wraithShell QScrollBar::handle { background: #2c3244; border-radius: 3px; min-height: 24px; min-width: 24px; }
#wraithShell QScrollBar::handle:hover { background: #3b4259; }
#wraithShell QScrollBar::add-line, #wraithShell QScrollBar::sub-line { width: 0px; height: 0px; }
#wraithShell QScrollBar::add-page, #wraithShell QScrollBar::sub-page { background: transparent; }

#wraithShell QMenu { background-color: %surfaceRaised%; color: %text%; border: 1px solid %borderStrong%; border-radius: 10px; padding: 6px; }
#wraithShell QMenu::item { padding: 7px 26px 7px 12px; border-radius: 6px; background: transparent; }
#wraithShell QMenu::item:selected { background-color: #2f3650; }
#wraithShell QMenu::item:disabled { color: %textFaint%; }
#wraithShell QMenu::separator { height: 1px; background: %border%; margin: 5px 8px; }
#wraithShell QMenu::section { color: %textMuted%; padding: 6px 12px 4px 12px; font-weight: 600; }
#wraithShell QToolTip { background-color: %surfaceRaised%; color: %text%; border: 1px solid %borderStrong%; border-radius: 6px; padding: 4px 8px; }

#wraithShell QFrame#wraithToast { background-color: %surfaceRaised%; border: 1px solid %borderStrong%; border-radius: 12px; }
#wraithShell QFrame#wraithToast QLabel { color: %text%; background: transparent; }
#wraithShell QFrame#wraithToast QPushButton { background: transparent; border: none; color: %accent%; font-weight: 600; padding: 4px 8px; }
#wraithShell QFrame#wraithToast QPushButton:hover { color: #b5eef7; }
#wraithShell QFrame#wraithToast QToolButton { background: transparent; border: none; color: %textFaint%; padding: 2px; }
#wraithShell QFrame#wraithToast QToolButton:hover { color: %text%; }
)";
    const QList<std::pair<const char*, const char*>> tokens = {
        { "%bg%", bg },           { "%surfaceHover%", surfaceHover }, { "%surfaceRaised%", surfaceRaised },
        { "%surface%", surface }, { "%borderStrong%", borderStrong }, { "%border%", border },
        { "%textMuted%", textMuted }, { "%textFaint%", textFaint }, { "%text%", text },
        { "%accentText%", accentText }, { "%accent%", accent },   { "%violet%", violet },
        { "%danger%", danger },
    };
    for (const auto& [key, value] : tokens)
        css.replace(key, value);
    return css;
}

QIcon glyphIcon(const QString& name, const QString& color, const QString& checkedColor)
{
    QFile file(QString(":/wraith/icons/%1.svg").arg(name));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QByteArray svg = file.readAll();
    auto render = [&svg](const QString& tint) {
        QByteArray tinted = svg;
        tinted.replace("currentColor", tint.toUtf8());
        QBuffer buffer(&tinted);
        QImageReader reader(&buffer, "svg");
        reader.setScaledSize(QSize(48, 48));
        return QPixmap::fromImage(reader.read());
    };
    QIcon icon;
    icon.addPixmap(render(color), QIcon::Normal, QIcon::Off);
    if (!checkedColor.isEmpty())
        icon.addPixmap(render(checkedColor), QIcon::Normal, QIcon::On);
    return icon;
}

void modernizePage(QWidget* page)
{
    QList<QMainWindow*> windows = page->findChildren<QMainWindow*>();
    if (auto* window = qobject_cast<QMainWindow*>(page))
        windows.prepend(window);
    for (auto* window : windows) {
        for (auto* bar : window->findChildren<QToolBar*>(Qt::FindDirectChildrenOnly)) {
            window->removeToolBar(bar);
            window->addToolBar(Qt::TopToolBarArea, bar);
            bar->setOrientation(Qt::Horizontal);
            bar->setMovable(false);
            bar->setToolButtonStyle(Qt::ToolButtonTextOnly);
            for (auto* button : bar->findChildren<QToolButton*>())
                button->setToolButtonStyle(Qt::ToolButtonTextOnly);
            bar->show();
        }
    }
    for (auto* frame : page->findChildren<QFrame*>()) {
        if (frame->frameShape() == QFrame::StyledPanel || frame->frameShape() == QFrame::Panel || frame->frameShape() == QFrame::Box)
            frame->setFrameShadow(QFrame::Plain);
    }
}

}  // namespace Wraith
