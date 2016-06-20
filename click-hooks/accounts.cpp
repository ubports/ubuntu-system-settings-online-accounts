/*
 * Copyright (C) 2015 Canonical Ltd.
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
#include <Accounts/Service>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QStringList>
#include <click.h>
#include <sys/types.h>
#include <utime.h>
#include "acl-updater.h"

static QString findPackageDir(const QString &appId)
{
    /* For testing */
    QByteArray packageDirEnv = qgetenv("OAH_CLICK_DIR");
    if (!packageDirEnv.isEmpty()) {
        return QString::fromUtf8(packageDirEnv);
    }

    QStringList components = appId.split('_');
    QByteArray package = components.first().toUtf8();

    GError *error = NULL;
    ClickUser *user = click_user_new_for_user(NULL, NULL, &error);
    if (Q_UNLIKELY(!user)) {
        qWarning() << "Unable to read Click database:" << error->message;
        g_error_free(error);
        return QString();
    }

    gchar *pkgDir = click_user_get_path(user, package.constData(), &error);
    if (Q_UNLIKELY(!pkgDir)) {
        qWarning() << "Unable to get the Click package directory for" <<
            package << ":" << error->message;
        g_error_free(error);
        g_object_unref(user);
        return QString();
    }

    QString ret = QString::fromUtf8(pkgDir);
    g_object_unref(user);
    g_free(pkgDir);
    return ret;
}

static QString stripVersion(const QString &appId)
{
    QStringList components = appId.split('_');
    if (components.count() != 3) return appId;

    /* Click packages have a profile of the form
     *  $name_$application_$version
     * (see https://wiki.ubuntu.com/SecurityTeam/Specifications/ApplicationConfinement/Manifest#Click)
     *
     * So we just need to strip out the last part.
     */
    components.removeLast();
    return components.join('_');
}

class ManifestFile {
public:
    ManifestFile(const QFileInfo &hookFileInfo,
                 const QString &appId, const QString &shortAppId);

    bool writeFiles(const QDir &accountsDir);
    bool writeServiceFile(const QDir &accountsDir, const QString &id,
                          const QString &provider, const QJsonObject &json);
    bool writePlugins(const QDir &accountsDir);
    bool writeProviderFile(const QDir &accountsDir, const QString &id,
                           const QJsonObject &json);
    QDomDocument createDocument() const;
    QDomElement createGroup(QDomDocument &doc, const QString &name);
    void addTemplate(QDomDocument &doc, const QJsonObject &json);
    void addSetting(QDomElement &parent, const QString &name,
                    const QString &value, const QString &type = QString());
    void addSettings(QDomElement &parent, const QJsonObject &json);
    void parseManifest(const QJsonObject &mainObject);
    void checkId(const QString &shortAppId);
    void addProfile(QDomDocument &doc);
    void addPackageDir(QDomDocument &doc);
    void addTranslations(QDomDocument &doc);
    QString profile() const;
    void addDesktopFile(QDomDocument &doc);
    bool writeXmlFile(const QDomDocument &doc, const QString &fileName) const;
    bool isValid() const { return m_isValid; }

private:
    QFileInfo m_hookFileInfo;
    QString m_packageDir;
    QJsonArray m_services;
    QJsonObject m_plugin;
    QString m_appId;
    QString m_shortAppId;
    QString m_trDomain;
    bool m_isScope;
    bool m_isValid;
};

ManifestFile::ManifestFile(const QFileInfo &hookFileInfo,
                           const QString &appId,
                           const QString &shortAppId):
    m_hookFileInfo(hookFileInfo),
    m_appId(appId),
    m_shortAppId(shortAppId),
    m_isScope(false),
    m_isValid(false)
{
    QFile file(hookFileInfo.filePath());
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject mainObject = doc.object();
        file.close();

        m_services = mainObject.value("services").toArray();
        m_trDomain = mainObject.value("translations").toString();
        m_plugin = mainObject.value("plugin").toObject();
        m_isScope = mainObject.value("scope").toBool();
        m_isValid = !m_services.isEmpty() || !m_plugin.isEmpty();

        m_packageDir = findPackageDir(appId);
    }
}

