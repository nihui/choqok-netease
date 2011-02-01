#ifndef NETEASEPOSTWIDGET_H
#define NETEASEPOSTWIDGET_H

#include <choqok/postwidget.h>

class NeteaseAccount;
class NeteasePostWidget : public Choqok::UI::PostWidget
{
    Q_OBJECT
    public:
        explicit NeteasePostWidget( NeteaseAccount* account, const Choqok::Post& post, QWidget* parent = 0 );
        virtual ~NeteasePostWidget();
        virtual void initUi();
    protected Q_SLOTS:
        virtual void slotResendPost();
    private Q_SLOTS:
        void slotReply();
        void slotWrite();
        void slotReplyAll();
        void slotFavorite();
        void slotFavoriteCreated( Choqok::Account* account, Choqok::Post* post );
        void slotFavoriteRemoved( Choqok::Account* account, Choqok::Post* post );
    private:
        KPushButton* favoriteButton;
};

#endif // NETEASEPOSTWIDGET_H
