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

#include <Accounts/Account>
#include <Accounts/Manager>
#include <QDebug>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTest>
#include <QTextStream>

#define TEST_DIR "/tmp/hooks-test2"

namespace QTest {
template<>
char *toString(const QSet<QString> &set)
{
    QByteArray ba = "QSet<QString>(";
    QStringList list = set.toList();
    ba += list.join(", ");
    ba += ")";
    return qstrdup(ba.data());
}
} // QTest namespace

// map file name -> contents
typedef QHash<QString,QString> GeneratedFiles;

class OnlineAccountsHooksTest: public QObject
{
    Q_OBJECT

public:
    OnlineAccountsHooksTest();

private Q_SLOTS:
    void initTestCase();
    void testNoFiles();
    void testValidHooks_data();
    void testValidHooks();
    void testRemoval();

private:
    void clearHooksDir();
    void clearInstallDir();
    bool runHookProcess();
    bool runXmlDiff(const QString &generated, const QString &expected);
    void writeHookFile(const QString &name, const QString &contents);
    void writeInstalledFile(const QString &name, const QString &contents);
    void writePackageFile(const QString &name,
                          const QString &contents = QString());
    QStringList findGeneratedFiles() const;

private:
    QDir m_testDir;
    QDir m_hooksDir;
    QDir m_installDir;
    QDir m_packageDir;
};

OnlineAccountsHooksTest::OnlineAccountsHooksTest():
    QObject(0),
    m_testDir(TEST_DIR),
    m_hooksDir(TEST_DIR "/online-accounts-hooks"),
    m_installDir(TEST_DIR "/accounts"),
    m_packageDir(TEST_DIR "/package")
{
}

void OnlineAccountsHooksTest::clearHooksDir()
{
    m_hooksDir.removeRecursively();
    m_hooksDir.mkpath(".");
}

void OnlineAccountsHooksTest::clearInstallDir()
{
    m_installDir.removeRecursively();
    m_installDir.mkpath(".");
}

bool OnlineAccountsHooksTest::runHookProcess()
{
    QProcess process;
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.start(HOOK_PROCESS);
    if (!process.waitForFinished()) return false;

    return process.exitCode() == EXIT_SUCCESS;
}

bool OnlineAccountsHooksTest::runXmlDiff(const QString &generated,
                                         const QString &expected)
{
    QProcess process;
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    QStringList args;
    args.append(generated);
    args.append(expected);
    process.start("xmldiff", args);
    if (!process.waitForFinished()) return false;

    return process.exitCode() == EXIT_SUCCESS;
}

void OnlineAccountsHooksTest::writeHookFile(const QString &name,
                                            const QString &contents)
{
    QFile file(m_hooksDir.filePath(name));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not write file" << name;
        return;
    }

    file.write(contents.toUtf8());
}

void OnlineAccountsHooksTest::writeInstalledFile(const QString &name,
                                                 const QString &contents)
{
    QFileInfo fileInfo(name);
    m_installDir.mkpath(fileInfo.path());
    QFile file(m_installDir.filePath(name));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not write file" << name;
        return;
    }

    file.write(contents.toUtf8());
}

void OnlineAccountsHooksTest::writePackageFile(const QString &name,
                                               const QString &contents)
{
    QFileInfo fileInfo(name);
    m_packageDir.mkpath(fileInfo.path());
    QFile file(m_packageDir.filePath(name));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not write file" << name;
        return;
    }

    file.write(contents.toUtf8());
}

static QStringList findFiles(const QDir &dir)
{
    QStringList files = dir.entryList(QDir::Files);

    QStringList dirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    Q_FOREACH(const QString &d, dirs) {
        QStringList dirFiles = findFiles(QDir(dir.filePath(d)));
        Q_FOREACH(const QString &file, dirFiles) {
            files.append(dir.relativeFilePath(d + "/" + file));
        }
    }

    return files;
}

QStringList OnlineAccountsHooksTest::findGeneratedFiles() const
{
    return findFiles(m_installDir);
}

void OnlineAccountsHooksTest::initTestCase()
{
    qputenv("XDG_DATA_HOME", TEST_DIR);
    qputenv("OAH_CLICK_DIR", m_packageDir.path().toUtf8());
    qputenv("ACCOUNTS", TEST_DIR);

    // The hook must be able to run without a D-Bus session
    qunsetenv("DBUS_SESSION_BUS_ADDRESS");

    clearHooksDir();
    clearInstallDir();
}

void OnlineAccountsHooksTest::testNoFiles()
{
    clearHooksDir();
    clearInstallDir();

    QVERIFY(runHookProcess());
    QVERIFY(m_hooksDir.entryList(QDir::Files).isEmpty());
    QVERIFY(findGeneratedFiles().isEmpty());
}

