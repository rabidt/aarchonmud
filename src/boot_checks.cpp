extern "C" {
#include "boot_checks.h"
}

#include <string>
#include <set>

extern "C" {
#include "merc.h"
}


bool boot_check_skills(BUFFER *str)
{
    // MAX_SKILL should match # of skills in table
    {
        int skill_count = 0;
        for (int i = 0; skill_table[i].name; ++i)
        {
            ++skill_count;
        }

        if (skill_count != MAX_SKILL)
        {
            addf_buf(str, "skill_count: %d, MAX_SKILL: %d\r\n", skill_count, MAX_SKILL);
            return false;
        }
    }

    // Should be no duplicate names
    {
        bool isgood = true;
        std::set< std::string > skname_set;

        for (int i = 0; skill_table[i].name; ++i)
        {
            std::string name(skill_table[i].name);
            if (skname_set.find(name) != skname_set.end())
            {
                addf_buf(str, "Multiple skills with name: %s\r\n", skill_table[i].name);
                isgood = false;
            }
            else
            {
                skname_set.insert(name);
            }
        }

        if (!isgood)
        {
            return false;
        }
    }

    return true;
}
