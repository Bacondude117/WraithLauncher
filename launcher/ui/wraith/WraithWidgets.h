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

#include <QFrame>
#include <functional>

class QLabel;
class QSlider;
class QVBoxLayout;

/** Small building blocks for the widget-based parts of the shell (settings rows, cards, disclosure buttons). */
namespace Wraith::Widgets {

/** A rounded card with an optional title; add rows with addRow(). */
class Card : public QFrame {
    Q_OBJECT
   public:
    explicit Card(const QString& title = QString(), QWidget* parent = nullptr);
    /** A settings row: label and description on the left, the control on the right. */
    QWidget* addRow(const QString& label, const QString& description, QWidget* control);
    void addWidget(QWidget* widget);

   private:
    QVBoxLayout* m_layout;
    int m_rows = 0;
};

/** Page/section title. */
QLabel* title(const QString& text, QWidget* parent, int pointSize = 18);
QLabel* muted(const QString& text, QWidget* parent);

/** Wide "Name  description  >" button used for Advanced sections. */
QWidget* disclosure(const QString& label, const QString& description, std::function<void()> onClick, QWidget* parent);

/** Memory slider (MiB, 256 MiB steps) with a live "4.0 GB" label; calls onChange when released. */
QWidget* memorySlider(int valueMiB, std::function<void(int)> onChange, QWidget* parent);

/** Toggle switch drawn like the QML ones. */
class Switch : public QWidget {
    Q_OBJECT
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
   public:
    explicit Switch(QWidget* parent = nullptr);
    bool isChecked() const { return m_checked; }
    void setChecked(bool checked);
    QSize sizeHint() const override { return { 42, 24 }; }

   signals:
    void toggled(bool checked);

   protected:
    void paintEvent(QPaintEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void enterEvent(QEnterEvent*) override { update(); }
    void leaveEvent(QEvent*) override { update(); }

   private:
    bool m_checked = false;
};

}  // namespace Wraith::Widgets
