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


def run(**extra):
    params = {
        "atom_numbers": [4],
        "coords":       [[0, 0, 0]],
        #
        "basis_type":   "sturmian/atomic/cs_reference_pc",
        "k_exp":        2.1,
        #
        "eigensolver":   "lapack",
        "guess_esolver": "lapack",
    }
    params.update(extra)
    res = molsturm.hartree_fock(**params)

    molsturm.print_convergence_summary(res)
    molsturm.print_energies(res)
    molsturm.print_mo_occupation(res)
    molsturm.print_quote(res)
    return res


def run_matrix_free(**extra):
    return run(n_eigenpairs=8, eigensolver="arpack", guess_esolver="arpack", **extra)


if __name__ == "__main__":
    run_matrix_free(n_max=4, l_max=1)
