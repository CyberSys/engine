/**
 * @file
 */

#pragma once

#include "HttpStatus.h"
#include "HttpHeader.h"
#include <SDL_stdinc.h>

namespace http {

struct HttpResponse {
	HeaderMap headers;
	HttpStatus status = HttpStatus::Ok;
	// the memory is managed by the server and freed after the response was sent.
	const char *body = nullptr;
	size_t bodySize = 0u;
	// if the route handler sets this to false, the memory is not freed. Can be useful for static content
	// like error pages.
	bool freeBody = true;

	void contentLength(size_t len) {
		bodySize = len;
	}

	void setText(const char *body) {
		this->body = body;
		contentLength(strlen(body));
		freeBody = false;
		if (headers.find(http::header::CONTENT_TYPE) == headers.end()) {
			headers.put(http::header::CONTENT_TYPE, "text/plain");
		}
	}
};

}
