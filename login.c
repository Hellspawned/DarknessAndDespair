//      /adm/obj/login.c
//      The object handling user connections
//
//      Biker (941212) watch sites and allow a wizard only to logon from a specific
//                     set of sites listed in /adm/site_watch/<name>.ip
//      Ari   (950427) implemented virtual players, added cap_name support

#include <config.h>
#include <news.h>
#include <flags.h>
#include <security.h>
#include <daemons.h>
#include <objects.h>
#include <dirs.h>

/* Define this if you want to active Biker's login size check for wizards */
#undef DO_SITE_CHECK
/* ARI: Turn this on if you suspect SYN FLOOD attacks or such */
#undef DO_LOG
 
static private int __CrackCount, __MXP;
static private string __Name, __TestName, __TermType;
static private object __Player;
 
static void logon();
static void get_name(string str);
static void get_password(string str);
static void get_wizard_password(string str);
static private int locked_access();
static private int check_password(string str);
static private int check_tester_password(string str);
static private int valid_site(string ip);
static private int is_copy();
static void disconnect_copy(string str, object ob);
static private void exec_user();
static void new_user(string str);
static void choose_password(string str);
static void confirm_password(string str2, string str1);
static void choose_gender(string str);
static void enter_email(string str);
static void enter_real_name(string str);
static void idle();
static void receive_message(string cl, string msg);
static private void internal_remove();
void remove();


#ifdef DO_LOG
void log_login(string str)
{
    secure_log_file( "login_log", str);
}
#else
#define log_login(x) /**/
#endif

void create() 
{
   seteuid(UID_ROOT);
   __Name = "";
   __CrackCount = 0;
   __Player = 0;
   __MXP = 0;
   __TermType = "unknown";
}
 
static void logon() 
{
    log_login( "\tvoid logon() - Initiating callout of idle in " + LOGON_TIMEOUT+ " seconds.\n" );
    call_out("idle", LOGON_TIMEOUT);
    if(catch(__Player = new(OB_USER))) 
    {
	log_login( "\tvoid_logon() - Couldn't log in due to user.c edit.\n" );
	    message("logon", "Someone appears to be messing with the user object.\n"
		    "Please try again in a few minutes.\n", this_object());
        internal_remove();
        return;
    }
    destruct(__Player);
    log_login( "\tvoid_logon() - user.c loads ok...\n" );
    __Player = 0;
    log_login( "\tvoid_logon() - reading welcome file.\n" );
    message("logon", read_file(WELCOME), this_object());
    log_login( "\tvoid_logon() - Printing driver version.\n" );
    message("logon", sprintf("\n        Driver: %s        Mudlib: %s %s\n",
            version(), mudlib(), mudlib_version()), this_object());
    log_login( "\tvoid_logon() - Prompting for name.\n" );
    message("logon", "\nWhat name do you wish? ", this_object());
    log_login( "\tvoid_logon() - Calling get_name().\n" );
    input_to("get_name");
}

// called from fluffos 
static void terminal_type(string str) {
	if (!strlen(str)) {
		__TermType = "unknown";
	}
	else {
		__TermType = lower_case(str);
	}
	
}
static void mxp_enable() {
	//__MXP = 1;
}

