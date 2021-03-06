/* This file is part of the KDE libraries
   Copyright (C) 2003 David Faure <faure@kde.org>

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

#include "kdbusservicestarter.h"
#include "kservicetypetrader.h"
#include "kservice.h"
#include <klocalizedstring.h>
#include <ktoolinvocation.h>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDebug>

class KDBusServiceStarterPrivate
{
public:
    KDBusServiceStarterPrivate() : q(nullptr) {}
    ~KDBusServiceStarterPrivate()
    {
        delete q;
    }
    KDBusServiceStarter *q;
};

Q_GLOBAL_STATIC(KDBusServiceStarterPrivate, privateObject)

KDBusServiceStarter *KDBusServiceStarter::self()
{
    if (!privateObject()->q) {
        new KDBusServiceStarter;
        Q_ASSERT(privateObject()->q);
    }
    return privateObject()->q;
}

KDBusServiceStarter::KDBusServiceStarter()
{
    // Set the singleton instance - useful when a derived KDBusServiceStarter
    // was created (before self() was called)
    Q_ASSERT(!privateObject()->q);
    privateObject()->q = this;
}

KDBusServiceStarter::~KDBusServiceStarter()
{
}

int KDBusServiceStarter::findServiceFor(const QString &serviceType,
                                        const QString &_constraint,
                                        QString *error, QString *pDBusService,
                                        int flags)
{
    // Ask the trader which service is preferred for this servicetype
    // We want one that provides a DBus interface
    QString constraint = _constraint;
    if (!constraint.isEmpty()) {
        constraint += QLatin1String(" and ");
    }
    constraint += QLatin1String("exist [X-DBUS-ServiceName]");
    const KService::List offers = KServiceTypeTrader::self()->query(serviceType, constraint);
    if (offers.isEmpty()) {
        if (error) {
            *error = i18n("No service implementing %1",  serviceType);
        }
        qWarning() << "KDBusServiceStarter: No service implementing " << serviceType;
        return -1;
    }
    KService::Ptr ptr = offers.first();
    QString dbusService = ptr->property(QStringLiteral("X-DBUS-ServiceName")).toString();

    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(dbusService)) {
        QString err;
        if (startServiceFor(serviceType, constraint, &err, &dbusService, flags) != 0) {
            if (error) {
                *error = err;
            }
            qWarning() << "Couldn't start service" << dbusService << "for" << serviceType << ":" << err;
            return -2;
        }
    }
    //qDebug() << "DBus service is available now, as" << dbusService;
    if (pDBusService) {
        *pDBusService = dbusService;
    }
    return 0;
}

int KDBusServiceStarter::startServiceFor(const QString &serviceType,
        const QString &constraint,
        QString *error, QString *dbusService, int /*flags*/)
{
    const KService::List offers = KServiceTypeTrader::self()->query(serviceType, constraint);
    if (offers.isEmpty()) {
        return -1;
    }
    KService::Ptr ptr = offers.first();
    //qDebug() << "starting" << ptr->entryPath();
    return KToolInvocation::startServiceByDesktopPath(ptr->entryPath(), QStringList(), error, dbusService);
}
