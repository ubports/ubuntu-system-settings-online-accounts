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
from testtools.matchers import Contains, Equals, NotEquals, GreaterThan
from time import sleep
import BaseHTTPServer, SimpleHTTPServer, SocketServer, ssl, cgi
import threading

from online_accounts_ui.emulators.items import EmulatorBase

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
        self.app = self.launch_test_application('system-settings', 'online-accounts',
                emulator_base=EmulatorBase,
                capture_output=True)
        self.window = self.app.select_single("QQuickView")
        self.assertThat(self.window.visible, Eventually(Equals(True)))

    def test_title(self):
        """ Checks whether the Online Accounts window title is correct """
        header = self.window.select_single('Header', visible=True)
        self.assertThat(header, NotEquals(None))
        self.assertThat(header.title, Eventually(Equals('Accounts')))

    def test_available_providers(self):
        """ Checks whether all the expected providers are available """
        required_providers = [
                'FakeOAuth',
                ]
        for provider in required_providers:
            provider_item = self.app.select_single('Standard', text=provider)
            self.assertThat(provider_item, NotEquals(None))

    def test_create_oauth2_account(self):
        """ Test the creation of an OAuth 2.0 account """
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
        while provider_item.center[1] > page_bottom:
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
        sleep(1)
        account_item = self.app.select_single('AccountItem', text='FakeOAuth')
        self.assertThat(account_item, Equals(None))
