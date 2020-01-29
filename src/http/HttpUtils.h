/*
obs-websocket
Copyright (C) 2016-2017	Stéphane Lepin <stephane.lepin@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#pragma once

#include <functional>
#include <QtCore/QList>

#include "../WSServer.h"

namespace http 
{
    enum Method {
        UNKNOWN_METHOD = 0,
        ANY_METHOD,
        GET,
        OPTIONS,
        HEAD,
        POST,
        PUT,
        DELETE
    };

    typedef std::function<void()> RouteHandler;

    typedef struct {
        Method method;
        QString spec;
        RouteHandler routeCallback;
    } RouterEntry;

    void handleIfAuthorized(server::connection_ptr con, std::function<std::string(ConnectionProperties&, std::string)> handlerCb);
    // void handleRouteAsync(server::connection_ptr con, std::string method, std::string routePrefix, std::function<void()> handlerCb);

    bool simpleAsyncRouter(server::connection_ptr connection, QList<http::RouterEntry> routes);
}