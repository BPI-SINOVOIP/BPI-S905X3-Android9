#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import httplib
import logging
import os
import urlparse

import arrow
import stripe
import webapp2
from google.appengine.api import users
from webapp2_extras import jinja2 as wa2_jinja2
from webapp2_extras import sessions

import errors


class BaseHandler(webapp2.RequestHandler):
    """BaseHandler for all requests."""

    def initialize(self, request, response):
        """Initializes this request handler."""
        webapp2.RequestHandler.initialize(self, request, response)
        self.session_backend = 'datastore'

    def verify_origin(self):
        """This function will check the request is comming from the same domain."""
        server_host = os.environ.get('ENDPOINTS_SERVICE_NAME')
        request_host = self.request.headers.get('Host')
        request_referer = self.request.headers.get('Referer')
        if request_referer:
            request_referer = urlparse.urlsplit(request_referer)[1]
        else:
            request_referer = request_host
        logging.info('server: %s, request: %s', server_host, request_referer)
        if server_host and request_referer and server_host != request_referer:
            raise errors.Error(httplib.FORBIDDEN)

    def dispatch(self):
        """Dispatch the request.

        This will first check if there's a handler_method defined
        in the matched route, and if not it'll use the method correspondent to
        the request method (get(), post() etc).
        """
        self.session_store = sessions.get_store(request=self.request)
        # Forwards the method for RESTful support.
        self.forward_method()
        # Security headers.
        # https://www.owasp.org/index.php/List_of_useful_HTTP_headers
        self.response.headers['x-content-type-options'] = 'nosniff'
        self.response.headers['x-frame-options'] = 'SAMEORIGIN'
        self.response.headers['x-xss-protection'] = '1; mode=block'
        try:
            webapp2.RequestHandler.dispatch(self)
        finally:
            self.session_store.save_sessions(self.response)
        # Disabled for now because host is appspot.com in production.
        #self.verify_origin()

    @webapp2.cached_property
    def session(self):
        # Returns a session using the default cookie key.
        return self.session_store.get_session()

    def handle_exception(self, exception, debug=False):
        """Render the exception as HTML."""
        logging.exception(exception)

        # Create response dictionary and status defaults.
        tpl = 'error.html'
        status = httplib.INTERNAL_SERVER_ERROR
        resp_dict = {
            'message': 'A server error occurred.',
        }
        url_parts = self.urlsplit()
        redirect_url = '%s?%s' % (url_parts[2], url_parts[4])

        # Use error code if a HTTPException, or generic 500.
        if isinstance(exception, webapp2.HTTPException):
            status = exception.code
            resp_dict['message'] = exception.detail
        elif isinstance(exception, errors.FormValidationError):
            status = exception.code
            resp_dict['message'] = exception.msg
            resp_dict['errors'] = exception.errors
            self.session['form_errors'] = exception.errors
            # Redirect user to current view URL.
            return self.redirect(redirect_url)
        elif isinstance(exception, stripe.StripeError):
            status = exception.http_status
            resp_dict['errors'] = exception.json_body['error']['message']
            self.session['form_errors'] = [
                exception.json_body['error']['message']
            ]
            return self.redirect(redirect_url)
        elif isinstance(exception, (errors.Error, errors.AclError)):
            status = exception.code
            resp_dict['message'] = exception.msg

        resp_dict['status'] = status

        # Render output.
        self.response.status_int = status
        self.response.status_message = httplib.responses[status]
        # Render the exception response into the error template.
        self.response.write(self.jinja2.render_template(tpl, **resp_dict))

    # @Override
    def get(self, *args, **kwargs):
        self.abort(httplib.NOT_IMPLEMENTED)

    # @Override
    def post(self, *args, **kwargs):
        self.abort(httplib.NOT_IMPLEMENTED)

    # @Override
    def put(self, *args, **kwargs):
        self.abort(httplib.NOT_IMPLEMENTED)

    # @Override
    def delete(self, *args, **kwargs):
        self.abort(httplib.NOT_IMPLEMENTED)

    # @Override
    def head(self, *args, **kwargs):
        pass

    def urlsplit(self):
        """Return a tuple of the URL."""
        return urlparse.urlsplit(self.request.url)

    def path(self):
        """Returns the path of the current URL."""
        return self.urlsplit()[2]

    def forward_method(self):
        """Check for a method override param and change in the request."""
        valid = (None, 'get', 'post', 'put', 'delete', 'head', 'options')
        method = self.request.POST.get('__method__')
        if not method:  # Backbone's _method parameter.
            method = self.request.POST.get('_method')
        if method not in valid:
            logging.debug('Invalid method %s requested!', method)
            method = None
        logging.debug('Method being changed from %s to %s by request',
                      self.request.route.handler_method, method)
        self.request.route.handler_method = method

    def render(self, resp, status=httplib.OK):
        """Render the response as HTML."""
        user = users.get_current_user()
        if user:
            url = users.create_logout_url(self.request.uri)
            url_linktext = "Logout"
        else:
            url = users.create_login_url(self.request.uri)
            url_linktext = "Login"

        resp.update({
            # Defaults go here.
            'now': arrow.utcnow(),
            'dest_url': str(self.request.get('dest_url', '')),
            'form_errors': self.session.pop('form_errors', []),
            'user': user,
            'url': url,
            'url_linktext': url_linktext,
        })

        if 'preload' not in resp:
            resp['preload'] = {}

        self.response.status_int = status
        self.response.status_message = httplib.responses[status]
        self.response.write(self.jinja2.render_template(self.template, **resp))

    @webapp2.cached_property
    def jinja2(self):
        """Returns a Jinja2 renderer cached in the app registry."""
        jinja_config = {
            'template_path':
            os.path.join(os.path.dirname(__file__), "../../static"),
            'compiled_path':
            None,
            'force_compiled':
            False,
            'environment_args': {
                'autoescape': True,
                'extensions': [
                    'jinja2.ext.autoescape',
                    'jinja2.ext.with_',
                ],
            },
            'globals':
            None,
            'filters':
            None,
        }
        return wa2_jinja2.Jinja2(app=self.app, config=jinja_config)