void OnlineAccountsHooksTest::testValidHooks_data()
{
    QTest::addColumn<QString>("hookName");
    QTest::addColumn<QString>("contents");
    QTest::addColumn<GeneratedFiles>("expectedFiles");

    GeneratedFiles files;
    QTest::newRow("empty file") <<
        "com.ubuntu.test_MyApp_0.1.accounts" <<
        "" <<
        files;

    QTest::newRow("empty dictionary") <<
        "com.ubuntu.test_MyApp_0.1.accounts" <<
        "{}" <<
        files;

    files["applications/com.ubuntu.test_MyApp.application"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<application id=\"com.ubuntu.test_MyApp\">\n"
        "  <profile>com.ubuntu.test_MyApp_0.2</profile>\n"
        "  <package-dir>/tmp/hooks-test2/package</package-dir>\n"
        "  <desktop-entry>com.ubuntu.test_MyApp_0.2</desktop-entry>\n"
        "  <services>\n"
        "    <service id=\"com.ubuntu.test_MyApp_google\">\n"
        "      <description>Publish to Picasa</description>\n"
        "    </service>\n"
        "  </services>\n"
        "</application>\n";
    files["services/com.ubuntu.test_MyApp_google.service"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<service id=\"com.ubuntu.test_MyApp_google\">\n"
        "  <type>com.ubuntu.test_MyApp</type>\n"
        "  <provider>google</provider>\n"
        "  <name>Picasa</name>\n"
        "  <profile>com.ubuntu.test_MyApp_0.2</profile>\n"
        "</service>\n";
    QTest::newRow("one service") <<
        "com.ubuntu.test_MyApp_0.2.accounts" <<
        "{"
        "  \"services\": ["
        "    {"
        "      \"provider\": \"google\","
        "      \"description\": \"Publish to Picasa\","
        "      \"name\": \"Picasa\""
        "    }"
        "  ]"
        "}" <<
        files;

    files.clear();
    files["applications/com.ubuntu.test_MyApp.application"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<application id=\"com.ubuntu.test_MyApp\">\n"
        "  <profile>com.ubuntu.test_MyApp_0.3</profile>\n"
        "  <package-dir>/tmp/hooks-test2/package</package-dir>\n"
        "  <desktop-entry>com.ubuntu.test_MyApp_0.3</desktop-entry>\n"
        "  <translations>my-app</translations>\n"
        "  <services>\n"
        "    <service id=\"com.ubuntu.test_MyApp_facebook\">\n"
        "      <description>Publish to Facebook</description>\n"
        "    </service>\n"
        "    <service id=\"com.ubuntu.test_MyApp_google\">\n"
        "      <description>Publish to Picasa</description>\n"
        "    </service>\n"
        "  </services>\n"
        "</application>\n";
    files["services/com.ubuntu.test_MyApp_google.service"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<service id=\"com.ubuntu.test_MyApp_google\">\n"
        "  <type>com.ubuntu.test_MyApp</type>\n"
        "  <provider>google</provider>\n"
        "  <name>Picasa</name>\n"
        "  <profile>com.ubuntu.test_MyApp_0.3</profile>\n"
        "  <translations>my-app</translations>\n"
        "</service>\n";
    files["services/com.ubuntu.test_MyApp_facebook.service"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<service id=\"com.ubuntu.test_MyApp_facebook\">\n"
        "  <type>com.ubuntu.test_MyApp</type>\n"
        "  <provider>facebook</provider>\n"
        "  <name>Facebook photos</name>\n"
        "  <profile>com.ubuntu.test_MyApp_0.3</profile>\n"
        "  <translations>my-app</translations>\n"
        "</service>\n";
    QTest::newRow("two services") <<
        "com.ubuntu.test_MyApp_0.3.accounts" <<
        "{"
        "  \"translations\": \"my-app\","
        "  \"services\": ["
        "    {"
        "      \"provider\": \"facebook\","
        "      \"description\": \"Publish to Facebook\","
        "      \"name\": \"Facebook photos\""
        "    },"
        "    {"
        "      \"provider\": \"google\","
        "      \"description\": \"Publish to Picasa\","
        "      \"name\": \"Picasa\""
        "    }"
        "  ]"
        "}" <<
        files;

    files.clear();
    files["applications/com.ubuntu.test_MyApp2.application"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<application id=\"com.ubuntu.test_MyApp2\">\n"
        "  <profile>com.ubuntu.test_MyApp2_0.2</profile>\n"
        "  <package-dir>/tmp/hooks-test2/package</package-dir>\n"
        "  <desktop-entry>com.ubuntu.test_MyApp2_0.2</desktop-entry>\n"
        "  <services>\n"
        "    <service id=\"com.ubuntu.test_MyApp2_google\">\n"
        "      <description>Publish to Picasa</description>\n"
        "    </service>\n"
        "  </services>\n"
        "</application>\n";
    files["services/com.ubuntu.test_MyApp2_google.service"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<service id=\"com.ubuntu.test_MyApp2_google\">\n"
        "  <type>com.ubuntu.test_MyApp2</type>\n"
        "  <provider>google</provider>\n"
        "  <name>Picasa</name>\n"
        "  <profile>com.ubuntu.test_MyApp2_0.2</profile>\n"
        "  <template>\n"
        "    <group name=\"auth\">\n"
        "      <setting name=\"method\">oauth2</setting>\n"
        "      <setting name=\"mechanism\">web_server</setting>\n"
        "      <group name=\"oauth2\">\n"
        "        <group name=\"web_server\">\n"
        "          <setting name=\"ClientId\">foo</setting>\n"
        "          <setting name=\"ClientSecret\">bar</setting>\n"
        "          <setting name=\"Scopes\" type=\"as\">['one scope','and another']</setting>\n"
        "          <setting name=\"UseSSL\" type=\"b\">false</setting>\n"
        "        </group>\n"
        "      </group>\n"
        "    </group>\n"
        "  </template>\n"
        "</service>\n";
    QTest::newRow("one service, with auth data") <<
        "com.ubuntu.test_MyApp2_0.2.accounts" <<
        "{"
        "  \"services\": ["
        "    {"
        "      \"provider\": \"google\","
        "      \"description\": \"Publish to Picasa\","
        "      \"name\": \"Picasa\","
        "      \"auth\": {"
        "        \"oauth2/web_server\": {"
        "          \"ClientId\": \"foo\","
        "          \"ClientSecret\": \"bar\","
        "          \"UseSSL\": false,"
        "          \"Scopes\": [\"one scope\",\"and another\"]"
        "        }"
        "      }"
        "    }"
        "  ]"
        "}" <<
        files;

}

void OnlineAccountsHooksTest::testValidHooks()
{
    QFETCH(QString, hookName);
    QFETCH(QString, contents);
    QFETCH(GeneratedFiles, expectedFiles);

    clearHooksDir();
    clearInstallDir();

    writeHookFile(hookName, contents);
    QVERIFY(runHookProcess());

    QCOMPARE(findGeneratedFiles().toSet(), expectedFiles.keys().toSet());

    GeneratedFiles::const_iterator i = expectedFiles.constBegin();
    while (i != expectedFiles.constEnd()) {
        QString generatedXml = m_installDir.filePath(i.key());
        /* Dump the expected XML into a file */
        QString expectedXml = generatedXml + ".expected";
        QFile file(expectedXml);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QByteArray expectedContents = i.value().toUtf8();
        QCOMPARE(file.write(expectedContents), expectedContents.length());
        file.close();

        QVERIFY(runXmlDiff(generatedXml, expectedXml));
        i++;
    }
}

void OnlineAccountsHooksTest::testRemoval()
{
    clearHooksDir();
    clearInstallDir();

    QString stillInstalled("applications/com.ubuntu.test_StillInstalled.application");
    writeInstalledFile(stillInstalled,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application>\n"
        "  <description>My application</description>\n"
        "  <service-types>\n"
        "    <service-type>some type</service-type>\n"
        "  </service-types>\n"
        "  <profile>com-ubuntu.test_StillInstalled_2.0</profile>\n"
        "</application>");
    QVERIFY(m_installDir.exists(stillInstalled));

    QString myApp("applications/com.ubuntu.test_MyApp.application");
    writeInstalledFile(myApp,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application>\n"
        "  <description>My application</description>\n"
        "  <services>\n"
        "    <service id=\"com-ubuntu.test_MyApp_example\">\n"
        "      <description>Publish somewhere</description>\n"
        "    </service>\n"
        "  </services>\n"
        "  <profile>com-ubuntu.test_MyApp_3.0</profile>\n"
        "</application>");
    QVERIFY(m_installDir.exists(myApp));

    QString myService("services/com.ubuntu.test_MyApp_example.service");
    writeInstalledFile(myService,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<service>\n"
        "  <name>Hello world</name>\n"
        "  <provider>example</provider>\n"
        "  <description>My application</description>\n"
        "  <profile>com-ubuntu.test_MyApp_3.0</profile>\n"
        "</service>");
    QVERIFY(m_installDir.exists(myService));

    QString noProfile("applications/com.ubuntu.test_NoProfile.application");
    writeInstalledFile(noProfile,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application>\n"
        "  <description>My application</description>\n"
        "  <service-types>\n"
        "    <service-type>some type</service-type>\n"
        "  </service-types>\n"
        "</application>");
    QVERIFY(m_installDir.exists(noProfile));

    writeHookFile("com-ubuntu.test_StillInstalled_2.0.accounts",
        "{"
        "  \"services\": ["
        "    {"
        "      \"provider\": \"example\","
        "      \"description\": \"Publish somewhere\","
        "      \"name\": \"Hello world\""
        "    }"
        "  ]"
        "}");

    QVERIFY(runHookProcess());

    QVERIFY(m_installDir.exists(stillInstalled));
    QVERIFY(!m_installDir.exists(myApp));
    QVERIFY(!m_installDir.exists(myService));
    QVERIFY(m_installDir.exists(noProfile));
}

QTEST_MAIN(OnlineAccountsHooksTest);

#include "tst_online_accounts_hooks2.moc"
