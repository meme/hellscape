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

#include "SUB.h"

#include <function.h>
#include <tree.h>
#include <basic-block.h>

#include <gimple.h>
#include <gimple-pretty-print.h>
#include <gimple-iterator.h>
#include <gimplify-me.h>

#include <iostream>
#include "Utils.h"

unsigned int SUBPass::execute(function* f) {
  if(!toObfuscate(mEnable, f, "sub")) return 0;

  for (uint32_t count = 0; count < mSubLoop; count++) {
    basic_block bb;
    // For all basic blocks.
    FOR_ALL_BB_FN(bb, f) {
      gimple_bb_info* bb_info = &bb->il.gimple;
      // For all IR statements.
      for (gimple_stmt_iterator i = gsi_start(bb_info->seq); !gsi_end_p(
        i); gsi_next(&i)) {
        gimple* gs = gsi_stmt(i);

        if (is_gimple_assign(gs)) {
          tree_code expr_code = gimple_expr_code(gs);

          switch (expr_code) {
          case BIT_AND_EXPR: {
            /* a = b & c => a = (b ^ ~c) & b */
            tree a = gimple_assign_lhs(gs);
            tree b = gimple_assign_rhs1(gs);
            tree c = gimple_assign_rhs2(gs);

            tree not_c = build1(BIT_NOT_EXPR, TREE_TYPE(c), c);
            tree left = build2(BIT_XOR_EXPR, TREE_TYPE(b), b, not_c);
            tree res = build2(BIT_AND_EXPR, TREE_TYPE(left), left, b);

            gimple* assign = gimple_build_assign(a,
                                                 force_gimple_operand_gsi(&i,
                                                                          res,
                                                                          true,
                                                                          NULL,
                                                                          true,
                                                                          GSI_SAME_STMT));
            gimple_set_bb(assign, bb);
            gsi_set_stmt(&i, assign);
          }
            break;
          case BIT_IOR_EXPR: {
            /* a = b | c => a = (b & c) | (b ^ c) */
            tree a = gimple_assign_lhs(gs);
            tree b = gimple_assign_rhs1(gs);
            tree c = gimple_assign_rhs2(gs);

            tree left = build2(BIT_AND_EXPR, TREE_TYPE(b), b, c);
            tree right = build2(BIT_XOR_EXPR, TREE_TYPE(b), b, c);
            tree res = build2(BIT_IOR_EXPR, TREE_TYPE(left), left, right);

            // Force sub-expression expansion.
            gimple* assign = gimple_build_assign(a,
                                                 force_gimple_operand_gsi(&i,
                                                                          res,
                                                                          true,
                                                                          NULL,
                                                                          true,
                                                                          GSI_SAME_STMT));
            gimple_set_bb(assign, bb);
            gsi_set_stmt(&i, assign);
          }
            break;
          case BIT_XOR_EXPR: {
            /* a = b ^ c => a = (~b & c) | (b & ~c) */
            tree a = gimple_assign_lhs(gs);
            tree b = gimple_assign_rhs1(gs);
            tree c = gimple_assign_rhs2(gs);

            tree left = build2(BIT_AND_EXPR, TREE_TYPE(b),
                               build1(BIT_NOT_EXPR, TREE_TYPE(b), b), c);
            tree right = build2(BIT_AND_EXPR, TREE_TYPE(b), b,
                                build1(BIT_NOT_EXPR, TREE_TYPE(c), c));
            tree res = build2(BIT_IOR_EXPR, TREE_TYPE(left), left, right);

            gimple* assign = gimple_build_assign(a,
                                                 force_gimple_operand_gsi(&i,
                                                                          res,
                                                                          true,
                                                                          NULL,
                                                                          true,
                                                                          GSI_SAME_STMT));
            gimple_set_bb(assign, bb);
            gsi_set_stmt(&i, assign);
          }
            break;
          default:
            break;
          }
        }
      }
    }
  }

//    GraphVizGFG(g).execute(f);

  return 0;
}
