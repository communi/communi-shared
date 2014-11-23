/*
  Copyright (C) 2008-2014 The Communi Project

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

#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include <QObject>
#include <QPointer>
#include <IrcGlobal>

IRC_FORWARD_DECLARE_CLASS(IrcBuffer)
IRC_FORWARD_DECLARE_CLASS(IrcBufferModel)
IRC_FORWARD_DECLARE_CLASS(IrcAwayMessage)
IRC_FORWARD_DECLARE_CLASS(IrcInviteMessage)
IRC_FORWARD_DECLARE_CLASS(IrcNoticeMessage)
IRC_FORWARD_DECLARE_CLASS(IrcNumericMessage)
IRC_FORWARD_DECLARE_CLASS(IrcPongMessage)
IRC_FORWARD_DECLARE_CLASS(IrcMessage)

class MessageHandler : public QObject
{
    Q_OBJECT

public:
    explicit MessageHandler(QObject* parent = 0);
    virtual ~MessageHandler();

    IrcBufferModel* model() const;
    void setModel(IrcBufferModel* model);

    IrcBuffer* defaultBuffer() const;
    IrcBuffer* currentBuffer() const;

public slots:
    void setDefaultBuffer(IrcBuffer* buffer);
    void setCurrentBuffer(IrcBuffer* buffer);

protected slots:
    void handleMessage(IrcMessage* message);

protected:
    void handleAwayMessage(IrcAwayMessage* message);
    void handleInviteMessage(IrcInviteMessage* message);
    void handleNoticeMessage(IrcNoticeMessage* message);
    void handleNumericMessage(IrcNumericMessage* message);
    void handlePongMessage(IrcPongMessage* message);
    void handleUnknownMessage(IrcMessage* message);

private:
    void sendMessage(IrcMessage* message, IrcBuffer* buffer);
    void sendMessage(IrcMessage* message, const QString& buffer);

    struct Private {
        QPointer<IrcBufferModel> model;
        QPointer<IrcBuffer> defaultBuffer;
        QPointer<IrcBuffer> currentBuffer;
    } d;
};

#endif // MESSAGEHANDLER_H
