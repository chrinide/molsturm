//
// Copyright (C) 2017 by the molsturm authors
//
// This file is part of molsturm.
//
// molsturm is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// molsturm is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with molsturm. If not, see <http://www.gnu.org/licenses/>.
//

#pragma once
#include <string>
#include <vector>

#ifdef SWIG
// clang-format off

// Setting the molecular structure
%apply (long* IN_ARRAY1, int DIM1) {(long* atom_numbers, int n_atoms_an)};
%apply (double* IN_ARRAY2, int DIM1, int DIM2)
                {(double* coords, int n_atoms_c, int three_c)};

// Setting the nlm vectors for coulomb-sturmians
%apply (long* IN_ARRAY2, int DIM1, int DIM2) {(long* nlm, int n_nlm, int three_n)};

// Setting an external guess
%apply (double* IN_ARRAY1, int DIM1) {(double* orbena_f, int n_fock_ae)};
%apply (double* IN_ARRAY1, int DIM1) {(double* orbenb_f, int n_fock_be)};
%apply (double* IN_ARRAY2, int DIM1, int DIM2)
                {(double* orbcoeffa_bf, int n_bas_ac, int n_fock_ac)};
%apply (double* IN_ARRAY2, int DIM1, int DIM2)
                {(double* orbcoeffb_bf, int n_bas_bc, int n_fock_bc)};
// clang-format on
#else
#include <gint/Structure.hh>
#include <krims/GenMap.hh>
#endif  // SWIG

namespace molsturm {
namespace iface {

/** HfParameters which are available from the python interface.
 *  All parameters prefixed with internal_ are internal and will not
 *  be exposed to the user or available for the user
 */
struct HfParameters {
#ifndef SWIG
  //! The integral parameters to pass to the IntegralLookup object
  krims::GenMap integral_params;

  //! The guess parameters to pass to the guess method
  krims::GenMap guess_params;

  //! The SCF parameters to pass to the SCF solver
  krims::GenMap scf_params;
#endif  // SWIG

//
// System setup
//
/** Molecular system: Structure, charge and multiplicity
 *
 * All quantities in atomic units.
 * */
//@{

#ifndef SWIG
  size_t n_alpha = 0;
  size_t n_beta  = 0;

  gint::Structure structure{};
#endif  // SWIG

  /** Set the molecular system from python */
  void set_molecular_system(long* atom_numbers, int n_atoms_an, double* coords,
                            int n_atoms_c, int three_c, size_t n_alpha, size_t n_beta);
  //@}

  //
  // Setters for the GenMap objects
  //
  /** The type of parameter which is set by the function call of the
   *  set function below */
  enum ParameterKind {
    INTEGRAL,
    GUESS,
    SCF,
  };

  /** Set a parameter in one of the GenMap objects */
  template <typename T>
  void set_param(ParameterKind kind, std::string key, T value);
#ifdef SWIG
  // clang-format off
  %template(set_param_bool)   set_param<bool>;
  %template(set_param_double) set_param<double>;
  %template(set_param_int)    set_param<int>;
  %template(set_param_size_t) set_param<size_t>;
  %template(set_param_string) set_param<std::string>;
// clang-format on
#endif

  //
  // Special parameter setters
  //
  /** Set the nlm basis stucture inside the integral_params */
  void set_integral_param_nlm_basis(long* nlm, int n_nlm, int three_n);

  /** Set the external guess inside the scf_params
   * This only works for rhf calculations.
   *
   * \param orbcoeff_bf  alpha-alpha and beta-beta block of the guess.
   * */
  void set_scf_param_external_guess_rhf(double* orbena_f, int n_fock_ae,
                                        double* orbcoeffa_bf, int n_bas_ac,
                                        int n_fock_ac) {
    set_scf_param_external_guess(orbena_f, n_fock_ae, nullptr, 0, orbcoeffa_bf, n_bas_ac,
                                 n_fock_ac, nullptr, n_bas_ac, 0);
  }

  /** Set the external guess inside the scf_params
   *
   * \param orbcoeffa_bf   alpha-alpha block of the guess
   * \param orbcoeffb_bf   beta-beta block of the guess
   *                       Leave it empty for RHF calculations.
   */
  void set_scf_param_external_guess(double* orbena_f, int n_fock_ae, double* orbenb_f,
                                    int n_fock_be, double* orbcoeffa_bf, int n_bas_ac,
                                    int n_fock_ac, double* orbcoeffb_bf, int n_bas_bc,
                                    int n_fock_bc);

  //
  // TODO
  // Temporary:
  //
  bool export_repulsion_integrals = true;
  bool export_fock_matrix         = true;
  bool export_hcore_matrix        = true;
  bool export_overlap_matrix      = true;
};

}  // namespace iface
}  // namespace molsturm
