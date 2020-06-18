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

#include <gcc-plugin.h>
#include <plugin-version.h>

#include <tree-pass.h>

#include <iostream>
#include <memory>

#include "Random.h"
#include "Viz.h"
#include "SUB.h"
#include "BCF.h"
#include "FLA.h"

#include <sys/random.h>

int plugin_is_GPL_compatible = 6 * 6 + 6;

static struct plugin_info my_plugin_info = {"1.0.0",
                                            "The de-optimizing compiler."};


static tree
handle_obfus_attribute (tree *node, tree name, tree args, int flags, bool *no_add_attrs)
{
  return NULL_TREE;
}

/* Attribute definition */

static struct attribute_spec fla_attr =
{ "obfus", 1, 1, false,  false, false, false, handle_obfus_attribute, NULL};


/* Plugin callback called during attribute registration */

static void 
register_attributes (void *event_data, void *data) 
{
  register_attribute (&fla_attr);
}

void finish_gcc(void* gcc_data, void* user_data) {
  // Delete the RNG.
  delete (Random*) user_data;
}

int plugin_init(struct plugin_name_args* plugin_info,
                struct plugin_gcc_version* version) {
  if (!plugin_default_version_check(version, &gcc_version)) {
    std::cerr << "error: expected GCC " << GCCPLUGIN_VERSION_MAJOR << "."
              << GCCPLUGIN_VERSION_MINOR << "\n";
    return 1;
  }

  register_callback(plugin_info->base_name, PLUGIN_INFO, nullptr,
                    &my_plugin_info);

  // Default subLoop to 1.
  uint32_t subLoop = 1;

  // Disable all passes by default.
  bool enableFLA, enableBCF, enableSUB;
  enableFLA = enableBCF = enableSUB = false;

  // Seed the RNG if no seed is provided.
  uint32_t seed;
  if (getrandom(&seed, sizeof(seed), GRND_NONBLOCK) != sizeof(seed)) {
    std::cerr << "error: getrandom initialization failed\n";
    return 1;
  }

  for (int i = 0; i < plugin_info->argc; i++) {
    std::string key = plugin_info->argv[i].key;
    std::string value = plugin_info->argv[i].value
                        ? plugin_info->argv[i].value
                        : "";

    // -fplugin-arg-hellscape-seed=deadbeef
    if (key == "seed") {
      if (value.size() == 8) {
        char* none;
        seed = strtoul(value.c_str(), &none, 16);

        if (*none != 0) {
          goto seed_error;
        }
      } else if (value.rfind("0x", 0) == 0 && value.size() == 10) {
        char* none;
        seed = strtoul(value.c_str() + sizeof("0x") - 1, &none, 16);

        if (*none != 0) {
          goto seed_error;
        }
      } else {
seed_error:
        std::cerr << "error: seed argument malformed\n";
        return 1;
      }
    }

    // -fplugin-arg-hellscape-fla
    // -fplugin-arg-hellscape-perFLA=30
    if (key == "fla") {
      enableFLA = true;
    }

    // -fplugin-arg-hellscape-bcf
    // -fplugin-arg-hellscape-perBCF=30
    if (key == "bcf") {
      enableBCF = true;
    }

    // -fplugin-arg-hellscape-sub
    // -fplugin-arg-hellscape-perSUB=30
    if (key == "sub") {
      enableSUB = true;
    }

    // -fplugin-arg-hellscape-subLoop=3
    if (key == "subLoop") {
      char* none;
      subLoop = strtoul(value.c_str(), &none, 10);

      if (*none != 0) {
        std::cerr << "error: subLoop argument malformed\n";
        return 1;
      }
    }
  }

  // Allocate RNG, freed in finish_gcc
  auto* random = new Random (seed);

  struct register_pass_info sub_pass_info{};
  sub_pass_info.pass = new SUBPass(g, *random, subLoop, enableSUB);
  sub_pass_info.reference_pass_name = "cfg";
  sub_pass_info.ref_pass_instance_number = 1;
  sub_pass_info.pos_op = PASS_POS_INSERT_AFTER;

  struct register_pass_info bcf_pass_info{};
  bcf_pass_info.pass = new BCFPass(g, *random, enableBCF);
  bcf_pass_info.reference_pass_name = "sub";
  bcf_pass_info.ref_pass_instance_number = 1;
  bcf_pass_info.pos_op = PASS_POS_INSERT_AFTER;

  struct register_pass_info fla_pass_info{};
  fla_pass_info.pass = new FLAPass(g, *random, enableFLA);
  fla_pass_info.reference_pass_name = "bcf";
  fla_pass_info.ref_pass_instance_number = 1;
  fla_pass_info.pos_op = PASS_POS_INSERT_AFTER;

  // Viz is disabled by default and cannot be enabled via CLI, so this is a no-op.
  struct register_pass_info viz_pass_info{};
  viz_pass_info.pass = new VizPass(g /*, true */);
  viz_pass_info.reference_pass_name = "fla";
  viz_pass_info.ref_pass_instance_number = 1;
  viz_pass_info.pos_op = PASS_POS_INSERT_AFTER;

  register_callback (plugin_info->base_name, PLUGIN_ATTRIBUTES, register_attributes, NULL);
  
  register_callback(plugin_info->base_name, PLUGIN_PASS_MANAGER_SETUP, nullptr,
                    &sub_pass_info);
  register_callback(plugin_info->base_name, PLUGIN_PASS_MANAGER_SETUP, nullptr,
                    &bcf_pass_info);
  register_callback(plugin_info->base_name, PLUGIN_PASS_MANAGER_SETUP, nullptr,
                    &fla_pass_info);
  register_callback(plugin_info->base_name, PLUGIN_PASS_MANAGER_SETUP, nullptr,
                    &viz_pass_info);

  register_callback(plugin_info->base_name, PLUGIN_FINISH, finish_gcc, random);

  return 0;
}
