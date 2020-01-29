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

#include <QtConcurrent/QtConcurrent>

#include "HttpUtils.h"

#include "../Config.h"

void handleIfAuthorized(server::connection_ptr con, std::function<std::string(ConnectionProperties&, std::string)> handlerCb)
{
	websocketpp::http::parser::request request = con->get_request();

	ConnectionProperties connProperties;
	if (GetConfig()->AuthRequired) {
		QString authHeaderValue = QString::fromStdString(
			request.get_header("Authorization")
		);

		if (GetConfig()->CheckHttpAuth(authHeaderValue)) {
			connProperties.setAuthenticated(true);
		} else {
			con->set_status(websocketpp::http::status_code::unauthorized);
			con->append_header("WWW-Authenticate", "Basic realm=\"obs-websocket\"");
			con->set_body("");
			return;
		}
	}

	// can get overriden by the handler callback
	con->set_status(websocketpp::http::status_code::ok);

	std::string requestBody = request.get_body();
	std::string responseBody = handlerCb(connProperties, requestBody);

	if (!responseBody.empty()) {
		con->set_body(responseBody);
	}
}

void handleRouteAsync(server::connection_ptr con, std::string method, std::string routePrefix, std::function<void()> handlerCb)
{
	auto httpRequest = con->get_request();
	if (httpRequest.get_method() != method) {
		return;
	}

	websocketpp::uri_ptr requestUri = con->get_uri();
	auto uriResource = QString::fromStdString(requestUri->get_resource());
	
	if (!uriResource.startsWith(QString::fromStdString(routePrefix))) {
		return;
	}

	con->defer_http_response();
	QtConcurrent::run([=]() {
		handlerCb();
		websocketpp::lib::error_code ec;
		con->send_http_response(ec);
	});
}