static void get_name(string str) {
    string wiz_lock, pos;
 
    log_login( "\tvoid get_name() - Beginning Function.\n" );

    if(!str || str == "") {
    	log_login( "\tvoid get_name() - No name passed - Removing..\n" );
    	message("logon", "\nInvalid entry.  Please try again.\n", this_object());
    	internal_remove();
    	log_login( "\tvoid get_name() - User removed successfully.\n" );
    	return;
    }
    log_login( "\tvoid get_name() - Converting " + str + " to an uncaped name.\n" );
    __Name = convert_name(str);


    if (!PLAYTIME_D->allow_login(__Name, query_ip_number())) {
      message("logon", "\nAdmins have placed you on playtime restriction. You are able to log in during the following times:\n", this_object());
      message("logon", PLAYTIME_D->schedule_string(__Name) + "\n", this_object());
      internal_remove();
      return; 
    }
    
    log_login( "\tvoid get_name() - Reading wizlock news.\n" );
    wiz_lock = read_file(WIZLOCK_NEWS);
    log_login( "\tvoid get_name() - Wizlock news read.\n" );
    if (wiz_lock) 
    {
	message("logon", wiz_lock, this_object());
        pos = (string)USERIO_D->query_position(__Name);
        if (sscanf(__Name,"%stest", __TestName) != 1)
	    __TestName = 0;
        if (!locked_access()) {  // arches my always log in.
	    if (!pos || member_array(pos, MORTAL_POSITIONS) != -1) {
		if (__TestName) {
		    pos = (string)USERIO_D->query_position(__TestName);
		    if (pos && member_array(pos, MORTAL_POSITIONS) == -1) {
                        message("logon", "\n    Test character access allowed. Give your wizard's password now."
					    "\n\nPassword: ",this_object());
			input_to("get_wizard_password", I_NOECHO | I_NOESC);
                        return;
		    }
		} 
                message("logon", "\n    >>> Access denied <<<\n", this_object());
                internal_remove();
                return;
	    }
        }
        message("logon", "\n    >>> Access allowed <<<\n", this_object());
    }
    if((int)master()->is_locked()) {
    	wiz_lock = read_file(LOCKED_NEWS);
    	if (wiz_lock)
    	    message("logon", wiz_lock, this_object());
            if(locked_access())
    	    message("logon", "\n    >>> Access allowed <<<\n", this_object());
            else {
                message("logon", "\n    >>> Access denied <<<\n", this_object());
                internal_remove();
                return;
    	}
    }

    log_login( "\tvoid get_name() - Checking to see if arrested.\n" );
    if(file_exists(DIR_USERS+"/arrest/"+str+".o")) 
    {
        log_login( "\tvoid get_name() - User was arrested, Printing reason.\n" );
        message("logon", sprintf("\n%s is currently arrested for %s.\n",
              capitalize(__Name), mud_name()), this_object());
	log_login( "\tvoid get_name() - Reason printed.\n" );
        message("logon", "\n    >>> Access denied <<<\n", this_object());
	log_login( "\tvoid get_name() - Printed access denied.\n" );
        internal_remove();
	log_login( "\tvoid get_name() - User removed successfully.\n" );
        return;
    }

    log_login( "\tvoid get_name() - calling user_exists().\n" );

    if(!user_exists(__Name)) 
    {
	log_login( "\tvoid get_name() - User exists.  Checking for valid name.\n" );

        if(!((int)BANISH_D->valid_name(__Name))) 
	{
            log_login( "\tvoid get_name() - User name was invalid.\n" );

            message("logon", sprintf("\n%s is not a valid name choice for %s.\n",
		  capitalize(__Name), mud_name()), this_object());

            log_login( "\tvoid get_name() - Told user it was invalid.\n" );

            message("logon", sprintf("Names must be alphabetic characters no "
		  "longer than %d letters,\nand no less than %d letters.\n",
		  MAX_USER_NAME_LENGTH, MIN_USER_NAME_LENGTH), this_object());

            log_login( "\tvoid get_name() - Told user name rules.\n" );

            message("logon", "\nPlease enter another name: ", this_object());
            log_login( "\tvoid get_name() - Prompted for another name.  Calling get_name().\n" );
            input_to("get_name");
            return;
	}
 
        log_login( "\tvoid get_name() - Checking to see if allowed even though banished.\n" );


	if(!((int)BANISH_D->allow_logon(__Name, query_ip_number()))) 
	{
	    log_login( "\tvoid get_name() - Banished login ok for " + query_ip_number() + ".\n" );

            message("logon", read_file(REGISTRATION_NEWS), this_object());

            log_login( "\tvoid get_name() - Read registration news for user.\n" );

            internal_remove();
            log_login( "\tvoid get_name() - User removed successfully.\n" );
            return;
	}

	log_login( "\tvoid get_name() - Asking if name is really waht they want..\n" );

        message("logon", sprintf("Do you really wish %s to be your name? (y/n) ",
	      capitalize(__Name)), this_object());

	log_login( "\tvoid get_name() - Passing input to new_user().\n" );

        input_to("new_user");
        return;
    }

    if(!((int)BANISH_D->allow_logon(__Name, query_ip_number()))) 
    {
	log_login( "\tvoid get_name() - Banished user was not allowed from " + query_ip_number() + ".\n"
                "\t\tReading banished news.\n" );
 
        message("logon", read_file(BANISHED_NEWS), this_object());
	log_login( "\tvoid get_name() -Banished news read, removing object.\n" );

        internal_remove();
	log_login( "\tvoid get_name() - User removed successfully.\n" );

        return;
    }
    log_login( "\tvoid get_name() - Prompting for password.\n" );

    message("logon", "Password: ", this_object());
    log_login( "\tvoid get_name() - Calling get_password.\n" );

    input_to("get_password", I_NOECHO | I_NOESC);
}
 
