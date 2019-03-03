#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <security/pam_appl.h>

struct pam_response *reply;

int function_conversation(int num_msg, const struct pam_message **msg, struct pam_response **resp, void *appdata_ptr) {

	*resp = reply;
	return PAM_SUCCESS;

}

int pam_auth_check(const char *username, const char *password) {

	const struct pam_conv local_conversation = { function_conversation, NULL };
    pam_handle_t *local_auth_handle = NULL;
	int retval;

	retval = pam_start("common-auth", username, &local_conversation, &local_auth_handle);

	if (retval != PAM_SUCCESS)
		return 0;

	reply = (struct pam_response *)malloc(sizeof(struct pam_response));

	reply[0].resp = strdup(password);
	reply[0].resp_retcode = 0;
	retval = pam_authenticate(local_auth_handle, 0);

//	free(reply[0].resp);
//	free(reply);

	if (retval != PAM_SUCCESS)
		return 0;

	retval = pam_end(local_auth_handle, retval);

//	if (retval != PAM_SUCCESS)
//		return 0;

	return 1;

}
