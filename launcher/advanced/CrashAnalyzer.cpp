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

#include "CrashAnalyzer.h"

#include <QObject>
#include <QRegularExpression>
#include <QSet>

namespace Wraith::Advanced {

LogLevel lineLevel(const QString& line, LogLevel previous)
{
    static const QRegularExpression s_bracket(R"(^\[[^\]]*[/ ](TRACE|DEBUG|INFO|WARN|WARNING|ERROR|FATAL|SEVERE)\])");
    static const QRegularExpression s_plain(R"(^\[?\d{2}:\d{2}:\d{2}\]?\s*\[?(TRACE|DEBUG|INFO|WARN|WARNING|ERROR|FATAL|SEVERE)\]?)");
    QRegularExpressionMatch m = s_bracket.match(line);
    if (!m.hasMatch())
        m = s_plain.match(line);
    if (m.hasMatch()) {
        const QString level = m.captured(1);
        if (level == "TRACE" || level == "DEBUG")
            return LogLevel::Debug;
        if (level == "INFO")
            return LogLevel::Info;
        if (level == "WARN" || level == "WARNING")
            return LogLevel::Warn;
        if (level == "FATAL")
            return LogLevel::Fatal;
        return LogLevel::Error;
    }
    // stack traces and "Caused by" continue the previous error
    const QString trimmed = line.trimmed();
    if (trimmed.startsWith("at ") || trimmed.startsWith("Caused by:") || trimmed.startsWith("...") || line.startsWith('\t'))
        return previous == LogLevel::Unknown ? LogLevel::Error : previous;
    if (trimmed.startsWith("Exception in thread") || trimmed.contains(QRegularExpression(R"(^[\w.$]+(Exception|Error)(:|$))")))
        return LogLevel::Error;
    return LogLevel::Unknown;
}

QString levelName(LogLevel level)
{
    switch (level) {
        case LogLevel::Debug:
            return "debug";
        case LogLevel::Info:
            return "info";
        case LogLevel::Warn:
            return "warn";
        case LogLevel::Error:
            return "error";
        case LogLevel::Fatal:
            return "fatal";
        default:
            return "";
    }
}

namespace {
QString firstMatch(const QString& text, const QRegularExpression& re, int group = 0)
{
    const auto m = re.match(text);
    return m.hasMatch() ? m.captured(group) : QString();
}

QString lineContaining(const QString& text, const QString& needle)
{
    const int at = text.indexOf(needle, 0, Qt::CaseInsensitive);
    if (at < 0)
        return {};
    const int start = text.lastIndexOf('\n', at) + 1;
    int end = text.indexOf('\n', at);
    if (end < 0)
        end = static_cast<int>(text.size());
    return text.mid(start, end - start).trimmed().left(300);
}

int javaForClassVersion(int classVersion)
{
    return classVersion - 44;  // 52 = Java 8, 61 = 17, 65 = 21
}
}  // namespace

QList<CrashFinding> analyzeCrash(const QString& text)
{
    QList<CrashFinding> findings;
    QSet<QString> seen;
    auto add = [&](CrashFinding f) {
        if (!seen.contains(f.id)) {
            seen.insert(f.id);
            findings.append(std::move(f));
        }
    };

    // wrong Java version
    static const QRegularExpression s_classVersion(R"(class file version (\d+)\.\d+\)?, this version of the Java Runtime only recognizes class file versions up to (\d+))");
    if (const auto m = s_classVersion.match(text); m.hasMatch()) {
        const int needed = javaForClassVersion(m.captured(1).toInt());
        const int have = javaForClassVersion(m.captured(2).toInt());
        CrashFinding f{ "java-too-old", QObject::tr("This needs Java %1, but Java %2 was used").arg(needed).arg(have),
                        QObject::tr("Something in the game (Minecraft itself or a mod) was built for a newer Java than the one that "
                                    "started the game."),
                        QObject::tr("Choose Java %1 or newer in the instance's Settings → Java, or turn on automatic Java downloads.").arg(needed),
                        lineContaining(text, "class file version") };
        f.java = true;
        f.rollback = false;
        add(f);
    }
    // out of memory
    if (text.contains("java.lang.OutOfMemoryError")) {
        CrashFinding f{ "out-of-memory", QObject::tr("The game ran out of memory"),
                        QObject::tr("Minecraft used all the memory it is allowed to use. Big modpacks, shaders and high render distances "
                                    "need more."),
                        QObject::tr("Raise the maximum memory in the instance's Settings (the recommendation there is a good start), "
                                    "or lower the render distance."),
                        lineContaining(text, "OutOfMemoryError") };
        f.memory = true;
        f.rollback = false;
        add(f);
    }
    // Fabric/Quilt dependency resolution
    if (text.contains("Mod resolution failed") || text.contains("Incompatible mods found!") || text.contains("Incompatible mod set!")) {
        static const QRegularExpression s_requires(R"(Mod '([^']+)' \(([^)]+)\) [^\n]*requires [^\n]*?'([^']+)')");
        static const QRegularExpression s_install(R"(Install ([\w\-\. ]+?)(?:,| version| \(|$))", QRegularExpression::MultilineOption);
        CrashFinding f{ "dependencies", QObject::tr("A mod is missing something it needs, or two mods don't work together"),
                        QObject::tr("The mod loader checked the mods before starting and refused, so the game never loaded."),
                        QObject::tr("Run Creator → Health check for the details, then install, update or remove the mods it names."),
                        lineContaining(text, "requires") };
        if (const auto m = s_requires.match(text); m.hasMatch()) {
            f.mods = { m.captured(2), m.captured(3) };
            f.title = QObject::tr("%1 needs %2").arg(m.captured(1), m.captured(3));
        }
        add(f);
    }
    // Forge/NeoForge missing dependencies
    if (text.contains("Missing or unsupported mandatory dependencies")) {
        static const QRegularExpression s_forgeMissing(R"(Mod ID: '([^']+)', Requested by: '([^']+)')");
        CrashFinding f{ "forge-dependencies", QObject::tr("A mod is missing a required mod"),
                        QObject::tr("Forge/NeoForge found mods whose required mods are missing or the wrong version."),
                        QObject::tr("Install or update the required mods; Creator → Health check lists them."),
                        lineContaining(text, "Mod ID:") };
        auto it = s_forgeMissing.globalMatch(text);
        while (it.hasNext()) {
            const auto m = it.next();
            f.mods.append(m.captured(1));
            if (f.mods.size() == 1)
                f.title = QObject::tr("%1 needs %2").arg(m.captured(2), m.captured(1));
        }
        add(f);
    }
    // duplicate mods
    if (text.contains("Duplicate mod", Qt::CaseInsensitive) || text.contains("Found duplicate mods") || text.contains("Duplicate mods found")) {
        add({ "duplicates", QObject::tr("The same mod is installed twice"),
              QObject::tr("Two files contain the same mod (often two versions), and the loader refuses to pick one."),
              QObject::tr("Keep one version and remove or disable the other (Content tab)."), lineContaining(text, "duplicate") });
    }
    // mixin failures name the mod that broke
    static const QRegularExpression s_mixin(R"(Mixin (?:apply|transformation) (?:for mod (\S+) )?failed[^\n]*?(?:from mod (\S+))?)");
    static const QRegularExpression s_mixinConfig(R"((\w[\w\-]*)\.mixins\.json)");
    if (text.contains("MixinApplyError") || text.contains("Mixin apply failed") || text.contains("MixinTransformerError")) {
        CrashFinding f{ "mixin", QObject::tr("A mod failed to modify the game"),
                        QObject::tr("Mods change Minecraft's code while it loads (mixins). One of these changes failed, usually because "
                                    "the mod doesn't match this Minecraft version or clashes with another mod."),
                        QObject::tr("Update or remove the mod named below. If you just changed mods, rolling back to a snapshot from "
                                    "before the change is the quickest fix."),
                        lineContaining(text, "mixin") };
        const QString fromMixin = firstMatch(text, s_mixin, 1);
        const QString fromConfig = firstMatch(text, s_mixinConfig, 1);
        const QString mod = !fromMixin.isEmpty() ? fromMixin : fromConfig;
        if (!mod.isEmpty()) {
            f.mods = { mod };
            f.title = QObject::tr("%1 failed to load").arg(mod);
        }
        add(f);
    }
    // missing classes: usually a missing library mod
    static const QRegularExpression s_noClass(R"((?:NoClassDefFoundError|ClassNotFoundException): ([\w/.$]+))");
    if (const auto m = s_noClass.match(text); m.hasMatch()) {
        const QString cls = m.captured(1).replace('/', '.');
        add({ "missing-class", QObject::tr("A mod is looking for code that isn't there"),
              QObject::tr("The game tried to use %1, which no installed mod provides. A library mod is usually missing, or a mod "
                          "is for a different version.").arg(cls),
              QObject::tr("Check the mod's page for required libraries (Creator → Health check helps)."), m.captured(0) });
    }
    // graphics drivers
    if (text.contains("Pixel format not accelerated") || text.contains("GLFW error 65542") || text.contains("WGL: The driver does not appear to support OpenGL")) {
        CrashFinding f{ "graphics-driver", QObject::tr("The graphics driver couldn't start OpenGL"),
                        QObject::tr("Minecraft needs OpenGL from your graphics card's driver. The driver is missing, outdated, or "
                                    "the game is running on the wrong graphics card."),
                        QObject::tr("Install the latest driver from your graphics card maker (NVIDIA, AMD or Intel)."),
                        lineContaining(text, "GLFW") };
        f.rollback = false;
        add(f);
    }
    if (text.contains("EXCEPTION_ACCESS_VIOLATION") && (text.contains("atio6axx") || text.contains("nvoglv") || text.contains("ig9icd") || text.contains("ig7icd"))) {
        CrashFinding f{ "driver-crash", QObject::tr("The graphics driver crashed"),
                        QObject::tr("The crash happened inside the graphics driver, not in Minecraft or a mod."),
                        QObject::tr("Update the graphics driver. Shader packs and some rendering mods make this more likely."),
                        lineContaining(text, "EXCEPTION_ACCESS_VIOLATION") };
        f.rollback = false;
        add(f);
    }
    if (text.contains("java.lang.StackOverflowError")) {
        add({ "stack-overflow", QObject::tr("Two mods are calling each other endlessly"),
              QObject::tr("A StackOverflowError usually means two mods hook into the same thing and keep triggering each other."),
              QObject::tr("If it started after adding or updating mods, roll back or remove the newest ones one at a time."),
              lineContaining(text, "StackOverflowError") });
    }
    // the mod a crash report blames ("Suspected Mod(s):" in Forge, "suspected mods" in Fabric's crash reports)
    static const QRegularExpression s_suspected(R"(Suspected Mods?:\s*([^\n]+))", QRegularExpression::CaseInsensitiveOption);
    if (const auto m = s_suspected.match(text); m.hasMatch() && !m.captured(1).contains("None", Qt::CaseInsensitive)) {
        add({ "suspected", QObject::tr("The crash report points at %1").arg(m.captured(1).trimmed().left(80)),
              QObject::tr("Minecraft's crash report names the mod whose code was running when it crashed. It is often, but not "
                          "always, the cause."),
              QObject::tr("Update that mod, or disable it and try again."), m.captured(0).trimmed() });
    }
    // nothing specific: the exception that ended it
    if (findings.isEmpty()) {
        static const QRegularExpression s_description(R"(Description: ([^\n]+))");
        static const QRegularExpression s_exception(R"(^([\w.$]+(?:Exception|Error)[^\n]*))", QRegularExpression::MultilineOption);
        const QString description = firstMatch(text, s_description, 1);
        const QString exception = firstMatch(text, s_exception, 1);
        if (!description.isEmpty() || !exception.isEmpty())
            add({ "generic", QObject::tr("The game stopped with an error"),
                  description.isEmpty() ? QObject::tr("No known cause matched.") : QObject::tr("Minecraft says: %1").arg(description),
                  QObject::tr("If it started after a change, roll back to a snapshot. Otherwise share the log (Upload in Logs) when "
                              "asking for help."),
                  exception.left(300) });
    }
    return findings;
}

}  // namespace Wraith::Advanced
