// please assume this file as the main file of the c module created 
// location of the module will be as per your setup
// if the installation of the freeswitch is source based then module will be kept in "/usr/src/freeswitch/src/mod/application"
// if the installation of the freeswitch is package based then module will be kept in "/usr/src/freeswitch"

#include <switch.h>
#include <esl.h>
#include <string.h>

SWITCH_MODULE_LOAD_FUNCTION(mod_custom_ivr_load);
SWITCH_MODULE_DEFINITION(mod_custom_ivr, mod_custom_ivr_load, NULL, NULL);

static switch_status_t custom_ivr_function(switch_core_session_t *session, const char *data) {
    switch_channel_t *channel = switch_core_session_get_channel(session);
    const char *uuid = switch_core_session_get_uuid(session);

    switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_NOTICE, "custom_ivr: Session started for UUID: %s\n", uuid);

    // ESL connection
    esl_connection_t conn;
    if (esl_connect(&conn, "127.0.0.1", 8021, "ClueCon") != ESL_SUCCESS) {
        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "ESL connection failed\n");
        switch_channel_hangup(channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
        return SWITCH_STATUS_FALSE;
    }

    switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "custom_ivr: ESL connected\n");

    // Fire custom start event
    esl_event_t *start_event = esl_event_create(ESL_EVENT_CUSTOM, "ivr::start");
    esl_event_add_header_string(start_event, ESL_STACK_BOTTOM, "UUID", uuid);
    esl_event_add_header_string(start_event, ESL_STACK_BOTTOM, "Caller-ID", switch_channel_get_variable(channel, "caller_id_number"));
    esl_send_event(&conn, start_event);
    esl_event_destroy(&start_event);

    // Play welcome
    switch_ivr_play_file(session, NULL, "/usr/share/freeswitch/sounds/en/us/callie/ivr/ivr-welcome_to_freeswitch.wav", NULL);

    // Digit input loop
    int tries = 0;
    int max_tries = 3;
    char dtmf[2] = "";
    switch_status_t status;

    while (tries < max_tries) {
        memset(dtmf, 0, sizeof(dtmf));
        status = switch_ivr_play_and_get_digits(session, 1, 1, 1, 5000, "#", 
            "/usr/share/freeswitch/sounds/en/us/callie/ivr/ivr-please_press_a_key.wav",
            "/usr/share/freeswitch/sounds/en/us/callie/ivr/ivr-that_was_an_invalid_entry.wav",
            "\\d", dtmf, sizeof(dtmf), NULL);

        if (status != SWITCH_STATUS_SUCCESS || dtmf[0] == '\0') {
            switch_ivr_play_file(session, NULL, "/usr/share/freeswitch/sounds/en/us/callie/ivr/ivr-no_entry_received.wav", NULL);
            switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING, "No input or error\n");
            tries++;
        } else if (dtmf[0] == '1' || dtmf[0] == '2') {
            switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_NOTICE, "User selected option: %c\n", dtmf[0]);
            break;
        } else {
            switch_ivr_play_file(session, NULL, "/usr/share/freeswitch/sounds/en/us/callie/ivr/ivr-that_was_an_invalid_entry.wav", NULL);
            switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING, "Invalid input: %c\n", dtmf[0]);
            tries++;
        }
    }

    // Send input event
    esl_event_t *input_event = esl_event_create(ESL_EVENT_CUSTOM, "ivr::input_received");
    esl_event_add_header_string(input_event, ESL_STACK_BOTTOM, "UUID", uuid);
    esl_event_add_header_string(input_event, ESL_STACK_BOTTOM, "Digit", *dtmf ? dtmf : "none");
    esl_event_add_header_string(input_event, ESL_STACK_BOTTOM, "Valid", (*dtmf == '1' || *dtmf == '2') ? "true" : "false");
    esl_send_event(&conn, input_event);
    esl_event_destroy(&input_event);

    if (*dtmf == '1') {
        switch_ivr_speak_text(session, NULL, "You selected option one. Transferring to sales.");
    } else if (*dtmf == '2') {
        switch_ivr_speak_text(session, NULL, "You selected option two. Transferring to support.");
    } else {
        switch_ivr_play_file(session, NULL, "/usr/share/freeswitch/sounds/en/us/callie/ivr/ivr-you_have_reached_the_maximum_attempts.wav", NULL);
        switch_channel_hangup(channel, SWITCH_CAUSE_NORMAL_CLEARING);
    }

    // Hangup cause
    const char *hangup_cause = switch_channel_cause2str(switch_channel_get_cause(channel));
    switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_NOTICE, "Call ended with cause: %s\n", hangup_cause);

    esl_event_t *hangup_event = esl_event_create(ESL_EVENT_CUSTOM, "ivr::hangup");
    esl_event_add_header_string(hangup_event, ESL_STACK_BOTTOM, "UUID", uuid);
    esl_event_add_header_string(hangup_event, ESL_STACK_BOTTOM, "Hangup-Cause", hangup_cause);
    esl_send_event(&conn, hangup_event);
    esl_event_destroy(&hangup_event);

    esl_disconnect(&conn);
    return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_LOAD_FUNCTION(mod_custom_ivr_load) {
    switch_application_interface_t *app_interface;

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Loading mod_custom_ivr\n");

    switch_api_interface_t *commands = NULL;
    *module_interface = switch_loadable_module_create_module_interface(pool, modname);
    SWITCH_ADD_APP(app_interface, "custom_ivr", "Custom IVR", "Plays an IVR with ESL logic", custom_ivr_function, "", SAF_NONE);

    return SWITCH_STATUS_SUCCESS;
}

