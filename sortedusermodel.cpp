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

#include "sortedusermodel.h"
#include <IrcSessionInfo>
#include <IrcBufferModel>
#include <IrcUserModel>
#include <IrcSession>
#include <IrcBuffer>

SortedUserModel::SortedUserModel(QObject* parent) : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setSourceModel(new IrcUserModel(this));
    setBuffer(qobject_cast<IrcBuffer*>(parent));
}

IrcBuffer* SortedUserModel::buffer() const
{
    return static_cast<IrcUserModel*>(sourceModel())->buffer();
}

void SortedUserModel::setBuffer(IrcBuffer* buffer)
{
    static_cast<IrcUserModel*>(sourceModel())->setBuffer(buffer);

    if (buffer) {
        m_prefixes = IrcSessionInfo(buffer->session()).prefixes();
        sort(0, Qt::AscendingOrder);
    }
}

bool SortedUserModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    const QString p1 = left.data(Irc::PrefixRole).toString();
    const QString p2 = right.data(Irc::PrefixRole).toString();

    const int i1 = !p1.isEmpty() ? m_prefixes.indexOf(p1.at(0)) : -1;
    const int i2 = !p2.isEmpty() ? m_prefixes.indexOf(p2.at(0)) : -1;

    if (i1 >= 0 && i2 < 0)
        return true;
    if (i1 < 0 && i2 >= 0)
        return false;
    if (i1 >= 0 && i2 >= 0 && i1 != i2)
        return i1 < i2;

    const QString n1 = left.data(Irc::NameRole).toString();
    const QString n2 = right.data(Irc::NameRole).toString();
    return n1.compare(n2, Qt::CaseInsensitive) < 0;
}
