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
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <SystemSettings/ItemBase>

#include "plugin.h"

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

/* mocking libintl { */
static QHash<QString,QHash<QByteArray,QByteArray> > m_translations;
extern "C" {
char *dgettext(const char *domainname, const char *msgid)
{
    return (char *)m_translations[domainname].value(msgid).data();
}
char *dcgettext(const char *__domainname, const char *__msgid, int __category)
{
    Q_UNUSED(__category);
    return dgettext(__domainname, __msgid);
}
} // extern C
/* } mocking libintl */

class PluginTest: public QObject
{
    Q_OBJECT

    struct FileData {
        FileData() {}
        FileData(const QString &name, const QString &domain):
            name(name), domain(domain) {}
        QString name;
        QString domain;
    };

public:
    PluginTest();

private Q_SLOTS:
    void testKeywords_data();
    void testKeywords();

private:
    void setDataDir(const QTemporaryDir &dir);
    void writeProvider(const QString &id, const QString &name,
                       const QString &domain);
    void writeService(const QString &id, const QString &name,
                      const QString &domain);
    void writeLibaccountsFile(const QString &type, const QString &id,
                              const QString &name, const QString &domain);
    void createAccount(const QString &name);

private:
    QHash<QString,FileData> m_providersData;
    QHash<QString,FileData> m_servicesData;
    QDir m_dataDir;
};

PluginTest::PluginTest():
    QObject(0)
{
    m_providersData["provider_noname"] = FileData("", "");
    m_providersData["provider_nodomain"] = FileData("Happy", "");
    m_providersData["provider_translated"] = FileData("Joyful", "translations1");

    m_servicesData["service_nodomain"] = FileData("Sad", "");
    m_servicesData["service_translated"] = FileData("Depressed", "translations1");
    m_servicesData["service_translated2"] = FileData("Depressed", "translations2");

    m_translations["translations1"]["Happy"] = "Contento";
    m_translations["translations1"]["Joyful"] = "Gioioso";
    m_translations["translations1"]["Sad"] = "Triste";
    m_translations["translations1"]["Depressed"] = "Depresso1";
    m_translations["translations1"]["Black"] = "Nero";
    m_translations["translations1"]["White"] = "Bianco";

    m_translations["translations2"]["Depressed"] = "Depresso2";
}

void PluginTest::setDataDir(const QTemporaryDir &dir)
{
    QVERIFY(dir.isValid());

    m_dataDir = QDir(dir.path());

    QByteArray dataPath = dir.path().toUtf8();
    qputenv("ACCOUNTS", dataPath);
    qputenv("AG_PROVIDERS", dataPath);
    qputenv("AG_SERVICES", dataPath);
}

void PluginTest::writeProvider(const QString &id, const QString &name,
                               const QString &domain)
{
    writeLibaccountsFile("provider", id, name, domain);
}

void PluginTest::writeService(const QString &id, const QString &name,
                              const QString &domain)
{
    writeLibaccountsFile("service", id, name, domain);
}

void PluginTest::writeLibaccountsFile(const QString &type, const QString &id,
                                      const QString &name, const QString &domain)
{
    QDomDocument doc;
    QDomElement root = doc.createElement(type);
    root.setAttribute("id", id);
    doc.appendChild(root);

    QDomElement nameTag = doc.createElement("name");
    nameTag.appendChild(doc.createTextNode(name));
    root.appendChild(nameTag);

    QDomElement domainTag = doc.createElement("translations");
    domainTag.appendChild(doc.createTextNode(domain));
    root.appendChild(domainTag);

    if (type == "service") {
        QDomElement typeTag = doc.createElement("type");
        typeTag.appendChild(doc.createTextNode("something"));
        root.appendChild(typeTag);
    }

    QFile file(m_dataDir.filePath(id + "." + type));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not write file" << file.fileName();
        return;
    }

    file.write(doc.toString().toUtf8());
}

void PluginTest::createAccount(const QString &name)
{
    Accounts::Manager *manager = new Accounts::Manager;

    Accounts::Account *account = manager->createAccount("any");
    account->setDisplayName(name);
    account->syncAndBlock();

    delete manager;
}

void PluginTest::testKeywords_data()
{
    QTest::addColumn<QStringList>("providers");
    QTest::addColumn<QStringList>("services");
    QTest::addColumn<QStringList>("accountNames");
    QTest::addColumn<QStringList>("keywords");

    QTest::newRow("provider, no name") <<
        (QStringList() << "provider_noname") <<
        QStringList() <<
        QStringList() <<
        (QStringList() << "provider_noname");

    QTest::newRow("provider, untranslated") <<
        (QStringList() << "provider_nodomain") <<
        QStringList() <<
        QStringList() <<
        (QStringList() << "provider_nodomain" << "happy");

    QTest::newRow("provider, translated") <<
        (QStringList() << "provider_translated") <<
        QStringList() <<
        QStringList() <<
        (QStringList() << "provider_translated" << "joyful" << "gioioso");

    QTest::newRow("service, untranslated") <<
        QStringList() <<
        (QStringList() << "service_nodomain") <<
        QStringList() <<
        (QStringList() << "service_nodomain" << "sad");

    QTest::newRow("service, translated 1") <<
        QStringList() <<
        (QStringList() << "service_translated") <<
        QStringList() <<
        (QStringList() << "service_translated" << "depressed" << "depresso1");

    QTest::newRow("service, translated 2") <<
        QStringList() <<
        (QStringList() << "service_translated2") <<
        QStringList() <<
        (QStringList() << "service_translated2" << "depressed" << "depresso2");

    QTest::newRow("one account, one word") <<
        QStringList() <<
        QStringList() <<
        (QStringList() << "tom@example.com") <<
        (QStringList() << "tom@example.com");

    QTest::newRow("one account, many words") <<
        QStringList() <<
        QStringList() <<
        (QStringList() << "My little sweet account") <<
        (QStringList() << "my little sweet account");

    QTest::newRow("combined") <<
        (QStringList() << "provider_translated" << "provider_nodomain") <<
        (QStringList() << "service_translated" << "service_translated2") <<
        (QStringList() << "john@invalid" << "harry@mysite.com") <<
        (QStringList() << "provider_translated" << "joyful" << "gioioso" <<
         "provider_nodomain" << "happy" <<
         "service_translated" << "depressed" << "depresso1" <<
         "service_translated2" << "depressed" << "depresso2" <<
         "john@invalid" << "harry@mysite.com");
}

void PluginTest::testKeywords()
{
    QFETCH(QStringList, providers);
    QFETCH(QStringList, services);
    QFETCH(QStringList, accountNames);
    QFETCH(QStringList, keywords);

    QTemporaryDir dataDir;

    setDataDir(dataDir);

    /* Create the needed files */
    Q_FOREACH(const QString &providerId, providers) {
        writeProvider(providerId,
                      m_providersData[providerId].name,
                      m_providersData[providerId].domain);
    }

    Q_FOREACH(const QString &serviceId, services) {
        writeService(serviceId,
                     m_servicesData[serviceId].name,
                     m_servicesData[serviceId].domain);
    }

    /* create the accounts */
    Q_FOREACH(const QString &displayName, accountNames) {
        createAccount(displayName);
    }

    /* Now do the actual test */
    Plugin plugin;

    SystemSettings::ItemBase *item = plugin.createItem(QVariantMap());
    QStringList pluginKeywords = item->keywords();

    QCOMPARE(pluginKeywords.toSet(), keywords.toSet());
}

QTEST_MAIN(PluginTest);

#include "tst_plugin.moc"
