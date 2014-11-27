/*
 * Copyright (C) 2013 Canonical Ltd.
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

#ifndef OAU_ACCESS_MODEL_H
#define OAU_ACCESS_MODEL_H

#include <QSortFilterProxyModel>

namespace OnlineAccountsUi {

class AccessModelPrivate;
class AccessModel: public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *accountModel READ accountModel \
               WRITE setAccountModel NOTIFY accountModelChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QString applicationId READ applicationId \
               WRITE setApplicationId NOTIFY applicationIdChanged)

public:
    explicit AccessModel(QObject *parent = 0);
    ~AccessModel();

    void setAccountModel(QAbstractItemModel *accountModel);
    QAbstractItemModel *accountModel() const;

    void setApplicationId(const QString &applicationId);
    QString applicationId() const;

    Q_INVOKABLE QVariant get(int row, const QString &roleName) const;

protected:
    bool filterAcceptsRow(int source_row,
                          const QModelIndex &source_parent) const Q_DECL_OVERRIDE;

Q_SIGNALS:
    void accountModelChanged();
    void countChanged();
    void applicationIdChanged();

private:
    AccessModelPrivate *d_ptr;
    Q_DECLARE_PRIVATE(AccessModel)
};

} // namespace

#endif // OAU_ACCESS_MODEL_H
