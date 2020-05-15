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

#pragma once

#include <gcc-plugin.h>
#include <tree-pass.h>
#include <context.h>
#include <function.h>
#include <tree.h>

#include <memory>

#include "Random.h"

const pass_data bcf_pass_data = {
  GIMPLE_PASS,
  "bcf",

  OPTGROUP_NONE,
  TV_NONE,
  PROP_gimple_any,
  0, 0, 0, 0
};

struct BCFPass : gimple_opt_pass {
  Random& mRandom;
  tree mX = NULL_TREE;
  tree mY = NULL_TREE;
  bool mEnable;

  BCFPass(gcc::context* context, Random& random, bool enable = true) : gimple_opt_pass(
    bcf_pass_data, context), mRandom(random), mEnable(enable) {
  }

  void create_globals();

  unsigned int execute(function* f) override;

  BCFPass* clone() override {
    return this;
  }
};
