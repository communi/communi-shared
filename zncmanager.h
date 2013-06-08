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

#ifndef ZNCMANAGER_H
#define ZNCMANAGER_H

#include <QObject>
#include <QElapsedTimer>
#include <IrcChannelModel>
#include <IrcMessageFilter>

class IrcNoticeMessage;
class IrcPrivateMessage;

class ZncManager : public QObject, public IrcMessageFilter
{
    Q_OBJECT
    Q_PROPERTY(bool playbackActive READ isPlaybackActive NOTIFY playbackActiveChanged)
    Q_PROPERTY(QString playbackTarget READ playbackTarget NOTIFY playbackTargetChanged)
    Q_PROPERTY(IrcChannelModel* model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QString timeStampFormat READ timeStampFormat WRITE setTimeStampFormat NOTIFY timeStampFormatChanged)

public:
    explicit ZncManager(QObject* parent = 0);
    virtual ~ZncManager();

    bool isPlaybackActive() const;
    QString playbackTarget() const;

    IrcChannelModel* model() const;
    void setModel(IrcChannelModel* model);

    QString timeStampFormat() const;
    void setTimeStampFormat(const QString& format);

    bool messageFilter(IrcMessage* message);

signals:
    void playbackActiveChanged(bool active);
    void modelChanged(IrcChannelModel* model);
    void playbackTargetChanged(const QString& target);
    void timeStampFormatChanged(const QString& format);

protected:
    bool processMessage(IrcPrivateMessage* message);
    bool processNotice(IrcNoticeMessage* message);

private slots:
    void onConnected();
    void onCapabilities(const QStringList& available, QStringList* request);

private:
    mutable struct Private {
        bool playback;
        long timestamp;
        QString target;
        IrcChannel* channel;
        IrcChannelModel* model;
        QString timeStampFormat;
        QElapsedTimer timestamper;
    } d;
};

#endif // ZNCMANAGER_H
