#!/usr/bin/env python3
import molsturm
import pyadc

params = {
  "atom_numbers": [4],
  "coords":       [[0,0,0]],
  #
  "basis_type":   "atomic/cs_reference_pc",
  "n_max":        11,
  "l_max":        0,
  #
  "print_iterations": True,
  #
  "eigensolver":   "lapack",
  "guess_esolver": "lapack",
  "error":          1e-10,
}

res = molsturm.hartree_fock(**params)

print("\nFinal energies:")
molsturm.print_energies(res, indention=6*" ")

print("\nOrbital occupation:")
molsturm.print_mo_occupation(res,indention=6*" ")

params_adc = molsturm.build_pyadc_input(res)
res_adc = pyadc.adc(**params_adc, max_memory=32*1024*1024)
print("MP2 energy", res_adc["energy_mp2"])
print("tot energy", res_adc["energy_ground_state"])