static void get_password(string str) {
	string tmp;
	log_login( "\tstatic void get_password() - Entering get_password().\n" );

    	if (!str || str == "") {
		log_login( "\tstatic void get_password() - No string was passed..\n" );

        	message("logon", "\nYou must enter a password.  Try again later.\n",
	          this_object());

		log_login( "\tstatic void get_password() - Told them to try again later.\n" );

        	internal_remove();
		log_login( "\tstatic void get_password() - User removed successfully.\n" );

        	return;
    	}
    	if (!check_password(str)) {
		log_login( "\tstatic void get_password() - Password entered was invalid.\n" );
       	message("logon", "\nInvalid password.\n", this_object());

        	if (++__CrackCount > MAX_PASSWORD_TRIES) {
         		log_login( "\tstatic void get_password() - Crack count was too high.\n" );

	   		message("logon", "No more attempts allowed.\n", this_object());
	    		log_login( "\tstatic void get_password() - Crack message sent.\n" );
            	internal_remove();

	    		log_login( "\tstatic void get_password() - User removed successfully.\n" );
               return;
		}
		
		log_login( "\tstatic void get_password() - Logging login.\n" );
        	seteuid(UID_LOG);
        	log_file("watch/logon", sprintf("%s from %s\n", __Name, query_ip_number()));
        	seteuid(getuid());
		log_login( "\tstatic void get_password() - Logged.\n" );
          log_login( "\tstatic void get_password() - Prompting for password again.\n" );

        	message("logon", "Password: ", this_object());
		log_login( "\tstatic void get_password() - calling get_password().\n" );

        	input_to("get_password", I_NOECHO | I_NOESC);
        	return;
	}

	// prevent alts from logging in
	if (tmp = (string)MULTI_D->query_prevent_login(__Name)) {
		receive("\n"+tmp);
        	internal_remove();
        	return;		
	}
	
    	log_login( "\tstatic void get_password() - Checking to see if !is_copy.\n" );
    	if (!is_copy()) {
		log_login( "\tstatic void get_password() - Execing user.\n" );
		exec_user();
	}
}

static void get_wizard_password(string str) 
{
    if(!str || str == "") {
	message("logon", "\nYou must enter the password.  Try again later.\n", this_object());
	internal_remove();
	return;
    }
    if(!check_tester_password(str)) {
        message("logon", "\nInvalid password.\n", this_object());
        if(++__CrackCount > MAX_PASSWORD_TRIES) {
            message("logon", "No more attempts allowed.\n", this_object());
            internal_remove();
            return;
        }
        seteuid(UID_LOG);
        log_file("watch/logon", sprintf("%s [Testchar] from %s\n", __Name, query_ip_number()));
        seteuid(getuid());
        message("logon", "Password: ", this_object());
        input_to("get_wizard_password", I_NOECHO | I_NOESC);
        return;
    }
    if(!user_exists(__Name)) {
	if(!((int)BANISH_D->allow_logon(__Name, query_ip_number()))) {
	    message("logon", read_file(REGISTRATION_NEWS), this_object());
            internal_remove();
            return;
	}
        message("logon", sprintf("\n\n    Do you really want to create a %s character? (y/n) ",
          capitalize(__Name)), this_object());
        input_to("new_user");
        return;
    }
    if(!((int)BANISH_D->allow_logon(__Name, query_ip_number()))) {
	message("logon", read_file(BANISHED_NEWS), this_object());
	internal_remove();
	return;
    }
    message("logon", "\n\n    Now give your test character password.\n\nPassword: ", this_object());
    input_to("get_password", I_NOECHO | I_NOESC);
    return;
}
 
