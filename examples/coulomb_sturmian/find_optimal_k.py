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

import molsturm
import molsturm.sturmian

l_max = 2
n_max = 5

for atom, mult in [("Be", 1), ("C", 3), ("Ne", 1)]:
    dummy = 0.0

    scfparams = molsturm.ScfParameters()
    scfparams.system = molsturm.MolecularSystem(atom, multiplicity=mult)
    scfparams.basis = molsturm.Basis.construct("sturmian/atomic/cs_reference_pc",
                                               scfparams.system, k_exp=dummy,
                                               n_max=n_max, l_max=l_max)

    scfparams["scf/eigensolver/method"] = "lapack"
    scfparams["guess/eigensolver/method"] = "lapack"

    print("Running for " + atom + " please wait")
    best = molsturm.sturmian.cs.find_kopt(scfparams)

    k = best["input_parameters"]["discretisation"]["k_exp"]
    print("kopt for " + atom + " is   {0:.4g}".format(k) +
          " with energy {0:.10g}".format(best["energy_ground_state"]))

    out = atom + "_" + str(n_max) + "_" + str(l_max) + "_kopt.hdf5"
    print("Dumping kopt solution for " + atom + " at " + out)
    molsturm.dump_hdf5(best, out)
