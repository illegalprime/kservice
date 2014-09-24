/******************************************************************************
 *  Copyright 2014 Sebastian Kügler <sebas@kde.org>                           *
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

#ifndef KPLUGININDEXER_H
#define KPLUGININDEXER_H

#include <qcommandlineoption.h>
#include <QString>

class QCommandLineParser;

class KPluginIndexer
{
public:
    KPluginIndexer(QCommandLineParser *parser, const QCommandLineOption &p, const QCommandLineOption &o, const QCommandLineOption &a);
    int runMain();

private:
    QVariantMap convert(const QString &src);
    bool convertAll();
    bool convertDirectory(const QString &dir, const QString &dest);
    bool resolveFiles();
    QStringList findPluginSubDirectories(const QString &dir);

    QCommandLineParser *m_parser;
    QCommandLineOption all;
    QCommandLineOption plugindir;
    QCommandLineOption output;
    QString m_pluginDir;
    QStringList m_pluginDirectories;
    QString m_outFile;
};

#endif
