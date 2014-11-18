/*
 * Copyright (C) 2014 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
 *
 * This file is part of online-accounts-ui
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <QStringList>

QStringList parseParameters(const QByteArray &cmdLine)
{
    QStringList params;
    QByteArray nextParam;

    bool insideDoubleQuotes = false;

    for (int i = 0; i < cmdLine.count(); i++) {
        char c = cmdLine.at(i);
        if (c == '\'' && !insideDoubleQuotes) {
            int endQuote = cmdLine.indexOf('\'', i + 1);
            if (Q_UNLIKELY(endQuote < 0)) {
                qWarning() << "Cannot parse parameters" << cmdLine;
                return params;
            }
            nextParam += cmdLine.mid(i + 1, endQuote - i - 1);
            i = endQuote;
        } else if (c == '"') {
            if (insideDoubleQuotes) {
                insideDoubleQuotes = false;
            } else {
                insideDoubleQuotes = true;
            }
        } else if (c == '\\') {
            i++;
            if (Q_UNLIKELY(i == cmdLine.count())) {
                qWarning() << "Incomplete command line" << cmdLine;
                return params;
            }
            nextParam += cmdLine.at(i);
        } else if (c == ' ' && !insideDoubleQuotes) {
            params.append(QString::fromUtf8(nextParam));
            nextParam.clear();
        } else {
            nextParam += cmdLine.at(i);
        }
    }

    if (!nextParam.isEmpty()) {
        params.append(QString::fromUtf8(nextParam));
    }

    return params;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QStringList appUris = parseParameters(qgetenv("APP_URIS"));
    if (Q_UNLIKELY(appUris.count() < 2)) {
        qCritical() << "Missing URIs";
        return EXIT_FAILURE;
    }

    QStringList arguments;
    arguments.append("set-env");
    arguments.append("APP_EXEC=" ONLINE_ACCOUNTS_UI_PATH);

    // IPC socket URL
    QString socketUrl =
        QString("IPC_SOCKET=%1").arg(appUris[0]);
    arguments.append(socketUrl);

    // Mir socket URL; might be invalid
    QString mirSocket = appUris[1];
    if (!mirSocket.startsWith("invalid")) {
        QString mirUrl =
            QString("MIR_SOCKET=%1").arg(mirSocket);
        arguments.append(mirUrl);
    }

    arguments.append("OAU_LOGGING_LEVEL=2");

    QStringList newAppUrisList = appUris.mid(2);
    QString newAppUris = QString("APP_URIS=%1").arg(newAppUrisList.join(' '));

    arguments.append(newAppUris);

    int ret = QProcess::execute("initctl", arguments);

    return (ret >= 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

