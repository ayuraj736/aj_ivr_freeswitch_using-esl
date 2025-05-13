<!-- second method -->
<!-- this file needs to place in the location as per your setup-->
<!-- usually with source based installation of freeswitch, location will be "/usr/share/freeswitch/conf/dialplan"-->
<!-- and with package based installation of freeswitch, location will be "/etc/freeswitch/dialplan"-->
<context name="aj_esl_connect_play_ivr">
    <extension name="ivr_test">
        <condition field="destination_number" expression="^9999$">
            <action application="answer"/>
            <action application="sleep" data="1000"/>
            <action application="esl_connect_play_ivr"/>
        </condition>
    </extension>
</context>