static private int locked_access() 
{
    int i;
 
    if((int)BANISH_D->is_guest(__Name)) return 1;
    i = sizeof(LOCKED_ACCESS_ALLOWED);
    while(i--) if(member_group(__Name, LOCKED_ACCESS_ALLOWED[i])) return 1;
    return 0;
}
 
static private int check_password(string str) 
{
    string pass;
    int valid, complete;
 
    log_login( "\tprivate int check_password() - Enter function.\n" );

    log_login( "\tprivate int check_password() -Player = new( OB_USER).\n" );

    __Player = new(OB_USER);

    log_login( "\tprivate int check_password() - Loading player from master obj func.\n" );

    master()->load_player_from_file(__Name, __Player);

    log_login( "\tprivate int check_password() - Querying password from master.\n" );

    pass = (string)__Player->query_password();
    complete = (int)__Player->query_email_set();
    valid = 0;
    if (pass == crypt(str, pass))
    {
        log_login( "\tprivate int check_password() - calling valid_site().\n" );
        valid = valid_site(query_ip_number());
    }

    log_login( "\tprivate int check_password() - Destructing player.\n" );

    destruct(__Player);
    log_login( "\tprivate int check_password() - Player destructed.\n" );

    __Player = 0;
    log_login( "\tprivate int check_password() - Retruning valid = " + valid + ".\n" );

    if (valid && !complete)
	valid++;
    return valid;
}
 
static private int check_tester_password(string str) 
{
    string pass;
    int valid;
 
    __Player = new(OB_USER);
    master()->load_player_from_file(__TestName, __Player);
    pass = (string)__Player->query_password();
    valid = 0;
    if (pass == crypt(str, pass))
        valid = valid_site(query_ip_number());
    destruct(__Player);
    __Player = 0;
    return valid;
}
 
static private int valid_site(string ip) 
{
    string a, b;
    string *miens;
    int i;
 
    if(!(i = sizeof(miens = (string *)__Player->query_valid_sites()))) return 1;
    while(i--) {
        if(ip == miens[i]) return 1;
        if(sscanf(miens[i], "%s.*s", a) && sscanf(ip, a+"%s", b)) return 1;
    }
    return 0;
}
 
static private int is_copy() {
    object ob;
 
    if(!(ob = find_player(__Name))) return 0;
    if(interactive(ob)) {
        message("logon", "\nThere currently exists an interactive copy of you.\n",
          this_object());
        message("logon", "Do you wish to take over this interactive copy? (y/n) ",
          this_object());
        input_to("disconnect_copy", I_NORMAL, ob);
        return 1;
    }
    seteuid(UID_LOG);
    log_file("enter", sprintf("%s (exec): %s\n", __Name, ctime(time())));
    seteuid(getuid());
    if(exec(ob, this_object())) ob->restart_heart();
    else message("logon", "Problem reconnecting.\n", this_object());
    internal_remove();
    return 1;
}
 
static void disconnect_copy(string str, object ob) {
    object tmp;
    string ipstr;
 
    if((str = lower_case(str)) == "" || str[0] != 'y') {
        message("logon", "\nThen please try again later!\n", this_object());
        internal_remove();
        return;
    }
    if(wizardp(ob)) {
    /*
     ipstr = query_ip_number();
         if (ipstr != "127.0.0.1" && ipstr[0..7] != "192.168." &&
	    (archp(ob) && ipstr != query_ip_number(ob))) {
            message("info",
                "%^RED%^%^FLASH%^"
                "\nWARNING!!! WARNING!!! WARNING!!!\n"
                "%^RESET%^%^WHITE%^%^BOLD%^"
                "Someone just attempted to take over your character!\n"
                "This attempt occured from the following address: "
                "%^RESET%^%^BLUE%^%^BOLD%^"+
                query_ip_number()+".%^RESET%^ ", ob);
            receive("\n");
            message("logon", "       >>> Access Denied <<<\n\n", this_object());
            write_file("/adm/log/HACKS",  "Takeover of "+__Name+" ("+query_ip_number(ob)+
                ") attempted from "+query_ip_number()+" - "+ctime(time())+".\n");
            internal_remove();
            return;
        } else {*/
            write_file("/adm/log/HACKS", "Takeover of "+__Name+" ("+query_ip_number(ob)+
                ") ALLOWED from "+query_ip_number()+" - "+ctime(time())+".\n");
        //}
    }
    message("info", "You are being taken over by hostile aliens!", ob);
    exec(tmp = new(OB_USER), ob);
    exec(ob, this_object());
    destruct(tmp);
    message("logon", "\nAllowing login.\n", ob);
    internal_remove();
}


