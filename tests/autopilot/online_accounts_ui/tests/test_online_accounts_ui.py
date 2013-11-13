#! /usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright (C) 2013 Canonical Ltd.
# Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from autopilot.testcase import AutopilotTestCase
from autopilot.input import Mouse, Touch, Pointer
from autopilot.platform import model
from autopilot.matchers import Eventually
from subprocess import Popen
from testtools.matchers import Contains, Equals, NotEquals, GreaterThan
from time import sleep
import BaseHTTPServer, SimpleHTTPServer, SocketServer, ssl, cgi
import threading
import oauth.oauth as oauth
import os

from online_accounts_ui.emulators.items import EmulatorBase

REQUEST_TOKEN_URL = 'http://localhost:5121/oauth1/request_token'
ACCESS_TOKEN_URL = 'http://localhost:5121/oauth1/access_token'
AUTHORIZATION_URL = 'http://localhost:5121/oauth1/authorize'
CALLBACK_URL = 'http://localhost:5121/success.html'
REALM = 'http://photos.example.net/'
VERIFIER = 'verifier'

class MockOAuthDataStore(oauth.OAuthDataStore):
    def __init__(self):
        self.consumer = oauth.OAuthConsumer('C0nsum3rKey', 'C0nsum3rS3cr3t')
        self.request_token = oauth.OAuthToken('requestkey', 'requestsecret')
        self.access_token = oauth.OAuthToken('accesskey', 'accesssecret')
        self.nonce = 'nonce'
        self.verifier = VERIFIER

    def lookup_consumer(self, key):
        if key == self.consumer.key:
            return self.consumer
        return None

    def lookup_token(self, token_type, token):
        token_attrib = getattr(self, '%s_token' % token_type)
        if token == token_attrib.key:
            ## HACK
            token_attrib.set_callback(CALLBACK_URL)
            return token_attrib
        return None

    def lookup_nonce(self, oauth_consumer, oauth_token, nonce):
        if oauth_token and oauth_consumer.key == self.consumer.key and (oauth_token.key == self.request_token.key or oauth_token.key == self.access_token.key) and nonce == self.nonce:
            return self.nonce
        return None

    def fetch_request_token(self, oauth_consumer, oauth_callback):
        if oauth_consumer.key == self.consumer.key:
            if oauth_callback:
                # want to check here if callback is sensible
                # for mock store, we assume it is
                self.request_token.set_callback(oauth_callback)
            return self.request_token
        return None

    def fetch_access_token(self, oauth_consumer, oauth_token, oauth_verifier):
        if oauth_consumer.key == self.consumer.key and oauth_token.key == self.request_token.key and oauth_verifier == self.verifier:
            # want to check here if token is authorized
            # for mock store, we assume it is
            return self.access_token
        return None

    def authorize_request_token(self, oauth_token, user):
        if oauth_token.key == self.request_token.key:
            # authorize the request token in the store
            # for mock store, do nothing
            self.access_token.username = user
            return self.request_token
        return None

