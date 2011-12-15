/*
 *  This file is part of choqok-netease
 *  Copyright (C) 2011 Ni Hui <shuizhuyuanluo@126.com>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of
 *  the License or (at your option) version 3 or any later version
 *  accepted by the membership of KDE e.V. (or its successor approved
 *  by the membership of KDE e.V.), which shall act as a proxy
 *  defined in Section 14 of version 3 of the license.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "neteaseaccount.h"

#include "neteasemicroblog.h"

#include <choqok/passwordmanager.h>
#include <KIO/AccessManager>
#include <QtOAuth/QtOAuth>

static const char neteaseConsumerKey[] = "pKUrJWMNcOC8pvju";
static const char neteaseConsumerSecret[] = "mmv5sfepYXRVnHiTyFe7Gbe9udNF3y0R";

NeteaseAccount::NeteaseAccount( NeteaseMicroBlog* parent, const QString& alias )
: Choqok::Account(parent,alias)
{
    m_oauthToken = configGroup()->readEntry( QString( "%1_OAuthToken" ).arg( alias ), QByteArray() );
    m_oauthTokenSecret = Choqok::PasswordManager::self()->readPassword( QString( "%1_OAuthTokenSecret" ).arg( alias ) ).toUtf8();
    m_timelineNames = configGroup()->readEntry( QString( "%1_Timelines" ).arg( alias ), QStringList() );

    qoauth = new QOAuth::Interface( new KIO::AccessManager( this ), this );
    qoauth->setConsumerKey( NeteaseAccount::oauthConsumerKey() );
    qoauth->setConsumerSecret( NeteaseAccount::oauthConsumerSecret() );
    qoauth->setRequestTimeout( 10000 );
    qoauth->setIgnoreSslErrors( true );
}

NeteaseAccount::~NeteaseAccount()
{
    delete qoauth;
}

void NeteaseAccount::writeConfig()
{
    configGroup()->writeEntry( QString( "%1_OAuthToken" ).arg( alias() ), m_oauthToken );
    Choqok::PasswordManager::self()->writePassword( QString( "%1_OAuthTokenSecret" ).arg( alias() ),
                                                    QString::fromUtf8( m_oauthTokenSecret ) );
    configGroup()->writeEntry( QString( "%1_Timelines" ).arg( alias() ), m_timelineNames );
    Choqok::Account::writeConfig();
}

const QByteArray NeteaseAccount::oauthConsumerKey()
{
    return neteaseConsumerKey;
}

const QByteArray NeteaseAccount::oauthConsumerSecret()
{
    return neteaseConsumerSecret;
}

QOAuth::Interface* NeteaseAccount::qoauthInterface() const
{
    return qoauth;
}

void NeteaseAccount::setOauthToken( const QByteArray& token )
{
    m_oauthToken = token;
}

const QByteArray NeteaseAccount::oauthToken() const
{
    return m_oauthToken;
}

void NeteaseAccount::setOauthTokenSecret( const QByteArray& tokenSecret )
{
    m_oauthTokenSecret = tokenSecret;
}

const QByteArray NeteaseAccount::oauthTokenSecret() const
{
    return m_oauthTokenSecret;
}

QStringList NeteaseAccount::timelineNames() const
{
    return m_timelineNames;
}

void NeteaseAccount::setTimelineNames( const QStringList& list )
{
    m_timelineNames.clear();
    foreach ( const QString& name, list ) {
        if ( microblog()->timelineNames().contains( name ) )
            m_timelineNames << name;
    }
}
