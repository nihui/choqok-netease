#include "neteasemicroblog.h"

#include "neteaseaccount.h"
#include "neteaseeditaccount.h"
#include "neteasepostwidget.h"

#include <choqok/accountmanager.h>
#include <choqok/application.h>
#include <choqok/choqokbehaviorsettings.h>
#include <choqok/notifymanager.h>
#include <choqok/postwidget.h>

#include <KGenericFactory>
#include <kio/job.h>

#include <QtOAuth/QtOAuth>

static const char apiUrl[] = "http://api.t.163.com";

K_PLUGIN_FACTORY( NeteaseMicroBlogFactory, registerPlugin<NeteaseMicroBlog>(); )
K_EXPORT_PLUGIN( NeteaseMicroBlogFactory( "choqok_netease" ) )

NeteaseMicroBlog::NeteaseMicroBlog( QObject* parent, const QVariantList& args )
: MicroBlog(NeteaseMicroBlogFactory::componentData(), parent)
{
    Q_UNUSED(args)
    setServiceName( "Netease" );
    setServiceHomepageUrl( "http://t.163.com/" );
    setCharLimit( 163 );

    QStringList m_timelineNames;
    m_timelineNames << "home" << "inbox" << "outbox"/* << "favorite"*/ << "public" << "mentions" << "user" << "retweets" << "location";
    setTimelineNames( m_timelineNames );

    m_timelineApiPath[ "home" ] = "/statuses/home_timeline.json";
    m_timelineApiPath[ "inbox" ] = "/direct_messages.json";
    m_timelineApiPath[ "outbox" ] = "/direct_messages/sent.json";
//     m_timelineApiPath[ "favorite" ] = "/favorites.json";
    m_timelineApiPath[ "public" ] = "/statuses/public_timeline.json";
    m_timelineApiPath[ "mentions" ] = "/statuses/mentions.json";
    m_timelineApiPath[ "user" ] = "/statuses/user_timeline.json";
    m_timelineApiPath[ "retweets" ] = "/statuses/retweets_of_me.json";
    m_timelineApiPath[ "location" ] = "/statuses/location_timeline.json";

    m_countOfTimelinesToSave = 0;
    monthes["Jan"] = 1;
    monthes["Feb"] = 2;
    monthes["Mar"] = 3;
    monthes["Apr"] = 4;
    monthes["May"] = 5;
    monthes["Jun"] = 6;
    monthes["Jul"] = 7;
    monthes["Aug"] = 8;
    monthes["Sep"] = 9;
    monthes["Oct"] = 10;
    monthes["Nov"] = 11;
    monthes["Dec"] = 12;

    /// set timeline info
    Choqok::TimelineInfo* info = new Choqok::TimelineInfo;
    info->name = i18nc( "Timeline Name", "Home" );
    info->description = i18nc( "Timeline description", "You and your friends" );
    info->icon = "user-home";
    m_timelineInfo[ "home" ] = info;

    info = new Choqok::TimelineInfo;
    info->name = i18nc( "Timeline Name", "Inbox" );
    info->description = i18nc( "Timeline description", "Your incoming private messages" );
    info->icon = "mail-folder-inbox";
    m_timelineInfo[ "inbox" ] = info;

    info = new Choqok::TimelineInfo;
    info->name = i18nc( "Timeline Name", "Outbox" );
    info->description = i18nc( "Timeline description", "Private messages you have sent" );
    info->icon = "mail-folder-outbox";
    m_timelineInfo[ "outbox" ] = info;

//     info = new Choqok::TimelineInfo;
//     info->name = i18nc( "Timeline Name", "Favorite" );
//     info->description = i18nc( "Timeline description", "Your favorites" );
//     info->icon = "favorites";
//     m_timelineInfo[ "favorite" ] = info;

    info = new Choqok::TimelineInfo;
    info->name = i18nc( "Timeline Name", "Public" );
    info->description = i18nc( "Timeline description", "Public timeline" );
    info->icon = "folder-green";
    m_timelineInfo[ "public" ] = info;

    info = new Choqok::TimelineInfo;
    info->name = i18nc( "Timeline Name", "Mentions" );
    info->description = i18nc( "Timeline description", "Mentions you" );
    info->icon = "edit-redo";
    m_timelineInfo[ "mentions" ] = info;

    info = new Choqok::TimelineInfo;
    info->name = i18nc( "Timeline Name", "User" );
    info->description = i18nc( "Timeline description", "Specified user" );
    info->icon = "start-here-kde";
    m_timelineInfo[ "user" ] = info;

    info = new Choqok::TimelineInfo;
    info->name = i18nc( "Timeline Name", "ReTweets" );
    info->description = i18nc( "Timeline description", "ReTweets of me" );
    info->icon = "folder-red";
    m_timelineInfo[ "retweets" ] = info;

    info = new Choqok::TimelineInfo;
    info->name = i18nc( "Timeline Name", "Location" );
    info->description = i18nc( "Timeline description", "Location" );
    info->icon = "folder-yellow";
    m_timelineInfo[ "location" ] = info;
}

