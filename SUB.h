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

#include <memory>

#include "Random.h"

const pass_data sub_pass_data = {
  GIMPLE_PASS,
  "sub",

  OPTGROUP_NONE,
  TV_NONE,
  PROP_gimple_any,
  0, 0, 0, 0
};

struct SUBPass : gimple_opt_pass {
  Random& mRandom;
  uint32_t mSubLoop;
  bool mEnable;

  SUBPass(gcc::context* context, Random& random, uint32_t subLoop, bool enable = true) : gimple_opt_pass(
    sub_pass_data, context), mRandom(random), mSubLoop(subLoop), mEnable(enable) {
  }

  unsigned int execute(function* f) override;

  SUBPass* clone() override {
    return this;
  }
};