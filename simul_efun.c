/*
// File: simul_efun.c
// This file holds a list of simulated efuns, that is loaded when
// the game is booted.  
// Written by Buddha@TMI (2-19-92)
// This file is part of the TMI distribution mudlib.
// Please be considerate and retain the header file if you use it.
*/

#include <simul_efun.h>

#include "/adm/simul_efun/arraysub.c"
#include "/adm/simul_efun/base_name.c"
#include "/adm/simul_efun/center.c"
#include "/adm/simul_efun/log_file.c"
#include "/adm/simul_efun/wrap.c"
#include "/adm/simul_efun/hiddenp.c"
#include "/adm/simul_efun/identify.c"
#include "/adm/simul_efun/creator_file.c"
#include "/adm/simul_efun/exclude_array.c"
#include "/adm/simul_efun/find_object_or_load.c"
#include "/adm/simul_efun/format_string.c"
#include "/adm/simul_efun/idle.c"
#include "/adm/simul_efun/path_file.c"
#include "/adm/simul_efun/substr.c"
#include "/adm/simul_efun/distinct_array.c"
#include "/adm/simul_efun/user_path.c"
#include "/adm/simul_efun/resolv_path.c"
#include "/adm/simul_efun/member_group.c"
#include "/adm/simul_efun/archp.c"
#include "/adm/simul_efun/owner_euid.c"
#include "/adm/simul_efun/overrides.c"
#include "/adm/simul_efun/copy.c"
#include "/adm/simul_efun/parse_objects.c"
#include "/adm/simul_efun/arrange_string.c"
#include "/adm/simul_efun/total_light.c"
#include "/adm/simul_efun/to_object.c"
#include "/adm/simul_efun/tell_player.c"
#include "/adm/simul_efun/percent.c"
#include "/adm/simul_efun/effective_light.c"
#include "/adm/simul_efun/interact.c"
#include "/adm/simul_efun/time.c"
#include "/adm/simul_efun/atoi.c"
#include "/adm/simul_efun/file_exists.c"
#include "/adm/simul_efun/absolute_value.c"
#include "/adm/simul_efun/mud_info.c"
#include "/adm/simul_efun/get_object.c"
#include "/adm/simul_efun/alignment_ok.c"
#include "/adm/simul_efun/visible.c"
#include "/adm/simul_efun/pluralize.c"
#include "/adm/simul_efun/format_page.c"
#include "/adm/simul_efun/ordinal.c"
#include "/adm/simul_efun/personal_log.c"
#include "/adm/simul_efun/consolidate.c"
#include "/adm/simul_efun/user_exists.c"
#include "/adm/simul_efun/read_database.c"
#include "/adm/simul_efun/ambassadorp.c"
#include "/adm/simul_efun/communications.c"
#include "/adm/simul_efun/translate.c"
#include "/adm/simul_efun/economy.c"
#include "/adm/simul_efun/events.c"
#include "/adm/simul_efun/leaderp.c"
#include "/adm/simul_efun/gender.c"
#include "/adm/simul_efun/convert_name.c"
#include "/adm/simul_efun/compatibility.c"
#include "/adm/simul_efun/kewlp.c"
#include "/adm/simul_efun/superuserp.c"
#include "/adm/simul_efun/runnerp.c"
#include "/adm/simul_efun/lawp.c"

/*
//  Added by SoulBlighter on 20130122
*/
#include "/adm/simul_efun/file_lines.c"
#include "/adm/simul_efun/tail.c"

// squid
#include "/adm/simul_efun/trim.c"
#include "/adm/simul_efun/get_stack.c"