NeteaseMicroBlog::~NeteaseMicroBlog()
{
}

void NeteaseMicroBlog::aboutToUnload()
{
    m_countOfTimelinesToSave = 0;
    const QList<Choqok::Account*> accounts = Choqok::AccountManager::self()->accounts();
    QList<Choqok::Account*>::ConstIterator it = accounts.constBegin();
    QList<Choqok::Account*>::ConstIterator end = accounts.constEnd();
    while ( it != end ) {
        const Choqok::Account* acc = *it;
        if ( acc->microblog() == this) {
//             acc->writeConfig();
            m_countOfTimelinesToSave += acc->timelineNames().count();
        }
        ++it;
    }
    emit saveTimelines();
}

ChoqokEditAccountWidget* NeteaseMicroBlog::createEditAccountWidget( Choqok::Account* account, QWidget* parent )
{
    NeteaseAccount* acc = dynamic_cast<NeteaseAccount*>(account);
    return new NeteaseEditAccountWidget( this, acc, parent );
}

Choqok::UI::PostWidget* NeteaseMicroBlog::createPostWidget( Choqok::Account* account, const Choqok::Post& post, QWidget* parent )
{
    NeteaseAccount* acc = dynamic_cast<NeteaseAccount*>(account);
    return new NeteasePostWidget( acc, post, parent );
}

void NeteaseMicroBlog::createPost( Choqok::Account* theAccount, Choqok::Post* post )
{
    if ( !post || post->content.isEmpty() ) {
        qWarning() << "Creating the new post failed. Text is empty.";
        return;
    }

    NeteaseAccount* acc = dynamic_cast<NeteaseAccount*>(theAccount);
    QOAuth::ParamMap params;
    QByteArray data;
    if ( post->isPrivate ) {
        /// direct message
        KUrl url( apiUrl );
        url.addPath( "/direct_messages/new.json" );

        params.insert( "user", post->replyToUserName.toUtf8() );
        data = "user=";
        data += post->replyToUserName.toUtf8();
        params.insert( "text", QUrl::toPercentEncoding( post->content ) );
        data += "&text=";
        data += QUrl::toPercentEncoding( post->content );
        params.insert( "source", "Choqok" );
        data += "&source=Choqok";

        KIO::StoredTransferJob* job = KIO::storedHttpPost( data, url, KIO::HideProgressInfo );
        QOAuth::Interface* qoauth = acc->qoauthInterface();
        QByteArray hs = qoauth->createParametersString( url.url(), QOAuth::POST, acc->oauthToken(), acc->oauthTokenSecret(),
                                                        QOAuth::HMAC_SHA1, params, QOAuth::ParseForHeaderArguments );
        job->addMetaData( "content-type", "Content-Type: application/x-www-form-urlencoded" );
        job->addMetaData( "customHTTPHeader", "Authorization: " + hs );
        m_createPost[ job ] = post;
        m_jobAccount[ job ] = acc;
        connect( job, SIGNAL(result(KJob*)), this, SLOT(slotCreatePost(KJob*)) );
        job->start();
    }
    else {
        /// status update
        KUrl url( apiUrl );
        url.addPath( "/statuses/update.json" );

        params.insert( "status", QUrl::toPercentEncoding( post->content ) );
        data = "status=";
        data += QUrl::toPercentEncoding( post->content );
        if ( !post->replyToPostId.isEmpty() ) {
            params.insert( "in_reply_to_status_id", post->replyToPostId.toUtf8() );
            data += "&in_reply_to_status_id=";
            data += post->replyToPostId.toUtf8();
        }
        params.insert( "source", "Choqok" );
        data += "&source=Choqok";

        KIO::StoredTransferJob* job = KIO::storedHttpPost( data, url, KIO::HideProgressInfo );
        QOAuth::Interface* qoauth = acc->qoauthInterface();
        QByteArray hs = qoauth->createParametersString( url.url(), QOAuth::POST, acc->oauthToken(), acc->oauthTokenSecret(),
                                                        QOAuth::HMAC_SHA1, params, QOAuth::ParseForHeaderArguments );
        job->addMetaData( "content-type", "Content-Type: application/x-www-form-urlencoded" );
        job->addMetaData( "customHTTPHeader", "Authorization: " + hs );
        m_createPost[ job ] = post;
        m_jobAccount[ job ] = acc;
        connect( job, SIGNAL(result(KJob*)), this, SLOT(slotCreatePost(KJob*)) );
        job->start();
    }
}