class OAuth1Handler(BaseHTTPServer.BaseHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        BaseHTTPServer.BaseHTTPRequestHandler.__init__(self, *args, **kwargs)

    def do_HEAD(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        s.end_headers()

    def do_GET(self):
        print "Got GET to %s. headers: %s" % (self.path, self.headers)
        if self.path.startswith('/oauth1/authorize'):
            oauth_request = oauth.OAuthRequest.from_request(self.command,
                    'http://localhost:%s%s' % (self.server.server_port, self.path),
                    headers=self.headers)
            # get the request token
            self.server.token = self.server.oauth_server.fetch_request_token(oauth_request)

            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.send_header('Content-Encoding', 'utf-8')
            self.end_headers()
            self.wfile.write("""
    <!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
      "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
    <html xmlns="http://www.w3.org/1999/xhtml">
    <head><title>Login here</title></head>
    <body>
    <h3>Login form</h3>
    <form method="POST" action="http://localhost:%(port)s/login.html">
      Username: <input type="text" name="username" size="15" /><br />
      <p><input type="submit" value="Login" /></p>
    </form>
    </body>
    </html>
    """ % { 'port': self.server.server_port })
            self.server.show_login_event.set()

    def do_POST(self):
        if self.path == '/login.html':
            form = cgi.FieldStorage(
                fp=self.rfile, 
                headers=self.headers,
                environ={'REQUEST_METHOD':'POST',
                         'CONTENT_TYPE':self.headers['Content-Type'],
                         })
            # authorize the token
            token = self.server.oauth_server.authorize_token(self.server.token, form['username'].value)
            token.set_verifier(VERIFIER)
            self.send_response(301)
            self.send_header("Location", token.get_callback_url())
            self.end_headers()
            self.server.login_done_event.set()
            return

        # construct the oauth request from the request parameters
        length = int(self.headers.getheader('content-length'))
        postdata = self.rfile.read(length)
        oauth_request = oauth.OAuthRequest.from_request(self.command,
                'http://localhost:%s%s' % (self.server.server_port, self.path),
                headers=self.headers, query_string=postdata)

        if self.path == '/oauth1/request_token':
            # create a request token
            token = self.server.oauth_server.fetch_request_token(oauth_request)
            # send okay response
            self.send_response(200, 'OK')
            self.send_header('Content-Type', 'application/x-www-form-urlencoded')
            self.end_headers()
            # return the token
            self.wfile.write(token.to_string())
        elif self.path == '/oauth1/access_token':
            # create an access token
            token = self.server.oauth_server.fetch_access_token(oauth_request)
            # send okay response
            self.send_response(200, 'OK')
            self.send_header('Content-Type', 'application/x-www-form-urlencoded')
            self.end_headers()
            # return the token
            self.wfile.write('%s&ScreenName=%s' % (token.to_string(), token.username))


class OAuth1LocalServer:
    def __init__(self):
        self.PORT = 5121
        self.handler = OAuth1Handler
        self.httpd = BaseHTTPServer.HTTPServer(('localhost', self.PORT), self.handler)
        self.httpd.oauth_server = oauth.OAuthServer(MockOAuthDataStore())
        self.httpd.oauth_server.add_signature_method(oauth.OAuthSignatureMethod_PLAINTEXT())
        self.httpd.oauth_server.add_signature_method(oauth.OAuthSignatureMethod_HMAC_SHA1())
        self.httpd.show_login_event = threading.Event()
        self.httpd.login_done_event = threading.Event()
        self.httpd_thread = threading.Thread(target=self.httpd.serve_forever)

    def run(self):
        self.httpd_thread.setDaemon(True)
        self.httpd_thread.start()


class Handler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_HEAD(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        s.end_headers()

    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.send_header('Content-Encoding', 'utf-8')
        self.end_headers()
        self.wfile.write("""
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
  "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head><title>Login here</title></head>
<body>
<h3>Login form</h3>
<form method="POST" action="https://localhost:%(port)s/login.html">
  Username: <input type="text" name="username" size="15" /><br />
  Password: <input type="password" name="password" size="15" /><br />
  <p><input type="submit" value="Login" /></p>
</form>
</body>
</html>
""" % { 'port': self.server.server_port })
        self.server.show_login_event.set()

    def do_POST(self):
        form = cgi.FieldStorage(
            fp=self.rfile, 
            headers=self.headers,
            environ={'REQUEST_METHOD':'POST',
                     'CONTENT_TYPE':self.headers['Content-Type'],
                     })
        self.send_response(301)
        self.send_header("Location",
            "https://localhost:%(port)s/success.html#access_token=%(username)s%(password)s&expires_in=3600" % {
                'port': self.server.server_port,
                'username': form['username'].value,
                'password': form['password'].value
            })
        self.end_headers()
        self.server.login_done_event.set()


class LocalServer:
    def __init__(self):
        self.PORT = 5120
        #self.handler = SimpleHTTPServer.SimpleHTTPRequestHandler
        self.handler = Handler
        self.httpd = BaseHTTPServer.HTTPServer(("localhost", self.PORT), self.handler)
        self.httpd.show_login_event = threading.Event()
        self.httpd.login_done_event = threading.Event()
        self.httpd.socket = ssl.wrap_socket (self.httpd.socket, certfile='/etc/ssl/certs/uoa-test-server.pem', server_side=True)
        self.httpd_thread = threading.Thread(target=self.httpd.serve_forever)

    def run(self):
        self.httpd_thread.setDaemon(True)
        self.httpd_thread.start()


class OnlineAccountsUiTests(AutopilotTestCase):
    if model() == 'Desktop':
        scenarios = [
                ('with mouse', dict(input_device_class=Mouse)),
                ]
    else:
        scenarios = [
                ('with touch', dict(input_device_class=Touch)),
                ]

    def setUp(self):
        super(OnlineAccountsUiTests, self).setUp()
        self.pointer = Pointer(self.input_device_class.create())
        # Increase the timeout of online-accounts-ui, to make sure it won't
        # quit before the system settings panel asks it to open.
        self.patch_environment('OAU_DAEMON_TIMEOUT', '120')
        self.app = self.launch_test_application('online-accounts-ui',
                '--desktop_file_hint=/usr/share/applications/online-accounts-ui.desktop',
                app_type='qt',
                emulator_base=EmulatorBase,
                capture_output=True)
        self.system_settings = Popen(['system-settings', 'online-accounts',
            '--desktop_file_hint=/usr/share/applications/ubuntu-system-settings.desktop'])
        sleep(1)
        self.window = self.app.select_single("QQuickView")
        self.assertThat(self.window.visible, Eventually(Equals(True)))

    def tearDown(self):
        super(OnlineAccountsUiTests, self).tearDown()
        self.system_settings.terminate()

    def test_title(self):
        """ Checks whether the Online Accounts window title is correct """
        header = self.window.select_single('Header', visible=True)
        self.assertThat(header, NotEquals(None))
        self.assertThat(header.title, Eventually(Equals('Online Accounts')))

    def test_available_providers(self):
        """ Checks whether all the expected providers are available """
        required_providers = [
                'FakeOAuth',
                ]
        for provider in required_providers:
            provider_item = self.app.select_single('Standard', text=provider)
            self.assertThat(provider_item, NotEquals(None))

    def test_create_account_with_form(self):
        """ Test the creation of an account using a username/password form"""
        page = self.app.select_single('NoAccountsPage')
        self.assertThat(page, NotEquals(None))

        provider_item = self.app.select_single('Standard', text='TestLogin')
        self.assertThat(provider_item, NotEquals(None))

        # Depending on the number of installed providers, it may be that our
        # test provider is not visible; in that case, scroll the page
        self.pointer.move_to_object(page)
        (page_center_x, page_center_y) = self.pointer.position()
        page_bottom = page.globalRect[1] + page.globalRect[3]
        while provider_item.center[1] > page_bottom - 20:
            self.pointer.move(page_center_x, page_center_y)
            self.pointer.press()
            self.pointer.move(page_center_x, page_center_y - provider_item.height * 2)
            # wait some time before releasing, to avoid a flick
            sleep(0.2)
            self.pointer.release()
        self.pointer.move_to_object(provider_item)
        self.pointer.click()

        # Move to the username field
        username_field = self.app.select_single('TextField', objectName='usernameField')
        self.pointer.move_to_object(username_field)
        self.pointer.click()
        self.keyboard.type('pinkuser')
        self.keyboard.press_and_release('Tab')
        self.keyboard.type('lolcat')
        # Submit
        continue_btn = self.app.select_single('Button', objectName='continueButton')
        self.pointer.move_to_object(continue_btn)
        self.pointer.click()

        # The account should be created shortly
        sleep(5)
        account_item = self.app.select_single('AccountItem', text='TestLogin')
        self.assertThat(account_item, NotEquals(None))
        self.assertThat(account_item.subText, Equals('pinkuser'))

        # Delete it
        self.pointer.move_to_object(account_item)
        self.pointer.click()

        sleep(1)
        edit_page = self.app.select_single('AccountEditPage')
        self.assertThat(edit_page, NotEquals(None))
        remove_button = edit_page.select_single('Button')
        self.assertThat(remove_button, NotEquals(None))
        self.pointer.move_to_object(remove_button)
        self.pointer.click()

        sleep(1)
        removal_page = self.app.select_single('RemovalConfirmation')
        self.assertThat(removal_page, NotEquals(None))
        remove_button = removal_page.select_single('Button', text='Remove')
        self.assertThat(remove_button, NotEquals(None))
        self.pointer.move_to_object(remove_button)
        self.pointer.click()

        # Check that the account has been deleted
        sleep(2) # weird: with 1 second only, org.freedesktop.DBus.Error.NoReply is received
        account_item = self.app.select_single('AccountItem', text='TestLogin')
        self.assertThat(account_item, Equals(None))

    def test_create_oauth1_account(self):
        """ Test the creation of an OAuth 1.0 account """
        # On the phone, this fails because of https://bugs.launchpad.net/bugs/1231968
        if model() != 'Desktop':
            return
        server = OAuth1LocalServer()
        server.run()

        page = self.app.select_single('NoAccountsPage')
        self.assertThat(page, NotEquals(None))

        provider_item = self.app.select_single('Standard', text='FakeOAuthOne')
        self.assertThat(provider_item, NotEquals(None))

        # Depending on the number of installed providers, it may be that our
        # test provider is not visible; in that case, scroll the page
        self.pointer.move_to_object(page)
        (page_center_x, page_center_y) = self.pointer.position()
        page_bottom = page.globalRect[1] + page.globalRect[3]
        while provider_item.center[1] > page_bottom - 20:
            self.pointer.move(page_center_x, page_center_y)
            self.pointer.press()
            self.pointer.move(page_center_x, page_center_y - provider_item.height * 2)
            # wait some time before releasing, to avoid a flick
            sleep(0.2)
            self.pointer.release()
        self.pointer.move_to_object(provider_item)
        self.pointer.click()

        # At this point, the signon-ui process should be spawned by D-Bus and
        # try to connect to our local webserver.
        # Here we wait until we know that the webserver has served the login page:
        server.httpd.show_login_event.wait(30)
        self.assertThat(server.httpd.show_login_event.is_set(), Equals(True))
        server.httpd.show_login_event.clear()

        # Give some time to signon-ui to render the page
        sleep(2)
        #self.signon_ui_window = self.signon_ui.select_single("QQuickView")
        #self.assertThat(self.signon_ui_window.visible, Eventually(Equals(True)))

        # Move to the username field
        self.keyboard.press_and_release('Tab')
        self.keyboard.type('funnyguy')
        self.keyboard.press_and_release('Enter')

        # At this point signon-ui should make a post request with the login
        # data; let's wait for it:
        server.httpd.login_done_event.wait(30)
        self.assertThat(server.httpd.login_done_event.is_set(), Equals(True))
        server.httpd.login_done_event.clear()

        if model() == 'Desktop':
            # Close the signon-ui window
            self.keyboard.press_and_release('Enter')

        # The account should be created shortly
        sleep(5)
        account_item = self.app.select_single('AccountItem', text='FakeOAuthOne')
        self.assertThat(account_item, NotEquals(None))
        self.assertThat(account_item.subText, Equals('funnyguy'))

        # Delete it
        self.pointer.move_to_object(account_item)
        self.pointer.click()

        sleep(1)
        edit_page = self.app.select_single('AccountEditPage')
        self.assertThat(edit_page, NotEquals(None))
        remove_button = edit_page.select_single('Button')
        self.assertThat(remove_button, NotEquals(None))
        self.pointer.move_to_object(remove_button)
        self.pointer.click()

        sleep(1)
        removal_page = self.app.select_single('RemovalConfirmation')
        self.assertThat(removal_page, NotEquals(None))
        remove_button = removal_page.select_single('Button', text='Remove')
        self.assertThat(remove_button, NotEquals(None))
        self.pointer.move_to_object(remove_button)
        self.pointer.click()

        # Check that the account has been deleted
        sleep(2)
        account_item = self.app.select_single('AccountItem', text='FakeOAuth')
        self.assertThat(account_item, Equals(None))

    def test_create_oauth2_account(self):
        """ Test the creation of an OAuth 2.0 account """
        # WebKit2 cannot ignore SSL errors, so this test fails on the phone
        if model() != 'Desktop':
            return

        self.server = LocalServer()
        self.server.run()

        page = self.app.select_single('NoAccountsPage')
        self.assertThat(page, NotEquals(None))

        provider_item = self.app.select_single('Standard', text='FakeOAuth')
        self.assertThat(provider_item, NotEquals(None))

        # Depending on the number of installed providers, it may be that our
        # test provider is not visible; in that case, scroll the page
        self.pointer.move_to_object(page)
        (page_center_x, page_center_y) = self.pointer.position()
        page_bottom = page.globalRect[1] + page.globalRect[3]
        while provider_item.center[1] > page_bottom - 20:
            self.pointer.move(page_center_x, page_center_y)
            self.pointer.press()
            self.pointer.move(page_center_x, page_center_y - provider_item.height * 2)
            # wait some time before releasing, to avoid a flick
            sleep(0.2)
            self.pointer.release()
        self.pointer.move_to_object(provider_item)
        self.pointer.click()

        # At this point, the signon-ui process should be spawned by D-Bus and
        # try to connect to our local webserver.
        # Here we wait until we know that the webserver has served the login page:
        self.server.httpd.show_login_event.wait(30)
        self.assertThat(self.server.httpd.show_login_event.is_set(), Equals(True))
        self.server.httpd.show_login_event.clear()

        # Give some time to signon-ui to render the page
        sleep(2)
        #self.signon_ui_window = self.signon_ui.select_single("QQuickView")
        #self.assertThat(self.signon_ui_window.visible, Eventually(Equals(True)))

        # Move to the username field
        self.keyboard.press_and_release('Tab')
        self.keyboard.type('john')
        self.keyboard.press_and_release('Tab')
        self.keyboard.type('loser')
        self.keyboard.press_and_release('Enter')

        # At this point signon-ui should make a post request with the login
        # data; let's wait for it:
        self.server.httpd.login_done_event.wait(30)
        self.assertThat(self.server.httpd.login_done_event.is_set(), Equals(True))
        self.server.httpd.login_done_event.clear()

        if model() == 'Desktop':
            # Close the signon-ui window
            self.keyboard.press_and_release('Enter')

        # The account should be created shortly
        sleep(5)
        account_item = self.app.select_single('AccountItem', text='FakeOAuth')
        self.assertThat(account_item, NotEquals(None))
        self.assertThat(account_item.subText, Equals('john'))

        # Delete it
        self.pointer.move_to_object(account_item)
        self.pointer.click()

        sleep(1)
        edit_page = self.app.select_single('AccountEditPage')
        self.assertThat(edit_page, NotEquals(None))
        remove_button = edit_page.select_single('Button')
        self.assertThat(remove_button, NotEquals(None))
        self.pointer.move_to_object(remove_button)
        self.pointer.click()

        sleep(1)
        removal_page = self.app.select_single('RemovalConfirmation')
        self.assertThat(removal_page, NotEquals(None))
        remove_button = removal_page.select_single('Button', text='Remove')
        self.assertThat(remove_button, NotEquals(None))
        self.pointer.move_to_object(remove_button)
        self.pointer.click()

        # Check that the account has been deleted
        sleep(2)
        account_item = self.app.select_single('AccountItem', text='FakeOAuth')
        self.assertThat(account_item, Equals(None))
