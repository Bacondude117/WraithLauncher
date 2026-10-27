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
#include "PackCardDelegate.h"

#include <QPainter>
#include <QPainterPath>
#include <QTextLayout>

namespace {
constexpr int s_margin = 6;     // space between cards
constexpr int s_padding = 12;   // space inside a card
constexpr int s_iconSize = 64;
constexpr int s_radius = 10;
constexpr int s_cardHeight = s_iconSize + 2 * s_padding + 8;

QColor mix(const QColor& a, const QColor& b, double t)
{
    return QColor::fromRgbF(static_cast<float>(a.redF() * (1 - t) + b.redF() * t), static_cast<float>(a.greenF() * (1 - t) + b.greenF() * t),
                            static_cast<float>(a.blueF() * (1 - t) + b.blueF() * t));
}

/** Draw text wrapped to at most maxLines lines, eliding the last one. Returns the height used. */
int drawWrapped(QPainter* painter, const QRect& rect, const QString& text, int maxLines)
{
    const QFontMetrics fm(painter->font());
    QTextLayout layout(QString(text).replace('\n', ' '), painter->font());
    layout.beginLayout();
    int y = 0;
    for (int line = 0; line < maxLines; line++) {
        QTextLine tl = layout.createLine();
        if (!tl.isValid())
            break;
        tl.setLineWidth(rect.width());
        const bool last = line == maxLines - 1;
        QString lineText = layout.text().mid(tl.textStart(), last ? -1 : tl.textLength());
        if (last)
            lineText = fm.elidedText(lineText, Qt::ElideRight, rect.width());
        painter->drawText(QRect(rect.left(), rect.top() + y, rect.width(), fm.height()), Qt::AlignLeft | Qt::AlignVCenter, lineText.trimmed());
        y += fm.lineSpacing();
    }
    layout.endLayout();
    return y;
}
}  // namespace

PackCardDelegate::PackCardDelegate(Roles roles, QObject* parent) : QStyledItemDelegate(parent), m_roles(roles) {}

QColor PackCardDelegate::sourceColor(const QString& source, const QPalette& palette)
{
    if (source == "Modrinth")
        return QColor(27, 217, 106);
    if (source == "CurseForge")
        return QColor(241, 100, 54);
    if (source == "FTB")
        return QColor(240, 80, 90);
    if (source == "ATLauncher")
        return QColor(90, 200, 220);
    if (source == "Technic")
        return QColor(90, 150, 255);
    return palette.color(QPalette::Highlight);
}

QString PackCardDelegate::formatDownloads(qint64 downloads)
{
    if (downloads <= 0)
        return {};
    if (downloads >= 1'000'000)
        return tr("%1M downloads").arg(QString::number(static_cast<double>(downloads) / 1e6, 'f', 1));
    if (downloads >= 1'000)
        return tr("%1K downloads").arg(QString::number(static_cast<double>(downloads) / 1e3, 'f', 1));
    return tr("%1 downloads").arg(downloads);
}

QSize PackCardDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex&) const
{
    return { option.rect.width() > 0 ? option.rect.width() : 400, s_cardHeight + 2 * s_margin };
}

void PackCardDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    const QPalette& pal = option.palette;
    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;
    const QColor base = pal.color(QPalette::Base);
    const QColor text = pal.color(QPalette::Text);
    const QColor accent = pal.color(QPalette::Highlight);
    const QColor muted = mix(text, base, 0.45);

    // card background
    const QRect card = option.rect.adjusted(s_margin, s_margin, -s_margin, -s_margin);
    QPainterPath path;
    path.addRoundedRect(QRectF(card).adjusted(0.5, 0.5, -0.5, -0.5), s_radius, s_radius);
    QColor fill = pal.color(QPalette::AlternateBase);
    if (selected)
        fill = mix(fill, accent, 0.18);
    else if (hovered)
        fill = mix(fill, accent, 0.07);
    painter->fillPath(path, fill);
    painter->setPen(QPen(selected ? accent : mix(fill, text, 0.12), selected ? 1.5 : 1.0));
    painter->drawPath(path);

    // icon, clipped to a rounded square
    const QRect iconRect(card.left() + s_padding, card.top() + (card.height() - s_iconSize) / 2, s_iconSize, s_iconSize);
    const QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    if (!icon.isNull()) {
        painter->save();
        QPainterPath iconClip;
        iconClip.addRoundedRect(QRectF(iconRect), 8, 8);
        painter->setClipPath(iconClip);
        icon.paint(painter, iconRect, Qt::AlignCenter);
        painter->restore();
    }

    const int textLeft = iconRect.right() + s_padding;
    const int textRight = card.right() - s_padding;
    int y = card.top() + s_padding - 2;

    // title + author on one line
    QFont titleFont = option.font;
    titleFont.setBold(true);
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.12);
    painter->setFont(titleFont);
    painter->setPen(text);
    const QFontMetrics titleFm(titleFont);
    const QString title = titleFm.elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideRight, textRight - textLeft);
    painter->drawText(QRect(textLeft, y, textRight - textLeft, titleFm.height()), Qt::AlignLeft | Qt::AlignVCenter, title);

    const QString author = m_roles.author >= 0 ? index.data(m_roles.author).toString() : QString();
    if (!author.isEmpty()) {
        const int titleWidth = titleFm.horizontalAdvance(title) + 8;
        painter->setFont(option.font);
        painter->setPen(muted);
        const QFontMetrics fm(option.font);
        const QRect authorRect(textLeft + titleWidth, y, textRight - textLeft - titleWidth, titleFm.height());
        painter->drawText(authorRect, Qt::AlignLeft | Qt::AlignVCenter, fm.elidedText(tr("by %1").arg(author), Qt::ElideRight, authorRect.width()));
    }
    y += titleFm.height() + 2;

    // summary, two lines
    painter->setFont(option.font);
    painter->setPen(mix(text, base, 0.2));
    const QString summary = m_roles.summary >= 0 ? index.data(m_roles.summary).toString() : QString();
    if (!summary.isEmpty())
        drawWrapped(painter, QRect(textLeft, y, textRight - textLeft, card.bottom() - y), summary, 2);

    // bottom row: source chips on the left, downloads on the right
    QFont smallFont = option.font;
    smallFont.setPointSizeF(smallFont.pointSizeF() * 0.88);
    smallFont.setBold(true);
    const QFontMetrics smallFm(smallFont);
    const int chipHeight = smallFm.height() + 4;
    const int chipTop = card.bottom() - s_padding - chipHeight + 4;
    painter->setFont(smallFont);
    int x = textLeft;
    const QStringList sources = m_roles.sources >= 0 ? index.data(m_roles.sources).toStringList() : QStringList();
    for (const auto& source : sources) {
        const QColor color = sourceColor(source, pal);
        const int w = smallFm.horizontalAdvance(source) + 14;
        const QRectF chip(x, chipTop, w, chipHeight);
        QPainterPath chipPath;
        chipPath.addRoundedRect(chip, chipHeight / 2.0, chipHeight / 2.0);
        QColor chipFill = color;
        chipFill.setAlphaF(0.18f);
        painter->fillPath(chipPath, chipFill);
        painter->setPen(mix(color, text, base.lightness() < 128 ? 0.25 : 0.55));
        painter->drawText(chip, Qt::AlignCenter, source);
        x += w + 6;
    }

    const qint64 downloads = m_roles.downloads >= 0 ? index.data(m_roles.downloads).toLongLong() : 0;
    const QString downloadsText = formatDownloads(downloads);
    if (!downloadsText.isEmpty()) {
        QFont dlFont = option.font;
        dlFont.setPointSizeF(dlFont.pointSizeF() * 0.9);
        painter->setFont(dlFont);
        painter->setPen(muted);
        painter->drawText(QRect(x, chipTop, textRight - x, chipHeight), Qt::AlignRight | Qt::AlignVCenter, downloadsText);
    }

    painter->restore();
}
