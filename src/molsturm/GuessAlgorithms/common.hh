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
#include "GuessAlgorithmsKeysBase.hh"
#include <gint/IntegralType.hh>
#include <gscf/FocklikeMatrix_i.hh>
#include <linalgwrap/eigensystem.hh>
#include <linalgwrap/rescue.hh>
#include <molsturm/MolecularSystem.hh>

namespace molsturm {

/** Thrown if obtaining the scf guess failed (mostly because the eigensolver
 * had trouble finding the required eigenvalues)
 */
DefException1(ExcObtainingScfGuessFailed, std::string,
              << "Obtaining the guess for the SCF failed. Reason: " << arg1);

/** Thrown if solver parameters are wrong or inconsistent */
DefException1(ExcInvalidScfGuessParametersEncountered, std::string,
              << "The Scf guess could not be obtained, because the set of "
                 "parameters passed is not valid. Details: "
              << arg1);

/** Replicate a guess solution for a restricted closed operator
 * such that it exists twice on the diagonal.
 * with zeros padded before and after in the coefficients.
 *
 * This will make the guess suitable for unrestricted calculations
 */
template <typename Solution>
Solution replicate_block(Solution solution) {
  typedef typename Solution::evector_type evector_type;

  auto& evecs               = solution.evectors();
  const size_t n_orbs_alpha = evecs.n_vectors();
  const size_t n_bas        = evecs.n_elem();

  linalgwrap::MultiVector<evector_type> guess(2 * n_bas, 2 * n_orbs_alpha);
  for (size_t f = 0; f < n_orbs_alpha; ++f) {
    std::copy(evecs[f].begin(), evecs[f].end(), guess[f].begin());
    std::copy(evecs[f].begin(), evecs[f].end(), guess[f + n_orbs_alpha].begin() + n_bas);
  }
  solution.evectors() = guess;

  auto& evals = solution.evalues();
  evals.resize(2 * n_orbs_alpha);
  std::copy(evals.begin(), evals.begin() + n_orbs_alpha, evals.begin() + n_orbs_alpha);

  return solution;
}

}  // namespace molsturm
