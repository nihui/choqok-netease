#ifndef NETEASEEDITACCOUNT_H
#define NETEASEEDITACCOUNT_H

#include <choqok/editaccountwidget.h>
#include "ui_neteaseeditaccount_base.h"

class NeteaseAccount;
class NeteaseMicroBlog;

class NeteaseEditAccountWidget : public ChoqokEditAccountWidget, public Ui::NeteaseEditAccountBase
{
    Q_OBJECT
    public:
        explicit NeteaseEditAccountWidget( NeteaseMicroBlog* microblog, NeteaseAccount* account, QWidget* parent );
        virtual ~NeteaseEditAccountWidget();
        virtual bool validateData();
        virtual Choqok::Account* apply();
    protected Q_SLOTS:
        virtual void authorizeUser();
    private:
        void getPinCode();
        void setAuthenticated( bool authenticated );
        void loadTimelinesTableState();
        void saveTimelinesTableState();
    private:
        QByteArray token;
        QByteArray tokenSecret;
        QString username;
        bool isAuthenticated;
};

#endif // NETEASEEDITACCOUNT_H
