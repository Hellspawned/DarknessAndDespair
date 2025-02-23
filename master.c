//      /adm/obj/master.c
//      from the Nightmare Mudlib
//      the mudlib master object
//      created by Descartes of Borg 03 august 1993
//      based upon master objects going back to mudlib.n

//              Nightmare Mudlib Version 3.0 Release 0.9.18
//              created by the founding arches of Nightmare MudOS
//              Darkone, Descartes, Flamme, Forlock, Shadowwolf
//              See detailed credits in /doc/mudlib/credits
// 		Error handler by Beek, as of Foundation B1
//		Virtual player handling and various access right
//		modifications by Ari@DnD


// This is the master object, the second object loaded after the
// simul_efun object.  Everything written with 'write()' at startup
// will be printed to stdout.  At startup, the following functions
// will be called in the following order:
// 1) create() 
// 2) flag() will be called once for every argument to the command flag -f
// 3) epilog()
// Finally, the game will enter multiuser mode and users can login.
// At this point preload() gets called once for each object to be
// preloaded.
//    *** Warnings ***
// Do not use any instructions in create() which will need to make
// calls externally to the master object.  For example, do not
// try seteuid(UID_ROOT) since the seteuid() efun has the driver call
// valid_seteuid() in the master object.  A master object will load fine
// and start the mud if it has such a call, but you will crash the
// mud if you try to reload it since it will not be able to find
// the master object (i.e no master exists until create() is done).

#include <config.h>
#include <objects.h>
#include <rooms.h>
#include <security.h>
#include <dirs.h>
#include <databases.h>
#include <daemons.h>
#include <law.h>

#define READ 0
#define WRITE 1

static mapping access, groups, privs;

string creator_file(string str);
int player_exists(string str);
void preload(string str);
void load_groups();
void load_privs();
int query_member_group(string who, string grp);

void flag(string str) {
    string file, arg;
    int i, x;

    if(previous_object()) return;
    if(sscanf(str, "for %d", x) == 1) {
        for(i=0; i<x; i++) {}
        return;
    }
    if(sscanf(str, "call %s %s", file, arg)) {
        write("Got "+(string)call_other(file, arg)+" back.\n");
        return;
    }
    write("Master: unknown flag.\n");
}

string *epilog(int x) {
    call_out("socket_preload", 5);
    return read_database(PRELOAD_DB);
}

void load_groups() {
    string *lines;
    string grp, str;
    int i, max;

    groups = ([]);
    if(!(max=sizeof(lines=explode(read_file(GROUPS_DB), "\n")))) {
        write("Error in reading groups database.\n");
        shutdown();
        return;
    }
    for(i=0; i<max; i++) {
        if(!lines[i] || lines[i] == "" || lines[i][0] == '#') continue;
        if(sscanf(lines[i], "(%s): %s", grp, str) != 2) {
            write("Error in reading groups database in line "+(i+1)+".\n");
            shutdown();
            return;
        }
        if(!sizeof(groups[grp] = explode(str, " ")))
          map_delete(groups, grp);
    }
}

void load_privs() {
    string *lines;
    string obj, euid;
    int i, max;

    privs = ([]);
    if(!(max = sizeof(lines=explode(read_file(PRIVS_DB), "\n")))) {
        write("Error in reading privs database.\n");
        shutdown();
        return;
    }
    for(i=0; i<max; i++) {
        if(!lines[i] || lines[i] == "" || lines[i][0] == '#') continue;
        if(sscanf(lines[i], "(%s): %s", obj, euid) != 2) {
            write("Error in reading privs database at line "+(i+1)+".\n");
            shutdown();
            return;
        }
        if(!sizeof(privs[obj] = explode(euid, " ")))
          map_delete(privs, obj);
    }
}

void preload(string str) {
    string err;
    int t;

    if(!file_exists(str+".c")) return;
    t = time();
    write("Preloading: "+str+"...");
    if(err=catch(call_other(str, "???")))
      write("\nGot error "+err+" when loading "+str+".\n");
    else {
        t = time() - t;
        write("("+(t/60)+"."+(t%60)+")\n");
    }
}

void socket_preload() {
    string *items;
    int i, max;

    max = sizeof(items=explode(read_file(PRELOAD_SOCKET_DB), "\n"));
    for(i=0; i<max; i++) {
        if(!items[i] || items[i] == "" || items[i][0] == '#') continue;
        catch(call_other(items[i], "???"));
    }
}

