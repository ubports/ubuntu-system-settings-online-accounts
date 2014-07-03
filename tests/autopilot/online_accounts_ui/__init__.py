# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
#
# Copyright (C) 2013, 2014 Canonical Ltd.
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

import logging

import autopilot.logging

import ubuntuuitoolkit


logger = logging.getLogger(__name__)


class OnlineAccountsUIException(ubuntuuitoolkit.ToolkitException):

    """Exception raised when there is a problem with Online Accounts UI."""


class OnlineAccountsUI():

    """Autopilot helper for the Online Accounts UI."""

    def __init__(self, application_proxy):
        super().__init__()
        self.application_proxy = application_proxy
        self.main_view = self.application_proxy.select_single(MainView)


class MainView(ubuntuuitoolkit.MainView):

    """Autopilot helper for the Main View."""

    @property
    def no_accounts_page(self):
        page = self.select_single(NoAccountsPage)
        try:
            page.visible.wait_for(True)
            return page
        except AssertionError:
            raise OnlineAccountsUIException(
                'The No Accounts page is not visible.')

    @property
    def accounts_page(self):
        page = self.select_single(AccountsPage)
        try:
            page.visible.wait_for(True)
            return page
        except AssertionError:
            raise OnlineAccountsUIException('The Accounts page is not visible')

    @autopilot.logging.log_action(logger.info)
    def add_test_login_account(self, user_name, password):
        account_creation_page = self.no_accounts_page.go_to_add_account(
            'TestLogin')
        account_creation_page.create_new_account(user_name, password)
        return self.accounts_page


class NoAccountsPage(ubuntuuitoolkit.QQuickFlickable):

    """Autopilot helper for the No Accounts Page."""

    def get_providers(self):
        """Return the list of account providers."""
        provider_plugin_list = self._get_provider_plugin_list()
        provider_items = provider_plugin_list.select_many(
            ubuntuuitoolkit.listitems.Standard)

        # sort by y
        provider_items = sorted(
            provider_items,
            key=lambda item: (item.globalRect.y, item.globalRect.x))

        result = []
        for item in provider_items:
            result.append(item.text)
        return result

    def _get_provider_plugin_list(self):
        return self.select_single(ProviderPluginList)

    @autopilot.logging.log_action(logger.info)
    def go_to_add_account(self, provider_name):
        """Go to the Add Acount page for a provider.

        :param provider_name: The name of the provider.

        """
        self._get_provider_plugin_list().click_provider(provider_name)
        account_creation_page = self.get_root_instance().select_single(
            AccountCreationPage)
        account_creation_page.visible.wait_for(True)
        return account_creation_page


class ProviderPluginList(ubuntuuitoolkit.UbuntuUIToolkitCustomProxyObjectBase):

    """Autopilot helper for the Provider Plugin list."""

    @autopilot.logging.log_action(logger.debug)
    def click_provider(self, name):
        """Click a provider item.

        :param name: The name of the provider to click.

        """
        provider_item = self.select_single(
            ubuntuuitoolkit.listitems.Standard, text=name)
        provider_item.swipe_into_view()
        self.pointing_device.click_object(provider_item)


class AccountCreationPage(
        ubuntuuitoolkit.UbuntuUIToolkitCustomProxyObjectBase):

    """Autopilot helper for the Account Creation page."""

    @autopilot.logging.log_action(logger.info)
    def create_new_account(self, *args, **kwargs):
        main = self.select_single('Main')
        new_account_form = main.select_single(NewAccount)
        new_account_form.fill(*args, **kwargs)


class NewAccount(ubuntuuitoolkit.UbuntuUIToolkitCustomProxyObjectBase):

    """Autopilot helper for the New Account form."""

    @autopilot.logging.log_action(logger.debug)
    def fill(self, user_name, password):
        """Fill the form.

        :param user_name: The user name of the account.
        :param password: The password of the account.

        """
        username_text_field = self.select_single(
            ubuntuuitoolkit.TextField, objectName='usernameField')
        username_text_field.write(user_name)
        password_text_field = self.select_single(
            ubuntuuitoolkit.TextField, objectName='passwordField')
        password_text_field.write(password)
        self._click_continue()

    @autopilot.logging.log_action(logger.debug)
    def _click_continue(self):
        continue_button = self.select_single(
            'Button', objectName='continueButton')
        self.pointing_device.click_object(continue_button)


class AccountsPage(ubuntuuitoolkit.QQuickFlickable):

    """Autopilot helper for the Accounts Page."""

    def get_accounts(self):
        """Return the list of accounts."""
        account_items = self.select_many(AccountItem)

        # sort by y
        account_items = sorted(
            account_items,
            key=lambda item: (item.globalRect.y, item.globalRect.x))

        result = []
        for item in account_items:
            result.append(item.get_information())
        return result

    @autopilot.logging.log_action(logger.info)
    def delete_account(self, provider_name, user_name):
        account_item = self.select_single(
            AccountItem, text=provider_name, subText=user_name)
        self.pointing_device.click_object(account_item)
        account_edit_page = self.get_root_instance().select_single(
            AccountEditPage)
        account_edit_page.visible.wait_for(True)
        account_edit_page.remove_account()
        account_item.wait_until_destroyed()


class AccountItem(ubuntuuitoolkit.UbuntuUIToolkitCustomProxyObjectBase):

    """Autopilot helper for the Account item."""

    def get_information(self):
        """Return the information of the account.

        :return: A tuple with the provider name and the user name.

        """
        return self.text, self.subText


class AccountEditPage(ubuntuuitoolkit.UbuntuUIToolkitCustomProxyObjectBase):

    """Autopilot helper for the Account Edit page."""

    @autopilot.logging.log_action(logger.debug)
    def remove_account(self):
        remove_account_button = self.select_single('Button')
        self.pointing_device.click_object(remove_account_button)
        self._confirm_removal()

    def _confirm_removal(self):
        removal_dialog = self.get_root_instance().select_single(
            'RemovalConfirmation')
        remove_button = removal_dialog.select_single('Button', text='Remove')
        self.pointing_device.click_object(remove_button)
