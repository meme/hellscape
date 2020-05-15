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

#include "Viz.h"

#include <tree.h>
#include <tree-pass.h>
#include <function.h>
#include <basic-block.h>

#include <gimple.h>
#include <gimple-pretty-print.h>
#include <gimple-iterator.h>

#include <string>
#include <sstream>

// For open_memstream.
#include <stdio_ext.h>
#include <fstream>

/**
 * Escape data so it is suitable for HTML embedding.
 *
 * @param data string to escape
 * @return escaped string
 */
static std::string escape(std::string&& data) {
  std::string buffer;
  buffer.reserve(data.size());
  for (size_t pos = 0; pos != data.size(); ++pos) {
    switch (data[pos]) {
    case '&':
      buffer.append("&amp;");
      break;
    case '\"':
      buffer.append("&quot;");
      break;
    case '\'':
      buffer.append("&apos;");
      break;
    case '<':
      buffer.append("&lt;");
      break;
    case '>':
      buffer.append("&gt;");
      break;
    case '|':
      buffer.append("\\|");
      break;
    default:
      buffer.append(&data[pos], 1);
      break;
    }
  }

  return buffer;
}

unsigned int VizPass::execute(function* f) {
  if (!mEnable) return 0;
  std::stringstream ss;

  ss << "digraph G {\n";

  basic_block bb;

  pretty_printer buffer;
  char* buffer_data = nullptr;
  size_t buffer_size = 0;
  buffer.buffer->stream = open_memstream(&buffer_data, &buffer_size);

  // For all basic blocks.
  FOR_ALL_BB_FN(bb, f) {
    ss << std::to_string(bb->index) << " [\n";
    ss << "shape=\"Mrecord\"\n";
    ss << "fontname=\"Courier New\"\n";

    ss << "label=<\n";
    ss << "<table border=\"0\" cellborder=\"0\" cellpadding=\"3\">\n";
    ss << "<tr><td align=\"center\" colspan=\"2\" bgcolor=\"grey\">"
              << std::to_string(bb->index) << "</td></tr>\n";

    gimple_bb_info* bb_info = &bb->il.gimple;
    // For all IR statements.
    for (gimple_stmt_iterator i = gsi_start(bb_info->seq); !gsi_end_p(
      i); gsi_next(&i)) {
      gimple* gs = gsi_stmt(i);

      pp_gimple_stmt_1(&buffer, gs, 0, static_cast<dump_flags_t>(0));
      pp_flush(&buffer);

      ss << "<tr><td align=\"left\">" << escape(buffer_data)
                << "</td></tr>\n";

      // Clear the contents for the next write.
      memset(buffer_data, 0, strlen(buffer_data) + 1);
      fseek(buffer.buffer->stream, 0, SEEK_SET);
    }

    ss << "</table>\n";
    ss << ">\n];\n";

    edge e;
    edge_iterator ei{};
    FOR_EACH_EDGE(e, ei, bb->succs) {
      if (e->flags & EDGE_TRUE_VALUE) {
        ss << std::to_string(bb->index) << " -> "
                  << std::to_string(e->dest->index)
                  << " [color=\"green\"];\n";
      } else if (e->flags & EDGE_FALSE_VALUE) {
        ss << std::to_string(bb->index) << " -> "
                  << std::to_string(e->dest->index) << " [color=\"red\"];\n";
      } else {
        ss << std::to_string(bb->index) << " -> "
                  << std::to_string(e->dest->index) << " [color=\"blue\"];\n";
      }
    }
  }

  fclose(buffer.buffer->stream);
  free(buffer_data);

  ss << "}\n";

  // Write the output to /tmp/name.dot.
  std::ofstream out_file;
  out_file.open("/tmp/" + std::string(function_name(f)) + ".dot");
  out_file << ss.rdbuf();

  return 0;

}