bool ManifestFile::writeFiles(const QDir &accountsDir)
{
    bool ok = true;
    QDomDocument doc = createDocument();
    QDomElement root = doc.createElement(QStringLiteral("application"));
    root.setAttribute(QStringLiteral("id"), m_shortAppId);
    doc.appendChild(root);

    addProfile(doc);
    addPackageDir(doc);
    addDesktopFile(doc);
    addTranslations(doc);

    QDomElement servicesElem = doc.createElement(QStringLiteral("services"));
    Q_FOREACH(const QJsonValue &v, m_services) {
        QJsonObject o = v.toObject();
        QString provider = o.value("provider").toString();
        QString id = QString("%1_%2").arg(m_shortAppId).arg(provider);
        QString description = o.value("description").toString();
        if (description.isEmpty()) description = ".";

        QDomElement serviceElem = doc.createElement(QStringLiteral("service"));
        serviceElem.setAttribute(QStringLiteral("id"), id);
        QDomElement elem = doc.createElement(QStringLiteral("description"));
        elem.appendChild(doc.createTextNode(description));
        serviceElem.appendChild(elem);
        servicesElem.appendChild(serviceElem);

        if (!writeServiceFile(accountsDir, id, provider, o)) {
            qWarning() << "Writing service file failed" << id;
            ok = false;
            break;
        }
    }
    root.appendChild(servicesElem);

    if (!writePlugins(accountsDir)) {
        ok = false;
    }

    if (ok && !m_services.isEmpty()) {
        QString applicationFile =
            QString("applications/%1.application").arg(m_shortAppId);
        if (!writeXmlFile(doc, accountsDir.filePath(applicationFile))) {
            qWarning() << "Writing application file failed" << applicationFile;
            ok = false;
        }
    }

    return ok;
}

bool ManifestFile::writeServiceFile(const QDir &accountsDir, const QString &id,
                                    const QString &provider,
                                    const QJsonObject &json)
{
    QDomDocument doc = createDocument();
    QDomElement root = doc.createElement(QStringLiteral("service"));
    root.setAttribute(QStringLiteral("id"), id);
    doc.appendChild(root);

    // type
    QDomElement elem = doc.createElement(QStringLiteral("type"));
    elem.appendChild(doc.createTextNode(m_shortAppId));
    root.appendChild(elem);

    // provider
    elem = doc.createElement(QStringLiteral("provider"));
    elem.appendChild(doc.createTextNode(provider));
    root.appendChild(elem);

    // name
    QString name = json.value(QStringLiteral("name")).toString();
    if (name.isEmpty()) {
        name = ".";
    }
    elem = doc.createElement(QStringLiteral("name"));
    elem.appendChild(doc.createTextNode(name));
    root.appendChild(elem);

    addProfile(doc);
    addTranslations(doc);
    addTemplate(doc, json);

    return writeXmlFile(doc, accountsDir.filePath(QString("services/%1.service").arg(id)));
}

bool ManifestFile::writePlugins(const QDir &accountsDir)
{
    if (m_plugin.isEmpty()) return true;

    QString qmlPlugin = m_plugin.value("qml").toString();
    if (Q_UNLIKELY(qmlPlugin.isEmpty())) {
        qWarning() << "Plugin is missing 'qml' key";
        return false;
    }

    if (!QDir::isAbsolutePath(qmlPlugin)) {
        qmlPlugin = m_packageDir + "/" + qmlPlugin;
    }

    if (!QFile::exists(qmlPlugin)) {
        qWarning() << "Can't find QML files in" << qmlPlugin;
        return false;
    }

    QString qmlDestination = QString("qml-plugins/%1").arg(m_shortAppId);
    QFile::remove(accountsDir.filePath(qmlDestination));
    if (!QFile::link(qmlPlugin, accountsDir.filePath(qmlDestination))) {
        qWarning() << "Cannot symlink QML files" << qmlPlugin;
        return false;
    }

    if (!writeProviderFile(accountsDir, m_shortAppId, m_plugin)) {
        qWarning() << "Writing provider file failed" << m_shortAppId;
        return false;
    }
    return true;
}

bool ManifestFile::writeProviderFile(const QDir &accountsDir,
                                     const QString &id,
                                     const QJsonObject &json)
{
    QDomDocument doc = createDocument();
    QDomElement root = doc.createElement(QStringLiteral("provider"));
    root.setAttribute(QStringLiteral("id"), id);
    doc.appendChild(root);

    // name
    QString name = json.value(QStringLiteral("name")).toString();
    if (Q_UNLIKELY(name.isEmpty())) {
        qWarning() << "Provider name is required";
        return false;
    }
    QDomElement elem = doc.createElement(QStringLiteral("name"));
    elem.appendChild(doc.createTextNode(name));
    root.appendChild(elem);

    // icon
    QString icon = json.value(QStringLiteral("icon")).toString();
    if (Q_UNLIKELY(icon.isEmpty())) {
        qWarning() << "Provider icon is required";
        return false;
    }
    if (!QDir::isAbsolutePath(icon)) {
        QString test = m_packageDir + "/" + icon;
        if (QFile::exists(test)) {
            icon = test;
        }
    }
    elem = doc.createElement(QStringLiteral("icon"));
    elem.appendChild(doc.createTextNode(icon));
    root.appendChild(elem);

    addProfile(doc);
    addTranslations(doc);
    addTemplate(doc, json);
    addPackageDir(doc);

    return writeXmlFile(doc, accountsDir.filePath(QString("providers/%1.provider").arg(id)));
}

