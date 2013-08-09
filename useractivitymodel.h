/*
* Copyright (C) 2008-2013 The Communi Project
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the <organization> nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
