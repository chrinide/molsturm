#include <catch.hpp>
#include <gint/IntegralLookup.hh>
#include <linalgwrap/TestingUtils.hh>
#include <molsturm/GuessLibrary.hh>
#include <molsturm/IopScf.hh>
#include <molsturm/RestrictedClosedIntegralOperator.hh>

namespace molsturm {
namespace tests {
using namespace gscf;
using namespace gint;
using namespace linalgwrap;
using namespace krims;

TEST_CASE("HF functionality test", "[hf functionality]") {
  //
  // Parameters of the test problem
  //
  double Z = 4.;  // Be atom
  double k_exp = 1.;
  size_t n_alpha = 2;
  size_t n_beta = 2;
  int n_max = 3;
  int l_max = 2;
  size_t n_eigenpairs = 4;
  const double tolerance = 1e-9;

  //
  // Types to use
  //
  // Types of scalar and matrix
  typedef double scalar_type;
  typedef SmallMatrix<scalar_type> stored_matrix_type;
  typedef stored_matrix_type::vector_type vector_type;

  // Types of the integrals we use:
  const gint::OrbitalType otype = gint::COMPLEX_ATOMIC;

  // The lookup class type to get the actual integrals
  typedef gint::IntegralLookup<stored_matrix_type, otype> int_lookup_type;

  // The type of the integral terms:
  typedef typename int_lookup_type::integral_type integral_type;

  //
  // Reference data
  //
  // The expected eigenvalues
  std::vector<scalar_type> eval_expected{
        -3.7496030113878,    -0.1755827076801148, -0.03942386983346891,
        0.08845420348863607, 0.08845420348863607, 0.2470756635901147,
        0.3644265733015022,  0.3644265733015022,  0.3663266541722434,
        0.3727104156955993,  0.3727104156955993,  0.3761194352770711,
        0.383552426594183,   0.383552426594183};

  // The expected eigenvectors
  MultiVector<vector_type> evec_expected{
        {-1.1884746467844811, 0., -0.24288050447076578, 0., 0., 0.16813161525502301, 0.,
         0., 0., 0., 0., 0.016396171817405908, 0., 0.},
        {-1.064786764522789, 0., 0.8777407505081162, 0., 0., -0.3081669311487548, 0., 0.,
         0., 0., 0., -0.028869768632584114, 0., 0.},
        {0., 0., 0., -3.69049544150639e-9, 0.8573394853277652, 0., 0., 0., 0., 0., 0., 0.,
         0.00002919059875836615, -0.6818863586007807},
        {0., 0.9857660367413854, 0., 0., 0., 0., 0., 0., 0.47777120131625944, 0., 0., 0.,
         0., 0.},
        {0., 0., 0., 0.8573394853277649, 3.69049544151069e-9, 0., 0., 0., 0., 0., 0., 0.,
         -0.6818863586007805, -0.000029190598758366127},
        {-0.5840485708369669, 0., 0.05174625401524502, 0., 0., -1.0729001918355632, 0.,
         0., 0., 0., 0., -0.07137766077631158, 0., 0.},
        {0., 0., 0., 1.1728582480320243e-9, -0.27246685510597846, 0., 0., 0., 0., 0., 0.,
         0., 0.000045420745388700296, -1.0610192320620837},
        {0., -0.033706141181753996, 0., 0., 0., 0., 0., 0., 1.0949264340797673, 0., 0.,
         0., 0., 0.},
        {0., 0., 0., -0.2724668551059781, -1.1728582480253274e-9, 0., 0., 0., 0., 0., 0.,
         0., -1.0610192320620837, -0.00004542074538870031},
        {0., 0., 0., 0., 0., 0., -1., 0., 0., 0., 0., 0., 0., 0.},
        {0., 0., 0., 0., 0., 0., 0., 0., 0., 0.000043369360575274524, 0.9999999990595501,
         0., 0., 0.},
        {-0.0019206466502236202, 0., -0.011672197660675484, 0., 0., 0.06685683586559842,
         0., 0., 0., 0., 0., -0.9976924548257627, 0., 0.},
        {0., 0., 0., 0., 0., 0., 0., 0., 0., -0.9999999990595487, 0.00004336936057527441,
         0., 0., 0.},
        {0., 0., 0., 0., 0., 0., 0., 1., 0., 0., 0., 0., 0., 0.}};

  // The expected energies:
  scalar_type exp_energy_coulomb = 5.268082025825932;
  scalar_type exp_energy_exchange = -1.73626312131552;
  scalar_type exp_energy_kinetic = 5.720460090270233;
  scalar_type exp_energy_nucattr = -20.6344714235971;
  scalar_type exp_energy_1e_terms = exp_energy_nucattr + exp_energy_kinetic;
  scalar_type exp_energy_2e_terms = exp_energy_coulomb + exp_energy_exchange;
  scalar_type exp_energy_total = exp_energy_1e_terms + exp_energy_2e_terms;

  //
  // Setup integrals
  //
  krims::ParameterMap intparams{{"basis_type", "atomic/static14"},
                                // {"basis_type", "atomic/cs_dummy"},
                                {"k_exponent", k_exp},
                                {"Z_charge", Z},
                                {"n_max", n_max},
                                {"l_max", l_max}};
  int_lookup_type integrals{intparams};

  // Get the integral as actual objects.
  integral_type S_bb = integrals("overlap");
  integral_type T_bb = integrals("kinetic");
  integral_type V0_bb = integrals("nuclear_attraction");
  integral_type J_bb = integrals("coulomb");
  integral_type K_bb = integrals("exchange");

  // Combine 1e terms:
  std::vector<integral_type> terms_1e{T_bb, V0_bb};

  //
  // Problem setup and run.
  //
  auto guess_bf_ptr = std::make_shared<MultiVector<vector_type>>(S_bb.n_rows(), 4);
  (*guess_bf_ptr)[0](0) = -0.9238795325112872;
  (*guess_bf_ptr)[0](1) = -1.306562964876376;
  (*guess_bf_ptr)[0](5) = -0.9238795325112864;
  (*guess_bf_ptr)[1](3) = (*guess_bf_ptr)[1](7) = -0.9192110607898044;
  (*guess_bf_ptr)[2](2) = (*guess_bf_ptr)[2](6) = -0.9192110607898044;
  (*guess_bf_ptr)[3](4) = (*guess_bf_ptr)[3](8) = -0.9192110607898044;

  /*
   * TODO the guess builder is currently pretty bad and unreliable
   *      ... so short circuit it here.
   *
   * auto guess_bf_ptr = std::make_shared<linalgwrap::MultiVector<vector_type>>(
   *    loewdin_guess(S_bb, n_eigenpairs));
   */

  // The term container for the fock operator matrix
  IntegralTermContainer<stored_matrix_type> integral_container(std::move(terms_1e), J_bb,
                                                               K_bb);

  RestrictedClosedIntegralOperator<stored_matrix_type> fock_bb(
        integral_container, guess_bf_ptr, n_alpha, n_beta);

  const krims::ParameterMap params{{IopScfKeys::max_iter, 15ul},
                                   {IopScfKeys::n_eigenpairs, n_eigenpairs},
                                   {IopScfKeys::max_error_norm, tolerance},
                                   {IopScfKeys::n_prev_steps, size_t(4)}};

#ifdef DEBUG
  std::cout << "Running test SCF ... please wait." << std::endl;
#endif
  auto res = run_scf(fock_bb, S_bb, params);

  //
  // Check the results
  //
  CHECK(res.n_iter() <= 10);

  // Check the eigenvalues
  const auto& evalues = res.orbital_energies();
  eval_expected.resize(n_eigenpairs);
  CHECK(vector_type(evalues) ==
        numcomp(vector_type(eval_expected)).tolerance(1000. * tolerance));

  const auto& evectors = res.orbital_coeff();
  // TODO For comparing all of them one needs to take rotations
  //      inside degenerate subspaces into account
  for (size_t i = 0; i < n_alpha; ++i) {
    linalgwrap::adjust_phase(evectors[i], evec_expected[i]);
    CHECK(evectors[i] == numcomp(evec_expected[i]).tolerance(100. * tolerance));
  }

  // Check the energies:
  double energytol = tolerance / 10.;
  const auto& fock = res.problem_matrix();
  auto energies = fock.energies();
  CHECK(energies[J_bb.id()] == numcomp(exp_energy_coulomb).tolerance(energytol));
  CHECK(energies[K_bb.id()] == numcomp(exp_energy_exchange).tolerance(energytol));
  CHECK(energies[V0_bb.id()] == numcomp(exp_energy_nucattr).tolerance(energytol));
  CHECK(energies[T_bb.id()] == numcomp(exp_energy_kinetic).tolerance(energytol));
  CHECK(fock.energy_1e_terms() == numcomp(exp_energy_1e_terms).tolerance(energytol));
  CHECK(fock.energy_2e_terms() == numcomp(exp_energy_2e_terms).tolerance(energytol));
  CHECK(fock.energy_total() == numcomp(exp_energy_total).tolerance(energytol));

}  // TEST_CASE

}  // namespace test
}  // namespace molsturm