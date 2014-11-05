/*
 *  Copyright 2014 Alex Richardson <arichardson.kde@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation;
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include <QtTest/QTest>
#include <QLocale>
#include <QJsonDocument>
#include <QFileInfo>
#include <QJsonArray>

//#include <KAboutData>
#include <KPluginMetaData>

#include <kplugininfo.h>
#include <kplugintrader.h>
#include <kservice.h>

#include <kservicetypetrader.h>
#include <QDebug>

Q_DECLARE_METATYPE(KPluginInfo)

class KPluginMetaDataTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void findPlugin_data()
    {
        QTest::addColumn<QString>("serviceType");
        QTest::addColumn<QString>("constraint");
        QTest::addColumn<int>("expectedResult");

        const QString _st = "KService/NSA";
        QString _c = "";
        QTest::newRow("no constraints") << _st << _c << 1;

//        return;
        _c = QString("[X-KDE-PluginInfo-Name] == '%1'").arg("fakeplugin");
        QTest::newRow("by pluginname") << _st << _c << 1;

        _c = QString("[X-KDE-PluginInfo-Category] == '%1'").arg("Examples");
        QTest::newRow("by category") << _st << _c << 1;

        _c = QString("([X-KDE-PluginInfo-Category] == 'Examples') AND ([X-KDE-PluginInfo-Email] == 'sebas@kde.org')");
        QTest::newRow("complex query") << _st << _c << 1;

        _c = QString("([X-KDE-PluginInfo-Category] == 'Examples') AND ([X-KDE-PluginInfo-Email] == 'prrrrt')");
        QTest::newRow("empty query") << _st << _c << 0;
    }

    void findPlugin()
    {
        QFETCH(QString, serviceType);
        QFETCH(QString, constraint);
        QFETCH(int, expectedResult);
        const KPluginInfo::List res = KPluginTrader::self()->query(QString(), serviceType, constraint);
        QCOMPARE(res.count(), expectedResult);
        QElapsedTimer t;
        t.start();
        KService::List offers = KServiceTypeTrader::self()->query(serviceType, constraint);
        qDebug() << "KServiceTypeTrader::query() took " << (t.nsecsElapsed() / 1000);
    }

    void findService()
    {


    }
};

QTEST_MAIN(KPluginMetaDataTest)

#include "kpluginmetadatatest.moc"
