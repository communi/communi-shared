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
