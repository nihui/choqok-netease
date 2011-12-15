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