void NeteaseMicroBlog::abortCreatePost( Choqok::Account* theAccount, Choqok::Post* post )
{
    if ( m_createPost.isEmpty() )
        return;

    if ( !post ) {
        QHash<KJob*, Choqok::Post*>::Iterator it = m_createPost.begin();
        QHash<KJob*, Choqok::Post*>::Iterator end = m_createPost.end();
        while ( it != end ) {
            KJob* job = it.key();
            if ( m_jobAccount.value( job ) == theAccount )
                job->kill( KJob::EmitResult );
            ++it;
        }
    }

    m_createPost.key( post )->kill( KJob::EmitResult );
}

void NeteaseMicroBlog::fetchPost( Choqok::Account* theAccount, Choqok::Post* post )
{
    if ( !post || post->postId.isEmpty() ) {
        qWarning() << "no id";
        return;
    }

    NeteaseAccount* acc = dynamic_cast<NeteaseAccount*>(theAccount);
    KUrl url( apiUrl );
    url.addPath( QString( "/statuses/show/%1.json" ).arg( post->postId ) );

    QOAuth::ParamMap params;
    KIO::StoredTransferJob* job = KIO::storedGet( url, KIO::Reload, KIO::HideProgressInfo );
    QOAuth::Interface* qoauth = acc->qoauthInterface();
    QByteArray hs = qoauth->createParametersString( url.url(), QOAuth::GET, acc->oauthToken(), acc->oauthTokenSecret(),
                                                    QOAuth::HMAC_SHA1, params, QOAuth::ParseForHeaderArguments );
    job->addMetaData( "customHTTPHeader", "Authorization: " + hs );
    m_fetchPost[ job ] = post;
    m_jobAccount[ job ] = acc;
    connect( job, SIGNAL(result(KJob*)), this, SLOT(slotFetchPost(KJob*)) );
    job->start();
}

void NeteaseMicroBlog::removePost( Choqok::Account* theAccount, Choqok::Post* post )
{
    if ( !post || post->postId.isEmpty() ) {
        qWarning() << "Deleting post failed. ID is empty.";
        return;
    }

    NeteaseAccount* acc = dynamic_cast<NeteaseAccount*>(theAccount);
    if ( post->isPrivate ) {
        /// direct message
        KUrl url( apiUrl );
        url.addPath( QString( "/direct_messages/destroy/%1.json" ).arg( post->postId ) );

        QOAuth::ParamMap params;
        KIO::StoredTransferJob* job = KIO::storedHttpPost( QByteArray(), url, KIO::HideProgressInfo );
        QOAuth::Interface* qoauth = acc->qoauthInterface();
        QByteArray hs = qoauth->createParametersString( url.url(), QOAuth::POST, acc->oauthToken(), acc->oauthTokenSecret(),
                                                        QOAuth::HMAC_SHA1, params, QOAuth::ParseForHeaderArguments );
        job->addMetaData( "customHTTPHeader", "Authorization: " + hs );
        m_removePost[ job ] = post;
        m_jobAccount[ job ] = acc;
        connect( job, SIGNAL(result(KJob*)), this, SLOT(slotRemovePost(KJob*)) );
        job->start();
    }
    else {
        /// status
        KUrl url( apiUrl );
        url.addPath( QString( "/statuses/destroy/%1.json" ).arg( post->postId ) );

        QOAuth::ParamMap params;
        KIO::StoredTransferJob* job = KIO::storedHttpPost( QByteArray(), url, KIO::HideProgressInfo );
        QOAuth::Interface* qoauth = acc->qoauthInterface();
        QByteArray hs = qoauth->createParametersString( url.url(), QOAuth::POST, acc->oauthToken(), acc->oauthTokenSecret(),
                                                        QOAuth::HMAC_SHA1, params, QOAuth::ParseForHeaderArguments );
        job->addMetaData( "customHTTPHeader", "Authorization: " + hs );
        m_removePost[ job ] = post;
        m_jobAccount[ job ] = acc;
        connect( job, SIGNAL(result(KJob*)), this, SLOT(slotRemovePost(KJob*)) );
        job->start();
    }
}

