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

#include "globals.h"
#include "mock/request-manager-mock.h"
#include "signonui-service.h"

#include <QDebug>
#include <QSignalSpy>
#include <QString>
#include <QTemporaryDir>
#include <QTest>
#include <sys/time.h>

using namespace SignOnUi;

class ServiceTest: public QObject
{
    Q_OBJECT

public:
    ServiceTest();

private:
    void writeFile(const QString &name, const QByteArray &contents);
    void setFileDate(const QString &name, qint64 timestamp);

private Q_SLOTS:
    void testCookies_data();
    void testCookies();

private:
    OnlineAccountsUi::RequestManager m_requestManager;
    Service m_service;
};

ServiceTest::ServiceTest():
    QObject(0)
{
}

void ServiceTest::writeFile(const QString &name, const QByteArray &contents)
{
    QFile file(name);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not write file" << name;
        return;
    }

    file.write(contents);
}

void ServiceTest::setFileDate(const QString &name, qint64 timestamp)
{
    struct timeval times[2];

    times[0].tv_sec = times[1].tv_sec = timestamp;
    times[0].tv_usec = times[1].tv_usec = 0;
    utimes(name.toUtf8().constData(), times);
}

void ServiceTest::testCookies_data()
{
    QTest::addColumn<QString>("contents");
    QTest::addColumn<qint64>("expectedTimestamp");
    QTest::addColumn<RawCookies>("expectedCookies");

    RawCookies cookies;

    QTest::newRow("empty") <<
        "" <<
        qint64(1406204196) <<
        cookies;

    QTest::newRow("invalid-cookies") <<
        "[\n"
        "  {\"path\": \"/\"},\n"
        "  {\"name\": \"C1\", \"path\": \"/\"},\n"
        "  {\"value\": \"else\", \"domain\": \"foo.com\"}\n"
        "]" <<
        qint64(1406104196) <<
        cookies;
    cookies.clear();

    cookies.append("C1=something");
    cookies.append("C2=else; domain=foo.com");
    cookies.append("C3=yes; HttpOnly");
    cookies.append("C4=OK; path=/");
    cookies.append("C5=no; expires=Sat, 20-Aug-2016 16:33:43 GMT");
    QTest::newRow("few-cookies") <<
        "[\n"
        "  {\"name\": \"C1\", \"value\": \"something\"},\n"
        "  {\"name\": \"C2\", \"value\": \"else\", \"domain\": \"foo.com\"},\n"
        "  {\"name\": \"C3\", \"value\": \"yes\", \"httponly\": \"true\"},\n"
        "  {\"name\": \"C4\", \"value\": \"OK\", \"path\": \"/\"},\n"
        "  {\"name\": \"C5\", \"value\": \"no\", \"expirationdate\": \"2016-08-20T16:33:43Z\"}\n"
        "]" <<
        qint64(1406104196) <<
        cookies;
}

void ServiceTest::testCookies()
{
    QFETCH(QString, contents);
    QFETCH(qint64, expectedTimestamp);
    QFETCH(RawCookies, expectedCookies);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    qputenv("XDG_CACHE_HOME", tempDir.path().toUtf8());
    quint32 id = 47;

    QString subDir =
        QString("online-accounts-ui/id-%2").arg(id);
    QDir identityDir(tempDir.path());
    identityDir.mkpath(subDir);
    identityDir.cd(subDir);

    QString cookieFile(identityDir.filePath("cookies.json"));
    writeFile(cookieFile, contents.toUtf8());
    setFileDate(cookieFile, expectedTimestamp);

    RawCookies cookies;
    qint64 timestamp = 0;
    m_service.cookiesForIdentity(id, cookies, timestamp);
    QCOMPARE(cookies, expectedCookies);
    /* Be more tolerant about the timestamp: divide by 10, to allow a 10
     * seconds gap */
    QCOMPARE(timestamp / 10, expectedTimestamp / 10);
}

QTEST_MAIN(ServiceTest);

#include "tst_signonui_service.moc"
