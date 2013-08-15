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

#include "sessionmanager.h"
#include <QCoreApplication>
#include <IrcBufferModel>
#include <IrcConnection>
#include <IrcNetwork>
#include <IrcCommand>
#include <IrcBuffer>

SessionManager::SessionManager(QObject* parent) : QObject(parent)
{
    d.connection = 0;
    d.enabled = true;
    setReconnectDelay(15);

    d.model = new IrcBufferModel(this);
    setSession(qobject_cast<IrcConnection*>(parent));

    connect(&d.reconnectTimer, SIGNAL(timeout()), this, SLOT(reconnect()));
}

IrcConnection* SessionManager::connection() const
{
    return d.connection;
}

void SessionManager::setSession(IrcConnection* connection)
{
    if (d.connection != connection) {
        if (d.connection) {
            disconnect(d.connection, SIGNAL(connected()), this, SLOT(onConnected()));
            disconnect(d.connection, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
            disconnect(d.connection, SIGNAL(socketError(QAbstractSocket::SocketError)), this, SLOT(onDisconnected()));
            disconnect(d.connection, SIGNAL(password(QString*)), this, SLOT(onPassword(QString*)));
            disconnect(d.connection, SIGNAL(nickNameReserved(QString*)), this, SLOT(onNickNameReserved(QString*)));
            disconnect(d.connection, SIGNAL(capabilities(QStringList, QStringList*)), this, SLOT(onCapabilities(QStringList, QStringList*)));
        }
        if (connection) {
            connect(connection, SIGNAL(connected()), this, SLOT(onConnected()));
            connect(connection, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
            connect(connection, SIGNAL(socketError(QAbstractSocket::SocketError)), this, SLOT(onDisconnected()));
            connect(connection, SIGNAL(password(QString*)), this, SLOT(onPassword(QString*)));
            connect(connection, SIGNAL(nickNameReserved(QString*)), this, SLOT(onNickNameReserved(QString*)));
            connect(connection, SIGNAL(capabilities(QStringList, QStringList*)), this, SLOT(onCapabilities(QStringList, QStringList*)));
        }
        d.connection = connection;
        d.model->setConnection(connection);
        IrcBuffer* buffer = d.model->add(QString());
        connect(d.model, SIGNAL(messageIgnored(IrcMessage*)), buffer, SIGNAL(messageReceived(IrcMessage*)));
        emit sessionChanged(connection);
    }
}

IrcBufferModel* SessionManager::model() const
{
    return d.model;
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
    return d.connection->sendCommand(command);
}

void SessionManager::reconnect()
{
    d.enabled = true;
    if (!d.connection->isActive()) {
        d.reconnectTimer.stop();
        emit reconnectingChanged(false);
        d.connection->open();
    }
}

void SessionManager::quit()
{
    sleep();
    d.enabled = false;
}

void SessionManager::destructLater()
{
    if (d.connection->isConnected()) {
        connect(d.connection, SIGNAL(disconnected()), d.connection, SLOT(deleteLater()));
        connect(d.connection, SIGNAL(socketError(QAbstractSocket::SocketError)), d.connection, SLOT(deleteLater()));
        QTimer::singleShot(1000, d.connection, SLOT(deleteLater()));
    } else {
        d.connection->deleteLater();
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

    if (d.connection->isConnected()) {
        if (d.connection->network()->activeCapabilities().contains("communi"))
            d.connection->sendCommand(IrcCommand::createCtcpRequest("*communi", "TIME"));
        d.connection->sendCommand(IrcCommand::createQuit(message));
    } else {
        d.connection->close();
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
        QString currentNick = d.connection->nickName();
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