void NeteaseMicroBlog::saveTimeline( Choqok::Account* account, const QString& timelineName,
                                     const QList<Choqok::UI::PostWidget*>& timeline )
{
    QString fileName = Choqok::AccountManager::generatePostBackupFileName(account->alias(), timelineName);
    KConfig postsBackup( "choqok/" + fileName, KConfig::NoGlobals, "data" );

    ///Clear previous data:
    QStringList prevList = postsBackup.groupList();
    int c = prevList.count();
    if ( c > 0 ) {
        for ( int i = 0; i < c; ++i ) {
            postsBackup.deleteGroup( prevList[i] );
        }
    }
    QList<Choqok::UI::PostWidget*>::ConstIterator it = timeline.constBegin();
    QList<Choqok::UI::PostWidget*>::ConstIterator end = timeline.constEnd();
    while ( it != end ) {
        const Choqok::Post *post = &((*it)->currentPost());
        KConfigGroup grp( &postsBackup, post->creationDateTime.toString() );
        grp.writeEntry( "creationDateTime", post->creationDateTime );
        grp.writeEntry( "postId", post->postId.toString() );
        grp.writeEntry( "text", post->content );
        grp.writeEntry( "source", post->source );
        grp.writeEntry( "inReplyToPostId", post->replyToPostId.toString() );
        grp.writeEntry( "inReplyToUserId", post->replyToUserId.toString() );
        grp.writeEntry( "favorited", post->isFavorited );
        grp.writeEntry( "inReplyToUserName", post->replyToUserName );
        grp.writeEntry( "authorId", post->author.userId.toString() );
        grp.writeEntry( "authorUserName", post->author.userName );
        grp.writeEntry( "authorRealName", post->author.realName );
        grp.writeEntry( "authorProfileImageUrl", post->author.profileImageUrl );
        grp.writeEntry( "authorDescription" , post->author.description );
        grp.writeEntry( "isPrivate" , post->isPrivate );
        grp.writeEntry( "authorLocation" , post->author.location );
        grp.writeEntry( "isProtected" , post->author.isProtected );
        grp.writeEntry( "authorUrl" , post->author.homePageUrl );
        grp.writeEntry( "isRead" , post->isRead );
        grp.writeEntry( "repeatedFrom", post->repeatedFromUsername);
        grp.writeEntry( "repeatedPostId", post->repeatedPostId.toString());
        ++it;
    }
    postsBackup.sync();
    if ( Choqok::Application::isShuttingDown() ) {
        --m_countOfTimelinesToSave;
        if ( m_countOfTimelinesToSave < 1 )
            emit readyForUnload();
    }
}

QList<Choqok::Post*> NeteaseMicroBlog::loadTimeline( Choqok::Account* theAccount, const QString& timelineName )
{
    QList<Choqok::Post*> list;

    NeteaseAccount* account = dynamic_cast<NeteaseAccount*>(theAccount);

    QString fileName = Choqok::AccountManager::generatePostBackupFileName(account->alias(), timelineName);
    KConfig postsBackup("choqok/" + fileName, KConfig::NoGlobals, "data");
    QStringList tmpList = postsBackup.groupList();
/// to don't load old archives
    if (tmpList.isEmpty() || !(QDateTime::fromString(tmpList.first()).isValid()))
        return list;
///--------------

    QList<QDateTime> groupList;
    foreach(const QString &str, tmpList)
        groupList.append(QDateTime::fromString(str) );
    qSort(groupList);
    int count = groupList.count();
    if( !count )
        return list;

    Choqok::Post* st = 0;
    for ( int i = 0; i < count; ++i ) {
        st = new Choqok::Post;
        KConfigGroup grp( &postsBackup, groupList[i].toString() );
        st->creationDateTime = grp.readEntry( "creationDateTime", QDateTime::currentDateTime() );
        st->postId = grp.readEntry( "postId", QString() );
        st->content = grp.readEntry( "text", QString() );
        st->source = grp.readEntry( "source", QString() );
        st->replyToPostId = grp.readEntry( "inReplyToPostId", QString() );
        st->replyToUserId = grp.readEntry( "inReplyToUserId", QString() );
        st->isFavorited = grp.readEntry( "favorited", false );
        st->replyToUserName = grp.readEntry( "inReplyToUserName", QString() );
        st->author.userId = grp.readEntry( "authorId", QString() );
        st->author.userName = grp.readEntry( "authorUserName", QString() );
        st->author.realName = grp.readEntry( "authorRealName", QString() );
        st->author.profileImageUrl = grp.readEntry( "authorProfileImageUrl", QString() );
        st->author.description = grp.readEntry( "authorDescription" , QString() );
        st->author.isProtected = grp.readEntry("isProtected", false);
        st->isPrivate = grp.readEntry( "isPrivate" , false );
        st->author.location = grp.readEntry("authorLocation", QString());
        st->author.homePageUrl = grp.readEntry("authorUrl", QString());
        st->link = postUrl( account, st->author.userName, st->postId);
        st->isRead = grp.readEntry("isRead", true);
        st->repeatedFromUsername = grp.readEntry("repeatedFrom", QString());
        st->repeatedPostId = grp.readEntry("repeatedPostId", QString());

        list.append( st );
    }

    if ( st )
        m_timelineLatestId[ account ][ timelineName ] = st->postId;

    return list;
}

Choqok::Account* NeteaseMicroBlog::createNewAccount( const QString& alias )
{
    NeteaseAccount* acc = dynamic_cast<NeteaseAccount*>(Choqok::AccountManager::self()->findAccount( alias ));
    if ( !acc )
        return new NeteaseAccount( this, alias );
    else
        return 0;
}

