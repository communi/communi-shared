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

#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QTimer>
#include <QObject>
#include <QStringList>

class IrcSession;
class IrcCommand;
class IrcBufferModel;

class SessionManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(IrcSession* session READ session WRITE setSession NOTIFY sessionChanged)
    Q_PROPERTY(IrcBufferModel* model READ model CONSTANT)
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY displayNameChanged)
    Q_PROPERTY(int reconnectDelay READ reconnectDelay WRITE setReconnectDelay)
    Q_PROPERTY(bool reconnecting READ isReconnecting NOTIFY reconnectingChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword)

public:
    explicit SessionManager(QObject* parent = 0);

    IrcSession* session() const;
    void setSession(IrcSession* session);

    IrcBufferModel* model() const;

    QString displayName() const;
    void setDisplayName(const QString& name);

    int reconnectDelay() const;
    void setReconnectDelay(int delay);

    QString password() const;
    void setPassword(const QString& password);

    bool enabled() const;
    void setEnabled(bool enabled);

    bool isReconnecting() const;

    Q_INVOKABLE bool sendCommand(IrcCommand* command);

public slots:
    void quit();
    void reconnect();
    void destructLater();
    void stopReconnecting();
    void sleep();
    void wake();

signals:
    void sessionChanged(IrcSession* session);
    void displayNameChanged(const QString& name);
    void reconnectingChanged(bool reconnecting);
    bool enabledChanged(bool enabled);

private slots:
    void onConnected();
    void onDisconnected();
    void onPassword(QString* password);
    void onNickNameReserved(QString* alternate);
    void onCapabilities(const QStringList& available, QStringList* request);

private:
    struct Private {
        bool enabled;
        QString password;
        QString displayName;
        IrcSession* session;
        IrcBufferModel* model;
        QTimer reconnectTimer;
        QStringList alternateNicks;
    } d;
};

#endif // SESSIONMANAGER_H
