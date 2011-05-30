#ifndef NETEASEACCOUNT_H
#define NETEASEACCOUNT_H

#include <choqok/account.h>

namespace QOAuth { class Interface; }
class NeteaseMicroBlog;

class NeteaseAccount : public Choqok::Account
{
    Q_OBJECT
    public:
        explicit NeteaseAccount( NeteaseMicroBlog* parent, const QString& alias );
        virtual ~NeteaseAccount();
        virtual void writeConfig();

        static const QByteArray oauthConsumerKey();
        static const QByteArray oauthConsumerSecret();

        QOAuth::Interface* qoauthInterface() const;
        void setOauthToken( const QByteArray& token );
        const QByteArray oauthToken() const;
        void setOauthTokenSecret( const QByteArray& tokenSecret );
        const QByteArray oauthTokenSecret() const;

        QStringList timelineNames() const;
        void setTimelineNames( const QStringList& list );
    private:
        QOAuth::Interface* qoauth;
        QByteArray m_oauthToken;
        QByteArray m_oauthTokenSecret;
        QStringList m_timelineNames;
};

#endif // NETEASEACCOUNT_H