int valid_write(string file, object ob, string fun) {
    if (ob == master()) return 1;
    return SECURITY_D->check_access(file, ob, WRITE);
}

int valid_read(string file, object ob, string fun) {
    if (ob == master()) return 1;
    return SECURITY_D->check_access(file, ob, READ);
}

object connect() {
    object ob;
    string err;

    if(err=catch(ob = clone_object(OB_LOGIN))) {
        write("It looks like someone is working on the user object.\n");
        write(err);
        destruct(this_object());
    }
    return ob;
}

mixed compile_object(string file) {
    object ob;
    int l;
    string str, tmp;
    
    // check for virtual users first
    l = strlen(file);


    if (l > 18) {
		if (file[0..15] ==  "/adm/save/users/") {
		    tmp = file[16..17];
		    str = file[18..(l-1)];
		    if (tmp == str[0..0] + "/" && player_exists(str)) 
			if (!catch(ob = clone_object(OB_USER)))
			    return ob;
		    return 0;
		}
    }
    return (mixed)VIRTUAL_D->compile_object(file);
}

static void crash(mixed args...) {
     string err = "";
     string guilty_stack = get_stack();
     string guilty_obs = identify(all_previous_objects());
     string command_giver = "";
     string current_object = "";
     string command_history = "";
     
     if (sizeof(args)) err = args[0];
     if (sizeof(args) >= 3) {
          if (args[1]) {
               command_giver = identify(args[1]);
               if (environment(args[1])) {
                    command_giver += " at " + identify(environment(args[1]));
               }
          }
          command_history = identify(args[1]->query_history());
          current_object = identify(args[2]);
          
     } else if (this_player()) {
          command_giver = identify(this_player());
          command_giver += " at " + identify(environment(this_player()));
          
          command_history = identify(this_player()->query_history());
     }
     log_file("crashes",
          mud_name() + " crashed " + ctime(time()) + " with error " +
          err+".\nstack:"+guilty_stack+
          "\nobjects:"+guilty_obs+
          "\ncommand giver:"+command_giver + 
          "\ncommand history:"+command_history +
          "\ncurrent object:" + current_object+"\n---\n");
            
     shout("%^BLUE%^MudOS announces: %^RESET%^I think that "+mud_name()+" is crashing!%^RESET%^");
     shout("%^BLUE%^Armageddon shouts: %^RESET%^It definitely wasn't me this time!");
     shout("%^RED%^MudOS forced you to: %^RESET%^quit\n");
     if (this_player()) {
          message("system", "MudOS tells you: This crash might have been caused by you!\n", this_player());
     }
     users()->force_me("quit");
}

int valid_shutdown(string euid) {
    log_file("shutdowns", mud_name()+" was shut down by "+
      (previous_object() ? (euid=geteuid(previous_object())) : euid)+" "+
      ctime(time())+"\n");
    if(euid != UID_ROOT && euid != UID_SHUTDOWN) return 0;
    return 1;
}

int valid_seteuid(object ob, string id) {
    string fn, uid;

    if(!ob) return 0;
    if((uid = getuid(ob)) == id) return 1;
    if(uid == UID_ROOT) return 1;
    if(file_name(ob) == OB_SIMUL_EFUN) return 1;
    if(uid == UID_SYSTEM && id != UID_ROOT && id != UID_BACKBONE)
      return 1;
    if(!privs) load_privs();
    return (privs[fn=base_name(ob)] && (member_array(id, privs[fn]) != -1));
}

int valid_shadow(object ob) {
    string shadower, *comps;
    int legal;

    shadower = base_name(previous_object());
    comps = explode(shadower,"/");
    legal = 0;
    if (!(getuid(ob) == UID_ROOT || ob->query_prevent_shadow()))
    {
	if ((sizeof(comps) == 3 && comps[0] == "adm" && comps[1] == "shadows") ||
	    (member_array(shadower,VALID_SHADOWS) != -1) ) legal = 1;
    }
    if (legal)
        write_file("/adm/log/shadows",
           shadower+": "+ob->query_name()+" "+ctime(time())+".\n");
    else
       write_file("/adm/log/ILLEGAL",
         shadower+" failed to shadow "+ob->query_name()+" "+ctime(time())+".\n");
    return legal;
}