#ifdef DO_SITE_CHECK
void check_for_site()
{
  // Biker: check for alternating IP numbers
    string pos = (string)USERIO_D->query_position(__Name);
    if ( pos && member_array(pos, MORTAL_POSITIONS) == -1 )
      {
      string s = "/adm/site_watch/";
      string this_ip = query_ip_number();
      string *this_digits = explode( this_ip, "." );
      int valid_ip = 0;
      s += __Name[0..0] + "/" + __Name + ".ip";
      if ( file_exists( s ))
        {
          string *org_ip = explode( read_file( s ), "\n" );
          int i = sizeof( org_ip );
          while ( i-- )
            {
              int x;
              string *org_digits = explode( org_ip[i], "." );
              int digits = sizeof( org_digits );
              if ( digits > sizeof( this_digits ))
                digits = sizeof( this_digits );
              valid_ip = 1;
              for ( x = 0; x < digits-1; x++ ) /* last digit irrelevant */
                valid_ip &= ( 
                             ( org_digits[x] == "*" )
                             ||
                             ( org_digits[x] == this_digits[x] )
                            );
              if ( valid_ip )
                break;
            }
        }
      else
        {
          write_file( s, this_ip + "\n" );
          valid_ip = 1;
        }
      if ( !valid_ip )
        {
          message("logon", "The site: "+query_ip_number()+" is an unusual site "
                  "for this account.\n"
                  "Please mail: dnd@spud.org to register "
                  "this site or\nlogin from your normal location.\n",
                  this_object() );
          write_file( "/adm/site_watch/Intruder.log", capitalize(__Name) + 
                     " tried to login from "+
                     query_ip_name() + "( " + query_ip_number() + " ) at " +
                     ctime( time() ) + ".\n" );
          destruct( this_object() );
          return;
        }
      }
}  
#endif DO_SITE_CHECK
 
 
static private void exec_user() {
	string tmp;

    	receive("\n");

    
#ifdef DO_SITE_CHECK
    check_for_site();
#endif
    if (__Player) // new user finaly arrived here - make it a virtual beeing
        destruct(__Player);
    if (catch(__Player = load_object(DIR_USERS+"/"+__Name[0..0]+"/"+__Name))) {
    	__Player = 0;
	}
    else {
        if (master()->load_player_from_file(__Name, __Player)) {
        	__Player->set_name(__Name);
		}
    }
    if(!__Player || !exec(__Player, this_object())) {
        message("logon", "Problem connecting.\n", this_object());
        internal_remove();
        return;
    }
    __Player->setup();
    
    // set the players terminal type to the type reported from negotiation
    if (strlen(__TermType)) {
	 __Player->terminal_type(__TermType);
    }
/*    if (__MXP) {
		__Player->enable_mxp();
    }
    else {
    		__Player->disable_mxp();
    }*/
    destruct(this_object()); 
} 
 
