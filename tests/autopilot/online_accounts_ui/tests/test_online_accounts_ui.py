#! /usr/bin/env python3
# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
#
# Copyright (C) 2013, 2014 Canonical Ltd.
# Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
#
# This file is part of ubuntu-system-settings-online-accounts.
#
# ubuntu-system-settings-online-accounts is free software: you can
# redistribute it and/or modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation; version 3.
#
# ubuntu-system-settings-online-accounts is distributed in the hope that it
# will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from online_accounts_ui import tests


# You can find a couple of OAuth 1.0a and 2.0 tests in the repository history
# (bazaar revision 100).
# These tests were removed because the signon-ui window where the webview is
# hosted will not be focused properly, both in Mir
# (https://bugs.launchpad.net/bugs/1231968) and Compiz
# (https://bugs.launchpad.net/bugs/455241)
# This issue will be solved when we move the signon-ui implementation in the
# same process as online-accounts-ui

class MainViewTestCase(tests.BaseOnlineAccountsUITestCase):

    def test_title(self):
        """Checks whether the Online Accounts window title is correct."""
        # TODO turn this into a QML test. --elopio - 2014-07-03
        header = self.application.main_view.get_header()
        self.assertTrue(header.visible)
        self.assertEquals(header.title, 'Accounts')


class AvailableProvidersTestCase(tests.BaseOnlineAccountsUITestCase):

    required_providers = ['FakeOAuth', 'TestLogin']
    scenarios = [(provider, {'provider': provider})
                 for provider in required_providers]

    def test_available_providers(self):
        """Checks whether all the expected providers are available."""
        no_accounts_page = self.application.main_view.no_accounts_page
        available_providers = no_accounts_page.get_providers()
        self.assertIn(self.provider, available_providers)


class CreateAccountTestCase(tests.BaseOnlineAccountsUITestCase):

    def test_create_account_with_form(self):
        """Test the creation of an account using a username/password form."""
        accounts_page = self.application.main_view.add_test_login_account(
            user_name='pinkuser', password='lolcat')

        self.assertIn(('TestLogin', 'pinkuser'), accounts_page.get_accounts())

        accounts_page.delete_account('TestLogin', 'pinkuser')