void NeteaseMicroBlog::updateTimelines( Choqok::Account* theAccount )
{
    NeteaseAccount* acc = dynamic_cast<NeteaseAccount*>(theAccount);
    if ( !acc )
        return;

    int countOfPost = Choqok::BehaviorSettings::countOfPosts();

    foreach ( const QString& timelineName, acc->timelineNames() ) {
        KUrl url( apiUrl );
        url.addPath( m_timelineApiPath[ timelineName ] );
        QString latestStatusId = m_timelineLatestId[ acc ][ timelineName ];

        QOAuth::ParamMap params;
        if ( !latestStatusId.isEmpty() ) {
            params.insert ( "since_id", latestStatusId.toUtf8() );
            countOfPost = 200;
        }
        params.insert( "count", QByteArray::number( countOfPost ) );

        KIO::StoredTransferJob* job = KIO::storedGet( url, KIO::Reload, KIO::HideProgressInfo );
        QOAuth::Interface* qoauth = acc->qoauthInterface();
        QByteArray hs = qoauth->createParametersString( url.url(), QOAuth::GET, acc->oauthToken(), acc->oauthTokenSecret(),
                                                        QOAuth::HMAC_SHA1, params, QOAuth::ParseForHeaderArguments );
        job->addMetaData( "customHTTPHeader", "Authorization: " + hs );
        m_jobTimeline[job] = timelineName;
        m_jobAccount[job] = acc;
        connect( job, SIGNAL(result(KJob*)), this, SLOT(slotRequestTimeline(KJob*)) );
        job->start();
    }
}

Choqok::TimelineInfo* NeteaseMicroBlog::timelineInfo( const QString& timelineName )
{
    if ( isValidTimeline( timelineName ) )
        return m_timelineInfo.value( timelineName );
    else
        return 0;
}

QString NeteaseMicroBlog::postUrl( Choqok::Account* account, const QString& username, const QString& postId ) const
{
    Q_UNUSED(account)
    return QString( "http://t.163.com/%1/status/%2" ).arg( username ).arg( postId );
}

QString NeteaseMicroBlog::profileUrl( Choqok::Account* account, const QString& username ) const
{
    Q_UNUSED(account)
    return QString( "http://t.163.com/%1" ).arg( username );
}

void NeteaseMicroBlog::retweetPost( Choqok::Account* theAccount, Choqok::Post* post )
{
    if ( !post || post->postId.isEmpty() ) {
        qWarning() << "Retweeting post failed. ID is empty.";
        return;
    }

    NeteaseAccount* acc = dynamic_cast<NeteaseAccount*>(theAccount);

    KUrl url( apiUrl );
    url.addPath( QString( "/statuses/retweet/%1.json" ).arg( post->postId ) );

    QOAuth::ParamMap params;
    params.insert( "id", post->postId.toUtf8() );
    params.insert( "source", "Choqok" );
    KIO::StoredTransferJob* job = KIO::storedHttpPost( QByteArray(), url, KIO::HideProgressInfo );
    QOAuth::Interface* qoauth = acc->qoauthInterface();
    QByteArray hs = qoauth->createParametersString( url.url(), QOAuth::POST, acc->oauthToken(), acc->oauthTokenSecret(),
                                                    QOAuth::HMAC_SHA1, params, QOAuth::ParseForHeaderArguments );
    job->addMetaData( "customHTTPHeader", "Authorization: " + hs );
    m_createPost[ job ] = post;
    m_jobAccount[ job ] = acc;
    connect( job, SIGNAL(result(KJob*)), this, SLOT(slotCreatePost(KJob*)) );
    job->start();
}

void NeteaseMicroBlog::createFavorite( Choqok::Account* theAccount, Choqok::Post* post )
{
    if ( !post || post->postId.isEmpty() ) {
        qWarning() << "Creating favorite failed. ID is empty.";
        return;
    }

    NeteaseAccount* acc = dynamic_cast<NeteaseAccount*>(theAccount);

    KUrl url( apiUrl );
    url.addPath( QString( "/favorites/create/%1.json" ).arg( post->postId ) );

    QOAuth::ParamMap params;
    params.insert( "id", post->postId.toUtf8() );
    KIO::StoredTransferJob* job = KIO::storedHttpPost( QByteArray(), url, KIO::HideProgressInfo );
    QOAuth::Interface* qoauth = acc->qoauthInterface();
    QByteArray hs = qoauth->createParametersString( url.url(), QOAuth::POST, acc->oauthToken(), acc->oauthTokenSecret(),
                                                    QOAuth::HMAC_SHA1, params, QOAuth::ParseForHeaderArguments );
    job->addMetaData( "customHTTPHeader", "Authorization: " + hs );
    m_createFavorite[ job ] = post;
    m_jobAccount[ job ] = acc;
    connect( job, SIGNAL(result(KJob*)), this, SLOT(slotCreateFavorite(KJob*)) );
    job->start();
}

