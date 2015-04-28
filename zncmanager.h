/*
  Copyright (C) 2008-2015 The Communi Project

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

#ifndef ZNCMANAGER_H
#define ZNCMANAGER_H

#include <QObject>
#include <QDateTime>
#include <IrcMessageFilter>

IRC_FORWARD_DECLARE_CLASS(IrcBuffer)
IRC_FORWARD_DECLARE_CLASS(IrcBufferModel)
IRC_FORWARD_DECLARE_CLASS(IrcNoticeMessage)
IRC_FORWARD_DECLARE_CLASS(IrcPrivateMessage)
IRC_FORWARD_DECLARE_CLASS(IrcMessage)

class ZncManager : public QObject, public IrcMessageFilter
{
    Q_OBJECT
    Q_INTERFACES(IrcMessageFilter)
    Q_PROPERTY(IrcBufferModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
    explicit ZncManager(QObject* parent = 0);
    virtual ~ZncManager();

    IrcBufferModel* model() const;
    void setModel(IrcBufferModel* model);

    bool messageFilter(IrcMessage* message);

signals:
    void modelChanged(IrcBufferModel* model);
    void playbackBegin(IrcBuffer* buffer);
    void playbackEnd(IrcBuffer* buffer);

protected:
    bool processMessage(IrcBuffer* buffer, IrcPrivateMessage* message);

private slots:
    void requestPlayback();
    void clearBuffer(IrcBuffer* buffer);

private:
    mutable struct Private {
        IrcBuffer* buffer;
        QDateTime timestamp;
        IrcBufferModel* model;
    } d;
};

#endif // ZNCMANAGER_H
