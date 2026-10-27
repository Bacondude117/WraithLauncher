// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Wraith Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Wraith Launcher contributors
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

#include "JvmPresets.h"

#include <QObject>
#include <QStringList>

#include <algorithm>

namespace Wraith::Advanced {

QList<JvmPreset> jvmPresets()
{
    return {
        { "g1-tuned", QObject::tr("G1, tuned for Minecraft"), QObject::tr("Smooth frame times for most modpacks"),
          QObject::tr("Java's default collector with settings widely used for Minecraft (based on Aikar's flags): shorter, more "
                      "predictable pauses and less stutter in big packs. A good default for anything modded."),
          "-XX:+UseG1GC -XX:+ParallelRefProcEnabled -XX:MaxGCPauseMillis=200 -XX:+UnlockExperimentalVMOptions "
          "-XX:+DisableExplicitGC -XX:+AlwaysPreTouch -XX:G1NewSizePercent=30 -XX:G1MaxNewSizePercent=40 -XX:G1HeapRegionSize=8M "
          "-XX:G1ReservePercent=20 -XX:G1HeapWastePercent=5 -XX:G1MixedGCCountTarget=4 -XX:InitiatingHeapOccupancyPercent=15 "
          "-XX:G1MixedGCLiveThresholdPercent=90 -XX:G1RSetUpdatingPauseTimePercent=5 -XX:SurvivorRatio=32 "
          "-XX:+PerfDisableSharedMem -XX:MaxTenuringThreshold=1",
          8 },
        { "zgc", QObject::tr("Generational ZGC"), QObject::tr("Tiny pauses, needs Java 21 and more memory"),
          QObject::tr("Pauses of about a millisecond whatever the heap size, so almost no garbage-collection stutter. It uses more "
                      "CPU and works best with plenty of memory (8 GB or more). Needs Java 21 or newer (Minecraft 1.20.5+ ships it)."),
          "-XX:+UseZGC -XX:+ZGenerational -XX:+AlwaysPreTouch -XX:+DisableExplicitGC", 21 },
        { "shenandoah", QObject::tr("Shenandoah"), QObject::tr("Low pauses on Java 17+, if your Java includes it"),
          QObject::tr("Another low-pause collector. Included in most OpenJDK builds (Eclipse Temurin, Microsoft) but not in "
                      "Oracle's. Try it if G1 stutters and ZGC isn't available."),
          "-XX:+UseShenandoahGC -XX:+AlwaysPreTouch -XX:+DisableExplicitGC", 17 },
        { "low-memory", QObject::tr("Low memory"), QObject::tr("For computers with little RAM"),
          QObject::tr("The serial collector uses the least memory and CPU of all, at the cost of longer pauses. For vanilla or "
                      "light packs on computers with 4–8 GB of RAM."),
          "-XX:+UseSerialGC -XX:+DisableExplicitGC", 8 },
    };
}

QString presetIdFor(const QString& args)
{
    const QString normalized = args.simplified();
    if (normalized.isEmpty())
        return {};
    for (const auto& preset : jvmPresets()) {
        if (preset.args.simplified() == normalized)
            return preset.id;
    }
    return {};
}

QString presetProblem(const JvmPreset& preset, int javaMajor)
{
    if (javaMajor > 0 && javaMajor < preset.minJava)
        return QObject::tr("Needs Java %1 or newer; this instance uses Java %2.").arg(preset.minJava).arg(javaMajor);
    return {};
}

MemoryAdvice recommendMemory(int modCount, const QString& minecraftVersion, qint64 systemRamMiB, int currentMaxMiB)
{
    MemoryAdvice advice;
    // newer versions need more for the same content (bigger worlds, more data)
    const QStringList parts = minecraftVersion.split('.');
    const int minor = parts.size() > 1 ? parts[1].toInt() : 0;
    const int major = parts.value(0).toInt();
    const bool modern = major > 1 || minor >= 18;

    int recommended;
    if (modCount <= 0) {
        recommended = modern ? 3072 : 2048;
        advice.reason = QObject::tr("Vanilla or nearly vanilla");
    } else if (modCount < 50) {
        recommended = 4096;
        advice.reason = QObject::tr("%1 mods").arg(modCount);
    } else if (modCount < 150) {
        recommended = 6144;
        advice.reason = QObject::tr("%1 mods").arg(modCount);
    } else if (modCount < 300) {
        recommended = 8192;
        advice.reason = QObject::tr("%1 mods, a large pack").arg(modCount);
    } else {
        recommended = 10240;
        advice.reason = QObject::tr("%1 mods, a very large pack").arg(modCount);
    }
    if (!modern && recommended > 6144)
        recommended = 6144;  // old versions rarely benefit from more

    // leave room for the system: at most half the RAM, and at least 3 GB free for it
    if (systemRamMiB > 0) {
        const int ceiling = static_cast<int>(std::max<qint64>(1024, std::min(systemRamMiB / 2, systemRamMiB - 3072)));
        if (recommended > ceiling) {
            recommended = ceiling - ceiling % 512;
            advice.reason += QObject::tr("; limited by this computer's %1 GB of RAM").arg(QString::number(systemRamMiB / 1024.0, 'f', 0));
        }
    }
    advice.recommendedMiB = recommended;

    if (currentMaxMiB > 0) {
        if (systemRamMiB > 0 && currentMaxMiB > systemRamMiB * 3 / 4)
            advice.warning = QObject::tr("The maximum is set to most of this computer's memory, which can slow the system down or "
                                         "crash the game.");
        else if (currentMaxMiB < recommended / 2)
            advice.warning = QObject::tr("The maximum looks too low for this pack; out-of-memory crashes are likely.");
        else if (currentMaxMiB > 16384)
            advice.warning = QObject::tr("More than 16 GB rarely helps and can make garbage-collection pauses longer.");
    }
    return advice;
}

}  // namespace Wraith::Advanced