QDomDocument ManifestFile::createDocument() const
{
    QDomDocument doc;
    doc.appendChild(doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\""));
    QString comment = QString("this file is auto-generated by %1; do not modify").
        arg(QCoreApplication::applicationName());
    doc.appendChild(doc.createComment(comment));
    return doc;
}

QDomElement ManifestFile::createGroup(QDomDocument &doc, const QString &name)
{
    QDomElement group = doc.createElement(QStringLiteral("group"));
    group.setAttribute(QStringLiteral("name"), name);
    return group;
}

void ManifestFile::addTemplate(QDomDocument &doc, const QJsonObject &json)
{
    QDomElement root = doc.documentElement();
    QDomElement templateElem = doc.createElement(QStringLiteral("template"));

    // auth
    QDomElement authElem = createGroup(doc, QStringLiteral("auth"));
    QJsonObject auth = json.value(QStringLiteral("auth")).toObject();
    for (QJsonObject::const_iterator i = auth.begin(); i != auth.end(); i++) {
        QStringList authParts = i.key().split("/");
        if (authParts.count() != 2) {
            qWarning() << "auth key must contain exactly one '/'!";
            continue;
        }
        QString method = authParts[0];
        QString mechanism = authParts[1];

        addSetting(authElem, QStringLiteral("method"), method);
        addSetting(authElem, QStringLiteral("mechanism"), mechanism);

        QDomElement methodGroup = createGroup(doc, method);
        authElem.appendChild(methodGroup);
        QDomElement mechanismGroup = createGroup(doc, mechanism);

        addSettings(mechanismGroup, i.value().toObject());
        methodGroup.appendChild(mechanismGroup);
    }

    if (!auth.isEmpty()) templateElem.appendChild(authElem);

    // settings
    QJsonObject settings = json.value(QStringLiteral("settings")).toObject();
    addSettings(templateElem, settings);

    if (templateElem.hasChildNodes()) {
        root.appendChild(templateElem);
    }
}

void ManifestFile::addSetting(QDomElement &parent, const QString &name,
                              const QString &value, const QString &type)
{
    QDomDocument doc = parent.ownerDocument();
    QDomElement setting = doc.createElement(QStringLiteral("setting"));
    setting.setAttribute(QStringLiteral("name"), name);
    if (!type.isEmpty()) {
        setting.setAttribute(QStringLiteral("type"), type);
    }
    setting.appendChild(doc.createTextNode(value));
    parent.appendChild(setting);
}

void ManifestFile::addSettings(QDomElement &parent, const QJsonObject &json)
{
    QDomDocument doc = parent.ownerDocument();
    for (QJsonObject::const_iterator i = json.begin(); i != json.end(); i++) {
        QDomElement elem = parent;
        QStringList parts = i.key().split('/');
        QString key = parts.takeLast();
        Q_FOREACH(const QString &groupName, parts) {
            QDomElement subGroup = createGroup(doc, groupName);
            elem.appendChild(subGroup);
            elem = subGroup;
        }
        QString value;
        QString type;
        switch (i.value().type()) {
        case QJsonValue::Bool:
            type = "b";
            value = i.value().toBool() ? "true" : "false";
            break;
        case QJsonValue::String:
            value = i.value().toString();
            break;
        case QJsonValue::Array:
            {
                QJsonArray array = i.value().toArray();
                QStringList values;
                Q_FOREACH(const QJsonValue &v, array) {
                    if (Q_UNLIKELY(v.type() != QJsonValue::String)) {
                        qWarning() << "Only arrays of strings are supported!";
                        continue;
                    }
                    values.append(QString("'%1'").arg(v.toString()));
                }
                type = "as";
                value = QString("[%1]").arg(values.join(','));
            }
            break;
        case QJsonValue::Object:
            {
                QDomElement subGroup = createGroup(doc, key);
                QJsonObject object = i.value().toObject();
                addSettings(subGroup, object);
                elem.appendChild(subGroup);
            }
            break;
        default:
            qWarning() << "Unsupported setting type:" << i.value();
        }
        if (value.isEmpty()) continue;
        addSetting(elem, key, value, type);
    }
}

