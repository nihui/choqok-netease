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
