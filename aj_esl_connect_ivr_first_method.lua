-- first method 
-- Load ESL module to interact with FreeSWITCH Event Socket
local esl = require("esl")

-- Connect to ESL
local conn = esl.Connection("127.0.0.1", "8021", "ClueCon")
    if not conn:connected() then
        freeswitch.consoleLog("err", "Failed to connect to ESL")
        if session:ready() then
            session:hangup("NORMAL_TEMPORARY_FAILURE")
        end
    else
    -- Get current call UUID
        local uuid = session:get_uuid()

    -- Ensure session is still active
        if not session:ready() then
            freeswitch.consoleLog("warning", "Session is not ready, exiting script")
            session:hangup("NORMAL_CLEARING")
        else
        -- Base log to confirm script is active
            freeswitch.consoleLog("notice", "IVR session starting for UUID: " .. uuid )

            -- Fetch caller ID for reference
            local caller_id = conn:api("uuid_getvar", uuid .. " caller_id_number"):getBody()
            freeswitch.consoleLog("debug", "Caller ID is: " .. (caller_id or "unknown"))

            -- Fire custom ESL event to mark IVR start
            local start_evt = esl.Event("CUSTOM", "ivr::start")
            start_evt:addHeader("UUID", uuid)
            start_evt:addHeader("Caller-ID", caller_id or "n/a")
            conn:sendEvent(start_evt)
            freeswitch.consoleLog("debug", "Sent IVR start event to ESL")

            -- Play welcome message
            session:streamFile("/usr/share/freeswitch/sounds/en/us/callie/ivr/ivr-welcome_to_freeswitch.wav")
            freeswitch.consoleLog("info", "Played welcome prompt")

            -- Allow up to 3 attempts to enter a valid digit
            local tries = 0
            local max_tries = 3
            local valid_input = false
            local digits = ""

            while tries < max_tries do
                -- Play prompt and collect digit
                digits = session:playAndGetDigits(1, 1, 1, 5000, "#","/usr/share/freeswitch/sounds/en/us/callie/ivr/ivr-please_press_a_key.wav","/usr/share/freeswitch/sounds/en/us/callie/ivr/ivr-that_was_an_invalid_entry.wav","\\d")

                if digits == "" then
                    -- No input case
                    session:streamFile("/usr/share/freeswitch/sounds/en/us/callie/ivr/ivr-no_entry_received.wav")
                    freeswitch.consoleLog("warning", "No input received")
                    tries = tries + 1
                elseif digits == "1" or digits == "2" then
                    valid_input = true
                    break
                else
                    -- Invalid input
                    session:streamFile("/usr/share/freeswitch/sounds/en/us/callie/ivr/ivr-that_was_an_invalid_entry.wav")
                    freeswitch.consoleLog("warning", "Invalid input received: " .. digits)
                    tries = tries + 1
                end
            end

            -- Send event with user input (even if it's "none")
            local input_evt = esl.Event("CUSTOM", "ivr::input_received")
            input_evt:addHeader("UUID", uuid)
            input_evt:addHeader("Digit", digits ~= "" and digits or "none")
            input_evt:addHeader("Valid", tostring(valid_input))
            conn:sendEvent(input_evt)

            if valid_input then
                if digits == "1" then
                    session:speak("You selected option one. Transferring to sales.")
                elseif digits == "2" then
                    session:speak("You selected option two. Transferring to support.")
                end
                freeswitch.consoleLog("notice", "User successfully selected: " .. digits)
            else
            -- If all tries used up
                session:streamFile("/usr/share/freeswitch/sounds/en/us/callie/ivr/ivr-you_have_reached_the_maximum_attempts.wav")
                freeswitch.consoleLog("err", "User failed to provide valid input in " .. max_tries .. " tries")
                session:hangup("NORMAL_CLEARING")
            end

        -- Double-check session state
            if not session:ready() then
                freeswitch.consoleLog("warning", "Call was dropped or hung up during IVR flow")
            end

        -- Capture final hangup cause
            local hangup_cause = conn:api("uuid_getvar", uuid .. " hangup_cause"):getBody()
            freeswitch.consoleLog("notice", "Call ended with hangup cause: " .. (hangup_cause or "unknown"))

        -- Send hangup event
            local hangup_evt = esl.Event("CUSTOM", "ivr::hangup")
            hangup_evt:addHeader("UUID", uuid)
            hangup_evt:addHeader("Hangup-Cause", hangup_cause or "n/a")
            conn:sendEvent(hangup_evt)
            freeswitch.consoleLog("debug", "Hangup event generated")

            -- End log
            freeswitch.consoleLog("info", "FREESWITCH IVR USING ESL")
        end
    end

