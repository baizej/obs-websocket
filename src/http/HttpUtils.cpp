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

#include "HttpUtils.h"

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QHash>
#include <QtCore/QMultiHash>

#include "../Config.h"

static const QHash<http::Method, QString> methodMap {
	{ http::Method::GET, "GET" },
	{ http::Method::OPTIONS, "OPTIONS" },
	{ http::Method::HEAD, "HEAD" },
	{ http::Method::POST, "POST" },
	{ http::Method::PUT, "PUT" },
	{ http::Method::DELETE, "DELETE" }
};

http::Method methodStringToEnum(const QString& method) {
	return methodMap.key(method, http::Method::UNKNOWN_METHOD);
}

void HttpUtils::handleIfAuthorized(server::connection_ptr con, std::function<std::string(ConnectionProperties&, std::string)> handlerCb)
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

bool matchRoute(const QString& routeSpec, const QString& requestUri)
{
	QString spec = routeSpec.trimmed();
	// TODO regex parsing
	if (spec.endsWith("/")) {
		spec.chop(1);
	}
	return requestUri.startsWith(routeSpec);
}

bool HttpUtils::simpleAsyncRouter(server::connection_ptr connection, QList<http::RouterEntry> routes)
{
	websocketpp::config::asio::request_type httpRequest = connection->get_request();

	QString requestUri = QString::fromStdString(
		connection->get_uri()->get_resource()
	);

	QString methodString = QString::fromStdString(
		httpRequest.get_method()
	);
	http::Method requestMethod = methodStringToEnum(methodString);

	for (http::RouterEntry route : routes) {
		if (
			matchRoute(route.spec, requestUri) &&
			(route.method == http::Method::ANY_METHOD || requestMethod == route.method)
		) {
			connection->defer_http_response();
			QtConcurrent::run([connection, route]() {
				route.routeCallback();

				websocketpp::lib::error_code ec;
				connection->send_http_response(ec);
			});

			return true;
		}
	}

	return false;
}
