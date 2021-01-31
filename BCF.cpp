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

#include "BCF.h"

#include <basic-block.h>
#include <function.h>
#include <tree.h>
#include <tree-cfg.h>
#include <stringpool.h>

#include <gimple.h>
#include <gimple-iterator.h>

#include <cgraph.h>
#include <cfgloop.h>

#include <iostream>
#include <vector>
#include "Utils.h"

void BCFPass::create_globals() {
  // Already created the declarations.
  if (mX != NULL_TREE && mY != NULL_TREE) return;

  // Create global declarations for x and y, used for BCF.
  mX = build_decl(UNKNOWN_LOCATION, VAR_DECL, get_identifier_with_length("$x", 2), integer_type_node);
  // It's static we want to allocate storage for it.
  TREE_STATIC(mX) = 1;
  DECL_INITIAL(mX) = build_zero_cst(integer_type_node);
  TREE_USED(mX) = 1;
  TREE_PUBLIC(mX) = 1;
  symtab_node* x_node = symtab_node::get_create(mX);
  x_node->force_output = 1;
  x_node->definition = 1;

  mY = build_decl(UNKNOWN_LOCATION, VAR_DECL, get_identifier_with_length("$y", 2), integer_type_node);
  // It's static we want to allocate storage for it.
  TREE_STATIC(mY) = 1;
  DECL_INITIAL(mY) = build_zero_cst(integer_type_node);
  TREE_USED(mY) = 1;
  TREE_PUBLIC(mY) = 1;
  symtab_node* y_node = symtab_node::get_create(mY);
  y_node->force_output = 1;
  y_node->definition = 1;
}

unsigned int BCFPass::execute(function* f) {
  if(!toObfuscate(mEnable, f, "bcf")) return 0;

  create_globals();

  // y < 10
  tree first_cond = build2(LT_EXPR, boolean_type_node, mY,
                           build_int_cst(integer_type_node, 10));
  // x + 1
  tree second_cond = build2(PLUS_EXPR, integer_type_node, mX,
                            build_one_cst(integer_type_node));
  // x * (x + 1)
  tree second_cond2 = build2(MULT_EXPR, integer_type_node, mX,
                             second_cond);
  // x * (x + 1) & 1
  tree second_cond3 = build2(BIT_AND_EXPR, integer_type_node, second_cond2,
                             build_one_cst(integer_type_node));
  // x * (x + 1) & 1 == 0
  tree second_cond4 = build2(EQ_EXPR, boolean_type_node, second_cond3,
                             build_zero_cst(integer_type_node));

  std::vector<int> collected_blocks;
  // For all basic blocks SKIPPING the special entry and the special exit and any exit block returns.
  for (basic_block bb = ENTRY_BLOCK_PTR_FOR_FN(f)->next_bb;
       bb && bb->next_bb && bb->next_bb->next_bb; bb = bb->next_bb) {
    collected_blocks.push_back(bb->index);
  }

  for (int i : collected_blocks) {
    basic_block target_block = BASIC_BLOCK_FOR_FN(f, i);

    // Create the guard block by splitting the edge between the entry and the real
    // basic block, then insert the condition into the guard block.
    edge cond_to_target = split_block_after_labels(target_block);
    basic_block conditional_block = cond_to_target->src;
    gimple_stmt_iterator gsi = gsi_last_bb(conditional_block);

    // Create a NOP so the builder has an insertion point.
    gsi_insert_after(&gsi, gimple_build_nop(), GSI_NEW_STMT);
    // Flatten the sub-expressions and store the GIMPLE value.
    tree opaque_cond = gimplify_build2(&gsi, TRUTH_OR_EXPR, boolean_type_node,
                                       copy_node(first_cond),
                                       copy_node(second_cond4));
    // Insert if (y < 10 || x * (x + 1) & 1 == 0)
    gsi_insert_after(&gsi, gimple_build_cond(NE_EXPR, opaque_cond,
                                             boolean_false_node, NULL_TREE,
                                             NULL_TREE), GSI_NEW_STMT);

    // Create a junk block by splitting the edge between the conditional block
    // and the real block.
    basic_block junk_block = split_edge(EDGE_SUCC(conditional_block, 0));
    gimple_stmt_iterator junk_gsi = gsi_last_bb(junk_block);
    gsi_insert_after(&junk_gsi, gimple_build_nop(), GSI_NEW_STMT);

    // Mark the default fallthrough of the conditional block pointing to the junk
    // block the false value.
    edge enter_e = EDGE_SUCC(conditional_block, 0);
    enter_e->flags &= ~EDGE_FALLTHRU;
    enter_e->flags |= EDGE_FALSE_VALUE;

    // Add a true value to the conditional block that jumps to the real basic
    // block.
    edge new_e2 = make_edge(conditional_block, single_succ(junk_block),
                            EDGE_TRUE_VALUE);
    (void) new_e2;

    // Now make the junk block loop back to the conditional block.
    remove_bb_from_loops(junk_block);
    add_bb_to_loop(junk_block, conditional_block->loop_father);
    redirect_edge_succ(single_succ_edge(junk_block), conditional_block);
  }

  // Fix-up since we've added loops.
  loops_state_set(LOOPS_NEED_FIXUP);

  // We've moved the CFG around a lot, so throw away the computed dominators.
  free_dominance_info(f, CDI_DOMINATORS);
  free_dominance_info(f, CDI_POST_DOMINATORS);

//    GraphVizGFG(g).execute(f);

  return 0;
}