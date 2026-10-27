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
#include "WraithWidgets.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

#include "HardwareInfo.h"
#include "WraithStyle.h"

namespace Wraith::Widgets {

Card::Card(const QString& title, QWidget* parent) : QFrame(parent)
{
    setObjectName("wraithCard");
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(20, 16, 20, 16);
    m_layout->setSpacing(0);
    if (!title.isEmpty()) {
        auto* label = new QLabel(title, this);
        label->setObjectName("wraithSectionTitle");
        m_layout->addWidget(label);
        m_layout->addSpacing(6);
    }
}

QWidget* Card::addRow(const QString& label, const QString& description, QWidget* control)
{
    auto* row = new QFrame(this);
    if (m_rows > 0) {
        auto* line = new QFrame(this);
        line->setFixedHeight(1);
        line->setStyleSheet(QString("background-color: %1;").arg(Color::border));
        m_layout->addWidget(line);
    }
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 12, 0, 12);
    layout->setSpacing(24);
    auto* text = new QVBoxLayout();
    text->setSpacing(3);
    auto* name = new QLabel(label, row);
    name->setStyleSheet("font-weight: 600;");
    text->addWidget(name);
    if (!description.isEmpty()) {
        auto* desc = muted(description, row);
        desc->setWordWrap(true);
        text->addWidget(desc);
    }
    layout->addLayout(text, 1);
    if (control) {
        control->setParent(row);
        layout->addWidget(control, 0, Qt::AlignVCenter);
    }
    m_layout->addWidget(row);
    m_rows++;
    return row;
}

void Card::addWidget(QWidget* widget)
{
    widget->setParent(this);
    m_layout->addWidget(widget);
}

QLabel* title(const QString& text, QWidget* parent, int pointSize)
{
    auto* label = new QLabel(text, parent);
    label->setStyleSheet(QString("font-size: %1pt; font-weight: 600;").arg(pointSize));
    return label;
}

QLabel* muted(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName("wraithMuted");
    return label;
}

namespace {
/** A frame that acts like a button (QPushButton ignores child layouts when sizing itself). */
class ClickableFrame : public QFrame {
   public:
    ClickableFrame(std::function<void()> onClick, QWidget* parent) : QFrame(parent), m_onClick(std::move(onClick))
    {
        setAttribute(Qt::WA_Hover);
        setCursor(Qt::PointingHandCursor);
    }

   protected:
    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && rect().contains(event->pos()))
            m_onClick();
    }

   private:
    std::function<void()> m_onClick;
};
}  // namespace

QString literal(QString text)
{
    return text.replace('&', "&&");
}

QWidget* disclosure(const QString& label, const QString& description, std::function<void()> onClick, QWidget* parent)
{
    auto* row = new ClickableFrame(std::move(onClick), parent);
    row->setObjectName("wraithDisclosure");
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(12);
    auto* text = new QVBoxLayout();
    text->setSpacing(3);
    auto* name = new QLabel(label, row);
    name->setStyleSheet("font-weight: 600; background: transparent;");
    auto* desc = muted(description, row);
    desc->setWordWrap(true);
    desc->setStyleSheet("background: transparent;");
    text->addWidget(name);
    text->addWidget(desc);
    layout->addLayout(text, 1);
    auto* arrow = new QLabel(QString::fromUtf8("›"), row);
    arrow->setStyleSheet(QString("font-size: 18pt; color: %1; background: transparent;").arg(Color::textFaint));
    layout->addWidget(arrow);
    return row;
}

QWidget* memorySlider(int valueMiB, std::function<void(int)> onChange, QWidget* parent)
{
    auto* box = new QWidget(parent);
    auto* layout = new QHBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    auto* slider = new QSlider(Qt::Horizontal, box);
    const int total = std::max<int>(4096, static_cast<int>(HardwareInfo::totalRamMiB()));
    slider->setRange(2, std::max(4, total / 256));  // in 256 MiB steps
    slider->setValue(std::clamp(valueMiB / 256, slider->minimum(), slider->maximum()));
    slider->setFixedWidth(240);
    slider->setCursor(Qt::PointingHandCursor);
    auto* label = new QLabel(box);
    label->setMinimumWidth(64);
    label->setStyleSheet(QString("font-weight: 600; color: %1;").arg(Color::accent));
    auto update = [label](int steps) { label->setText(QString("%1 GB").arg(steps * 256 / 1024.0, 0, 'f', 1)); };
    update(slider->value());
    QObject::connect(slider, &QSlider::valueChanged, label, update);
    QObject::connect(slider, &QSlider::sliderReleased, slider, [slider, onChange] { onChange(slider->value() * 256); });
    QObject::connect(slider, &QSlider::actionTriggered, slider, [slider, onChange](int action) {
        if (action != QAbstractSlider::SliderMove)
            QMetaObject::invokeMethod(slider, [slider, onChange] { onChange(slider->value() * 256); }, Qt::QueuedConnection);
    });
    layout->addWidget(slider);
    layout->addWidget(label);
    return box;
}

Switch::Switch(QWidget* parent) : QWidget(parent)
{
    setCursor(Qt::PointingHandCursor);
    setFixedSize(sizeHint());
}

void Switch::setChecked(bool checked)
{
    if (m_checked == checked)
        return;
    m_checked = checked;
    update();
    emit toggled(checked);
}

void Switch::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const bool hovered = underMouse();
    QColor track = m_checked ? QColor(Color::accent) : QColor(hovered ? "#3a4157" : Color::surfaceRaised);
    if (!isEnabled())
        track.setAlphaF(0.4f);
    painter.setPen(m_checked ? Qt::NoPen : QPen(QColor(Color::borderStrong), 1));
    painter.setBrush(track);
    painter.drawRoundedRect(QRectF(0.5, 0.5, width() - 1, height() - 1), height() / 2.0, height() / 2.0);
    const double knob = height() - 8;
    const double x = m_checked ? width() - knob - 4 : 4;
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_checked ? QColor(Color::accentText) : QColor(Color::textMuted));
    painter.drawEllipse(QRectF(x, 4, knob, knob));
}

void Switch::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && rect().contains(event->pos()))
        setChecked(!m_checked);
}

}  // namespace Wraith::Widgets
