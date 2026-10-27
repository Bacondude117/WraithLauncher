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
#include "WraithTiles.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QPainterPath>
#include <QTextLayout>

#include "FileSystem.h"
#include "InstanceList.h"
#include "minecraft/MinecraftInstance.h"

// defined in InstanceDelegate.cpp: running/crashed badges in the top-right corner
void drawBadges(QPainter* painter, const QStyleOptionViewItem& option, BaseInstance* instance, QIcon::Mode mode, QIcon::State state);

namespace WraithTiles {

namespace {
constexpr int s_margin = 5;
constexpr int s_padding = 12;
constexpr int s_iconSize = 64;
constexpr int s_radius = 12;

QColor mix(const QColor& a, const QColor& b, double t)
{
    return QColor::fromRgbF(static_cast<float>(a.redF() * (1 - t) + b.redF() * t), static_cast<float>(a.greenF() * (1 - t) + b.greenF() * t),
                            static_cast<float>(a.blueF() * (1 - t) + b.blueF() * t));
}

QFont nameFont(const QStyleOptionViewItem& option)
{
    QFont font = option.font;
    font.setBold(true);
    return font;
}

QFont subtitleFont(const QStyleOptionViewItem& option)
{
    QFont font = option.font;
    font.setPointSizeF(font.pointSizeF() * 0.88);
    return font;
}

/**
 * "Fabric 1.20.1", "Vanilla 1.21" ... read straight from mmc-pack.json (cached by file time) so painting never
 * has to load the instance's component list.
 */
QString subtitle(MinecraftInstance* instance)
{
    if (!instance)
        return {};
    struct Cached {
        QDateTime modified;
        QString text;
    };
    static QHash<QString, Cached> s_cache;
    const QFileInfo packFile(FS::PathCombine(instance->instanceRoot(), "mmc-pack.json"));
    auto it = s_cache.find(instance->id());
    if (it != s_cache.end() && it->modified == packFile.lastModified())
        return it->text;

    QHash<QString, QString> versions;
    QFile file(packFile.filePath());
    if (file.open(QIODevice::ReadOnly)) {
        for (const auto& component : QJsonDocument::fromJson(file.readAll()).object().value("components").toArray()) {
            const auto obj = component.toObject();
            const QString version = obj.value("version").toString(obj.value("cachedVersion").toString());
            versions.insert(obj.value("uid").toString(), version);
        }
    }
    QString loader = QObject::tr("Vanilla");
    if (versions.contains("net.fabricmc.fabric-loader"))
        loader = "Fabric";
    else if (versions.contains("org.quiltmc.quilt-loader"))
        loader = "Quilt";
    else if (versions.contains("net.neoforged"))
        loader = "NeoForge";
    else if (versions.contains("net.minecraftforge"))
        loader = "Forge";
    const QString mc = versions.value("net.minecraft");
    const QString text = mc.isEmpty() ? loader : QString("%1 %2").arg(loader, mc);
    s_cache.insert(instance->id(), { packFile.lastModified(), text });
    return text;
}

QRect cardRect(const QStyleOptionViewItem& option)
{
    return option.rect.adjusted(s_margin, s_margin, -s_margin, -s_margin);
}
}  // namespace

QSize tileSize(const QStyleOptionViewItem& option)
{
    const QFontMetrics nameFm(nameFont(option));
    const QFontMetrics subFm(subtitleFont(option));
    const int height = 2 * s_margin + s_padding + s_iconSize + 10 + 2 * nameFm.lineSpacing() + subFm.height() + s_padding;
    return { s_tileWidth, height };
}

QRect nameRect(const QStyleOptionViewItem& option)
{
    const QRect card = cardRect(option);
    const QFontMetrics nameFm(nameFont(option));
    return { card.left() + 8, card.top() + s_padding + s_iconSize + 8, card.width() - 16, 2 * nameFm.lineSpacing() + 4 };
}

void paintTile(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setClipRect(option.rect);

    const QPalette& pal = option.palette;
    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;
    const QColor text = pal.color(QPalette::Text);
    const QColor accent = pal.color(QPalette::Highlight);

    // card
    const QRect card = cardRect(option);
    QPainterPath path;
    path.addRoundedRect(QRectF(card).adjusted(0.5, 0.5, -0.5, -0.5), s_radius, s_radius);
    QColor fill = pal.color(QPalette::AlternateBase);
    if (selected)
        fill = mix(fill, accent, 0.16);
    else if (hovered)
        fill = mix(fill, accent, 0.06);
    painter->fillPath(path, fill);
    painter->setPen(QPen(selected ? accent : mix(fill, text, 0.10), selected ? 1.5 : 1.0));
    painter->drawPath(path);

    // icon on a soft rounded plate
    const QRect iconRect(card.center().x() - s_iconSize / 2, card.top() + s_padding, s_iconSize, s_iconSize);
    QPainterPath plate;
    plate.addRoundedRect(QRectF(iconRect).adjusted(-4, -4, 4, 4), 10, 10);
    painter->fillPath(plate, mix(fill, pal.color(QPalette::Base), 0.5));
    option.icon.paint(painter, iconRect, Qt::AlignCenter, (option.state & QStyle::State_Enabled) ? QIcon::Normal : QIcon::Disabled);

    // name: up to two centered lines
    const QRect nameArea = nameRect(option);
    painter->setFont(nameFont(option));
    painter->setPen(text);
    {
        const QFontMetrics fm(painter->font());
        QTextLayout layout(index.data(Qt::DisplayRole).toString(), painter->font());
        QTextOption textOption(Qt::AlignHCenter);
        textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        layout.setTextOption(textOption);
        layout.beginLayout();
        int y = nameArea.top();
        for (int line = 0; line < 2; line++) {
            QTextLine tl = layout.createLine();
            if (!tl.isValid())
                break;
            tl.setLineWidth(nameArea.width());
            QString lineText = layout.text().mid(tl.textStart(), line == 1 ? -1 : tl.textLength()).trimmed();
            if (line == 1)
                lineText = fm.elidedText(lineText, Qt::ElideRight, nameArea.width());
            painter->drawText(QRect(nameArea.left(), y, nameArea.width(), fm.height()), Qt::AlignHCenter | Qt::AlignVCenter, lineText);
            y += fm.lineSpacing();
        }
        layout.endLayout();
    }

    // loader + version
    auto* instance = static_cast<MinecraftInstance*>(index.data(InstanceList::InstancePointerRole).value<void*>());
    painter->setFont(subtitleFont(option));
    painter->setPen(mix(text, pal.color(QPalette::Base), 0.45));
    const QFontMetrics subFm(painter->font());
    const QRect subRect(card.left() + 8, card.bottom() - s_padding - subFm.height() + 2, card.width() - 16, subFm.height());
    painter->drawText(subRect, Qt::AlignHCenter | Qt::AlignVCenter, subFm.elidedText(subtitle(instance), Qt::ElideRight, subRect.width()));

    painter->restore();

    if (instance) {
        QStyleOptionViewItem badgeOption = option;
        badgeOption.rect = card.adjusted(0, 4, -4, 0);
        drawBadges(painter, badgeOption, instance, QIcon::Normal, QIcon::Off);
    }
}

}  // namespace WraithTiles
