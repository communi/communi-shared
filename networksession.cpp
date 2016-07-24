/*
  Copyright (C) 2008-2016 The Communi Project

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "networksession.h"
#include <QNetworkConfigurationManager>

NetworkSession::NetworkSession(QObject* parent) : QObject(parent)
{
    d.session = 0;
    d.enabled = false;
    d.manager = new QNetworkConfigurationManager(this);

    d.config = d.manager->defaultConfiguration();
    connect(d.manager, SIGNAL(onlineStateChanged(bool)), this, SLOT(onOnlineStateChanged(bool)));
    connect(d.manager, SIGNAL(configurationChanged(QNetworkConfiguration)), this, SLOT(onNetworkConfigurationChanged(QNetworkConfiguration)));
}

bool NetworkSession::isOnline() const
{
    return d.manager->isOnline();
}

bool NetworkSession::isEnabled() const
{
    return d.enabled;
}

void NetworkSession::setEnabled(bool enabled)
{
    if (d.enabled != enabled) {
        d.enabled = enabled;
        emit enabledChanged(enabled);
    }
}

bool NetworkSession::open()
{
    if (d.manager->capabilities() & QNetworkConfigurationManager::NetworkSessionRequired) {
        if (!d.session || d.session->configuration() != d.config) {
            delete d.session;
            d.session = new QNetworkSession(d.config, this);
        }
        d.session->open();
    }
    // TODO: return value?
    return true;
}

void NetworkSession::onOnlineStateChanged(bool online)
{
    if (d.enabled)
        emit onlineStateChanged(online);
}

void NetworkSession::onNetworkConfigurationChanged(const QNetworkConfiguration& config)
{
    if (d.enabled && config.state() == QNetworkConfiguration::Active && d.config.state() != QNetworkConfiguration::Active) {
        d.config = config;
        emit connectionChanged();
    }
}