void NeteaseMicroBlog::removeFavorite( Choqok::Account* theAccount, Choqok::Post* post )
{
    if ( !post || post->postId.isEmpty() ) {
        qWarning() << "Creating favorite failed. ID is empty.";
        return;
    }

    NeteaseAccount* acc = dynamic_cast<NeteaseAccount*>(theAccount);

    KUrl url( apiUrl );
    url.addPath( QString( "/favorites/destroy/%1.json" ).arg( post->postId ) );

    QOAuth::ParamMap params;
    params.insert( "id", post->postId.toUtf8() );
    KIO::StoredTransferJob* job = KIO::storedHttpPost( QByteArray(), url, KIO::HideProgressInfo );
    QOAuth::Interface* qoauth = acc->qoauthInterface();
    QByteArray hs = qoauth->createParametersString( url.url(), QOAuth::POST, acc->oauthToken(), acc->oauthTokenSecret(),
                                                    QOAuth::HMAC_SHA1, params, QOAuth::ParseForHeaderArguments );
    job->addMetaData( "customHTTPHeader", "Authorization: " + hs );
    m_removeFavorite[ job ] = post;
    m_jobAccount[ job ] = acc;
    connect( job, SIGNAL(result(KJob*)), this, SLOT(slotRemoveFavorite(KJob*)) );
    job->start();
}

void NeteaseMicroBlog::createFriendship( Choqok::Account* theAccount, Choqok::User* user )
{
    if ( !user || ( user->userId.isEmpty() && user->userName.isEmpty() ) ) {
        qWarning() << "Creating friendship failed. ID or username is empty.";
        return;
    }

    NeteaseAccount* acc = dynamic_cast<NeteaseAccount*>(theAccount);

    KUrl url( apiUrl );
    url.addPath( "/friendships/create.json" );

    QOAuth::ParamMap params;
    QByteArray data;
    if ( user->userId.isEmpty() ) {
        params.insert( "screen_name", user->userName.toUtf8() );
        data = "screen_name=";
        data += user->userName.toUtf8();
    }
    else {
        params.insert( "user_id", user->userId.toUtf8() );
        data = "user_id=";
        data += user->userId.toUtf8();
    }

    KIO::StoredTransferJob* job = KIO::storedHttpPost( data, url, KIO::HideProgressInfo );
    QOAuth::Interface* qoauth = acc->qoauthInterface();
    QByteArray hs = qoauth->createParametersString( url.url(), QOAuth::POST, acc->oauthToken(), acc->oauthTokenSecret(),
                                                    QOAuth::HMAC_SHA1, params, QOAuth::ParseForHeaderArguments );
    job->addMetaData( "content-type", "Content-Type: application/x-www-form-urlencoded" );
    job->addMetaData( "customHTTPHeader", "Authorization: " + hs );
    m_createFriendship[ job ] = user;
    m_jobAccount[ job ] = acc;
    connect( job, SIGNAL(result(KJob*)), this, SLOT(slotCreateFriendship(KJob*)) );
    job->start();
}

void NeteaseMicroBlog::removeFriendship( Choqok::Account* theAccount, Choqok::User* user )
{
    if ( !user || ( user->userId.isEmpty() && user->userName.isEmpty() ) ) {
        qWarning() << "Removing friendship failed. ID or username is empty.";
        return;
    }

    NeteaseAccount* acc = dynamic_cast<NeteaseAccount*>(theAccount);

    KUrl url( apiUrl );
    url.addPath( "/friendships/destroy.json" );

    QOAuth::ParamMap params;
    QByteArray data;
    if ( user->userId.isEmpty() ) {
        params.insert( "screen_name", user->userName.toUtf8() );
        data = "screen_name=";
        data += user->userName.toUtf8();
    }
    else {
        params.insert( "user_id", user->userId.toUtf8() );
        data = "user_id=";
        data += user->userId.toUtf8();
    }

    KIO::StoredTransferJob* job = KIO::storedHttpPost( data, url, KIO::HideProgressInfo );
    QOAuth::Interface* qoauth = acc->qoauthInterface();
    QByteArray hs = qoauth->createParametersString( url.url(), QOAuth::POST, acc->oauthToken(), acc->oauthTokenSecret(),
                                                    QOAuth::HMAC_SHA1, params, QOAuth::ParseForHeaderArguments );
    job->addMetaData( "content-type", "Content-Type: application/x-www-form-urlencoded" );
    job->addMetaData( "customHTTPHeader", "Authorization: " + hs );
    m_removeFriendship[ job ] = user;
    m_jobAccount[ job ] = acc;
    connect( job, SIGNAL(result(KJob*)), this, SLOT(slotRemoveFriendship(KJob*)) );
    job->start();
}

void NeteaseMicroBlog::slotCreatePost( KJob* job )
{
    if ( job->error() ) {
        qWarning() << "Job Error: " << job->errorString();
        return;
    }

    NeteaseAccount* acc = m_jobAccount.take( job );
    Choqok::Post* post = m_createPost.take( job );

    KIO::StoredTransferJob* j = static_cast<KIO::StoredTransferJob*>(job);

    if ( post->isPrivate ) {
        /// direct message
        Choqok::NotifyManager::success( i18n( "Private message sent successfully" ) );
    }
    else {
        /// status update
        bool ok;
        QVariantMap varmap = parser.parse( j->data(), &ok ).toMap();
        if ( !ok ) {
            qWarning() << "JSON parsing error.";
            emit errorPost( acc, post, Choqok::MicroBlog::ParsingError,
                            i18n( "Could not parse the data that has been received from the server." ) );
            return;
        }
        readPostFromJsonMap( varmap, post );

        Choqok::NotifyManager::success( i18n( "New post submitted successfully" ) );
    }

    emit postCreated( acc, post );
}

