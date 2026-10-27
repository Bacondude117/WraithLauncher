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

#include "LauncherImport.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>

namespace Wraith::Advanced {

namespace {
QJsonObject readJson(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(file.readAll()).object() : QJsonObject();
}

QString normalizeLoader(QString loader)
{
    loader = loader.toLower().remove(' ');
    if (loader.contains("neoforge"))
        return "neoforge";
    if (loader.contains("forge"))
        return "forge";
    if (loader.contains("quilt"))
        return "quilt";
    if (loader.contains("fabric"))
        return "fabric";
    return {};
}

// Prism, MultiMC, PolyMC: the same format Wraith uses
void scanPrismLike(const LauncherLocation& location, QList<ForeignInstance>& out)
{
    QString instancesDir = QDir(location.root).filePath("instances");
    // a custom instance folder set in the launcher's config
    for (const char* cfgName : { "prismlauncher.cfg", "polymc.cfg", "multimc.cfg" }) {
        const QString cfg = QDir(location.root).filePath(cfgName);
        if (QFile::exists(cfg)) {
            QSettings settings(cfg, QSettings::IniFormat);
            const QString custom = settings.value("InstanceDir").toString();
            if (!custom.isEmpty())
                instancesDir = QDir::isAbsolutePath(custom) ? custom : QDir(location.root).filePath(custom);
        }
    }
    for (const auto& dir : QDir(instancesDir).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString cfgPath = QDir(dir.absoluteFilePath()).filePath("instance.cfg");
        if (!QFile::exists(cfgPath))
            continue;
        QSettings cfg(cfgPath, QSettings::IniFormat);
        ForeignInstance inst;
        inst.launcher = location.launcher;
        inst.path = dir.absoluteFilePath();
        inst.name = cfg.value("name", dir.fileName()).toString();
        inst.prismFormat = true;
        const QDir instDir(inst.path);
        inst.gameDir = instDir.exists(".minecraft") ? instDir.filePath(".minecraft") : instDir.filePath("minecraft");
        for (const auto& c : readJson(instDir.filePath("mmc-pack.json")).value("components").toArray()) {
            const auto o = c.toObject();
            const QString uid = o.value("uid").toString();
            const QString version = o.value("version").toString();
            if (uid == "net.minecraft")
                inst.minecraft = version;
            else if (uid == "net.fabricmc.fabric-loader")
                inst.loader = "fabric", inst.loaderVersion = version;
            else if (uid == "org.quiltmc.quilt-loader")
                inst.loader = "quilt", inst.loaderVersion = version;
            else if (uid == "net.minecraftforge")
                inst.loader = "forge", inst.loaderVersion = version;
            else if (uid == "net.neoforged")
                inst.loader = "neoforge", inst.loaderVersion = version;
        }
        out.append(inst);
    }
}

void scanCurseForge(const LauncherLocation& location, QList<ForeignInstance>& out)
{
    for (const auto& dir : QDir(QDir(location.root).filePath("Instances")).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const auto json = readJson(QDir(dir.absoluteFilePath()).filePath("minecraftinstance.json"));
        if (json.isEmpty())
            continue;
        ForeignInstance inst;
        inst.launcher = location.launcher;
        inst.path = inst.gameDir = dir.absoluteFilePath();
        inst.name = json.value("name").toString(dir.fileName());
        inst.minecraft = json.value("gameVersion").toString();
        parseLoaderName(json.value("baseModLoader").toObject().value("name").toString(), &inst.loader, &inst.loaderVersion);
        out.append(inst);
    }
}

void scanModrinthApp(const LauncherLocation& location, QList<ForeignInstance>& out)
{
    for (const auto& dir : QDir(QDir(location.root).filePath("profiles")).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        ForeignInstance inst;
        inst.launcher = location.launcher;
        inst.path = inst.gameDir = dir.absoluteFilePath();
        inst.name = dir.fileName();
        // older app versions keep a profile.json next to the files; newer ones keep it in their database
        const auto meta = readJson(QDir(dir.absoluteFilePath()).filePath("profile.json")).value("metadata").toObject();
        if (!meta.isEmpty()) {
            inst.name = meta.value("name").toString(inst.name);
            inst.minecraft = meta.value("game_version").toString();
            inst.loader = normalizeLoader(meta.value("loader").toString());
            inst.loaderVersion = meta.value("loader_version").toObject().value("id").toString();
        }
        out.append(inst);
    }
}

void scanAtLauncher(const LauncherLocation& location, QList<ForeignInstance>& out)
{
    for (const auto& dir : QDir(QDir(location.root).filePath("instances")).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const auto json = readJson(QDir(dir.absoluteFilePath()).filePath("instance.json"));
        if (json.isEmpty())
            continue;
        ForeignInstance inst;
        inst.launcher = location.launcher;
        inst.path = inst.gameDir = dir.absoluteFilePath();
        const auto launcher = json.value("launcher").toObject();
        inst.name = launcher.value("name").toString(dir.fileName());
        inst.minecraft = json.value("id").toString(launcher.value("minecraftVersion").toString());
        const auto loader = launcher.value("loaderVersion").toObject();
        inst.loader = normalizeLoader(loader.value("type").toString());
        inst.loaderVersion = loader.value("version").toString();
        out.append(inst);
    }
}

void scanGdLauncher(const LauncherLocation& location, QList<ForeignInstance>& out)
{
    for (const auto& dir : QDir(QDir(location.root).filePath("instances")).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const auto json = readJson(QDir(dir.absoluteFilePath()).filePath("config.json"));
        const auto loader = json.value("loader").toObject();
        if (loader.isEmpty())
            continue;
        ForeignInstance inst;
        inst.launcher = location.launcher;
        inst.path = inst.gameDir = dir.absoluteFilePath();
        inst.name = json.value("name").toString(dir.fileName());
        inst.minecraft = loader.value("mcVersion").toString();
        inst.loader = normalizeLoader(loader.value("loaderType").toString());
        inst.loaderVersion = loader.value("loaderVersion").toString();
        out.append(inst);
    }
}

// The official launcher: one entry per profile, sharing .minecraft unless the profile sets its own game folder
void scanOfficial(const LauncherLocation& location, QList<ForeignInstance>& out)
{
    const auto profiles = readJson(QDir(location.root).filePath("launcher_profiles.json")).value("profiles").toObject();
    for (auto it = profiles.begin(); it != profiles.end(); ++it) {
        const auto p = it.value().toObject();
        const QString versionId = p.value("lastVersionId").toString();
        if (versionId.isEmpty() || versionId.startsWith("latest-"))
            continue;  // "latest release/snapshot" profiles have no fixed version
        ForeignInstance inst;
        inst.launcher = location.launcher;
        inst.path = location.root;
        inst.gameDir = p.value("gameDir").toString(location.root);
        inst.name = p.value("name").toString(versionId);
        static const QRegularExpression s_fabric(R"(^(fabric|quilt)-loader-([\w.+-]+)-(\d[\w.\-]*)$)");
        static const QRegularExpression s_forge(R"(^([\d.]+)-(forge|neoforge)-?([\w.\-]+)$)");
        if (const auto m = s_fabric.match(versionId); m.hasMatch()) {
            inst.loader = m.captured(1);
            inst.loaderVersion = m.captured(2);
            inst.minecraft = m.captured(3);
        } else if (const auto f = s_forge.match(versionId); f.hasMatch()) {
            inst.minecraft = f.captured(1);
            inst.loader = f.captured(2);
            inst.loaderVersion = f.captured(3);
        } else if (versionId.startsWith("neoforge-")) {
            inst.loader = "neoforge";
            inst.loaderVersion = versionId.mid(9);
        } else {
            inst.minecraft = versionId;
        }
        out.append(inst);
    }
}
}  // namespace

void parseLoaderName(const QString& name, QString* loader, QString* version)
{
    // "forge-47.2.0", "neoforge-20.4.237", "fabric-0.15.7-1.20.1", "quilt-0.23.1-1.20.1"
    *loader = normalizeLoader(name.section('-', 0, 0));
    *version = name.section('-', 1, 1);
}

QList<LauncherLocation> defaultLocations()
{
    const QString home = QDir::homePath();
#ifdef Q_OS_WIN
    const QString appData = qEnvironmentVariable("APPDATA");
    const QList<LauncherLocation> candidates = {
        { "Prism Launcher", appData + "/PrismLauncher" },
        { "PolyMC", appData + "/PolyMC" },
        { "CurseForge", home + "/curseforge/minecraft" },
        { "Modrinth App", appData + "/ModrinthApp" },
        { "Modrinth App", appData + "/com.modrinth.theseus" },
        { "ATLauncher", appData + "/ATLauncher" },
        { "GDLauncher", appData + "/gdlauncher_next" },
        { "Minecraft Launcher", appData + "/.minecraft" },
    };
#elif defined(Q_OS_MACOS)
    const QString support = home + "/Library/Application Support";
    const QList<LauncherLocation> candidates = {
        { "Prism Launcher", support + "/PrismLauncher" }, { "PolyMC", support + "/PolyMC" },
        { "CurseForge", home + "/Documents/curseforge/minecraft" }, { "Modrinth App", support + "/ModrinthApp" },
        { "ATLauncher", support + "/ATLauncher" }, { "GDLauncher", support + "/gdlauncher_next" },
        { "Minecraft Launcher", support + "/minecraft" },
    };
#else
    const QString data = home + "/.local/share";
    const QList<LauncherLocation> candidates = {
        { "Prism Launcher", data + "/PrismLauncher" }, { "PolyMC", data + "/PolyMC" }, { "Modrinth App", data + "/ModrinthApp" },
        { "ATLauncher", data + "/ATLauncher" }, { "GDLauncher", home + "/.config/gdlauncher_next" },
        { "Minecraft Launcher", home + "/.minecraft" },
    };
#endif
    QList<LauncherLocation> found;
    for (const auto& c : candidates) {
        if (QDir(c.root).exists())
            found.append(c);
    }
    return found;
}

QList<ForeignInstance> scanLauncher(const LauncherLocation& location)
{
    QList<ForeignInstance> out;
    if (location.launcher == "Prism Launcher" || location.launcher == "PolyMC" || location.launcher == "MultiMC")
        scanPrismLike(location, out);
    else if (location.launcher == "CurseForge")
        scanCurseForge(location, out);
    else if (location.launcher == "Modrinth App")
        scanModrinthApp(location, out);
    else if (location.launcher == "ATLauncher")
        scanAtLauncher(location, out);
    else if (location.launcher == "GDLauncher")
        scanGdLauncher(location, out);
    else if (location.launcher == "Minecraft Launcher")
        scanOfficial(location, out);
    return out;
}

}  // namespace Wraith::Advanced