void ManifestFile::addProfile(QDomDocument &doc)
{
    QDomElement root = doc.documentElement();
    QDomElement elem = doc.createElement(QStringLiteral("profile"));
    elem.appendChild(doc.createTextNode(m_appId));
    root.appendChild(elem);
}

void ManifestFile::addTranslations(QDomDocument &doc)
{
    if (m_trDomain.isEmpty()) return;
    QDomElement root = doc.documentElement();
    QDomElement elem = doc.createElement(QStringLiteral("translations"));
    elem.appendChild(doc.createTextNode(m_trDomain));
    root.appendChild(elem);
}

void ManifestFile::addPackageDir(QDomDocument &doc)
{
    if (Q_UNLIKELY(m_packageDir.isEmpty())) return;

    QDomElement root = doc.documentElement();
    QDomElement elem = doc.createElement(QStringLiteral("package-dir"));
    elem.appendChild(doc.createTextNode(m_packageDir));
    root.appendChild(elem);
}

void ManifestFile::addDesktopFile(QDomDocument &doc)
{
    QDomElement root = doc.documentElement();
    QDomElement elem = doc.createElement(QStringLiteral("desktop-entry"));
    elem.appendChild(doc.createTextNode(m_isScope ? m_shortAppId : m_appId));
    root.appendChild(elem);
}

bool ManifestFile::writeXmlFile(const QDomDocument &doc, const QString &fileName) const
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    bool ok = (file.write(doc.toByteArray(2)) > 0);
    file.close();

    if (ok) {
        return true;
    } else {
        QFile::remove(fileName);
        return false;
    }
}

class LibAccountsFile: public QDomDocument {
public:
    LibAccountsFile(const QFileInfo &fileInfo);

    QString profile() const;
    bool createdByUs() const;
    bool isValid() const { return m_isValid; }

private:
    QFileInfo m_fileInfo;
    bool m_isValid;
};

LibAccountsFile::LibAccountsFile(const QFileInfo &fileInfo):
    QDomDocument(),
    m_fileInfo(fileInfo),
    m_isValid(false)
{
    QFile file(fileInfo.filePath());
    if (file.open(QIODevice::ReadOnly)) {
        if (setContent(&file)) {
            m_isValid = true;
        }
        file.close();
    }
}

QString LibAccountsFile::profile() const
{
    QDomElement root = documentElement();
    return root.firstChildElement("profile").text();
}

bool LibAccountsFile::createdByUs() const
{
    QString creatorMark = QCoreApplication::applicationName() + ";";
    for (QDomNode n = firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (n.isComment() && n.nodeValue().contains(creatorMark)) {
            return true;
        }
    }
    return false;
}

static void disableService(Accounts::Manager *manager,
                           const QString &serviceId,
                           const QString &profile)
{
    Accounts::Service service = manager->service(serviceId);
    if (Q_UNLIKELY(!service.isValid())) return;

    AclUpdater aclUpdater;
    Q_FOREACH(Accounts::AccountId accountId, manager->accountListEnabled()) {
        Accounts::Account *account = manager->account(accountId);
        if (Q_UNLIKELY(!account)) continue;

        if (account->providerName() != service.provider()) continue;

        uint credentialsId = account->credentialsId();
        account->selectService(service);
        if (account->isEnabled()) {
            account->setEnabled(false);
            account->sync();
            aclUpdater.removeApp(stripVersion(profile), credentialsId);
        }
    }
}

static void removeStaleAccounts(Accounts::Manager *manager,
                                const QString &providerName)
{
    Q_FOREACH(Accounts::AccountId id, manager->accountList()) {
        Accounts::Account *account = manager->account(id);
        if (account->providerName() == providerName) {
            account->remove();
            account->syncAndBlock();
        }
    }
}

