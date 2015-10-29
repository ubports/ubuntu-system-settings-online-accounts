'''signond mock template

This creates the expected methods and properties of the
com.google.code.AccountsSSO.SingleSignOn service.
'''

# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.  See http://www.gnu.org/copyleft/lgpl.html for the full text
# of the license.

__author__ = 'Alberto Mardegan'
__email__ = 'alberto.mardegan@canonical.com'
__copyright__ = '(c) 2015 Canonical Ltd.'
__license__ = 'LGPL 3+'

import dbus
import time

from dbusmock import MOCK_IFACE
import dbusmock

BUS_NAME = 'com.google.code.AccountsSSO.SingleSignOn'
MAIN_OBJ = '/com/google/code/AccountsSSO/SingleSignOn'
AUTH_SERVICE_IFACE = 'com.google.code.AccountsSSO.SingleSignOn.AuthService'
MAIN_IFACE = AUTH_SERVICE_IFACE
IDENTITY_IFACE = 'com.google.code.AccountsSSO.SingleSignOn.Identity'
AUTH_SESSION_IFACE = 'com.google.code.AccountsSSO.SingleSignOn.AuthSession'
SYSTEM_BUS = False

ERROR_PREFIX = 'com.google.code.AccountsSSO.SingleSignOn.Error.'
ERROR_IDENTITY_NOT_FOUND = ERROR_PREFIX + 'IdentityNotFound'
ERROR_PERMISSION_DENIED = ERROR_PREFIX + 'PermissionDenied'
ERROR_USER_INTERACTION= ERROR_PREFIX + 'UserInteraction'


def identity_store(self, new_info):
    self.info.update(new_info)
    return self.info['Id']

def get_identity(self, identity):
    if identity not in self.identities:
        raise dbus.exceptions.DBusException('Identity not found',
                                            name=ERROR_IDENTITY_NOT_FOUND)
    path = '/Identity%s' % identity
    if not path in dbusmock.get_objects():
        self.AddObject(path, IDENTITY_IFACE, {}, [
            ('getInfo', '', 'a{sv}', 'ret = self.parent.identities[%s]' % identity),
            ('store', 'a{sv}', 'u', 'ret = self.store(self, args[0])'),
        ])
    identity_obj = dbusmock.get_object(path)
    identity_obj.parent = self
    identity_obj.store = identity_store
    identity_obj.info = self.identities[identity]
    return (path, self.identities[identity])


def auth_session_process(identity, params, method):
    if 'errorName' in params:
        raise dbus.exceptions.DBusException('Authentication error',
                                            name=params['errorName'])
    if 'delay' in params:
        time.sleep(params['delay'])
    return params

def get_auth_session_object_path(self, identity, method):
    if identity != 0 and (identity not in self.identities):
        raise dbus.exceptions.DBusException('Identity not found',
                                            name=ERROR_IDENTITY_NOT_FOUND)
    path = '/AuthSession%s' % self.sessions_counter
    self.sessions_counter += 1
    self.AddObject(path, AUTH_SESSION_IFACE, {}, [
        ('process', 'a{sv}s', 'a{sv}', 'ret = self.auth_session_process(self.identity, args[0], args[1])'),
    ])

    auth_session = dbusmock.get_object(path)
    auth_session.auth_session_process = auth_session_process
    auth_session.identity = identity
    auth_session.method = method

    return path


def load(mock, parameters):
    mock.get_identity = get_identity
    mock.get_auth_session_object_path = get_auth_session_object_path
    mock.AddMethods(AUTH_SERVICE_IFACE, [
        ('getIdentity', 'u', 'oa{sv}', 'ret = self.get_identity(self, args[0])'),
        ('getAuthSessionObjectPath', 'us', 's', 'ret = self.get_auth_session_object_path(self, args[0], args[1])'),
    ])

    mock.sessions_counter = 1
    mock.identities = {}
    mock.auth_sessions = {}
    mock.auth_replies = {}

@dbus.service.method(MOCK_IFACE, in_signature='ua{sv}', out_signature='')
def AddIdentity(self, identity, data):
    self.identities[identity] = data

