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

#include "export_hf_results.hh"
#include <molsturm/FockOperator.hh>
#include <molsturm/IopScf.hh>
#include <molsturm/OverlapMatrix.hh>

namespace molsturm {
namespace iface {

// TODO Refactor this when implementing UHF!

template <typename State>
HfResults export_hf_results(const State& state, const gint::ERITensor_i<scalar_type>& eri,
                            const Parameters& params) {
  const auto& fbb = state.problem_matrix();
  const auto& soln = state.eigensolution();
  const size_t n_orbs_alpha = soln.n_ep();
  const size_t n_orbs = 2 * n_orbs_alpha;
  const size_t n_bas = fbb.n_rows();

  // Copy results back
  HfResults ret;
  ret.n_beta = fbb.indices_orbspace(gscf::OrbitalSpace::OCC_BETA).length();
  ret.n_alpha = fbb.indices_orbspace(gscf::OrbitalSpace::OCC_ALPHA).length();
  ret.n_bas = n_bas;

  ret.restricted = true;
  ret.n_orbs_alpha = n_orbs_alpha;
  ret.n_orbs_beta = n_orbs_alpha;

  ret.threshold = params.error;

  // SCF results
  ret.n_iter = state.n_iter();
  ret.n_mtx_applies = state.n_mtx_applies();
  ret.final_error_norm = state.last_error_norm;
  ret.final_tot_energy_change = state.last_tot_energy_change;
  ret.final_1e_energy_change = state.last_1e_energy_change;

  // Energies:
  for (const auto& kv : fbb.energies()) {
    switch (kv.first.integral_type()) {
      case gint::IntegralType::kinetic:
        ret.energy_kinetic = kv.second;
        break;
      case gint::IntegralType::coulomb:
        ret.energy_coulomb = kv.second;
        break;
      case gint::IntegralType::exchange:
        ret.energy_exchange = kv.second;
        break;
      case gint::IntegralType::nuclear_attraction:
        ret.energy_nuclear_attraction = kv.second;
        break;
      default:
        continue;
    }
  }
  ret.energy_total = fbb.energy_total();
  ret.energy_nuclear_repulsion = fbb.energy_nuclear_repulsion();

  // Insert alpha and beta orbital energies:
  std::copy(soln.evalues().begin(), soln.evalues().end(),
            std::back_inserter(ret.orben_f));
  std::copy(soln.evalues().begin(), soln.evalues().end(),
            std::back_inserter(ret.orben_f));
  assert_size(ret.orben_f.size(), n_orbs);

  if (params.export_fock_matrix) {
    // Compute the full fock matrix in MO space, i.e.  C^T * (F * C)
    // => Need a dot product here, so actually the dot of all vectors with another
    auto fock_ff = linalgwrap::dot(soln.evectors(), fbb * soln.evectors());
    assert_size(2 * fock_ff.n_rows(), n_orbs);
    assert_size(2 * fock_ff.n_cols(), n_orbs);
    ret.fock_ff.resize(n_orbs * n_orbs);

    for (size_t i = 0; i < fock_ff.n_rows(); ++i) {
      for (size_t j = 0; j < fock_ff.n_cols(); ++j) {
        const size_t ij = i * n_orbs + j;
        const size_t ij_beta = ij + n_orbs_alpha * n_orbs + n_orbs_alpha;
        // (i + ret.n_orbs_alpha) * n_orbs + (j + ret.n_orbs_alpha)
        //   = i * n_orbs + j + n_orbs_alpha * n_orbs + n_orbs_alpha
        //   = ij + n_orbs_alpha * n_orbs + n_orbs_alpha
        assert_internal(ij < ret.fock_ff.size());
        assert_internal(ij_beta < ret.fock_ff.size());
        ret.fock_ff[ij] = fock_ff(i, j);
        ret.fock_ff[ij_beta] = fock_ff(i, j);
      }
    }
  } else {
    ret.fock_ff.resize(0);
  }

  if (params.export_hcore_matrix) {
    // Build one electron terms in atomic basis function space
    linalgwrap::LazyMatrixSum<matrix_type> hcore_bb;
    for (const auto& id_term : fbb.terms_1e()) hcore_bb += id_term.second;

    // Compute the full one electron matrix in MO space, i.e.  C^T * (F * C)
    // => Need a dot product here, so actually the dot of all vectors with another
    auto hcore_ff = linalgwrap::dot(soln.evectors(), hcore_bb * soln.evectors());
    assert_size(2 * hcore_ff.n_rows(), n_orbs);
    assert_size(2 * hcore_ff.n_cols(), n_orbs);
    ret.hcore_ff.resize(n_orbs * n_orbs);

    for (size_t i = 0; i < hcore_ff.n_rows(); ++i) {
      for (size_t j = 0; j < hcore_ff.n_cols(); ++j) {
        const size_t ij = i * n_orbs + j;
        const size_t ij_beta = ij + n_orbs_alpha * n_orbs + n_orbs_alpha;
        // (i + ret.n_orbs_alpha) * n_orbs + (j + ret.n_orbs_alpha)
        //   = i * n_orbs + j + n_orbs_alpha * n_orbs + n_orbs_alpha
        //   = ij + n_orbs_alpha * n_orbs + n_orbs_alpha
        assert_internal(ij < ret.hcore_ff.size());
        assert_internal(ij_beta < ret.hcore_ff.size());
        ret.hcore_ff[ij] = hcore_ff(i, j);
        ret.hcore_ff[ij_beta] = hcore_ff(i, j);
      }
    }
  } else {
    ret.hcore_ff.resize(0);
  }

  // Copy coefficients (also alpha and beta block is needed!)
  ret.orbcoeff_fb.resize(n_orbs * n_bas);
  for (size_t f = 0; f < soln.evectors().n_vectors(); ++f) {
    for (size_t b = 0; b < n_bas; ++b) {
      const size_t fb = f * n_bas + b;
      const size_t fb_beta = (f + ret.n_orbs_alpha) * n_bas + b;
      assert_internal(fb < ret.orbcoeff_fb.size());
      assert_internal(fb_beta < ret.orbcoeff_fb.size());
      ret.orbcoeff_fb[fb] = soln.evectors()[f][b];
      ret.orbcoeff_fb[fb_beta] = soln.evectors()[f][b];
    }  // j
  }    // i

  if (params.export_repulsion_integrals) {
    // Form the alpha-alpha-alpha-alpha spin block by contraction
    // and copy it to all the other non-zero places
    // i.e. (aa|aa), (aa|bb), (bb|aa), (bb|bb)
    std::vector<double> eri_aaaa(n_orbs_alpha * n_orbs_alpha * n_orbs_alpha *
                                 n_orbs_alpha);
    eri.contract_with(soln.evectors(), soln.evectors(), soln.evectors(), soln.evectors(),
                      eri_aaaa);

    ret.eri_ffff.resize(n_orbs * n_orbs * n_orbs * n_orbs);
    for (size_t i = 0, ijkl = 0; i < n_orbs_alpha; ++i) {
      for (size_t j = 0; j < n_orbs_alpha; ++j) {
        for (size_t k = 0; k < n_orbs_alpha; ++k) {
          for (size_t l = 0; l < n_orbs_alpha; ++l, ++ijkl) {
            // Index of the element in the alpha-alpha-alpha-alpha
            // block to which we wish to copy our extracted values
            const size_t aaaa = ((i * n_orbs + j) * n_orbs + k) * n_orbs + l;

            // Index offset introduced by shifting the last shell
            // pair from the alpha-alpha to the beta-beta position
            const size_t bb_off = n_orbs * n_orbs_alpha + n_orbs_alpha;

            // The indices of the other elements in the full eri tensor
            // equal to the element at index aaaa by symmetry.
            const size_t aabb = aaaa + bb_off;
            const size_t bbaa = aaaa + n_orbs * n_orbs * bb_off;
            const size_t bbbb = aaaa + bb_off + n_orbs * n_orbs * bb_off;

            assert_internal(aaaa < n_orbs * n_orbs * n_orbs * n_orbs);
            assert_internal(aabb < n_orbs * n_orbs * n_orbs * n_orbs);
            assert_internal(bbaa < n_orbs * n_orbs * n_orbs * n_orbs);
            assert_internal(bbbb < n_orbs * n_orbs * n_orbs * n_orbs);
            assert_internal(ijkl < eri_aaaa.size());

            ret.eri_ffff[aaaa] = eri_aaaa[ijkl];
            ret.eri_ffff[aabb] = eri_aaaa[ijkl];
            ret.eri_ffff[bbaa] = eri_aaaa[ijkl];
            ret.eri_ffff[bbbb] = eri_aaaa[ijkl];
          }  // l
        }    // k
      }      // j
    }        // i
  } else {
    ret.eri_ffff.resize(0);
  }

  return ret;
}

#define INSTANTIATE(STATE_TYPE)                                           \
  template HfResults export_hf_results(                                   \
        const IopScfState<FockOperator<matrix_type, STATE_TYPE>,          \
                          OverlapMatrix<matrix_type, STATE_TYPE>>& state, \
        const gint::ERITensor_i<scalar_type>& eri, const Parameters&)

INSTANTIATE(RestrictionType::Unrestricted);
INSTANTIATE(RestrictionType::RestrictedClosed);

#undef INSTANTIATE

}  // namespace iface
}  // namespace molsturm