static void removeStaleFiles(Accounts::Manager *manager,
                             const QStringList &fileTypes,
                             const QDir &accountsDir,
                             const QDir &hooksDirIn)
{
    /* Walk through all of
     * ~/.local/share/accounts/{providers,services,applications}/
     * and remove files which are no longer present in hooksDirIn.
     */
    Q_FOREACH(const QString &fileType, fileTypes) {
        QDir dir(accountsDir.filePath(fileType + "s"));
        dir.setFilter(QDir::Files | QDir::Readable);
        QStringList fileTypeFilter;
        fileTypeFilter << "*." + fileType;
        dir.setNameFilters(fileTypeFilter);

        Q_FOREACH(const QFileInfo &fileInfo, dir.entryInfoList()) {
            LibAccountsFile file(fileInfo.filePath());

            /* If this file was not created by our hook let's ignore it. */
            if (!file.createdByUs()) continue;

            QString profile = file.profile();

            /* Check that the hook file is still there; if it isn't, then it
             * means that the click package was removed, and we must remove our
             * copy as well. */
            QString hookFileName = stripVersion(profile) + "_*.accounts";
            QStringList nameFilters = QStringList() << hookFileName;
            if (!hooksDirIn.entryList(nameFilters).isEmpty()) continue;

            if (fileType == "service") {
                /* Make sure services get disabled. See also:
                 * https://bugs.launchpad.net/bugs/1417261 */
                disableService(manager, fileInfo.completeBaseName(), profile);
            } else if (fileType == "provider") {
                /* If this is a provider, we must also remove any accounts
                 * associated with it */
                removeStaleAccounts(manager, fileInfo.completeBaseName());
            }
            QFile::remove(fileInfo.filePath());
        }
    }
}

static void removeStaleTimestampFiles(const QDir &hooksDirIn)
{
    Q_FOREACH(const QFileInfo &fileInfo, hooksDirIn.entryInfoList()) {
        if (fileInfo.suffix() != "processed") continue;

        /* Find the corresponding hook file */
        QFileInfo hookInfo(fileInfo.absolutePath() + "/" + fileInfo.completeBaseName());

        if (!hookInfo.exists()) {
            QFile::remove(fileInfo.filePath());
        }
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    Accounts::Manager::Options managerOptions;
    if (qgetenv("DBUS_SESSION_BUS_ADDRESS").isEmpty()) {
        managerOptions |= Accounts::Manager::DisableNotifications;
    }
    Accounts::Manager *manager = new Accounts::Manager(managerOptions);

    /* Go through the hook files in ~/.local/share/online-accounts-hooks2/ and
     * check if they have already been processed into a file under
     * ~/.local/share/accounts/{services,applications}/;
     * if not, open the hook file, write the APP_ID somewhere in it, and
     * save the result in the location where libaccounts expects to find it.
     */

    QStringList fileTypes;
    fileTypes <<
        QStringLiteral("service") <<
        QStringLiteral("provider") <<
        QStringLiteral("application");

    // This is ~/.local/share/
    const QString localShare =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QDir hooksDirIn(localShare + "/" HOOK_FILES_SUBDIR);

    /* Make sure the directories exist */
    QDir accountsDir(localShare + "/accounts");
    accountsDir.mkpath("applications");
    accountsDir.mkpath("services");
    accountsDir.mkpath("providers");
    accountsDir.mkpath("qml-plugins");

    removeStaleFiles(manager, fileTypes, accountsDir, hooksDirIn);
    removeStaleTimestampFiles(hooksDirIn);

    Q_FOREACH(const QFileInfo &fileInfo, hooksDirIn.entryInfoList()) {
        if (fileInfo.suffix() != "accounts") continue;

        // Our click hook sets the base name to the APP_ID
        QString appId = fileInfo.completeBaseName();

        /* When publishing this file for libaccounts, we want to strip
         * the version number out. */
        QString shortAppId = stripVersion(appId);

        /* We create an empty file whenever we succesfully process a hook file.
         * The name of this file is the same as the hook file, with the
         * .processed suffix appended.
         */
        QFileInfo processedInfo(fileInfo.filePath() + ".processed");
        if (processedInfo.exists() &&
            processedInfo.lastModified() == fileInfo.lastModified()) {
            continue;
        }

        ManifestFile manifest = ManifestFile(fileInfo, appId, shortAppId);
        if (!manifest.isValid()) {
            qWarning() << "Invalid file" << fileInfo.filePath();
            continue;
        }

        if (manifest.writeFiles(accountsDir)) {
            QFile file(processedInfo.filePath());
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                file.close();
                struct utimbuf sourceTime;
                sourceTime.actime = sourceTime.modtime =
                    fileInfo.lastModified().toTime_t();
                utime(processedInfo.filePath().toUtf8().constData(),
                      &sourceTime);
            } else {
                qWarning() << "Could not create timestamp file" <<
                    processedInfo.filePath();
            }
        }
    }

    /* To ensure that all the installed services are parsed into
     * libaccounts' DB, we enumerate them now.
     */
    manager->serviceList();
    delete manager;

    return EXIT_SUCCESS;
}

