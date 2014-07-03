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

import fixtures
import subprocess
import time

import ubuntuuitoolkit
from autopilot import platform
from autopilot.matchers import Eventually
from testtools.matchers import Equals

import online_accounts_ui


class BaseOnlineAccountsUITestCase(
        ubuntuuitoolkit.base.UbuntuUIToolkitAppTestCase):

    def setUp(self):
        super(BaseOnlineAccountsUITestCase, self).setUp()

        if platform.model() != 'Desktop':
            self.skipTest('Skip because of bug http://pad.lv/1252294')

        application_proxy = self.launch_application()
        self.application = online_accounts_ui.OnlineAccountsUI(
            application_proxy)
        self.assertThat(
            self.application.main_view.visible, Eventually(Equals(True)))

    def launch_application(self):
        # Increase the timeout of online-accounts-ui, to make sure it won't
        # quit before the system settings panel asks it to open.
        self.useFixture(
            fixtures.EnvironmentVariable('OAU_DAEMON_TIMEOUT', '120'))
        application = self.launch_test_application(
            'online-accounts-ui',
            '--desktop_file_hint='
            '/usr/share/applications/online-accounts-ui.desktop',
            app_type='qt',
            emulator_base=ubuntuuitoolkit.UbuntuUIToolkitCustomProxyObjectBase,
            capture_output=True)
        system_settings = subprocess.Popen(
            ['system-settings', 'online-accounts',
             '--desktop_file_hint='
             '/usr/share/applications/ubuntu-system-settings.desktop'])
        self.addCleanup(system_settings.terminate)
        time.sleep(1)
        return application
