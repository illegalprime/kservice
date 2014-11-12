/* This file is part of the KDE libraries
   Copyright (C) 2000 Torben Weis <weis@kde.org>
   Copyright (C) 2006 David Faure <faure@kde.org>
   Copyright 2013 Sebastian Kügler <sebas@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kplugintrader.h"
#include "ktraderparsetree_p.h"

#include <KPluginMetaData>

#include <QtCore/QCoreApplication>
#include <QtCore/QDirIterator>
#include <QElapsedTimer>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QDebug>
#include <QJsonArray>

#include <KPluginLoader>
#include <KPluginMetaData>

using namespace KTraderParse;

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

class KPluginTraderSingleton
{
public:
    KPluginTrader instance;
};

Q_GLOBAL_STATIC(KPluginTraderSingleton, s_globalPluginTrader)

KPluginTrader *KPluginTrader::self()
{
    return &s_globalPluginTrader()->instance;
}

KPluginTrader::KPluginTrader()
    : d(0)
{
}

KPluginTrader::~KPluginTrader()
{
}

void KPluginTrader::applyConstraints(KPluginInfo::List &lst, const QString &constraint)
{
    if (lst.isEmpty() || constraint.isEmpty()) {
        return;
    }

    const ParseTreeBase::Ptr constr = parseConstraints(constraint); // for ownership
    const ParseTreeBase *pConstraintTree = constr.data(); // for speed

    if (!constr) { // parse error
        lst.clear();
    } else {
        // Find all plugin information matching the constraint and remove the rest
        KPluginInfo::List::iterator it = lst.begin();
        while (it != lst.end()) {
            if (matchConstraintPlugin(pConstraintTree, *it, lst) != 1) {
                it = lst.erase(it);
            } else {
                ++it;
            }
        }
    }
}

KPluginInfo::List KPluginTrader::query(const QString &subDirectory, const QString &servicetype, const QString &constraint)
{
    bool ps = (servicetype == "Plasma/PackageStructure");
    if (ps) qDebug() << "dir: " << subDirectory << "servicetype" << servicetype << "constraint: " << constraint;

    QStringList libraryPaths;

    QVector<KPluginMetaData> allMetaData;
    if (QDir::isAbsolutePath(subDirectory)) {
        //qDebug() << "ABSOLUTE path: " << subDirectory;
        if (subDirectory.endsWith(QDir::separator())) {
            libraryPaths << subDirectory;
        } else {
            libraryPaths << (subDirectory + QDir::separator());
        }
    } else {
        Q_FOREACH (const QString &dir, QCoreApplication::libraryPaths()) {
            QString d = dir + QDir::separator() + subDirectory;
            if (!d.endsWith(QDir::separator())) {
                d += QDir::separator();
            }
            libraryPaths << d;
        }
    }
    //qDebug() << "Lib paths:" << libraryPaths;
    QElapsedTimer t1;
    t1.start();
    QElapsedTimer t2;
    t2.start();

    auto filter = [&](const KPluginMetaData &md) -> bool {
        if (servicetype.isEmpty()) {
            return true;
        }
        QStringList servicetypes = md.serviceTypes();
        // compatibility with the old key names (kservice_desktop_to_json vs kcoreaddons_desktop_to_json)
        if (servicetypes.isEmpty()) {
            servicetypes = md.rawData().value("X-KDE-ServiceTypes").toVariant().toStringList();
        }
        if (servicetypes.isEmpty()) {
            servicetypes = md.rawData().value("ServiceTypes").toVariant().toStringList();
        }
        return servicetypes.contains(servicetype);
    };

    QPluginLoader loader;

    Q_FOREACH (const QString &plugindir, libraryPaths) {
        const QString &_ixfile = plugindir + QStringLiteral("kpluginindex.json");
        QFile indexFile(_ixfile);
        //qDebug() << "indexfile: " << _ixfile << indexFile.exists();
        if (indexFile.exists()) {
            t2.start();
            indexFile.open(QIODevice::ReadOnly);
            QJsonDocument jdoc = QJsonDocument::fromBinaryData(indexFile.readAll());
//             QJsonDocument jdoc = QJsonDocument::fromJson(indexFile.readAll());
            indexFile.close();
//             qDebug() << "Reading cache :   " << t2.nsecsElapsed()/1000 << "microsec";
//             t2.start();

            QJsonArray plugins = jdoc.array();
//             qDebug() << "decoding:         " << t2.nsecsElapsed()/1000 << "microsec";
//             t2.start();
            for (QJsonArray::const_iterator iter = plugins.constBegin(); iter != plugins.constEnd(); ++iter) {
                const QJsonObject &obj = QJsonValue(*iter).toObject();
                const QString &pluginFileName = obj.value(QStringLiteral("FileName")).toString();
                const KPluginMetaData m(obj, pluginFileName);
                if (m.isValid() && filter(m)) {
                //if (servicetype.isEmpty() || m.serviceTypes().contains(servicetype)) {
                    allMetaData << m;
                }
            }
            //qDebug() << "creating KPI::List:" << t2.nsecsElapsed()/1000 << "microsec";
            //qDebug() << "=== Indexed ===" << _ixfile << plugins.count() << t2.nsecsElapsed()/1000 << "microsec";

        } else {
            t2.start();
//             QVector<KPluginMetaData> plugins = servicetype.isEmpty() ?
//                     KPluginLoader::findPlugins(plugindir) : KPluginLoader::findPlugins(plugindir, filter);
//             QVectorIterator<KPluginMetaData> iter(plugins);
//             while (iter.hasNext()) {
//                 auto md = iter.next();
//                 allMetaData << md;
//             }
            QDirIterator it(plugindir, suffixFilters(), QDir::Files);
            while (it.hasNext()) {
                it.next();
                const QString _f = it.fileInfo().absoluteFilePath();
                loader.setFileName(_f);

                KPluginMetaData md(loader);
                if (md.isValid() && filter(md)) {
                    allMetaData << md;
                }
            }
            //qDebug() << "=== Listed ===" << plugindir << plugins.count() << t2.nsecsElapsed()/1000 << "microsec";
        }
    }

    KPluginInfo::List lst = KPluginInfo::fromMetaData(allMetaData);
    int _before = lst.count();
    applyConstraints(lst, constraint);
    qDebug() << "Query for " << servicetype << " returned " << lst.count() << "/" << _before << "plugins after filtering" << "in" << t1.nsecsElapsed() / 1000 << "microsec";
    return lst;
}

