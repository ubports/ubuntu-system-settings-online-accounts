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

#include <Accounts/Manager>
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

    bool writeFiles(const QDir &localShareDir);
    bool writeServiceFile(const QDir &localShare, const QString &id,
                          const QString &provider, const QJsonObject &json);
    QDomElement createGroup(QDomDocument &doc, const QString &name);
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
    QJsonArray m_services;
    QString m_appId;
    QString m_shortAppId;
    QString m_trDomain;
    bool m_isValid;
};

ManifestFile::ManifestFile(const QFileInfo &hookFileInfo,
                           const QString &appId,
                           const QString &shortAppId):
    m_hookFileInfo(hookFileInfo),
    m_appId(appId),
    m_shortAppId(shortAppId),
    m_isValid(false)
{
    QFile file(hookFileInfo.filePath());
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject mainObject = doc.object();
        file.close();

        m_services = mainObject.value("services").toArray();
        m_isValid = !m_services.isEmpty();

        m_trDomain = mainObject.value("translations").toString();
    }
}

bool ManifestFile::writeFiles(const QDir &localShareDir)
{
    bool ok = true;
    QDomDocument doc;
    doc.appendChild(doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\""));
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
        if (description.isEmpty()) description = " ";

        QDomElement serviceElem = doc.createElement(QStringLiteral("service"));
        serviceElem.setAttribute(QStringLiteral("id"), id);
        QDomElement elem = doc.createElement(QStringLiteral("description"));
        elem.appendChild(doc.createTextNode(description));
        serviceElem.appendChild(elem);
        servicesElem.appendChild(serviceElem);

        if (!writeServiceFile(localShareDir, id, provider, o)) {
            qWarning() << "Writing service file failed" << id;
            ok = false;
            break;
        }
    }
    root.appendChild(servicesElem);

    if (ok) {
        QString applicationFile =
            QString("accounts/applications/%1.application").arg(m_shortAppId);
        if (!writeXmlFile(doc, localShareDir.filePath(applicationFile))) {
            qWarning() << "Writing application file failed" << applicationFile;
            ok = false;
        }
    }

    return ok;
}

bool ManifestFile::writeServiceFile(const QDir &localShare, const QString &id,
                                    const QString &provider,
                                    const QJsonObject &json)
{
    QDomDocument doc;
    doc.appendChild(doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\""));
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
    elem = doc.createElement(QStringLiteral("name"));
    elem.appendChild(doc.createTextNode(name));
    root.appendChild(elem);

    addProfile(doc);
    addTranslations(doc);

    // template
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

    if (templateElem.hasChildNodes()) {
        root.appendChild(templateElem);
    }

    return writeXmlFile(doc, localShare.filePath(QString("accounts/services/%1.service").arg(id)));
}

QDomElement ManifestFile::createGroup(QDomDocument &doc, const QString &name)
{
    QDomElement group = doc.createElement(QStringLiteral("group"));
    group.setAttribute(QStringLiteral("name"), name);
    return group;
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
    for (QJsonObject::const_iterator i = json.begin(); i != json.end(); i++) {
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
                break;
            }
        default:
            qWarning() << "Unsupported setting type:" << i.value();
        }
        if (value.isEmpty()) continue;
        addSetting(parent, i.key(), value, type);
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
    QString packageDir = findPackageDir(m_appId);
    if (Q_UNLIKELY(packageDir.isEmpty())) return;

    QDomElement root = doc.documentElement();
    QDomElement elem = doc.createElement(QStringLiteral("package-dir"));
    elem.appendChild(doc.createTextNode(packageDir));
    root.appendChild(elem);
}

void ManifestFile::addDesktopFile(QDomDocument &doc)
{
    QDomElement root = doc.documentElement();
    QDomElement elem = doc.createElement(QStringLiteral("desktop-entry"));
    elem.appendChild(doc.createTextNode(m_appId));
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

static QString xmlFileProfile(const QString &fileName)
{
    QString profile;

    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        QDomDocument doc;
        if (doc.setContent(&file)) {
            QDomElement root = doc.documentElement();
            profile = root.firstChildElement("profile").text();
        }
        file.close();
    }

    return profile;
}

static void removeStaleFiles(const QStringList &fileTypes,
                             const QString &localShare,
                             const QDir &hooksDirIn)
{
    /* Walk through all of
     * ~/.local/share/accounts/{providers,services,service-types,applications}/
     * and remove files which are no longer present in hooksDirIn.
     */
    Q_FOREACH(const QString &fileType, fileTypes) {
        QDir dir(QString("%1/accounts/%2s").arg(localShare).arg(fileType));
        dir.setFilter(QDir::Files | QDir::Readable);
        QStringList fileTypeFilter;
        fileTypeFilter << "*." + fileType;
        dir.setNameFilters(fileTypeFilter);

        Q_FOREACH(const QFileInfo &fileInfo, dir.entryInfoList()) {
            QString profile = xmlFileProfile(fileInfo.filePath());

            /* If there is no <profile> element, then this file was not created
             * by our hook; let's ignore it. */
            if (profile.isEmpty()) continue;

            /* Check that the hook file is still there; if it isn't, then it
             * means that the click package was removed, and we must remove our
             * copy as well. */
            QString hookFileName = stripVersion(profile) + "_*.accounts";
            QStringList nameFilters = QStringList() << hookFileName;
            if (!hooksDirIn.entryList(nameFilters).isEmpty()) continue;

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
        QStringLiteral("application");

    // This is ~/.local/share/
    const QString localShare =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QDir hooksDirIn(localShare + "/" HOOK_FILES_SUBDIR);

    /* Make sure the directories exist */
    QDir localShareDir(localShare);
    localShareDir.mkpath("accounts/applications");
    localShareDir.mkpath("accounts/services");

    removeStaleFiles(fileTypes, localShare, hooksDirIn);

    Q_FOREACH(const QFileInfo &fileInfo, hooksDirIn.entryInfoList()) {
        if (fileInfo.suffix() != "accounts") continue;

        // Our click hook sets the base name to the APP_ID
        QString appId = fileInfo.completeBaseName();

        /* When publishing this file for libaccounts, we want to strip
         * the version number out. */
        QString shortAppId = stripVersion(appId);

        /* When building the destination file name, use the file suffix with an
         * "s" appended: remember that libaccounts uses
         * ~/.local/share/accounts/{providers,services,service-types,applications}.
         * */
        QString applicationFile = QString("%1/accounts/applications/%2.application").
            arg(localShare).arg(shortAppId);

        QFileInfo applicationInfo(applicationFile);
        /* If the destination is there and up to date, we have nothing to do */
        if (applicationInfo.exists() &&
            applicationInfo.lastModified() >= fileInfo.lastModified()) {
            continue;
        }

        ManifestFile manifest = ManifestFile(fileInfo, appId, shortAppId);
        if (!manifest.isValid()) {
            qWarning() << "Invalid file" << fileInfo.filePath();
            continue;
        }

        manifest.writeFiles(localShareDir);
    }

    /* To ensure that all the installed services are parsed into
     * libaccounts' DB, we enumerate them now.
     */
    manager->serviceList();
    delete manager;

    return EXIT_SUCCESS;
}

