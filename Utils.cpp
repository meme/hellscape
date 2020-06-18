#include "Utils.h"
#include <stringpool.h>
#include <attribs.h>

using namespace std;

bool toObfuscate(bool flag, function *f, string attribute)
{
    /* skip declaration*/
    //return false;

    /*skip external linkage*/
    //return false;

    /*skip some functions that cannot currently be handled */
    //return false;

    /*check attribute*/
    std::string attr = attribute;
    std::string attrNo = "no" + attr;

    tree attri = DECL_ATTRIBUTES(f->decl);
    if(attri)
    {
        tree q = lookup_attribute("obfus", attri);
        if(q)
        {
            tree qq = q;
            while(qq)
            {
                std::string name = TREE_STRING_POINTER (TREE_VALUE(TREE_VALUE(qq)));
                if(!name.empty())
                {
                    if(name.find(attrNo) != string::npos)
                    {
                        std::cerr << "in " << function_name(f) <<" find " << name << "\n";
                        return false;
                    }

                    if(name.find(attr) != string::npos)
                    {
                        std::cerr << "in " << function_name(f) <<" find " << name << "\n";
                        return true;
                    }
                    
                }
                qq = TREE_CHAIN (qq);
            }
        }
    }
    if (flag == true) {
        return true;
    } 
    return false;
}