static void new_user(string str) { 
    if((str = lower_case(str)) == "" || str[0] != 'y') { 
        message("logon", "\nOk, then enter the name you really want: ", this_object()); 
        input_to("get_name"); 
        return; 
    } 
    if(file_size(DIR_USERS+"/rid/"+__Name+".o") > -1) {
        message("logon", "\nYour character was removed this day!  Try again after the reboot.\n", this_object()); 
        internal_remove();
        return; 
    }
    __Player = new(OB_USER);
    __Player->set_name(__Name);         // prepare the save in set_password()
    seteuid(UID_LOG); 
    log_file("new_players", sprintf("%s : %s : %s\n", query_ip_number(), __Name,  
      ctime(time()))); 
    seteuid(getuid()); 
    message("logon", "Please choose a password of at least 5 letters: ", 
      this_object()); 
    input_to("choose_password", I_NOECHO | I_NOESC);
} 
 
static void choose_password(string str) { 
    if(strlen(str) < 5) { 
        message("logon", "\nYour password must be at least 5 letters long.\n", 
          this_object()); 
        message("logon", "Please choose another password: ", this_object()); 
        input_to("choose_password", I_NOECHO | I_NOESC); 
    } 
    message("logon", "\nPlease confirm your password choice: ", this_object()); 
    input_to("confirm_password", I_NOECHO | I_NOESC, str);
} 
 
static void confirm_password(string str2, string str1) { 
    if(str1 == str2) { 
        __Player->set_password(str2 = crypt(str2, 0)); 
        message("logon", "\nPlease choose an interesting gender (male or female): ", 
          this_object()); 
        input_to("choose_gender"); 
        return; 
    } else { 
        message("logon", "\nPassword entries do not match.  Choose a password: ", 
          this_object()); 
        input_to("choose_password", I_NOECHO | I_NOESC); 
        return; 
    } 
} 
 
static void choose_gender(string str) { 
    if(str == "m") str = "male";
    if(str == "f") str = "female";
    if(str != "male" && str != "female") { 
        message("logon", "\nCute, but pretend to be either male or female instead\n", 
          this_object()); 
        message("logon", "Gender: ", this_object()); 
        input_to("choose_gender"); 
        return; 
    } 
    __Player->set_gender(str); 
    message("logon", sprintf("You may format %s to appear however you want "
      "using alternative\ncapitalization, spaces, \"'\", or \"-\".\n", capitalize(__Name)), 
        this_object());
    message("logon", sprintf("Please choose a display name (default: %s): ",
         capitalize(__Name)), this_object());
    input_to("choose_cap_name");
}

static void choose_cap_name(string str) {
    if(!str || str == "") str = capitalize(__Name);
    if(!((int)BANISH_D->valid_cap_name(str, __Name))) {
        message("logon", "Incorrect format.  Choose again: ", this_object());
        input_to("choose_cap_name");
        return;
    }
    __Player->set_cap_name(str); 
    message("logon", sprintf("\nFor security reasons only, %s requires you to enter "
      "\na valid email address in the form of: USER@HOST. "
      "\nOnly those of the administration of this mud will have access to "
      "\nthis information so please answer as honestly as possible.\n"
      "\nEmail: ", mud_name()), this_object());
    input_to("enter_email"); 
} 
 
static void enter_email(string str) { 
    string a, b; 
 
    if(!str || str == "" || sscanf(str, "%s@%s", a, b) != 2) { 
        message("logon", "\nEmail must be in the form USER@HOST.\nEmail: ", 
          this_object()); 
        input_to("enter_email"); 
        return;
    } 
    __Player->set_email(str); 
    message("logon", "\nIf you do not mind, enter your real name (optional): ", 
      this_object()); 
    input_to("enter_real_name"); 
} 
 
static void enter_real_name(string str) { 
    if(!str || str == "") str = "Unknown"; 
    __Player->set_rname(str); 
    seteuid(UID_LOG); 
    log_file("enter", sprintf("%s (new player): %s\n", __Name, ctime(time()))); 
    seteuid(getuid()); 
    exec_user(); 
} 
 
static void idle() { 
    message("logon", "\nLogin timed out.\n", this_object()); 
    internal_remove();
} 
 
static void receive_message(string cl, string msg) { 
    if(cl != "logon") return; 
    receive(msg); 
} 
 
static private void internal_remove() {
    if(__Player) destruct(__Player);
    destruct(this_object());
}
 
void remove() {
    if(geteuid(previous_object()) != UID_ROOT) return;
    internal_remove();
}
