# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import random
import logging
import time
from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.cfm import cfm_base_test
from autotest_lib.client.common_lib.cros.manual import audio_helper
from autotest_lib.client.common_lib.cros.manual import cfm_helper
from autotest_lib.client.common_lib.cros.manual import meet_helper
from autotest_lib.client.common_lib.cros.manual import video_helper
from autotest_lib.client.common_lib.cros.manual import get_usb_devices

ATRUS = "Hangouts Meet speakermic"
CORE_DIR_LINES = 5
FAILURE_REASON = {
                 'telemetry':
                     ['No hangouts or meet telemetry API available',
                      'telemetry api',
                      'RPC: cfm_main_screen.',
                      'Failed RPC',
                      'cfm_main_screen',
                      'Webview with screen param',
                      'autotest_lib.client.common_lib.error.TestFail'],
                 'chrome':[],
                 'kernel':[],
                 'atrus':[]
}
NUM_AUDIO_STREAM_IN_MEETING = 3
LOG_CHECK_LIST = ['check_kernel_errorlog',
                  'check_video_errorlog',
                  'check_audio_errorlog',
                  'check_chrome_errorlog',
                  'check_atrus_errorlog',
                  'check_usb_stability']
TEST_DELAY = 0

class enterprise_CFM_Test(cfm_base_test.CfmBaseTest):
    """Executes multiple tests on CFM device based on control file,
    after each test, perform mulitple verifications based on control
    file. Test flow can be controlled by control file, such as abort
    on failure, or continue on failure.
    """
    version = 1

    def gpio_test(self):
        """
        Powercycle USB port on Guado.
        """
        if self.run_meeting_test:
            if self.random:
                min_meetings = random.randrange(-1, self.gpio_min_meets)
            else:
                min_meetings = self.gpio_min_meets
            if self.meets_last_gpio <=  min_meetings:
                logging.debug('Skip gpio test.')
                return True, None
        if cfm_helper.check_is_platform(self.client, 'guado'):
            status, errmsg =  cfm_helper.gpio_usb_test(self.client,
                              self.gpio_list,
                              self.puts, self.gpio_pause,
                              'guado')
            self.gpio_no += 1
            self.meets_last_gpio = 0
        else:
            logging.info('Skip gpio_test for non-guado CfM.')
            return True, None

        ## workaround for bug b/69261543
        if self.is_in_meet:
            self.is_camera_muted = self.cfm_facade.is_camera_muted()
        return status, errmsg


    def meeting_test(self):
        """
        Join/leave meeting.
        """
        if self.is_in_meet:
            status, errmsg = meet_helper.leave_meeting(self.cfm_facade,
                             self.is_meeting)
            if status:
                self.is_camera_muted = True
                self.is_in_meet = False
                return True, None
            else:
                if self.recover_on_failure or self.meets_last_reboot == 0:
                    self.is_in_meet = False
                else:
                    self.is_in_meet = True
                return False, errmsg
        else:
            status, errmsg = meet_helper.join_meeting(self.cfm_facade,
                             self.is_meeting, self.meeting_code)
            if status:
                self.is_camera_muted = False
                self.is_in_meet = True
                self.meet_no += 1
                self.meets_last_reboot += 1
                self.meets_last_gpio += 1
                return True, None
            else:
                if self.recover_on_failure or self.meets_last_reboot == 0:
                    self.is_in_meet = True
                else:
                    self.is_in_meet = False
                return False, errmsg

    def reboot_test(self):
        """
        Reboot CfM.
        """
        if self.run_meeting_test:
            if self.random:
                min_meetings = random.randrange(-1, self.reboot_min_meets)
            else:
                min_meetings = self.reboot_min_meets
            if self.meets_last_reboot <  min_meetings:
                logging.info('Skip reboot CfM test')
                return True, None
        try:
            self.client.reboot()
            time.sleep(self.reboot_timeout)
        except Exception as e:
            errmsg = 'Reboot test fails for %s' % self.ip_addr
            logging.exception(errmsg)
            self.reboot_log = cfm_helper.check_last_reboot(self.client)
            self.chrome_file = cfm_helper.check_chrome_logfile(self.client)
            return False, errmsg
        self.reboot_no += 1
        self.meets_last_reboot = 0
        if 'meeting_test' in self.action_config:
            self.reboot_log = cfm_helper.check_last_reboot(self.client)
            if list(set(self.verification_config) & set(LOG_CHECK_LIST)):
                self.log_checking_point = cfm_helper.find_last_log(self.client,
                                                                   self.speaker)
            return self.restart_chrome_and_meeting(True)
        self.reboot_log = cfm_helper.check_last_reboot(self.client)
        self.chrome_file = cfm_helper.check_chrome_logfile(self.client)
        if list(set(self.verification_config) & set(LOG_CHECK_LIST)):
            self.log_checking_point = cfm_helper.find_last_log(self.client,
                                                               self.speaker)
        return True, None

    def restart_chrome_and_meeting(self, recovery):
        """
        Restart Chrome and Join/Start meeting if previous state is in meeting.
        """
        result, errmsg = meet_helper.restart_chrome(self.cfm_facade,
                                                    self.is_meeting,
                                                    recovery)
        logging.info('restart chrome result:%s, msg = %s', result,errmsg)
        if not result:
            logging.info('restart chrome result: False, msg = %s', errmsg)
            self.chrome_file = cfm_helper.check_chrome_logfile(self.client)
            return False, errmsg
        if self.is_in_meet:
            logging.info('start meeting if needed')
            self.is_in_meet = False
            self.chrome_file = cfm_helper.check_chrome_logfile(self.client)
            test_result, ret_msg =  self.meeting_test()
            if test_result:
                try:
                    self.is_camera_muted =  self.cfm_facade.is_camera_muted()
                    self.is_mic_muted = self.cfm_facade.is_mic_muted()
                except Exception as e:
                    errmsg = 'Fail to run telemetry api to check camera..'
                    logging.exception(errmsg)
                    return False, errmsg
        self.chrome_file = cfm_helper.check_chrome_logfile(self.client)
        return True, None

    # TODO(mzhuo): Adding resetusb test.
    def reset_usb_test(self):
        """
        Reset USB port
        """
        return True, None

    def mute_unmute_camera_test(self):
        """
        Mute or unmute camera.
        """
        if not self.camera:
            logging.info('Skip mute/unmute camera testing.')
            return True, None
        if self.is_in_meet:
            if self.is_camera_muted:
                status, errmsg = meet_helper.mute_unmute_camera(
                                 self.cfm_facade, True)
                if status:
                    self.is_camera_muted = False
                else:
                    return False, errmsg
            else:
                status, errmsg =  meet_helper.mute_unmute_camera(
                                  self.cfm_facade, False)
                if status:
                    self.is_camera_muted = True
                else:
                    return False, errmsg
        return True, None

    def mute_unmute_mic_test(self):
        """
        Mute or unmute microphone.
        """
        if not self.speaker and not cfm_helper.check_is_platform(self.client,
                                                                 'buddy'):
            logging.info('Skip mute/unmute microphone testing.')
            return True, None
        if self.is_in_meet:
            if self.is_mic_muted:
                status, errmsg =  meet_helper.mute_unmute_mic(self.cfm_facade,
                                  True)
                if status:
                    self.is_mic_muted = False
                else:
                    return False, errmsg
            else:
                status, errmsg =  meet_helper.mute_unmute_mic(self.cfm_facade,
                                  False)
                if status:
                    self.is_mic_muted = True
                else:
                    return False, errmsg
        return True, None


    def speaker_volume_test(self):
        """
        Update speaker volume.
        """
        if not self.speaker and not cfm_helper.check_is_platform(self.client,
                                                                 'buddy'):
            logging.info('Skip update volume of speaker testing.')
            return True, None
        if self.is_in_meet:
            test_result, ret_msg =  meet_helper.speaker_volume_test(
                                   self.cfm_facade,
                                   self.vol_change_step,
                                   self.vol_change_mode, self.random)
            if test_result:
                self.speaker_volume = int(ret_msg)
                return True, None
            else:
                return False, ret_msg
        else:
            return True, None

    # TODO(mzhuo): Adding test to turn on/off monite.
    def flap_monitor_test(self):
        """
        Connect or disconnect monitor.
        """
        return True, None

    def check_usb_enumeration(self):
        """
        Verify all usb devices which were enumerated originally are enumerated.
        """
        return cfm_helper.check_usb_enumeration(self.client,
                                                self.puts)

    def check_usb_inf_init(self):
        """
        Verify all usb devices which were enumerated originally have
        valid interfaces: video interface, audio interface or touch
        interface.
        """
        return cfm_helper.check_usb_interface_initializion(self.client,
               self.puts)

    def check_v4l2_interface(self):
        """
        Verify camera has v4l2 file handler created.
        """
        if not self.camera:
            return True, None
        return video_helper.check_v4l2_interface(self.client,
               self.camera, self.name_camera)

    def check_audio_card(self):
        """
        Verify speaker/microphone has audip file handler created.
        """
        if not self.speaker:
            return True, None
        return audio_helper.check_soundcard_by_name(self.client,
               self.name_speaker)

    def check_cras_speaker(self):
        """
        Verify cras server detects speaker.
        """
        if not self.speaker:
            return True, None
        return audio_helper.check_speaker_exist_cras(self.client,
               self.name_speaker)

    def check_cras_mic(self):
        """
        Verify cras server detects microphone.
        """
        if not self.speaker:
            return True, None
        return audio_helper.check_microphone_exist_cras(self.client,
               self.name_speaker)

    def check_cras_mic_mute(self):
        """
        Verify cras shows mic muted or unmuted as expected.
        """
        if not self.speaker or not self.is_in_meet:
            return True, None
        return audio_helper.check_cras_mic_mute(self.client, self.cfm_facade)

    def check_cras_pspeaker(self):
        """
        Verify cras shows correct preferred speaker.
        """
        if not self.speaker:
            return True, None
        return  audio_helper.check_is_preferred_speaker(self.client,
                self.name_speaker)

    def check_cras_speaker_vol(self):
        """
        Verify cras shows correct volume for speaker.
        """
        if not self.speaker or not self.is_in_meet:
            return True, None
        return audio_helper.check_default_speaker_volume(self.client,
               self.cfm_facade)

    def check_cras_pmic(self):
        """
        Verify cras shows correct preferred microphone.
        """
        if not self.speaker:
            return True, None
        return  audio_helper.check_is_preferred_mic(self.client,
                self.name_speaker)

    # TODO(mzhuo): add verification for preferred camera
    def check_prefer_camera(self):
        """
        Verify preferred camera is correct.
        """
        return True, None

    #TODO(mzhuo): add verification to verify camera is muted or unmuted
    #in video stack in kernel space.
    def check_camera_mute(self):
        """
        Verify camera is muted or unmuted as expected.
        """
        return True, None

    def check_video_stream(self):
        """
        Verify camera is streaming or not streaming as expected.
        """
        if not self.camera:
            return True, None
        return video_helper.check_video_stream(self.client,
               self.is_camera_muted, self.camera, self.name_camera)

    def check_audio_stream(self):
        """
        Verify speaker is streaming or not streaming as expected.
        """
        if not self.speaker:
            return True, None
        return audio_helper.check_audio_stream(self.client,
               self.is_in_meet)

    # TODO(mzhuo): Adding verification for speaker in Hotrod App
    def check_hotrod_speaker(self):
        """
        Verify hotrod shows all speakers.
        """
        return True, None

    # TODO(mzhuo): Adding verification for mic in Hotrod App
    def check_hotrod_mic(self):
        """
        Verify hotrod shows all microphone.
        """
        return True, None

    # TODO(mzhuo): Adding verification for camera in Hotrod App
    def check_hotrod_camera(self):
        """
        Verify hotrod shows all cameras.
        """
        return True, None

     # TODO(mzhuo): Adding verification for speaker in Hotrod App
    def check_hotrod_pspeaker(self):
        """
        Verify hotrod selects correct preferred speaker.
        """
        return True, None

    # TODO(mzhuo): Adding verification for mic in Hotrod App
    def check_hotrod_pmic(self):
        """
        Verify hotrod selects correct preferred microphone.
        """
        return True, None


    # TODO(mzhuo): Adding verification for camera in Hotrod App
    def check_hotrod_pcamera(self):
        """
        Verify hotrod selects correct preferred camera.
        """
        return True, None

    #TODO(mzhuo): Adding verififaction in hotrod layer for speaker volume
    def check_hotrod_speaker_vol(self):
        """
        Verify hotrod can set volume for speaker.
        """
        return True, None

    #TODO(mzhuo): Adding verififaction in hotrod layer for mic mute status
    def check_hotrod_mic_state(self):
        """
        Verify hotrod can mute or unmute microphone.
        """
        return True, None

    #TODO(mzhuo): Adding verififaction in hotrod layer for camera status
    def check_hotrod_camera_state(self):
        """
        Verify hotrod can mute or unmute camera.
        """
        return True, None

    def check_kernel_errorlog(self):
        """
        Check /var/log/message does not contain any element in
        error_key_words['kernel'].
        """
        return cfm_helper.check_log(self.client, self.log_checking_point,
                                    self.error_key_words, 'kernel',
                                    'messages')

    def check_chrome_errorlog(self):
        """
        Check /var/log/chrome/chrome does not contain any element in
        error_key_words['chrome'].
        """
        return cfm_helper.check_log(self.client, self.log_checking_point,
                                    self.error_key_words, 'chrome',
                                    'chrome')

    def check_atrus_errorlog(self):
        """
        Check /var/log/atrus.log does not contain any element in
        error_key_words['atrus'].
        """
        if self.current_test in ['gpio_test', 'resetusb_test']:
            return True, None
        if not self.name_speaker:
            return True, None
        if self.name_speaker not in ATRUS:
            logging.info('Speaker %s', self.name_speaker)
            return True, None
        if cfm_helper.check_is_platform(self.client, 'guado'):
            return cfm_helper.check_log(self.client, self.log_checking_point,
                                        self.error_key_words, 'atrus', 'atrus')
        else:
            return True, None

    def check_video_errorlog(self):
        """
        Check /var/log/message does not contain any element in
        error_key_words['video'].
        """
        return cfm_helper.check_log(self.client, self.log_checking_point,
                                    self.error_key_words, 'video',
                                    'messages')

    def check_audio_errorlog(self):
        """
        Check /var/log/message does not contain any element in
        error_key_words['audio'].
        """
        return cfm_helper.check_log(self.client, self.log_checking_point,
                                    self.error_key_words, 'audio',
                                    'messages')

    def check_usb_errorlog(self):
        """
        Check /var/log/message does not contain any element in
        error_key_words['usb'].
        """
        return cfm_helper.check_log(self.client, self.log_checking_point,
               self.error_key_words, 'usb', 'messages')

    def check_usb_stability(self):
        """
        Check if no disruptive test done, USB device should not go offline.
        """
        if self.current_test in ['gpio_test', 'reboot_test', 'resetusb_test']:
            return True, None
        return cfm_helper.check_log(self.client, self.log_checking_point,
                                    self.error_key_words,
                                    'usb_stability', 'messages')

    def check_process_crash(self):
        """
        check no process crashing.
        """
        test_result, self.cdlines = cfm_helper.check_process_crash(self.client,
                                self.cdlines)
        return test_result, str(self.cdlines)

    #TODO(mzhuo): Adding verififaction to check whether there is kernel panic
    def check_kernel_panic(self):
        """
        Check no kernel panic reported.
        """
        return True, None


    def check_chrome_restarted(self):
        """
        Check whether chrome is killed and restarted.
        """
        if self.chrome_file == cfm_helper.check_chrome_logfile(self.client):
            return True, None
        else:
            self.chrome_file = cfm_helper.check_chrome_logfile(self.client)
            return False, 'Chrome is restarted unexpectly.'

    def check_cfm_rebooted(self):
        """
        Check whether CfM is rebooted.
        """
        logging.info('Last reboot: %s', self.reboot_log)
        if self.reboot_log == cfm_helper.check_last_reboot(self.client):
            return True, None
        else:
            self.reboot_log = cfm_helper.check_last_reboot(self.client)
            return False, 'CfM is rebooted unexpectly.'

    def initialize_action_check_config(self, action_config, verification_config,
                                       fixedmode):
        """
        Initialize action list based on control file.
        @param action_config: dict defines the number of test should be done
                              for each test
        @param fixedmode: if True all tests are executed in fixed order;
                     if False all tests are executed in random order.
        """
        self.action_config =  []
        if action_config['meeting_test'] == 1:
            self.action_config = ['meeting_test']
        if not self.camera:
            action_config['mute_unmute_camera_test'] = 0
            verification_config['check_v4l2_interface'] = False
            verification_config['check_video_stream'] = False
            verification_config['check_video_errorlog'] = False
        if not self.speaker:
            if not cfm_helper.check_is_platform(self.client, 'buddy'):
                action_config['mute_unmute_mic_test'] = 0
                action_config['speaker_volume_test']  = 0
            verification_config['check_audio_card'] = False
            verification_config['check_cras_speaker'] = False
            verification_config['check_cras_mic'] = False
            verification_config['check_cras_pspeaker'] = False
            verification_config['check_cras_pmic'] = False
            verification_config['check_audio_stream'] = False
            verification_config['check_audio_errorlog'] = False
            verification_config['check_cras_speaker_vol'] = False
            verification_config['check_cras_mic_mute'] = False
        not_in_meeting_action = ['meeting_test', 'gpio_test', 'reboot_test']

        if fixedmode:
            for action, nof_times in action_config.iteritems():
                if not action in not_in_meeting_action:
                    self.action_config.extend(nof_times * [action])
        else:
            for action, nof_times in action_config.iteritems():
                if not action in not_in_meeting_action and nof_times  > 0:
                    dup_test = max(1, random.randrange(0, nof_times))
                    for _ in range(dup_test):
                        self.action_config.insert(max(1, random.randrange(-1,
                             len(self.action_config))), action)

        if action_config['meeting_test'] == 1:
            self.action_config.append('meeting_test')

        for action, nof_times in action_config.iteritems():
            if action == 'meeting_test':
                continue
            if action in not_in_meeting_action and nof_times  > 0:
                dup_test = max(1, random.randrange(0, nof_times))
                for _ in range(dup_test):
                    self.action_config.insert(max(0, random.randrange(-1,
                         len(self.action_config))), action)

        logging.info('Test list = %s', self.action_config)
        self.verification_config = [v for v in verification_config.keys()
                                    if verification_config[v]]
        logging.info('Verification list = %s', self.verification_config)


    def initialize_test(self, test_config, action_config, verification_config,
                        error_key_words, test_flow_control):
        """
        Initialize the list of tests and verifications;
        and polulate data needed for test:
            USB devices: which can be retrieved from control file or
            automatically detected by script;
            Test loop, meeting mode, meeting code, test flow contro
            variables.
        """
        self.gpio_pause = test_config['gpiopause']
        self.reboot_timeout =  test_config['reboot_timeout']
        self.vol_change_step = test_config['vol_change_step']
        self.vol_change_mode = test_config['vol_change_mode']
        self.gpio_list = test_config['gpio_list']
        self.is_meeting = test_config['is_meeting']
        self.meeting_code = test_config ['meeting_code']
        self.reboot_min_meets = test_config['reboot_after_min_meets']
        self.gpio_min_meets = test_config['gpio_after_min_meets']
        self.run_meeting_test = action_config['meeting_test']
        self.random = test_flow_control['random_mode']
        self.recover_on_failure = test_flow_control['recovery_on_fatal_failure']
        self.error_key_words = error_key_words
        if test_config['puts']:
            self.puts = test_config['puts'].split(',')
        else:
            self.puts = None

        if verification_config['check_process_crash']:
            cfm_helper.clear_core_file(self.client)

        self.action_fun = {
            'gpio_test': self.gpio_test,
            'meeting_test': self.meeting_test,
            'reboot_test': self.reboot_test,
            'reset_usb_test': self.reset_usb_test,
            'mute_unmute_camera_test': self.mute_unmute_camera_test,
            'mute_unmute_mic_test': self.mute_unmute_mic_test,
            'speaker_volume_test': self.speaker_volume_test,
            'flap_monitor_test': self.flap_monitor_test
            }
        self.veri_fun = {
            'check_usb_enumeration': self.check_usb_enumeration,
            'check_usb_inf_init': self.check_usb_inf_init,
            'check_v4l2_interface': self.check_v4l2_interface,
            'check_audio_card': self.check_audio_card,
            'check_cras_speaker': self.check_cras_speaker,
            'check_cras_mic': self.check_cras_mic,
            'check_cras_pspeaker': self.check_cras_pspeaker,
            'check_cras_pmic': self.check_cras_pmic,
            'check_cras_speaker_vol': self.check_cras_speaker_vol,
            'check_cras_mic_mute': self.check_cras_mic_mute,
            'check_prefer_camera': self.check_prefer_camera,
            'check_camera_mute': self.check_camera_mute,
            'check_audio_stream': self.check_audio_stream,
            'check_video_stream': self.check_video_stream,
            'check_hotrod_speaker': self.check_hotrod_speaker,
            'check_hotrod_mic': self.check_hotrod_mic,
            'check_hotrod_camera': self.check_hotrod_camera,
            'check_hotrod_pspeaker': self.check_hotrod_pspeaker,
            'check_hotrod_pmic': self.check_hotrod_pmic,
            'check_hotrod_pcamera': self.check_hotrod_pcamera,
            'check_hotrod_speaker_vol': self.check_hotrod_speaker_vol,
            'check_hotrod_mic_state': self.check_hotrod_mic_state,
            'check_hotrod_camera_state': self.check_hotrod_camera_state,
            'check_usb_errorlog': self.check_usb_errorlog,
            'check_kernel_errorlog': self.check_kernel_errorlog,
            'check_video_errorlog': self.check_video_errorlog,
            'check_audio_errorlog': self.check_audio_errorlog,
            'check_chrome_errorlog': self.check_chrome_errorlog,
            'check_atrus_errorlog': self.check_atrus_errorlog,
            'check_usb_stability': self.check_usb_stability,
            'check_process_crash': self.check_process_crash,
            'check_kernel_panic': self.check_kernel_panic,
            'check_cfm_rebooted':self.check_cfm_rebooted,
            'check_chrome_restarted':self.check_chrome_restarted
             }

        self.usb_data = []
        self.speaker = None
        self.speaker_volume = None
        self.camera = None
        self.name_speaker = None
        self.mimo_sis = None
        self.mimo_display = None
        self.is_in_meet = False
        self.is_camera_muted = True
        self.is_mic_muted = False
        self.meets_last_reboot = 0
        self.meets_last_gpio = 0
        self.meet_no = 0
        self.reboot_no = 0
        self.gpio_no = 0
        self.cdlines = CORE_DIR_LINES

        usb_data = cfm_helper.retrieve_usb_devices(self.client)
        if not usb_data:
            logging.info('\n\nEnterprise_CFM_Test_Failed.')
            raise error.TestFail('Fails to find any usb devices on CfM.')
        peripherals = cfm_helper.extract_peripherals_for_cfm(usb_data)
        if not peripherals:
            logging.info('\n\nEnterprise_CFM_Test_Failed.')
            raise error.TestFail('Fails to find any periphereal on CfM.')
        if not self.puts:
            self.puts = peripherals.keys()
        else:
            if [put for put in self.puts if not put in peripherals.keys()]:
                logging.info('Fails to find target device %s', put)
                logging.info('\nEnterprise_CFM_Test_Failed.')
                raise error.TestFail('Fails to find device')
        for _put in self.puts:
            if _put in get_usb_devices.CAMERA_MAP.keys():
                self.camera = _put
            if _put in get_usb_devices.SPEAKER_MAP.keys():
                self.speaker = _put
        if self.camera:
            self.name_camera = get_usb_devices.get_device_prod(self.camera)
            logging.info('Camera under test: %s %s',
                          self.camera, self.name_camera)
        if self.speaker:
            self.name_speaker = get_usb_devices.get_device_prod(self.speaker)
            logging.info('Speaker under test: %s %s',
                          self.speaker, self.name_speaker)
        if not test_flow_control['skip_cfm_check']:
            if cfm_helper.check_is_platform(self.client, 'guado'):
                if not cfm_helper.check_peripherals_for_cfm(peripherals):
                    logging.info('Sanity Check on CfM fails.')
                    logging.info('\n\nEnterprise_CFM_Test_Failed.')
                    raise error.TestFail('Sanity Check on CfM fails.')
        self.ip_addr = cfm_helper.get_mgmt_ipv4(self.client)
        logging.info('CfM %s passes sanity check, will start test.',
                      self.ip_addr)

        self.initialize_action_check_config(action_config,
                                            verification_config, True)

        if list(set(self.verification_config) & set(LOG_CHECK_LIST)):
            self.log_checking_point = cfm_helper.find_last_log(self.client,
                                      self.speaker)
        self.chrome_file = cfm_helper.check_chrome_logfile(self.client)
        self.reboot_log = cfm_helper.check_last_reboot(self.client)


    def recovery_routine(self, failure_msg):
        """
        Telemetry api often returns API timeout, or javascript timeout due to
        various reasons. To workout the problem before the fix is ready script
        checks if errmsg from test failure or verification failure has keywords
        defined in FAILURE_REASON['telemetry']. If yes, script checks whether
        CfM is rebooted or Chrome browser is rebooted unexpectly. If no, script
        restarts chrome. If yes no chrome restart is done to preserve valid
        failure state.
        @param failure_msg: failure message returned from test failure or
                            verification failure
        @return True if recovery is successfully done,
                False, if recovery is not done, or fails.
        """
        loop_result = False
        to_be_recovered = False
        if not self.check_chrome_restarted():
            self.chrome_file = cfm_helper.check_chrome_logfile(self.client)
            return False
        if not self.check_cfm_rebooted():
            self.reboot_log = cfm_helper.check_last_reboot(self.client)
            return False
        for _err in FAILURE_REASON['telemetry']:
            if _err in failure_msg:
                to_be_recovered = True
                break
        if to_be_recovered:
            logging.info('Restart Chrome to recover......')
            result, emsg = self.restart_chrome_and_meeting(True)
            if result:
                loop_result = True
                if list(set(self.verification_config) & set(LOG_CHECK_LIST)):
                    self.log_checking_point = cfm_helper.find_last_log(
                                              self.client,
                                              self.speaker)
        return loop_result


    def process_test_result(self, loop_result, loop_no, test_no,
                            failed_tests, failed_verifications,
                            failed_tests_loop,
                            failed_verifications_loop, test_flow_control,
                            test_config, finished_tests_verifications,
                            test_done):
        """
        Proceess test result data, and print out test report.
        @params loop_result: True when all tests and verifications pass,
                             False if any test or verification fails.
        @param loop_no: sequence number of the loop.
        @param test_no: sequence number of the test.
        @param failed_tests: failed tests.
        @param failed_verifications: failed verifications.
        @param failed_tests_loop: failed tests in the loop.
        @param failed_verifications_loop: failed verifications in the loop.
        @param test_flow_control: variable of flow control defined in
                                  control file
        @param test_config: variable of test config defined in control file
        @param finished_tests_verifications: data to keep track number of
               tests and verifications performed.
        @param test_done: True if all loops are done; False otherwose.
        """
        if 'reboot_test' in finished_tests_verifications.keys():
            finished_tests_verifications['reboot_test'] = self.reboot_no
        if not loop_result and not test_done:
            logging.info('\n\nVerification_or_Test_Fail on %s for loop NO:'
                         ' %d, Test: %d', self.ip_addr, loop_no, test_no)
            if failed_tests_loop:
                logging.info('----- Failed_Tests: %s', failed_tests_loop)
            if failed_verifications_loop:
                logging.info('----- Failed_Verifications: %s',
                             failed_verifications_loop)
        if test_flow_control['report']:
            logging.info('\n\n\n----------------Summary---------------')
            logging.info('---Loop %d, Test: %d, Meet: %d, Reboot: %d, Gpio: %s',
                         loop_no, test_no, self.meet_no, self.reboot_no,
                         self.gpio_no)
            for testname, counter in failed_tests.iteritems():
                logging.info('----Test: %s, Failed times: %d, Total Run: %d',
                           testname, counter,
                           finished_tests_verifications[testname])
            for veriname, counter in failed_verifications.iteritems():
                logging.info('----Verification: %s, Failed times: %d,'
                             'Total Run: %d',
                             veriname, counter,
                             finished_tests_verifications[veriname])
            if self.random:
                time.sleep(random.randrange(0, test_config['loop_timeout']))
            else:
                 time.sleep(test_config['loop_timeout'])
        if not test_done:
            if list(set(self.verification_config) & set(LOG_CHECK_LIST)):
                self.log_checking_point = cfm_helper.find_last_log(self.client,
                                          self.speaker)

    def run_once(self, host, run_test_only, test_config, action_config,
                 verification_config,
                 error_key_words, test_flow_control):
        """Runs the test."""
        logging.info('Start_Test_Script:Enterprise_CFM_Test')
        self.client = host
        if test_flow_control['reboot_before_start']:
            try:
                self.client.reboot()
            except Exception as e:
                errmsg = ('Reboot test fails for %s.'
                          '% self.ip_addr')
                logging.exception(errmsg)
                raise error.TestFail(errmsg)
            if action_config['meeting_test'] > 0:
                result, errmsg = meet_helper.restart_chrome(self.cfm_facade,
                                 test_config['is_meeting'], True)
                if not result:
                    logging.info('Restart chrome fails, msg = %s', errmsg)
                    raise error.TestFail(errmsg)

        self.initialize_test(test_config, action_config, verification_config,
                              error_key_words, test_flow_control)
        test_no = 0
        failed_tests = {}
        failed_verifications = {}
        finished_tests_verifications = {}
        test_failure_reason = []
        verification_failure_reason = []


        for loop_no in xrange(1, test_config['repeat'] + 1):
            logging.info('=============%s:Test_Loop_No:%d=============',
                         self.ip_addr, loop_no)
            logging.info('Action list: %s', self.action_config)
            failed_tests_loop = []
            failed_verifications_loop = []
            for test in self.action_config:
                loop_result = True
                if not test in finished_tests_verifications.keys():
                    finished_tests_verifications[test] = 1
                else:
                    finished_tests_verifications[test] += 1
                self.current_test = test
                logging.info('\nStart_test:%s', test)
                test_result, test_msg = self.action_fun[test]()
                test_no += 1
                if test_result:
                    logging.info('Test_Result:%s:SUCCESS', test)
                else:
                    logging.info('Test_Result:%s:FAILURE:%s', test, test_msg)
                    test_failure_reason.append(test_msg)
                    failed_tests_loop.append(test)
                    loop_result = False
                    if not test in failed_tests.keys():
                        failed_tests[test] = 1
                    else:
                        failed_tests[test] += 1
                    logging.info('\n%s:Test_failure:%s:%s', self.ip_addr,
                                 test, test_msg)
                    if self.recover_on_failure or self.meets_last_reboot < 1:
                        loop_result = self.recovery_routine(test_msg)
                if self.random:
                    time.sleep(random.randrange(test_config['min_timeout'],
                                                test_config['action_timeout']))
                else:
                    time.sleep(test_config['min_timeout'])

                for verification in self.verification_config:
                    if not verification in finished_tests_verifications.keys():
                        finished_tests_verifications[verification] = 1
                    else:
                        finished_tests_verifications[verification] += 1

                    logging.info('\nStart_verification:%s', verification)
                    veri_result, veri_msg = self.veri_fun[verification]()
                    if veri_result:
                        logging.info('Verification_Result:%s:SUCCESS',
                                     verification)
                    else:
                        logging.info('Verification_Result:%s:FAILURE:%s',
                                     verification, veri_msg)
                        verification_failure_reason.append(veri_msg)
                        failed_verifications_loop.append(verification)
                        if not verification in failed_verifications.keys():
                            failed_verifications[verification] = 1
                        else:
                            failed_verifications[verification] += 1
                        logging.info('%s:Verification_fail:%s:%s',
                                     self.ip_addr, verification, veri_msg)
                        loop_result = False
                        if self.recover_on_failure:
                            loop_result = self.recovery_routine(veri_msg)

                self.process_test_result(loop_result, loop_no, test_no,
                                         failed_tests,
                                         failed_verifications,
                                         failed_tests_loop,
                                         failed_verifications_loop,
                                         test_flow_control,
                                         test_config,
                                         finished_tests_verifications, False)
                if not loop_result:
                    if test_flow_control['abort_on_failure']:
                        logging.info('Enterprise_CFM_Test_Failed.')
                        time.sleep(test_config['debug_timeout'])
                        logging.info('Enterprise_CFM_Test_Finished.')
                        raise error.TestFail(
                            'Test_or_Verification_fails after {}.'.format(test))
                    else:
                        logging.info('Enterprise_CFM_Test_Failure_Detected.')

            if self.random:
                self.initialize_action_check_config(action_config,
                                                    verification_config, False)

        logging.info('Enterprise_CFM_Test_Finished.')
        self.process_test_result(loop_result, loop_no, test_no,
                                 failed_tests,
                                 failed_verifications,
                                 failed_tests_loop,
                                 failed_verifications_loop,
                                 test_flow_control,
                                 test_config,
                                 finished_tests_verifications, True)
        if test_failure_reason:
            logging.debug('Test failure reason %s', test_failure_reason)
        if verification_failure_reason:
            logging.debug('Verification failure reason %s',
                         verification_failure_reason)
