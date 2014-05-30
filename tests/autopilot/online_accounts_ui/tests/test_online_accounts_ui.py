#! /usr/bin/env python3
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
import os

from online_accounts_ui.emulators.items import EmulatorBase


# You can find a couple of OAuth 1.0a and 2.0 tests in the repository history
# (bazaar revision 100).
# These tests were removed because the signon-ui window where the webview is
# hosted will not be focused properly, both in Mir
# (https://bugs.launchpad.net/bugs/1231968) and Compiz
# (https://bugs.launchpad.net/bugs/455241)
# This issue will be solved when we move the signon-ui implementation in the
# same process as online-accounts-ui


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

        # On the phone, this fails because of https://bugs.launchpad.net/bugs/1252294
        if model() != 'Desktop':
            return

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
        # This matches the logic in setUp()
        if model() != 'Desktop':
            return
        self.system_settings.terminate()

    def test_title(self):
        """ Checks whether the Online Accounts window title is correct """
        # On the phone, this fails because of https://bugs.launchpad.net/bugs/1252294
        if model() != 'Desktop':
            return

        header = self.window.select_single('Header', visible=True)
        self.assertThat(header, NotEquals(None))
        self.assertThat(header.title, Eventually(Equals('Accounts')))

    def test_available_providers(self):
        """ Checks whether all the expected providers are available """
        # On the phone, this fails because of https://bugs.launchpad.net/bugs/1252294
        if model() != 'Desktop':
            return

        required_providers = [
                'FakeOAuth',
                ]
        for provider in required_providers:
            provider_item = self.app.select_single('Standard', text=provider)
            self.assertThat(provider_item, NotEquals(None))

    def test_create_account_with_form(self):
        """ Test the creation of an account using a username/password form"""
        # On the phone, this fails because of https://bugs.launchpad.net/bugs/1252294
        if model() != 'Desktop':
            return
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
        account_item.wait_until_destroyed()

