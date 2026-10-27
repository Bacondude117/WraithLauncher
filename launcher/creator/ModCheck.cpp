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

#include "ModCheck.h"

#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QTemporaryFile>

#include <toml++/toml.h>

#include "archive/ArchiveReader.h"

namespace Wraith::Creator {

// ---------------------------------------------------------------- versions

int compareVersions(const QString& a, const QString& b)
{
    static const QRegularExpression s_parts(R"(\d+|[A-Za-z]+)");
    auto split = [](const QString& v) {
        QStringList parts;
        // build metadata (+...) never affects ordering
        auto it = s_parts.globalMatch(v.section('+', 0, 0));
        while (it.hasNext())
            parts.append(it.next().captured());
        return parts;
    };
    const QStringList pa = split(a);
    const QStringList pb = split(b);
    for (int i = 0; i < std::max(pa.size(), pb.size()); i++) {
        const QString x = i < pa.size() ? pa[i] : QString();
        const QString y = i < pb.size() ? pb[i] : QString();
        bool xn = false;
        bool yn = false;
        const qlonglong xv = x.toLongLong(&xn);
        const qlonglong yv = y.toLongLong(&yn);
        if (xn && yn) {
            if (xv != yv)
                return xv < yv ? -1 : 1;
        } else if (x.isEmpty() || y.isEmpty()) {
            // "1.0" vs "1.0.1": shorter is lower; "1.0" vs "1.0-beta": pre-release is lower
            if (x.isEmpty())
                return yn ? -1 : 1;
            return xn ? 1 : -1;
        } else if (xn != yn) {
            return xn ? 1 : -1;  // numbers sort after words (1.0.1 > 1.0.beta)
        } else if (const int c = x.compare(y, Qt::CaseInsensitive); c != 0) {
            return c < 0 ? -1 : 1;
        }
    }
    return 0;
}

namespace {
bool satisfiesOne(const QString& version, QString constraint, bool* understood)
{
    constraint = constraint.trimmed();
    if (constraint.isEmpty() || constraint == "*")
        return true;
    // Maven ranges: [a,b] (a,b) [a,) (,b] or a list of them
    if (constraint.startsWith('[') || constraint.startsWith('(')) {
        static const QRegularExpression s_range(R"(([\[\(])\s*([^,\]\)]*)\s*(?:,\s*([^\]\)]*))?\s*([\]\)]))");
        auto it = s_range.globalMatch(constraint);
        bool any = false;
        bool matched = false;
        while (it.hasNext()) {
            const auto m = it.next();
            any = true;
            const QString low = m.captured(2).trimmed();
            const bool single = !m.captured(0).contains(',');
            const QString high = single ? low : m.captured(3).trimmed();
            bool ok = true;
            if (!low.isEmpty()) {
                const int c = compareVersions(version, low);
                ok = ok && (m.captured(1) == "[" ? c >= 0 : c > 0);
            }
            if (!high.isEmpty()) {
                const int c = compareVersions(version, high);
                ok = ok && (m.captured(4) == "]" ? c <= 0 : c < 0);
            }
            matched = matched || ok;
        }
        if (!any && understood)
            *understood = false;
        return !any || matched;
    }
    // Fabric/Quilt: space separated comparators, all must hold
    bool all = true;
    for (QString part : constraint.split(' ', Qt::SkipEmptyParts)) {
        QString op;
        for (const char* candidate : { ">=", "<=", ">", "<", "=", "~", "^" }) {
            if (part.startsWith(candidate)) {
                op = candidate;
                break;
            }
        }
        QString target = part.mid(op.size());
        if (target.contains(QRegularExpression("[xX*]$")) && op.isEmpty()) {
            // 1.20.x: same leading parts
            const QString prefix = target.left(target.size() - 1);
            all = all && (version + ".").startsWith(prefix);
            continue;
        }
        const int c = compareVersions(version, target);
        if (op == ">=")
            all = all && c >= 0;
        else if (op == "<=")
            all = all && c <= 0;
        else if (op == ">")
            all = all && c > 0;
        else if (op == "<")
            all = all && c < 0;
        else if (op == "~") {  // same major.minor, at least target
            const QString minor = target.section('.', 0, 1);
            all = all && c >= 0 && (version.section('.', 0, 1) == minor);
        } else if (op == "^") {  // same major, at least target
            all = all && c >= 0 && version.section('.', 0, 0) == target.section('.', 0, 0);
        } else {
            all = all && c == 0;
        }
    }
    return all;
}

QString jsonRange(const QJsonValue& value)
{
    if (value.isArray()) {
        QStringList options;
        for (const auto& v : value.toArray())
            options.append(v.toString());
        return options.join(" || ");
    }
    return value.toString();
}
}  // namespace

bool versionSatisfies(const QString& version, const QString& range, bool* understood)
{
    if (understood)
        *understood = true;
    if (version.isEmpty()) {
        if (understood)
            *understood = false;
        return true;
    }
    for (const auto& option : range.split("||")) {
        if (satisfiesOne(version.trimmed(), option, understood))
            return true;
    }
    return false;
}

// ---------------------------------------------------------------- reading mods

namespace {
const QSet<QString> s_platformIds = { "minecraft", "java", "fabricloader", "fabric-loader", "quilt_loader", "quilt_base", "forge",
                                      "neoforge", "fml", "mixinextras" /* bundled by every modern loader */ };

void readFabricJson(const QByteArray& data, const QString& file, QList<ModInfo>& out)
{
    const auto obj = QJsonDocument::fromJson(data).object();
    if (obj.isEmpty())
        return;
    ModInfo mod;
    mod.file = file;
    mod.loader = "fabric";
    mod.id = obj.value("id").toString();
    mod.name = obj.value("name").toString(mod.id);
    mod.version = obj.value("version").toString();
    for (const auto& p : obj.value("provides").toArray())
        mod.provides.append(p.toString());
    const auto depends = obj.value("depends").toObject();
    for (auto it = depends.begin(); it != depends.end(); ++it)
        mod.depends.insert(it.key(), jsonRange(it.value()));
    const auto breaks = obj.value("breaks").toObject();
    for (auto it = breaks.begin(); it != breaks.end(); ++it)
        mod.breaks.insert(it.key(), jsonRange(it.value()));
    if (!mod.id.isEmpty())
        out.append(mod);
}

void readQuiltJson(const QByteArray& data, const QString& file, QList<ModInfo>& out)
{
    const auto loader = QJsonDocument::fromJson(data).object().value("quilt_loader").toObject();
    if (loader.isEmpty())
        return;
    ModInfo mod;
    mod.file = file;
    mod.loader = "quilt";
    mod.id = loader.value("id").toString();
    mod.name = loader.value("metadata").toObject().value("name").toString(mod.id);
    mod.version = loader.value("version").toString();
    for (const auto& p : loader.value("provides").toArray())
        mod.provides.append(p.isObject() ? p.toObject().value("id").toString() : p.toString());
    auto readList = [](const QJsonValue& list, QMap<QString, QString>& target, bool skipOptional) {
        for (const auto& d : list.toArray()) {
            if (d.isString()) {
                target.insert(d.toString().section(':', -1), {});
            } else {
                const auto o = d.toObject();
                if (skipOptional && o.value("optional").toBool())
                    continue;
                target.insert(o.value("id").toString().section(':', -1), jsonRange(o.value("versions")));
            }
        }
    };
    readList(loader.value("depends"), mod.depends, true);
    readList(loader.value("breaks"), mod.breaks, false);
    if (!mod.id.isEmpty())
        out.append(mod);
}

void readModsToml(const QByteArray& data, const QString& file, const QString& loaderName, QList<ModInfo>& out)
{
    toml::table table;
#if TOML_EXCEPTIONS
    try {
        table = toml::parse(data.toStdString());
    } catch ([[maybe_unused]] const std::exception& err) {
        return;
    }
#else
    toml::parse_result result = toml::parse(data.toStdString());
    if (!result)
        return;
    table = result.table();
#endif
    auto mods = table["mods"].as_array();
    if (!mods)
        return;
    for (auto& entry : *mods) {
        auto* modTable = entry.as_table();
        if (!modTable)
            continue;
        ModInfo mod;
        mod.file = file;
        mod.loader = loaderName;
        mod.id = QString::fromStdString((*modTable)["modId"].value_or(std::string()));
        mod.name = QString::fromStdString((*modTable)["displayName"].value_or(mod.id.toStdString()));
        mod.version = QString::fromStdString((*modTable)["version"].value_or(std::string()));
        if (mod.version.contains("${"))
            mod.version.clear();  // filled in at build time from the jar manifest; unknown here
        if (auto* deps = table["dependencies"][mod.id.toStdString()].as_array()) {
            for (auto& d : *deps) {
                auto* dep = d.as_table();
                if (!dep)
                    continue;
                const QString id = QString::fromStdString((*dep)["modId"].value_or(std::string()));
                const QString range = QString::fromStdString((*dep)["versionRange"].value_or(std::string()));
                const QString type = QString::fromStdString((*dep)["type"].value_or(std::string())).toLower();
                const bool mandatory = (*dep)["mandatory"].value_or(false);
                if (type == "incompatible")
                    mod.breaks.insert(id, range);
                else if (type == "required" || (type.isEmpty() && mandatory))
                    mod.depends.insert(id, range);
            }
        }
        if (!mod.id.isEmpty())
            out.append(mod);
    }
}

void readArchive(const QString& path, const QString& file, QList<ModInfo>& out, int depth)
{
    MMCZip::ArchiveReader zip(path);
    if (!zip.collectFiles())
        return;
    QList<ModInfo> found;
    if (zip.exists("fabric.mod.json"))
        if (auto data = zip.readFile("fabric.mod.json"))
            readFabricJson(*data, file, found);
    if (zip.exists("quilt.mod.json"))
        if (auto data = zip.readFile("quilt.mod.json"))
            readQuiltJson(*data, file, found);
    if (zip.exists("META-INF/neoforge.mods.toml"))
        if (auto data = zip.readFile("META-INF/neoforge.mods.toml"))
            readModsToml(*data, file, "neoforge", found);
    if (zip.exists("META-INF/mods.toml"))
        if (auto data = zip.readFile("META-INF/mods.toml"))
            readModsToml(*data, file, "forge", found);

    // mods bundled inside this one (Fabric/Quilt "jars", Forge/NeoForge jar-in-jar) provide their ids too
    if (depth < 2 && !found.isEmpty()) {
        for (const auto& inner : zip.getFiles()) {
            const bool nested = (inner.startsWith("META-INF/jars/") || inner.startsWith("META-INF/jarjar/")) && inner.endsWith(".jar");
            if (!nested)
                continue;
            auto bytes = zip.readFile(inner);
            if (!bytes)
                continue;
            QTemporaryFile temp;
            if (!temp.open())
                continue;
            temp.write(*bytes);
            temp.close();
            QList<ModInfo> bundled;
            readArchive(temp.fileName(), file, bundled, depth + 1);
            for (const auto& b : bundled) {
                found.first().provides.append(b.id);
                found.first().provides.append(b.provides);
            }
        }
    }
    out.append(found);
}
}  // namespace

QList<ModInfo> readModFile(const QString& path)
{
    QList<ModInfo> mods;
    QString file = QFileInfo(path).fileName();
    readArchive(path, file, mods, 0);
    const bool disabled = file.endsWith(".disabled");
    for (auto& m : mods)
        m.enabled = !disabled;
    return mods;
}

// ---------------------------------------------------------------- checks

QList<Issue> checkMods(const QList<ModInfo>& mods, const QString& loader, const QString& minecraft)
{
    QList<Issue> issues;
    QHash<QString, const ModInfo*> enabledById;
    QHash<QString, const ModInfo*> disabledById;
    QHash<QString, QStringList> filesById;
    for (const auto& m : mods) {
        auto& index = m.enabled ? enabledById : disabledById;
        for (const auto& id : QStringList{ m.id } + m.provides)
            index.insert(id, &m);
        if (m.enabled && !filesById.value(m.id).contains(m.file))
            filesById[m.id].append(m.file);
    }
    const bool quiltInstance = loader == "quilt";
    auto platformProvides = [&](const QString& id) {
        if (s_platformIds.contains(id))
            return true;
        // Quilt runs Fabric mods and ships Fabric API's Quilt counterpart
        return quiltInstance && (id == "fabric" || id == "fabric-api");
    };

    for (const auto& m : mods) {
        if (!m.enabled)
            continue;
        // made for another loader
        const bool loaderOk = loader.isEmpty() || m.loader == loader || (quiltInstance && m.loader == "fabric") ||
                              (loader == "neoforge" && m.loader == "forge");
        if (!loaderOk) {
            issues.append({ Issue::Severity::Error, QObject::tr("%1 is made for %2").arg(m.name, m.loader),
                            QObject::tr("This instance uses %1, so the mod won't load.").arg(loader),
                            QObject::tr("Remove it or replace it with the %1 version.").arg(loader), { m.file } });
            continue;
        }
        for (auto it = m.depends.begin(); it != m.depends.end(); ++it) {
            const QString& dep = it.key();
            if (dep == "minecraft") {
                bool understood = true;
                if (!minecraft.isEmpty() && !versionSatisfies(minecraft, it.value(), &understood) && understood)
                    issues.append({ Issue::Severity::Warning, QObject::tr("%1 asks for a different Minecraft version").arg(m.name),
                                    QObject::tr("It wants Minecraft %1; this instance is %2.").arg(it.value(), minecraft),
                                    QObject::tr("Look for a version of the mod made for %1.").arg(minecraft), { m.file } });
                continue;
            }
            if (platformProvides(dep))
                continue;
            if (const auto* provider = enabledById.value(dep)) {
                bool understood = true;
                if (!it.value().isEmpty() && !versionSatisfies(provider->version, it.value(), &understood) && understood)
                    issues.append({ Issue::Severity::Warning, QObject::tr("%1 needs a different version of %2").arg(m.name, provider->name),
                                    QObject::tr("It asks for %1 %2; %3 is installed.").arg(dep, it.value(), provider->version),
                                    QObject::tr("Update or change the version of %1.").arg(provider->name), { m.file, provider->file } });
                continue;
            }
            if (const auto* disabled = disabledById.value(dep)) {
                issues.append({ Issue::Severity::Error, QObject::tr("%1 needs %2, which is disabled").arg(m.name, disabled->name),
                                QString(), QObject::tr("Enable %1.").arg(disabled->name), { m.file, disabled->file } });
                continue;
            }
            issues.append({ Issue::Severity::Error, QObject::tr("%1 needs %2").arg(m.name, dep),
                            it.value().isEmpty() ? QString() : QObject::tr("Version: %1").arg(it.value()),
                            QObject::tr("Install %1 (Content → Browse content).").arg(dep), { m.file } });
        }
        for (auto it = m.breaks.begin(); it != m.breaks.end(); ++it) {
            const auto* other = enabledById.value(it.key());
            if (!other || other == &m)
                continue;
            bool understood = true;
            const bool applies = it.value().isEmpty() || versionSatisfies(other->version, it.value(), &understood) || !understood;
            if (applies)
                issues.append({ Issue::Severity::Error, QObject::tr("%1 doesn't work with %2").arg(m.name, other->name),
                                QObject::tr("%1 declares it incompatible%2.")
                                    .arg(m.name, it.value().isEmpty() ? QString() : QObject::tr(" (%1 %2)").arg(it.key(), it.value())),
                                QObject::tr("Remove or disable one of them."), { m.file, other->file } });
        }
    }
    for (auto it = filesById.begin(); it != filesById.end(); ++it) {
        if (it.value().size() > 1)
            issues.append({ Issue::Severity::Error, QObject::tr("%1 is installed more than once").arg(it.key()),
                            QObject::tr("Several files contain the same mod; the game usually refuses to start."),
                            QObject::tr("Keep one file and remove or disable the others."), it.value() });
    }
    std::stable_sort(issues.begin(), issues.end(), [](const Issue& a, const Issue& b) { return a.severity < b.severity; });
    return issues;
}

}  // namespace Wraith::Creator
