/*
* Copyright (C) 2008-2013 The Communi Project
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#ifndef USERACTIVITYMODEL_H
#define USERACTIVITYMODEL_H

#include <QSortFilterProxyModel>

class IrcUser;
class IrcChannel;
class IrcMessage;
class IrcUserModel;

class UserActivityModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(IrcChannel* channel READ channel WRITE setChannel)

public:
    explicit UserActivityModel(QObject* parent = 0);

    IrcChannel* channel() const;
    void setChannel(IrcChannel* channel);

protected:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const;

private slots:
    void onUserRemoved(IrcUser* user);
    void onMessageReceived(IrcMessage* message);

private:
    struct Private {
        qint64 counter;
        IrcUserModel* userModel;
        QHash<const IrcUser*, qint64> users;
    } d;
};

#endif // USERACTIVITYMODEL_H