int valid_snoop(object snooper, object snoopee) {
    if( !snoopee && snooper ) return 1;
    if( member_group(getuid(snooper), "admin")  &&
        !member_group(getuid(snoopee), "admin") ) return 1;
    if( member_group(getuid(snooper), "superuser") && 
        !member_group(getuid(snoopee), "superuser") ) return 1;
    if( archp(snooper) && !archp(snoopee) ) return 1;
//  Head arch of law can snoop even HA's, but not Admin - Vyle
    if( member_group(getuid(snoopee), "superuser") && 
        (member_group(getuid(snooper), "superuser") &&
         member_group(getuid(snooper), "law")) &&
        !member_group(getuid(snoopee), "admin") ) return 1;
//  Law arches can snoop other arches, but not HA's - Vyle
    if( !member_group(getuid(snoopee), "superuser") && archp(snooper) &&
        member_group(getuid(snooper), "law") ) return 1;
    if( (member_group(getuid(snooper), "mentor") ||
	     member_group(getuid(snooper), "advocate")) &&
	    !archp(snoopee) ) return 1;
    if( snoopee->query_snoop() ) {
        tell_object(snoopee, "You are now being snooped by "+
          (string)snooper->query_cap_name()+".\n");
        return 1;
    }
    return 0;
}

int valid_exec(string nom) {
    if(nom[0] != '/') nom = "/"+nom;
    if(nom == OB_LOGIN+".c") return 1;
    return 0;
}

int valid_hide(object who) {
    string str;

    if(!objectp(who)) return 0;
    str = geteuid(who);
    if(!str) str = getuid(who);
    return str == UID_ROOT || member_group(str, "superuser") ||
	(member_group(str, "law") && member_group(str, "assist"));
}

int valid_override(string file, string nom) {
    if(file == "/adm/simul_efun/overrides" || file == OB_SIMUL_EFUN) return 1;
    if(function_exists(nom, find_object(OB_SIMUL_EFUN))) return 0;
    return 1;
}

// ARI: caution: possible security hole!
//      Nightmare IV has the comment version, but that may break here.
int valid_socket(object ob, string fun, mixed *info) {
    return 1;
//    return (geteuid(ob) == UID_SOCKET);
}

int valid_function(function f) {
    // ARI: below does not work with new_functions enabled in v20 drivers
    // return (geteuid(f) == geteuid(f[0]));
    return 1;
}

int load_player_from_file(string nom, object ob) {
    int x;

    if(!ob) return 0;
    if(geteuid(previous_object()) != UID_ROOT) return 0;
    export_uid(ob);
    x = (int)ob->restore_player(nom);
    seteuid(nom);
    export_uid(ob);
    seteuid(UID_ROOT);
    return x;
}

void save_player_to_file(object ob) {
    string nom;
    int x;

    if(!ob || !(nom = (string)ob->query_true_name()))
	return;
    if(base_name(previous_object()) != OB_USER &&
	    base_name(previous_object()) != DIR_USERS+"/"+nom[0..0]+"/"+nom)
	return;
    seteuid(UID_ROOT);
    export_uid(ob);
    ob->actually_save_player(nom);
    seteuid(nom);
    export_uid(ob);
    seteuid(UID_ROOT);
}

string get_wiz_name(string file) {
    string nom, dir, tmp;

    if(file[0] != '/') file = "/"+file;
    if(sscanf(file, REALMS_DIRS+"/%s/%s", nom, tmp) == 2) return nom;
    if(sscanf(file, DOMAINS_DIRS+"/%s/%s", nom, tmp) == 2) return nom;
    sscanf(file, "/%s/%s", nom, tmp);
    return nom;
}

int different(string fn, string pr) {
    int tmp;

    sscanf(fn, "%s#%d", fn, tmp);
    fn += ".c";
    return (fn != pr) && (fn != ("/" + pr));
}

string trace_line(object obj, string prog, string file, int line) {
    string ret;
    string objfn = obj ? file_name(obj) : "<none>";

    ret = objfn;
    if (different(objfn, prog)) ret += sprintf(" (%s)", prog);
    if (file != prog) ret += sprintf(" at %s:%d\n", file, line);
    else ret += sprintf(" at line %d\n", line);
    return ret;
}

