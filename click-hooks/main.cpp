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

class LibAccountsFile: public QDomDocument {
public:
    LibAccountsFile(const QFileInfo &hookFileInfo);

    void checkId(const QString &shortAppId);
    void addProfile(const QString &appId);
    void addPackageDir(const QString &appId);
    QString profile() const;
    void addDesktopFile(const QString &appId);
    void addServiceType(const QString &shortAppId);
    void checkIconPath(const QString &appId);
    bool writeTo(const QString &fileName) const;
    bool isValid() const { return m_isValid; }

private:
    QFileInfo m_hookFileInfo;
    bool m_isValid;
};

LibAccountsFile::LibAccountsFile(const QFileInfo &hookFileInfo):
    QDomDocument(),
    m_hookFileInfo(hookFileInfo),
    m_isValid(false)
{
    QFile file(hookFileInfo.filePath());
    if (file.open(QIODevice::ReadOnly)) {
        if (setContent(&file)) {
            m_isValid = true;
        }
        file.close();
    }
}

void LibAccountsFile::checkId(const QString &shortAppId)
{
    /* checks that the root element's "id" attributes is consistent with the
     * file name */
    QDomElement root = documentElement();
    root.setAttribute(QStringLiteral("id"), shortAppId);
}

void LibAccountsFile::addProfile(const QString &appId)
{
    QDomElement root = documentElement();
    QDomElement elem = createElement(QStringLiteral("profile"));
    elem.appendChild(createTextNode(appId));
    root.appendChild(elem);
}

void LibAccountsFile::addPackageDir(const QString &appId)
{
    QString packageDir = findPackageDir(appId);
    if (Q_UNLIKELY(packageDir.isEmpty())) return;

    QDomElement root = documentElement();
    QDomElement elem = createElement(QStringLiteral("package-dir"));
    elem.appendChild(createTextNode(packageDir));
    root.appendChild(elem);
}

QString LibAccountsFile::profile() const
{
    QDomElement root = documentElement();
    return root.firstChildElement("profile").text();
}

void LibAccountsFile::addDesktopFile(const QString &appId)
{
    QString desktopEntryTag = QStringLiteral("desktop-entry");
    QDomElement root = documentElement();
    /* if a <desktop-entry> element already exists, don't touch it */
    QDomElement elem = root.firstChildElement(desktopEntryTag);
    if (!elem.isNull()) return;

    elem = createElement(desktopEntryTag);
    elem.appendChild(createTextNode(appId));
    root.appendChild(elem);
}

void LibAccountsFile::addServiceType(const QString &shortAppId)
{
    QString serviceTypeTag = QStringLiteral("type");
    QDomElement root = documentElement();
    /* if a <service-type> element already exists, don't touch it */
    QDomElement elem = root.firstChildElement(serviceTypeTag);
    if (!elem.isNull()) return;

    elem = createElement(serviceTypeTag);
    elem.appendChild(createTextNode(shortAppId));
    root.appendChild(elem);
}

void LibAccountsFile::checkIconPath(const QString &appId)
{
    QString iconTag = QStringLiteral("icon");
    QDomElement root = documentElement();
    /* if the <icon> element does not exist, do nothing*/
    QDomElement elem = root.firstChildElement(iconTag);
    if (elem.isNull()) return;

    /* If the icon path is absolute, do nothing */
    QString icon = elem.text();
    if (QDir::isAbsolutePath(icon)) return;

    /* Otherwise, try appending it to the click package install dir */
    QString packageDir = findPackageDir(appId);
    if (Q_UNLIKELY(packageDir.isEmpty())) return;

    QFileInfo iconFile(packageDir + "/" + icon);
    if (iconFile.exists()) {
        while (elem.hasChildNodes()) {
            elem.removeChild(elem.firstChild());
        }
        elem.appendChild(createTextNode(iconFile.canonicalFilePath()));
    }
}

bool LibAccountsFile::writeTo(const QString &fileName) const
{
    /* Make sure that the target directory exists */
    QDir fileAsDirectory(fileName);
    fileAsDirectory.mkpath("..");

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    bool ok = (file.write(toByteArray(2)) > 0);
    file.close();

    if (ok) {
        return true;
    } else {
        QFile::remove(fileName);
        return false;
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
            LibAccountsFile file(fileInfo.filePath());

            QString profile = file.profile();

            /* If there is no <profile> element, then this file was not created
             * by our hook; let's ignore it. */
            if (profile.isEmpty()) continue;

            /* Check that the hook file is still there; if it isn't, then it
             * means that the click package was removed, and we must remove our
             * copy as well. */
            QString hookFileName = profile + "." + fileType;
            if (hooksDirIn.exists(hookFileName)) continue;

            QFile::remove(fileInfo.filePath());
            /* If this is a provider, we must also remove any accounts
             * associated with it */
            if (fileType == QStringLiteral("provider")) {
                removeStaleAccounts(manager, fileInfo.completeBaseName());
            }
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

    /* Go through the hook files in ~/.local/share/online-accounts-hooks/ and
     * check if they have already been processed into a file under
     * ~/.local/share/accounts/{providers,services,service-types,applications}/;
     * if not, open the hook file, write the APP_ID somewhere in it, and
     * save the result in the location where libaccounts expects to find it.
     */

    QStringList fileTypes;
    fileTypes << QStringLiteral("provider") <<
        QStringLiteral("service") <<
        QStringLiteral("service-type") <<
        QStringLiteral("application");

    // This is ~/.local/share/
    const QString localShare =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QDir hooksDirIn(localShare + "/" HOOK_FILES_SUBDIR);

    removeStaleFiles(manager, fileTypes, localShare, hooksDirIn);

    Q_FOREACH(const QFileInfo &fileInfo, hooksDirIn.entryInfoList()) {
        const QString fileType = fileInfo.suffix();
        // Filter out the files which we don't support
        if (!fileTypes.contains(fileType)) continue;

        // Our click hook sets the base name to the APP_ID
        QString appId = fileInfo.completeBaseName();

        /* When publishing this file for libaccounts, we want to strip
         * the version number out. */
        QString shortAppId = stripVersion(appId);

        /* When building the destination file name, use the file suffix with an
         * "s" appended: remember that libaccounts uses
         * ~/.local/share/accounts/{providers,services,service-types,applications}.
         * */
        QString destination = QString("%1/accounts/%2s/%3.%2").
            arg(localShare).arg(fileInfo.suffix()).arg(shortAppId);

        QFileInfo destinationInfo(destination);
        /* If the destination is there and up to date, we have nothing to do */
        if (destinationInfo.exists() &&
            destinationInfo.lastModified() >= fileInfo.lastModified()) {
            continue;
        }

        LibAccountsFile xml = LibAccountsFile(fileInfo);
        if (!xml.isValid()) continue;

        xml.checkId(shortAppId);
        xml.addProfile(appId);
        xml.addPackageDir(appId);
        xml.checkIconPath(appId);
        if (fileType == "application") {
            xml.addDesktopFile(appId);
        } else if (fileType == "service") {
            xml.addServiceType(shortAppId);
        }
        xml.writeTo(destination);
    }

    /* To ensure that all the installed services are parsed into
     * libaccounts' DB, we enumerate them now.
     */
    manager->serviceList();
    delete manager;

    return EXIT_SUCCESS;
}

