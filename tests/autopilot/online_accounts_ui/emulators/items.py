#! /usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright (C) 2013 Canonical Ltd.
# Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from autopilot.introspection import CustomEmulatorBase

class EmulatorBase(CustomEmulatorBase):
    """A base class for all emulators within this test suite."""

    @property
    def center(self):
        (x, y, w, h) = self.globalRect
        return (x + w / 2, y + h / 2)