varargs
string standard_trace(mapping mp, int flag) {
    string obj, ret;
    mapping *trace;
    int i,n;

    ret = mp["error"] + "Object: " + trace_line(mp["object"], mp["program"], mp["file"], mp["line"]);
    ret += "\n";
    trace = mp["trace"];

    n = sizeof(trace);

    for (i=0; i<n; i++) {
    if (flag) ret += sprintf("#%d: ", i);
        ret += sprintf("'%s' at %s", trace[i]["function"], trace_line(trace[i]["object"],
		trace[i]["program"], trace[i]["file"], trace[i]["line"]));
    }
    return ret;
}

string error_handler(mapping mp, int caught) {
    string ret;

    ret = "---\n"+ctime()+"\n"+standard_trace(mp, 1);
    if (caught) {
        write_file("/log/catch", ret);
    } else {
        write_file("/log/runtime", ret);
    }
    if (this_player(1)) {
        /* If an object didn't load, they got compile errors; no need to
         * tell them about the error.
         */
        if (mp["error"][0..23] != "*Error in loading object")
	message("my_action",
		sprintf("%sTrace written to /log/%s\n", mp["error"], (caught ? "catch" : "runtime")),
		this_player(1));
    }
    return 0;
}

void log_error(string file, string msg) {
    string nom, home;

    if(this_player(1) && wizardp(this_player(1)))
       message("Nerror", msg, this_player(1));
    if(!(nom = get_wiz_name(file))) nom = "log";
    catch(write_file(DIR_ERROR_LOGS+"/"+nom, msg));
}

#if 1 // v20.21+ uses move_or_destruct() in the object itself 
void destruct_env_of(object ob) {
    if(!interactive(ob)) return;
    message("environment", "The world about you swirls into nothingness, as "
      "you are quickly teleported somewhere else.",ob);
    ob->move(ROOM_VOID);
}
#endif

// new syntax for 0.9.19.18 and above ( old still works ) :
// string make_path_absolute(string file,object command_giver, int write_flag);
string make_path_absolute(string file) {
    return resolv_path((string)this_player()->get_path(), file);
}

int player_exists(string str) {
    if (!str) return 0;
    return file_exists(DIR_USERS+"/"+str[0..0]+"/"+str+".o");
}

string get_root_uid() { return UID_ROOT; }

string get_bb_uid() { return UID_BACKBONE; }

string creator_file(string str) {
    return (string)call_other(OB_SIMUL_EFUN, "creator_file", str);
}

string domain_file(string str) {
    string nom, tmp;

    if(str[0] != '/') str = "/"+str;
    if(sscanf(str, DOMAINS_DIRS+"/%s/%s", nom, tmp) == 2) return nom;
    return 0;
}

string author_file(string str) {
    string nom, tmp;

    if(str[0] != '/') str = "/"+str;
    if(sscanf(str, REALMS_DIRS+"/%s/%s", nom, tmp) == 2) return nom;
    return 0;
}

static int slow_shutdown() {
    log_file("game_log", "Armageddon loaded by master: "+ctime(time())+".\n");
    new(OB_SHUT)->move(ROOM_START);
    SHUT_D->do_armageddon();
    return 1;
}

int save_ed_setup(object who, int code) {
    string file;

    if(!intp(code)) return 0;
    rm(file = user_path(getuid(who))+".edrc");
    return write_file(file, code+"");
}

int retrieve_ed_setup(object who) {
    string file;
    int x;

    if(!file_exists(file = user_path(getuid(who))+".edrc")) return 0;
    return atoi(read_file(file));
}

string get_save_file_name(string file) {
    string str;

    return (file_size(str=user_path(geteuid(this_player()))) == -2 ?
      str+"dead.edit" : DIR_TMP+"/"+geteuid(this_player())+".edit");
}

int is_locked() { return MUD_IS_LOCKED; }

int query_member_group(string who, string grp) {
    if(!groups) load_groups();
    if(groups[grp]) return (member_array(who, groups[grp]) != -1);
    else return 0;
}

mapping query_groups() {
    if(!groups) load_groups();
    return copy(groups);
}

#define KEWLDB "/wizards/zaknaifen/secure/kewl.db"

int is_kewl(object ob) {
    string *sp, *members;
    int i;

    if (catch(sp = read_database(KEWLDB)))
	return 0;
    members = ({ });
    if (sizeof(sp)) {
	for (i = 0; i < sizeof(sp); i++)
	    members += explode(sp[i],":");
	if (ob && sizeof(members) && member_array(ob->query_name(),members) != -1)
	    return 1;
    }
    return 0;
}
