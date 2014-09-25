/******************************************************************************
 *  Copyright 2013 Sebastian Kügler <sebas@kde.org>                           *
 *                                                                            *
 *  This library is free software; you can redistribute it and/or             *
 *  modify it under the terms of the GNU Lesser General Public                *
 *                                                                            *
 *  License as published by the Free Software Foundation; either              *
 *  version 2.1 of the License, or (at your option) version 3, or any         *
 *  later version accepted by the membership of KDE e.V. (or its              *
 *  successor approved by the membership of KDE e.V.), which shall            *
 *  act as a proxy defined in Section 6 of version 3 of the license.          *
 *                                                                            *
 *  This library is distributed in the hope that it will be useful,           *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 *  Lesser General Public License for more details.                           *
 *                                                                            *
 *  You should have received a copy of the GNU Lesser General Public          *
 *  License along with this library.  If not, see                             *
 *  <http://www.gnu.org/licenses/>.                                           *
 *                                                                            *
 ******************************************************************************/

#include "kpluginindexer.h"

#include <qcommandlineparser.h>
#include <qcommandlineoption.h>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPluginLoader>
#include <QSet>
#include <QTextStream>

#include <QDebug>

#include <kpluginmetadata.h>

#include <kdesktopfile.h>
#include <kconfiggroup.h>

static QTextStream cout(stdout);
static QTextStream cerr(stderr);

static inline QStringList suffixFilters()
{
#if defined(Q_OS_WIN) || defined(Q_OS_CYGWIN)
    return QStringList() << QStringLiteral(".dll");
#else
    return QStringList() << QStringLiteral("*.so")
           << QStringLiteral("*.dylib")
           << QStringLiteral("*.bundle")
           << QStringLiteral("*.sl");
#endif
}


KPluginIndexer::KPluginIndexer(QCommandLineParser *parser, const QCommandLineOption &p, const QCommandLineOption &o, const QCommandLineOption &a)
    : m_parser(parser),
      all(a),
      plugindir(p),
      output(o)
{
}

int KPluginIndexer::runMain()
{
    if (!m_parser->isSet(plugindir) && !m_parser->isSet(all)) {
        cout << "Usage --help. In short: kplugin-update-index -p plugindir -o outputfile.json" << endl;
        return 1;
    }

    if (!resolveFiles()) {
        cerr << "Failed to resolve filenames" << m_pluginDir << m_outFile << endl;
        return 1;
    }
    return convertAll();
}

bool KPluginIndexer::resolveFiles()
{
    QString subdir;
    if (m_parser->isSet(plugindir)) {
        m_pluginDir = m_parser->value(plugindir);
        const QFileInfo fi(m_pluginDir);
        if (!fi.exists() && fi.isAbsolute()) {
            cerr << "File not found: " + m_pluginDir;
            return false;
        }
        if (!m_pluginDir.endsWith(QDir::separator())) {
            m_pluginDir = m_pluginDir + QDir::separator();
        }
        if (!fi.isAbsolute()) {
            //m_pluginDir = QDir::currentPath() + QDir::separator() + m_pluginDir;
            subdir = m_pluginDir;
        } else {
            m_pluginDirectories << m_pluginDir;
        }
    }
    if (m_parser->isSet(all)) {
        Q_FOREACH (const QString &dir, QCoreApplication::libraryPaths()) {
            //qDebug() << "Lib path: " << dir + subdir << QDir::currentPath();
            //m_pluginDirectories << dir + QDir::separator() + subDirectory;
            if (dir != QDir::currentPath()) {
                //qDebug() << "cwd: " << QDir::currentPath();
                m_pluginDirectories.append(findPluginSubDirectories(dir + QDir::separator() + subdir));
            }
        }

    }
    if (m_parser->isSet(output)) {
        m_outFile = m_parser->value(output);
    } else if (!m_pluginDir.isEmpty()) {
        m_outFile = m_pluginDir + QStringLiteral("kpluginindex.json");
    }

    return !m_pluginDirectories.isEmpty();// && !m_outFile.isEmpty();
}

QStringList KPluginIndexer::findPluginSubDirectories(const QString& dir)
{
    QStringList subs;
    const QStringList nameFilters;// = QStringList(QStringLiteral("*.so"));

    QDirIterator it(dir,
                    suffixFilters(),
                    QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        if (it.fileInfo().fileName().endsWith(".so")) {
            if (!subs.contains(it.fileInfo().absoluteDir().absolutePath() + QDir::separator())) {
                //qDebug() << it.fileInfo().absoluteDir().absolutePath();
                subs << it.fileInfo().absoluteDir().absolutePath() + QDir::separator();
            }

        }

    }
    //qDebug() << "Subs: " << subs;
    return subs;
}

bool KPluginIndexer::convertAll()
{
    bool ok = true;
    QPluginLoader loader;
    //KPluginInfo::List lst;
    Q_FOREACH (const QString &plugindir, m_pluginDirectories) {
        convertDirectory(plugindir, QStringLiteral("kpluginindex.json"));
//         qDebug() << "Converted " << plugindir << "";
    }
    return ok;
}

bool KPluginIndexer::convertDirectory(const QString& dir, const QString& dest)
{
    QVariantMap vm;
    QVariantMap pluginsVm;
    vm[QStringLiteral("Version")] = QStringLiteral("1.0");
    vm[QStringLiteral("Timestamp")] = QDateTime::currentMSecsSinceEpoch();

    QJsonArray plugins;

    QElapsedTimer t;
    t.start();

    int i = 0;
    QPluginLoader loader;
    QDirIterator it(dir, suffixFilters(), QDir::Files, QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        it.next();
        const QString _f = it.fileInfo().absoluteFilePath();
        loader.setFileName(_f);
        const KPluginMetaData &md(loader);
        QJsonObject obj;
        obj["FileName"] = _f;
        obj["KPlugin"] = md.rawData();
        plugins.insert(i, obj);
        i++;
    }


    if (!plugins.count()) {
        const QFileInfo fi(dest);
        if (fi.exists()) {
            QFile f(dest);
            if (!f.remove()) {
                qWarning() << "Could not remove stale plugin index file " << dest;
            }
        }
        return true;
    }

    t.start();
    QJsonDocument jdoc;
    jdoc.setArray(plugins);

    QString destfile = dest;
    const QFileInfo fi(dest);
    if (!fi.isAbsolute()) {
        destfile = dir + dest;
    }
    QFile file(destfile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        cerr << "Failed to open " << destfile << endl;
        return false;
    }

//     file.write(jdoc.toJson());
    file.write(jdoc.toBinaryData());
    cout << "Generated " << destfile << " (" << i << " plugins)" << endl;

    return true;
}


QVariantMap KPluginIndexer::convert(const QString &src)
{
    KDesktopFile df(src);
    KConfigGroup c = df.desktopGroup();

    static const QSet<QString> boolkeys = QSet<QString>()
                                          << QStringLiteral("Hidden") << QStringLiteral("X-KDE-PluginInfo-EnabledByDefault");
    static const QSet<QString> stringlistkeys = QSet<QString>()
            << QStringLiteral("X-KDE-ServiceTypes") << QStringLiteral("X-KDE-PluginInfo-Depends")
            << QStringLiteral("X-Plasma-Provides");

    QVariantMap vm;
    foreach (const QString &k, c.keyList()) {
        if (boolkeys.contains(k)) {
            vm[k] = c.readEntry(k, false);
        } else if (stringlistkeys.contains(k)) {
            vm[k] = c.readEntry(k, QStringList());
        } else {
            vm[k] = c.readEntry(k, QString());
        }
    }

    return vm;
}
