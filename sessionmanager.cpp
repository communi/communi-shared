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

#include "sessionmanager.h"
#include <QCoreApplication>
#include <IrcSessionInfo>
#include <IrcSession>
#include <IrcCommand>

SessionManager::SessionManager(QObject* parent) : QObject(parent)
{
    d.session = 0;
    d.enabled = true;
    setReconnectDelay(15);
    setSession(qobject_cast<IrcSession*>(parent));

    connect(&d.reconnectTimer, SIGNAL(timeout()), this, SLOT(reconnect()));
}

IrcSession* SessionManager::session() const
{
    return d.session;
}

void SessionManager::setSession(IrcSession* session)
{
    if (d.session != session) {
        if (d.session) {
            disconnect(d.session, SIGNAL(connected()), this, SLOT(onConnected()));
            disconnect(d.session, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
            disconnect(d.session, SIGNAL(socketError(QAbstractSocket::SocketError)), this, SLOT(onDisconnected()));
            disconnect(d.session, SIGNAL(password(QString*)), this, SLOT(onPassword(QString*)));
            disconnect(d.session, SIGNAL(nickNameReserved(QString*)), this, SLOT(onNickNameReserved(QString*)));
            disconnect(d.session, SIGNAL(capabilities(QStringList, QStringList*)), this, SLOT(onCapabilities(QStringList, QStringList*)));
        }
        if (session) {
            connect(session, SIGNAL(connected()), this, SLOT(onConnected()));
            connect(session, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
            connect(session, SIGNAL(socketError(QAbstractSocket::SocketError)), this, SLOT(onDisconnected()));
            connect(session, SIGNAL(password(QString*)), this, SLOT(onPassword(QString*)));
            connect(session, SIGNAL(nickNameReserved(QString*)), this, SLOT(onNickNameReserved(QString*)));
            connect(session, SIGNAL(capabilities(QStringList, QStringList*)), this, SLOT(onCapabilities(QStringList, QStringList*)));
        }
        d.session = session;
        emit sessionChanged(session);
    }
}

QString SessionManager::displayName() const
{
    return d.displayName;
}

void SessionManager::setDisplayName(const QString& name)
{
    if (d.displayName != name) {
        d.displayName = name;
        emit displayNameChanged(name);
    }
}

int SessionManager::reconnectDelay() const
{
    return d.reconnectTimer.interval() / 1000;
}

void SessionManager::setReconnectDelay(int delay)
{
    d.reconnectTimer.setInterval(qMax(0, delay) * 1000);
}

QString SessionManager::password() const
{
    return d.password;
}

void SessionManager::setPassword(const QString& password)
{
    d.password = password;
}

bool SessionManager::enabled() const
{
    return d.enabled;
}

void SessionManager::setEnabled(bool enabled)
{
    if (!d.enabled != enabled) {
        d.enabled = enabled;
        emit enabledChanged(enabled);
    }
}

bool SessionManager::isReconnecting() const
{
    return d.reconnectTimer.isActive();
}

bool SessionManager::sendCommand(IrcCommand* command)
{
    return d.session->sendCommand(command);
}

void SessionManager::reconnect()
{
    d.enabled = true;
    if (!d.session->isActive()) {
        d.reconnectTimer.stop();
        emit reconnectingChanged(false);
        d.session->open();
    }
}

void SessionManager::quit()
{
    sleep();
    d.enabled = false;
}

void SessionManager::destructLater()
{
    if (d.session->isConnected()) {
        connect(d.session, SIGNAL(disconnected()), d.session, SLOT(deleteLater()));
        connect(d.session, SIGNAL(socketError(QAbstractSocket::SocketError)), d.session, SLOT(deleteLater()));
        QTimer::singleShot(1000, d.session, SLOT(deleteLater()));
    } else {
        d.session->deleteLater();
    }
}

void SessionManager::stopReconnecting()
{
    d.reconnectTimer.stop();
    emit reconnectingChanged(false);
}

void SessionManager::sleep()
{
    QString message = tr("%1 %2").arg(QCoreApplication::applicationName())
                                 .arg(QCoreApplication::applicationVersion());

    if (d.session->isConnected()) {
        IrcSessionInfo info(d.session);
        if (info.activeCapabilities().contains("communi"))
            d.session->sendCommand(IrcCommand::createCtcpRequest("*communi", "TIME"));
        d.session->sendCommand(IrcCommand::createQuit(message));
    } else {
        d.session->close();
    }
}

void SessionManager::wake()
{
    if (d.enabled)
        reconnect();
}

void SessionManager::onConnected()
{
}

void SessionManager::onDisconnected()
{
    if (d.enabled && !d.reconnectTimer.isActive() && d.reconnectTimer.interval() > 0) {
        d.reconnectTimer.start();
        emit reconnectingChanged(true);
    }
}

void SessionManager::onPassword(QString* password)
{
    *password = d.password;
}

void SessionManager::onNickNameReserved(QString* alternate)
{
    if (d.alternateNicks.isEmpty()) {
        QString currentNick = d.session->nickName();
        d.alternateNicks << (currentNick + "_")
                         <<  currentNick
                         << (currentNick + "__")
                         <<  currentNick;
    }
    *alternate = d.alternateNicks.takeFirst();
}

void SessionManager::onCapabilities(const QStringList& available, QStringList* request)
{
    if (available.contains("identify-msg"))
        request->append("identify-msg");
}