void NeteaseMicroBlog::slotFetchPost( KJob* job )
{
    if ( job->error() ) {
        qWarning() << "Job Error: " << job->errorString();
        return;
    }

    NeteaseAccount* acc = m_jobAccount.take( job );
    Choqok::Post* post = m_fetchPost.take( job );

    KIO::StoredTransferJob* j = static_cast<KIO::StoredTransferJob*>(job);
    bool ok;
    QVariantMap varmap = parser.parse( j->data(), &ok ).toMap();
    if ( !ok ) {
        qWarning() << "JSON parsing error.";
        emit errorPost( acc, post, Choqok::MicroBlog::ParsingError,
                        i18n( "Could not parse the data that has been received from the server." ) );
        return;
    }
    readPostFromJsonMap( varmap, post );

    emit postFetched( acc, post );
}

void NeteaseMicroBlog::slotRemovePost( KJob* job )
{
    if ( job->error() ) {
        qWarning() << "Job Error: " << job->errorString();
        return;
    }

    Choqok::Post* post = m_removePost.take( job );
    NeteaseAccount* acc = m_jobAccount.take( job );

    KIO::StoredTransferJob* j = static_cast<KIO::StoredTransferJob*>(job);
    qWarning() << QString::fromUtf8( j->data() );

    emit postRemoved( acc, post );
}

void NeteaseMicroBlog::slotRequestTimeline( KJob* job )
{
    if ( job->error() ) {
        qWarning() << "Job Error: " << job->errorString();
        return;
    }

    NeteaseAccount* acc = m_jobAccount.take( job );
    QString timelineName = m_jobTimeline.take( job );
    if ( !isValidTimeline( timelineName ) )
        return;

    KIO::StoredTransferJob* j = static_cast<KIO::StoredTransferJob*>(job);

    QList<Choqok::Post*> postlist;

    bool ok;
    QVariantList list = parser.parse( j->data(), &ok ).toList();
    if ( !ok ) {
        qWarning() << "JSON parsing failed.";
        return;
    }

    QVariantList::ConstIterator it = list.constBegin();
    QVariantList::ConstIterator end = list.constEnd();

    if ( timelineName == "inbox" || timelineName == "outbox" ) {
        while ( it != end ) {
            QVariantMap varmap = it->toMap();
            Choqok::Post* post = new Choqok::Post;

            readDMessageFromJsonMap( acc, varmap, post );
            postlist.prepend( post );
            ++it;
        }
    }
    else {
        while ( it != end ) {
            QVariantMap varmap = it->toMap();
            Choqok::Post* post = new Choqok::Post;

            readPostFromJsonMap( varmap, post );
            postlist.prepend( post );
            ++it;
        }
    }

    if ( !postlist.isEmpty() )
        m_timelineLatestId[ acc ][ timelineName ] = postlist.last()->postId;
    emit timelineDataReceived( acc, timelineName, postlist );
}

void NeteaseMicroBlog::slotCreateFavorite( KJob* job )
{
    if ( job->error() ) {
        qWarning() << "Job Error: " << job->errorString();
        return;
    }

    NeteaseAccount* acc = m_jobAccount.take( job );
    Choqok::Post* post = m_createFavorite.take( job );

    KIO::StoredTransferJob* j = static_cast<KIO::StoredTransferJob*>(job);
    qWarning() << QString::fromUtf8( j->data() );

    emit favoriteCreated( acc, post );
}

void NeteaseMicroBlog::slotRemoveFavorite( KJob* job )
{
    if ( job->error() ) {
        qWarning() << "Job Error: " << job->errorString();
        return;
    }

    NeteaseAccount* acc = m_jobAccount.take( job );
    Choqok::Post* post = m_removeFavorite.take( job );

    KIO::StoredTransferJob* j = static_cast<KIO::StoredTransferJob*>(job);
    qWarning() << QString::fromUtf8( j->data() );

    emit favoriteRemoved( acc, post );
}

void NeteaseMicroBlog::slotCreateFriendship( KJob* job )
{
    if ( job->error() ) {
        qWarning() << "Job Error: " << job->errorString();
        return;
    }

    NeteaseAccount* acc = m_jobAccount.take( job );
    Choqok::User* user = m_createFriendship.take( job );

    KIO::StoredTransferJob* j = static_cast<KIO::StoredTransferJob*>(job);

    bool ok;
    QVariantMap varmap = parser.parse( j->data(), &ok ).toMap();
    if ( !ok ) {
        qWarning() << "JSON parsing error in slotCreateFriendship.";
        return;
    }
    readUserFromJsonMap( varmap, user );

    emit friendshipCreated( acc, user );
}

