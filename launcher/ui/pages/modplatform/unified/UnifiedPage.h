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

#include <QTimer>
#include <QWidget>

#include "modplatform/unified/UnifiedPack.h"
#include "ui/pages/modplatform/ModpackProviderBasePage.h"

class NewInstanceDialog;
class QLabel;
class QLineEdit;
class QListView;
class QTextBrowser;
class QHBoxLayout;

namespace Unified {
class UnifiedModel;
}

/**
 * "Browse Modpacks": searches every modpack source at once and shows merged cards.
 * Installing hands off to the existing per-source page (pre-filled search), which owns version
 * selection and the install task.
 */
class UnifiedPage : public QWidget, public ModpackProviderBasePage {
    Q_OBJECT

   public:
    explicit UnifiedPage(NewInstanceDialog* dialog, QWidget* parent = nullptr);
    ~UnifiedPage() override = default;

    QString displayName() const override { return tr("Browse Modpacks"); }
    QIcon icon() const override;
    QString id() const override { return "unified"; }
    QString helpPage() const override { return "Modpack-platforms"; }
    void openedImpl() override;
    void retranslate() override;

    void setSearchTerm(QString term) override;
    QString getSerachTerm() const override;

    /** Id of the per-source page in the New Instance dialog that installs from source. */
    static QString pageIdFor(Unified::Source source);

   private:
    void triggerSearch();
    void updateStatus();
    void updateDetails();
    void installFrom(Unified::Source source);

   private:
    NewInstanceDialog* m_dialog = nullptr;
    Unified::UnifiedModel* m_model = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QLabel* m_status = nullptr;
    QListView* m_view = nullptr;
    QTextBrowser* m_details = nullptr;
    QHBoxLayout* m_installButtons = nullptr;
    QTimer m_searchTimer;
    bool m_searchedOnce = false;
};
