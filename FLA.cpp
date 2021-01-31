/*
 * This file is part of Hellscape.
 *
 * Hellscape is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Hellscape is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hellscape.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "FLA.h"

#include <basic-block.h>
#include <tree.h>
#include <tree-cfg.h>

#include <gimple-expr.h>
#include <gimple.h>
#include <gimple-iterator.h>

#include <iostream>
#include <vector>
#include <unordered_map>

#include "Utils.h"
#include "Viz.h"

unsigned int FLAPass::execute(function* f) {
  if(!toObfuscate(mEnable, f, "fla")) return 0;

  // If there's only one block... not much to do.
  if (f->cfg->x_n_basic_blocks <= 3) {
    return 0;
  }

  std::unordered_map<int, uint32_t> block_to_rnd;
  std::vector<int> collected_blocks;

  // For all basic blocks SKIPPING the special entry and the special exit.
  for (basic_block bb = ENTRY_BLOCK_PTR_FOR_FN(f)->next_bb;
       bb && bb->next_bb; bb = bb->next_bb) {
    collected_blocks.push_back(bb->index);

    // Generate a positive random number and ensure it is not already used.
redo:
    auto n = (uint32_t) std::abs(mRandom.nextInt());
    for (auto e : block_to_rnd) {
      if (e.second == n) goto redo;
    }

    block_to_rnd[bb->index] = n;
  }

  // Create the switchVar, used to denote the next destination
  tree switchVar = create_tmp_var(integer_type_node, "switchVar");

  basic_block entry_block = ENTRY_BLOCK_PTR_FOR_FN(f);
  // Initialize the switchVar to the entry block.
  basic_block initialization_block = split_edge(EDGE_SUCC(entry_block, 0));
  gimple_stmt_iterator init_gsi = gsi_last_bb(initialization_block);
  // Set switchVar = <entry>.
  gsi_insert_after(&init_gsi, gimple_build_assign(switchVar, build_int_cst(
    integer_type_node, block_to_rnd[single_succ_edge(initialization_block)->dest->index])),
                   GSI_NEW_STMT);

  // Entry -> switchVar = <first> -> switch -> <first>, etc.
  basic_block switch_block = split_edge(EDGE_SUCC(initialization_block, 0));
  gimple_stmt_iterator switch_gsi = gsi_last_bb(switch_block);
  // We need a NOP here because we need a valid jump destination when switch
  // lowering occurs.
  gsi_insert_after(&switch_gsi, gimple_build_nop(), GSI_NEW_STMT);

  // Create a return block which simply branches back to the switch.
  basic_block return_block = split_edge(single_succ_edge(switch_block));
  gimple_stmt_iterator ret_gsi = gsi_last_bb(return_block);
  gsi_insert_after(&ret_gsi, gimple_build_nop(), GSI_NEW_STMT);
  edge ret_e = single_succ_edge(return_block);
  redirect_edge_succ(ret_e, switch_block);

  // Create a dummy block which is used to create an infinite loop between the default case
  // and the switch.
  basic_block dummy_block = split_edge(EDGE_SUCC(switch_block, 0));
  gimple_stmt_iterator dummy_gsi = gsi_last_bb(dummy_block);
  gsi_insert_after(&dummy_gsi, gimple_build_nop(), GSI_NEW_STMT);
  redirect_edge_succ(EDGE_SUCC(dummy_block, 0), return_block);

  tree default_lab = build_case_label(NULL_TREE, NULL_TREE,
                                      gimple_block_label(dummy_block));

  // Labels MUST be sorted in GIMPLE. Must not include the default case label.
  auto_vec<tree> case_label_vec;
  case_label_vec.create(collected_blocks.size());

  // Add all blocks to the switch.
  for (auto& bbi : collected_blocks) {
    basic_block target = BASIC_BLOCK_FOR_FN(f, bbi);

    // Set the statement iterator to the last element in the block (the
    // conditional).
    gimple_stmt_iterator last_gsi = gsi_last_bb(target);
    gimple* last = NULL;

    while (!gsi_end_p(last_gsi) &&
           is_gimple_debug((last = gsi_stmt(last_gsi)))) {
      gsi_prev(&last_gsi);
      last = NULL;
    }

    if (last->code == GIMPLE_COND) {
      auto* condptr = (gcond*) last;
      // Extract the condition and place it into a condition expression which
      // is assigned to the switchVar.
      tree_code cond_code = gimple_cond_code(condptr);
      tree lhs = gimple_cond_lhs(condptr);
      tree rhs = gimple_cond_rhs(condptr);

      // TODO Q: can if statements with no else have a fallthrough or do they create an else case?

      // Get the destination edges.
      edge true_e;
      edge false_e;
      extract_true_false_edges_from_block(target, &true_e, &false_e);

      // Make a destination conditional, e.g.: switchVar = condition ? trueI : falseI.
      tree destination = build3(COND_EXPR, integer_type_node,
                                build2(cond_code, boolean_type_node, lhs,
                                       rhs), build_int_cst(integer_type_node,
                                                           block_to_rnd[true_e->dest->index]),
                                build_int_cst(integer_type_node,
                                              block_to_rnd[false_e->dest->index]));
      // Remove the if statement, replace it with the destination assignment.
      gimple* assign = gimple_build_assign(switchVar, destination);
      gimple_set_bb(assign, target);
      gsi_set_stmt(&last_gsi, assign);

      // Remove all outbound edges and replace them with a connection back to the switch.
      remove_edge(true_e);
      remove_edge(false_e);
      make_edge(target, return_block, EDGE_FALLTHRU);
    } else {
      // It's not a conditional, re-route the fallthrough case to an assignment.
      gimple_stmt_iterator target_gsi = gsi_last_bb(target);
      edge fall_e = single_succ_edge(target);

      // If it is NOT pointing to the exit block, flatten.
      if (fall_e->dest->index != 1 /* BLOCK_EXIT */) {
        gimple* assign = gimple_build_assign(switchVar,
                                             build_int_cst(integer_type_node,
                                                           block_to_rnd[fall_e->dest->index]));
        gimple_set_bb(assign, target);
        gsi_insert_after(&target_gsi, assign, GSI_NEW_STMT);
        remove_edge(fall_e);
        make_edge(target, return_block, EDGE_FALLTHRU);
      }
    }

    tree lab = build_case_label(
      build_int_cst(integer_type_node, block_to_rnd[bbi]), NULL,
      gimple_block_label(target));
    case_label_vec.quick_push(lab);
  }

  // IR requires that the labels are sorted.
  sort_case_labels(case_label_vec);
  gsi_insert_after(&switch_gsi, gimple_build_switch(switchVar, default_lab,
                                                    case_label_vec),
                   GSI_NEW_STMT);

  // Send the default case to the dummy block for an infinite loop.
  redirect_edge_succ(EDGE_SUCC(switch_block, 0), dummy_block);
  EDGE_SUCC(switch_block, 0)->flags = 0;
  EDGE_SUCC(switch_block,
            0)->probability = profile_probability::uninitialized();

  for (auto& bbi : collected_blocks) {
    make_edge(switch_block, BASIC_BLOCK_FOR_FN(f, bbi), 0);
  }

  // We've moved the CFG around a lot, so throw away the computed dominators.
  free_dominance_info(f, CDI_DOMINATORS);
  free_dominance_info(f, CDI_POST_DOMINATORS);

  return 0;
}