void NeteaseMicroBlog::slotRemoveFriendship( KJob* job )
{
    if ( job->error() ) {
        qWarning() << "Job Error: " << job->errorString();
        return;
    }

    NeteaseAccount* acc = m_jobAccount.take( job );
    Choqok::User* user = m_removeFriendship.take( job );

    KIO::StoredTransferJob* j = static_cast<KIO::StoredTransferJob*>(job);

    bool ok;
    QVariantMap varmap = parser.parse( j->data(), &ok ).toMap();
    if ( !ok ) {
        qWarning() << "JSON parsing error in slotRemoveFriendship.";
        return;
    }
    readUserFromJsonMap( varmap, user );

    emit friendshipRemoved( acc, user );
}

QDateTime NeteaseMicroBlog::dateFromString( const QString &date )
{
    char s[10];
    int year, day, hours, minutes, seconds;
    sscanf( qPrintable ( date ), "%*s %s %d %d:%d:%d %*s %d", s, &day, &hours, &minutes, &seconds, &year );
    int month = monthes[s];
    QDateTime recognized ( QDate ( year, month, day ), QTime ( hours, minutes, seconds ) );
    recognized.setTimeSpec( Qt::UTC );
    return recognized.toLocalTime();
}

void NeteaseMicroBlog::readPostFromJsonMap( const QVariantMap& varmap, Choqok::Post* post )
{
    post->isPrivate = false;

    post->postId = varmap["id"].toString();
    post->source = varmap["source"].toString();
    QVariantMap usermap = varmap["user"].toMap();
    post->author.realName = usermap["name"].toString();
    post->author.location = usermap["location"].toString();
    post->author.userId = usermap["id"].toString();
    post->author.description = usermap["description"].toString();
    post->author.userName = usermap["screen_name"].toString();
    post->author.followersCount = usermap["followers_count"].toInt();
    post->author.profileImageUrl = usermap["profile_image_url"].toString();
    post->author.homePageUrl = usermap["url"].toString();
    post->content = varmap["text"].toString();
    post->creationDateTime = dateFromString( varmap["created_at"].toString() );
    post->replyToPostId = varmap["in_reply_to_status_id"].toString();
    post->replyToUserId = varmap["in_reply_to_user_id"].toString();
    post->replyToUserName = varmap["in_reply_to_user_name"].toString();
    post->isFavorited = varmap["favorited"].toBool();
}

void NeteaseMicroBlog::readDMessageFromJsonMap( Choqok::Account* account, const QVariantMap& varmap, Choqok::Post* post )
{
    post->isPrivate = true;
    QString senderId = varmap["sender_id"].toString();
    QString senderScreenName = varmap["sender_screen_name"].toString();

    post->postId = varmap["id"].toString();
    QVariantMap sendermap = varmap["sender"].toMap();
    QVariantMap recipientmap = varmap["recipient"].toMap();
    if ( senderScreenName.compare( account->username(), Qt::CaseInsensitive ) == 0 ) {
        post->author.realName = recipientmap["name"].toString();
        post->author.location = recipientmap["location"].toString();
        post->author.userId = recipientmap["id"].toString();
        post->author.description = recipientmap["description"].toString();
        post->author.userName = recipientmap["screen_name"].toString();
        post->author.followersCount = recipientmap["followers_count"].toInt();
        post->author.profileImageUrl = recipientmap["profile_image_url"].toString();
        post->author.homePageUrl = recipientmap["url"].toString();
        post->isRead = true;
    }
    else {
        post->author.realName = sendermap["name"].toString();
        post->author.location = sendermap["location"].toString();
        post->author.userId = sendermap["id"].toString();
        post->author.description = sendermap["description"].toString();
        post->author.userName = sendermap["screen_name"].toString();
        post->author.followersCount = sendermap["followers_count"].toInt();
        post->author.profileImageUrl = sendermap["profile_image_url"].toString();
        post->author.homePageUrl = sendermap["url"].toString();
    }
    post->content = varmap["text"].toString();
    post->creationDateTime = dateFromString( varmap["created_at"].toString() );
    post->replyToUserId = varmap["recipient_id"].toString();
    post->replyToUserName = varmap["recipient_screen_name"].toString();
}

void NeteaseMicroBlog::readUserFromJsonMap( const QVariantMap& varmap, Choqok::User* user )
{
    user->realName = varmap["name"].toString();
    user->location = varmap["location"].toString();
    user->userId = varmap["id"].toString();
    user->description = varmap["description"].toString();
    user->userName = varmap["screen_name"].toString();
    user->followersCount = varmap["followers_count"].toInt();
    user->profileImageUrl = varmap["profile_image_url"].toString();
    user->homePageUrl = varmap["url"].toString();
}
