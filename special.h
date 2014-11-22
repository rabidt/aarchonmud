#ifndef SPECIAL_H
#define SPECIAL_H

DECLARE_SPEC_FUN( spec_breath_any       );
DECLARE_SPEC_FUN( spec_breath_acid      );
DECLARE_SPEC_FUN( spec_breath_fire      );
DECLARE_SPEC_FUN( spec_breath_frost     );
DECLARE_SPEC_FUN( spec_breath_gas       );
DECLARE_SPEC_FUN( spec_breath_lightning );
DECLARE_SPEC_FUN( spec_cast_adept       );
DECLARE_SPEC_FUN( spec_cast_cleric      );
DECLARE_SPEC_FUN( spec_cast_judge       );
DECLARE_SPEC_FUN( spec_cast_mage        );
DECLARE_SPEC_FUN( spec_cast_draconic    );
DECLARE_SPEC_FUN( spec_cast_undead      );
DECLARE_SPEC_FUN( spec_executioner      );
DECLARE_SPEC_FUN( spec_fido             );
DECLARE_SPEC_FUN( spec_guard            );
DECLARE_SPEC_FUN( spec_janitor          );
DECLARE_SPEC_FUN( spec_mayor            );
DECLARE_SPEC_FUN( spec_poison           );
DECLARE_SPEC_FUN( spec_thief            );
DECLARE_SPEC_FUN( spec_nasty            );
DECLARE_SPEC_FUN( spec_troll_member     );
DECLARE_SPEC_FUN( spec_ogre_member      );
DECLARE_SPEC_FUN( spec_patrolman        );
DECLARE_SPEC_FUN( spec_questmaster      );
DECLARE_SPEC_FUN( spec_bounty_hunter    );
DECLARE_SPEC_FUN( spec_remort           );
DECLARE_SPEC_FUN( spec_temple_guard     );

struct spell_type
{
    char    *spell;
    sh_int  min_level;
    sh_int  max_level;
};

const struct spell_type* get_spell_list( CHAR_DATA *ch );

#endif
