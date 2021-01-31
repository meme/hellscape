#include "Utils.h"
#include <stringpool.h>
#include <attribs.h>

#include <string>

using std::string;

bool toObfuscate(bool flag, function* f, string attribute) {
  /* skip declaration */
  // return false;

  /* skip external linkage */
  // return false;

  /* skip some functions that cannot currently be handled */
  // return false;

  // Check the attribute
  string attr = attribute;
  string attrNo = "no" + attr;

  tree attri = DECL_ATTRIBUTES(f->decl);
  if (attri) {
    tree q = lookup_attribute("obfus", attri);
    if (q) {
      tree qq = q;
      while (qq) {
        string name = TREE_STRING_POINTER(TREE_VALUE(TREE_VALUE(qq)));

        if (!name.empty()) {
          if (name.find(attrNo) != string::npos) {
            return false;
          }

          if (name.find(attr) != string::npos) {
            return true;
          }
        }

        qq = TREE_CHAIN (qq);
      }
    }
  }

  return flag;
}