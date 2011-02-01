#include "neteaseeditaccount.h"
#include "neteaseaccount.h"
#include "neteasemicroblog.h"

#include <choqok/accountmanager.h>
#include <choqok/choqoktools.h>

#include <KDebug>
#include <KMessageBox>
#include <KInputDialog>
#include <kio/netaccess.h>
#include <kio/accessmanager.h>

#include <QtOAuth/QtOAuth>

NeteaseEditAccountWidget::NeteaseEditAccountWidget( NeteaseMicroBlog* microblog, NeteaseAccount* account, QWidget* parent )
: ChoqokEditAccountWidget(account,parent)
{
    setupUi( this );
    connect( kcfg_authorize, SIGNAL(clicked(bool)),
             this, SLOT(authorizeUser()) );
    if ( account ) {
        if ( account->oauthToken().isEmpty() || account->oauthTokenSecret().isEmpty() )
            setAuthenticated( false );
        else {
            setAuthenticated( true );
            token = account->oauthToken();
            tokenSecret = account->oauthTokenSecret();
            username = account->username();
        }
        kcfg_alias->setText( account->alias() );
    }
    else {
        setAuthenticated( false );
        QString newAccountAlias = microblog->serviceName();
        /// find a default unique alias name for new account
        QString serviceName = newAccountAlias;
        int counter = 1;
        while ( Choqok::AccountManager::self()->findAccount( newAccountAlias ) ) {
            newAccountAlias = QString( "%1%2" ).arg( serviceName ).arg( counter );
            counter++;
        }
        setAccount( new NeteaseAccount( microblog, newAccountAlias ) );
        kcfg_alias->setText( newAccountAlias );
    }
    kcfg_alias->setFocus( Qt::OtherFocusReason );

}

NeteaseEditAccountWidget::~NeteaseEditAccountWidget()
{
}

bool NeteaseEditAccountWidget::validateData()
{
    return ( !kcfg_alias->text().isEmpty() && isAuthenticated );
}

Choqok::Account* NeteaseEditAccountWidget::apply()
{
    NeteaseAccount* acc = static_cast<NeteaseAccount*>(account());
    acc->setAlias( kcfg_alias->text() );
    acc->setUsername( username );
    acc->setOauthToken( token );
    acc->setOauthTokenSecret( tokenSecret );
//     saveTimelinesTableState();
    acc->writeConfig();
    return acc;
}

void NeteaseEditAccountWidget::authorizeUser()
{
    NeteaseAccount* acc = static_cast<NeteaseAccount*>(account());
    QOAuth::Interface* qoauth = acc->qoauthInterface();
    QOAuth::ParamMap reply = qoauth->requestToken( "http://api.t.163.com/oauth/request_token",
                                                   QOAuth::GET, QOAuth::HMAC_SHA1 );
    if ( qoauth->error() == QOAuth::NoError ) {
        token = reply.value( QOAuth::tokenParameterName() );
        tokenSecret = reply.value( QOAuth::tokenSecretParameterName() );
        qWarning() << token;
        qWarning() << tokenSecret;
        QUrl url( "http://api.t.163.com/oauth/authorize" );
        url.addQueryItem( "oauth_token", token );
        Choqok::openUrl( url );
        getPinCode();
    }
    else {
        qWarning() << "ERROR: " << qoauth->error();
        KMessageBox::detailedError( this, i18n( "Authorization Error" ), Choqok::qoauthErrorText( qoauth->error() ) );
    }
}

void NeteaseEditAccountWidget::getPinCode()
{
    NeteaseAccount* acc = static_cast<NeteaseAccount*>(account());
    QOAuth::Interface* qoauth = acc->qoauthInterface();
    isAuthenticated = false;
    while ( !isAuthenticated ) {
        QString verifier = KInputDialog::getText( i18n( "PIN number" ),
                                                  i18n( "Enter PIN number received from Netease:" ) );
        if ( verifier.isEmpty() )
            return;

        QOAuth::ParamMap otherArgs;
        otherArgs.insert( "oauth_verifier", verifier.toUtf8() );
        QOAuth::ParamMap reply = qoauth->accessToken( "http://api.t.163.com/oauth/access_token",
                                                      QOAuth::POST, token, tokenSecret, QOAuth::HMAC_SHA1, otherArgs );

        if ( qoauth->error() == QOAuth::NoError ) {
            username = QString::fromUtf8( reply.value( "screen_name" ) );
            token = reply.value( QOAuth::tokenParameterName() );
            tokenSecret = reply.value( QOAuth::tokenSecretParameterName() );
            setAuthenticated( true );
            KMessageBox::information( this, i18n( "Choqok is authorized successfully." ),
                                     i18n( "Authorized" ) );
        }
        else {
            qWarning() << "ERROR: " << qoauth->error();
            KMessageBox::detailedError( this, i18n( "Authorization Error" ), Choqok::qoauthErrorText( qoauth->error() ) );
        }
    }
}

void NeteaseEditAccountWidget::setAuthenticated( bool authenticated )
{
    isAuthenticated = authenticated;
    if ( authenticated ) {
        kcfg_authorize->setIcon( KIcon( "object-unlocked" ) );
        kcfg_authenticateLed->on();
        kcfg_authenticateStatus->setText( i18n( "Authenticated" ) );
    }
    else {
        kcfg_authorize->setIcon( KIcon( "object-locked" ) );
        kcfg_authenticateLed->off();
        kcfg_authenticateStatus->setText( i18n( "Not Authenticated" ) );
    }
}