# flash ir v1
target = "ppu32"
name = "ternary_logic"
format = "fir-v1"
[ir]
node 0 { kind = "route", op = "trit_lit", from = (0m, 0m, 0m), to = (0m, 0m, 0m), power_mw = 1250m, pol = 1, pcm_row = 0, pcm_col = 0, phase_a_rad = 0u, phase_b_rad = 0u, phase_result_rad = 0u, lattice_op = 0 }
node 1 { kind = "route", op = "trit_lit", from = (0m, 0m, 0m), to = (0m, 0m, 0m), power_mw = 1250m, pol = -1, pcm_row = 2, pcm_col = 0, phase_a_rad = 0u, phase_b_rad = 0u, phase_result_rad = 0u, lattice_op = 0 }
node 2 { kind = "route", op = "trit_lit", from = (30000m, 21000m, 0m), to = (30000m, 21000m, 0m), power_mw = 1000m, pol = 0, pcm_row = 4, pcm_col = 0, phase_a_rad = 0u, phase_b_rad = 0u, phase_result_rad = 0u, lattice_op = 0 }
node 3 { kind = "route", op = "trit_var", from = (30000m, 21000m, 0m), to = (30000m, 21000m, 0m), power_mw = 1250m, pol = 1, pcm_row = 6, pcm_col = 0, phase_a_rad = 0u, phase_b_rad = 0u, phase_result_rad = 0u, lattice_op = 0 }
node 4 { kind = "route", op = "trit_var", from = (60000m, 0m, 0m), to = (60000m, 0m, 0m), power_mw = 1250m, pol = -1, pcm_row = 8, pcm_col = 0, phase_a_rad = 0u, phase_b_rad = 0u, phase_result_rad = 0u, lattice_op = 0 }
node 5 { kind = "route", op = "trit_add", from = (60000m, 0m, 0m), to = (60000m, 0m, 0m), power_mw = 1500m, pol = 0, pcm_row = 10, pcm_col = 0, phase_a_rad = 1047197u, phase_b_rad = -1047197u, phase_result_rad = 0u, lattice_op = 3 }
node 6 { kind = "route", op = "trit_var", from = (90000m, 21000m, 0m), to = (90000m, 21000m, 0m), power_mw = 1250m, pol = 1, pcm_row = 12, pcm_col = 0, phase_a_rad = 0u, phase_b_rad = 0u, phase_result_rad = 0u, lattice_op = 0 }
node 7 { kind = "route", op = "trit_var", from = (90000m, 21000m, 0m), to = (90000m, 21000m, 0m), power_mw = 1000m, pol = 0, pcm_row = 14, pcm_col = 0, phase_a_rad = 0u, phase_b_rad = 0u, phase_result_rad = 0u, lattice_op = 0 }
node 8 { kind = "route", op = "trit_mul", from = (0m, 30000m, 0m), to = (0m, 30000m, 0m), power_mw = 1250m, pol = 0, pcm_row = 16, pcm_col = 0, phase_a_rad = 1047197u, phase_b_rad = 0u, phase_result_rad = 0u, lattice_op = 5 }
node 9 { kind = "route", op = "trit_var", from = (0m, 30000m, 0m), to = (0m, 30000m, 0m), power_mw = 1000m, pol = 0, pcm_row = 18, pcm_col = 0, phase_a_rad = 0u, phase_b_rad = 0u, phase_result_rad = 0u, lattice_op = 0 }
node 10 { kind = "route", op = "trit_var", from = (30000m, 51000m, 0m), to = (30000m, 51000m, 0m), power_mw = 1000m, pol = 0, pcm_row = 20, pcm_col = 0, phase_a_rad = 0u, phase_b_rad = 0u, phase_result_rad = 0u, lattice_op = 0 }
node 11 { kind = "route", op = "trit_nand", from = (30000m, 51000m, 0m), to = (30000m, 51000m, 0m), power_mw = 1000m, pol = 0, pcm_row = 22, pcm_col = 0, phase_a_rad = 0u, phase_b_rad = 0u, phase_result_rad = 0u, lattice_op = 7 }
node 12 { kind = "route", op = "trit_var", from = (60000m, 30000m, 0m), to = (60000m, 30000m, 0m), power_mw = 1000m, pol = 0, pcm_row = 24, pcm_col = 0, phase_a_rad = 0u, phase_b_rad = 0u, phase_result_rad = 0u, lattice_op = 0 }
node 13 { kind = "route", op = "trit_neg", from = (60000m, 30000m, 0m), to = (60000m, 30000m, 0m), power_mw = 1000m, pol = 0, pcm_row = 26, pcm_col = 0, phase_a_rad = 0u, phase_b_rad = 0u, phase_result_rad = 0u, lattice_op = 2 }
