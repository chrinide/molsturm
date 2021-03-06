#!/usr/bin/env python3
## vi: tabstop=4 shiftwidth=4 softtabstop=4 expandtab
## ---------------------------------------------------------------------
##
## Copyright (C) 2017 by the molsturm authors
##
## This file is part of molsturm.
##
## molsturm is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published
## by the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## molsturm is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with molsturm. If not, see <http://www.gnu.org/licenses/>.
##
## ---------------------------------------------------------------------

import dissociation
from matplotlib import pyplot as plt
import molsturm.posthf

z, f = dissociation.compute_curve("h", "def2-sv(p)", n_points=35,
                                  restricted=True, method="hf")
plt.plot(z, f, label="restricted")
z, f = dissociation.compute_curve("h", "def2-sv(p)", n_points=35,
                                  restricted=False, method="hf")
plt.plot(z, f, label="unrestricted")
z, f = dissociation.compute_curve("h", "def2-sv(p)", n_points=35,
                                  restricted=True, method="mp2")
plt.plot(z, f, label="MP2 restricted")
z, f = dissociation.compute_curve("h", "def2-sv(p)", n_points=35,
                                  restricted=False, method="mp2")
plt.plot(z, f, label="MP2 unrestricted")

if "fci" in molsturm.posthf.available_methods:
    z, f = dissociation.compute_curve("h", "def2-sv(p)", n_points=35,
                                      restricted=False, method="fci")
    plt.plot(z, f, label="FCI")

plt.legend()
plt.show()
