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

#include <QWidget>

#include "ui/pages/BasePage.h"

class QVBoxLayout;

/**
 * Small "Advanced" heading page placed before the per-source modpack pages in the New Instance dialog.
 * It explains them and links to each one; the pages themselves are unchanged and always listed.
 */
class AdvancedSourcesPage : public QWidget, public BasePage {
    Q_OBJECT
   public:
    explicit AdvancedSourcesPage(QStringList pageIds, QWidget* parent = nullptr);

    QString displayName() const override { return tr("Advanced"); }
    QIcon icon() const override { return QIcon::fromTheme("settings"); }
    QString id() const override { return "advanced_sources"; }
    void openedImpl() override;

   private:
    QStringList m_pageIds;
    QVBoxLayout* m_buttons = nullptr;
};
