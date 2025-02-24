<?php
/*
 * system_advanced_notifications.inc
 *
 * part of pfSense (https://www.pfsense.org)
 * Copyright (c) 2014-2024 Rubicon Communications, LLC (Netgate)
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

require_once("config.gui.inc");
require_once("functions.inc");
require_once("filter.inc");
require_once("shaper.inc");
require_once("system.inc");
require_once("util.inc");
require_once("pfsense-utils.inc");

// Functions included by system_advanced_notifications.php ====================

function getAdvancedNotifications($json = false) {
	$nconfig = array();

	// General Settings
	$nconfig['cert_enable_notify'] = (config_get_path('notifications/certexpire/enable') != "disabled");
	$nconfig['revoked_cert_ignore_notify'] = (config_get_path('notifications/certexpire/ignore_revoked') == "enabled");
	if (config_get_path('notifications/certexpire/expiredays')) {
		$nconfig['certexpiredays'] = config_get_path('notifications/certexpire/expiredays');
	}

	// SMTP
	$nconfig['disable_smtp'] = config_path_enabled('notifications/smtp', 'disable');
	if (!empty(config_get_path('notifications/smtp/ipaddress'))) {
		$nconfig['smtpipaddress'] = config_get_path('notifications/smtp/ipaddress');
	}

	if (!empty(config_get_path('notifications/smtp/port'))) {
		$nconfig['smtpport'] = config_get_path('notifications/smtp/port');
	}

	$nconfig['smtpssl'] = config_path_enabled('notifications/smtp', 'ssl');

	$nconfig['sslvalidate'] = (config_get_path('notifications/smtp/sslvalidate') != "disabled");

	if (!empty(config_get_path('notifications/smtp/timeout'))) {
		$nconfig['smtptimeout'] = config_get_path('notifications/smtp/timeout');
	}

	if (config_get_path('notifications/smtp/notifyemailaddress')) {
		$nconfig['smtpnotifyemailaddress'] = config_get_path('notifications/smtp/notifyemailaddress');
	}

	if (config_get_path('notifications/smtp/username')) {
		$nconfig['smtpusername'] = config_get_path('notifications/smtp/username');
	}

	if (config_get_path('notifications/smtp/password')) {
		$nconfig['smtppassword'] = config_get_path('notifications/smtp/password');
	}

	if (config_get_path('notifications/smtp/authentication_mechanism')) {
		$nconfig['smtpauthmech'] = config_get_path('notifications/smtp/authentication_mechanism');
	}

	if (config_get_path('notifications/smtp/fromaddress')) {
		$nconfig['smtpfromaddress'] = config_get_path('notifications/smtp/fromaddress');
	}

	// System Sounds
	$nconfig['consolebell'] = config_get_path('system/consolebell', 'enabled');
	$nconfig['disablebeep'] = config_path_enabled('system', 'disablebeep');

	// Telegram
	$nconfig['enable_telegram'] = config_path_enabled('notifications/telegram', 'enabled') ;

	if (config_get_path('notifications/telegram/api')) {
		$nconfig['api'] = config_get_path('notifications/telegram/api');
	}

	if (config_get_path('notifications/telegram/chatid')) {
		$nconfig['chatid'] = config_get_path('notifications/telegram/chatid');
	}
	// Pushover
	$nconfig['enable_pushover'] = config_path_enabled('notifications/pushover', 'enabled');

	if (config_get_path('notifications/pushover/apikey')) {
		$nconfig['pushoverapikey'] = config_get_path('notifications/pushover/apikey');
	}

	if (config_get_path('notifications/pushover/userkey')) {
		$nconfig['pushoveruserkey'] = config_get_path('notifications/pushover/userkey');
	}

	if (config_get_path('notifications/pushover/sound')) {
		$nconfig['pushoversound'] = config_get_path('notifications/pushover/sound');
	}

	if (config_get_path('notifications/pushover/priority')) {
		$nconfig['pushoverpriority'] = config_get_path('notifications/pushover/priority');
	}

	$nconfig['pushoverretry'] = config_get_path('notifications/pushover/retry', 60);
	$nconfig['pushoverexpire'] = config_get_path('notifications/pushover/expire', 300);

	// Slack
	$nconfig['enable_slack'] = config_path_enabled('notifications/slack', 'enabled');

	if (config_get_path('notifications/slack/api')) {
		$nconfig['slack_api'] = config_get_path('notifications/slack/api');
	}

	if (config_get_path('notifications/slack/channel')) {
		$nconfig['slack_channel'] = config_get_path('notifications/slack/channel');
	}

	return $json? json_encode($nconfig):$nconfig;
}

function saveAdvancedNotifications($post, $json = false) {
	global $smtp_authentication_mechanisms, $pushover_sounds;

	$input_errors = array();
	$rv = array();

	$testsmtp = isset($post['test-smtp']);
	$testtelegram = isset($post['test-telegram']);
	$testpushover = isset($post['test-pushover']);
	$testslack = isset($post['test-slack']);

	if (isset($post['save']) || $testsmtp || $testtelegram || $testpushover || $testslack) {

		// General Settings
		config_set_path('notifications/certexpire/enable', (($post['cert_enable_notify'] == "yes") ? "enabled" : "disabled"));
		config_set_path('notifications/certexpire/ignore_revoked', (($post['revoked_cert_ignore_notify'] == "yes") ? "enabled" : "disabled"));
		if (empty($post['certexpiredays'])) {
			config_set_path('notifications/certexpire/expiredays', g_get('default_cert_expiredays'));
		} elseif (is_numericint($post['certexpiredays']) && ($post['certexpiredays'] > 0)) {
			config_set_path('notifications/certexpire/expiredays', $post['certexpiredays']);
		} else {
			$input_errors[] = gettext("Certificate Expiration Threshold must be a positive integer");
		}

		// SMTP
		$validate_smtp = $testsmtp || (empty($post['disable_smtp']) &&
		    !empty($post['smtpipaddress']));

		if ($validate_smtp && (!is_ipaddr($post['smtpipaddress']) &&
		    !is_fqdn($post['smtpipaddress']))) {
			$input_errors[] = gettext("Please enter valid E-Mail server address.");
		} else {
			config_set_path('notifications/smtp/ipaddress', $post['smtpipaddress']);
		}

		if ($validate_smtp && !is_port($post['smtpport'])) {
			$input_errors[] = gettext("Please enter valid SMTP port of E-Mail server address.");
		} else {
			config_set_path('notifications/smtp/port', $post['smtpport']);
		}

		if (isset($post['smtpssl'])) {
			config_set_path('notifications/smtp/ssl', true);
		} else {
			config_del_path('notifications/smtp/ssl');
		}

		config_set_path('notifications/smtp/sslvalidate', (isset($post['sslvalidate']) ? 'enabled' : 'disabled'));

		if ($validate_smtp && !empty($post['smtptimeout']) &&
		    !is_numeric($post['smtptimeout'])) {
			$input_errors[] = gettext("Please enter valid connection timeout.");
		} else {
			config_set_path('notifications/smtp/timeout', $post['smtptimeout']);
		}

		if ($validate_smtp && empty($post['smtpnotifyemailaddress'])) {
			$input_errors[] = gettext("Please enter valid notification E-Mail address.");
		} else {
			config_set_path('notifications/smtp/notifyemailaddress', $post['smtpnotifyemailaddress']);
		}

		config_set_path('notifications/smtp/username', $post['smtpusername']);

		if (strcmp($post['smtppassword'], DMYPWD)!= 0) {
			if ($post['smtppassword'] == $post['smtppassword_confirm']) {
				config_set_path('notifications/smtp/password', $post['smtppassword']);
			} else {
				if ($post['disable_smtp'] != "yes") {
					// Bug #7129 - do not nag people about passwords mismatch when SMTP notifications are disabled
					$input_errors[] = gettext("SMTP passwords must match");
				}
			}
		}

		if (!array_key_exists($post['smtpauthmech'], $smtp_authentication_mechanisms)) {
			$input_errors[] = gettext("Please select valid authentication mechanism.");
		} else {
			config_set_path('notifications/smtp/authentication_mechanism', $post['smtpauthmech']);
		}

		config_set_path('notifications/smtp/fromaddress', $post['smtpfromaddress']);

		if ($post['disable_smtp'] == "yes") {
			config_set_path('notifications/smtp/disable', true);
		} else {
			config_del_path('notifications/smtp/disable');
		}

		// System Sounds
		if ($post['consolebell'] == "yes") {
			config_set_path('system/consolebell', 'enabled');
			set_single_sysctl('kern.vt.enable_bell', '1');
		} else {
			config_set_path('system/consolebell', 'disabled');
			set_single_sysctl('kern.vt.enable_bell', '0');
		}
		if ($post['disablebeep'] == "yes") {
			config_set_path('system/disablebeep', true);
		} else {
			config_del_path('system/disablebeep');
		}

		// Telegram
		config_set_path('notifications/telegram/enabled', (($post['enable_telegram'] == "yes") ? true : false));
		config_set_path('notifications/telegram/api', $post['api']);
		config_set_path('notifications/telegram/chatid', $post['chatid']);

		if (preg_replace("/[^a-zA-Z0-9_:\-]/", "", config_get_path('notifications/telegram/api')) !== config_get_path('notifications/telegram/api')) {
			$input_errors[] = gettext("The only special characters permitted in the Telegram API string are _, - and :");
		}
		if (preg_replace("/[^a-zA-Z0-9@_\-]/", "", config_get_path('notifications/telegram/chatid')) !== config_get_path('notifications/telegram/chatid')) {
			$input_errors[] = gettext("The Chat ID can only contain @, _ or - as special characters");
		}

		// Pushover
		config_set_path('notifications/pushover/enabled', (($post['enable_pushover'] == "yes") ? true : false));
		config_set_path('notifications/pushover/apikey', $post['pushoverapikey']);
		config_set_path('notifications/pushover/userkey', $post['pushoveruserkey']);

		if (preg_replace("/[^A-Za-z0-9]/", "", config_get_path('notifications/pushover/apikey')) !== config_get_path('notifications/pushover/apikey')) {
			$input_errors[] = gettext("API keys are case-sensitive, 30 characters long, and can only use the character set A-Z,a-z,0-9");
		}
		if (preg_replace("/[^A-Za-z0-9]/", "", config_get_path('notifications/pushover/userkey')) !== config_get_path('notifications/pushover/userkey')) {
			$input_errors[] = gettext("User keys are case-sensitive, 30 characters long, and can only use the character set A-Z,a-z,0-9");
		}
		if (!array_key_exists($post['pushoversound'], $pushover_sounds)) {
			$input_errors[] = gettext("Please select a valid Pushover notification sound.");
		} else {
			config_set_path('notifications/pushover/sound', $post['pushoversound']);
		}
		if (!array_key_exists($post['pushoverpriority'], array_fill_keys(range(-2,2),''))) {
			$input_errors[] = gettext("Please select a valid Pushover message priority.");
		} else {
			config_set_path('notifications/pushover/priority', $post['pushoverpriority']);
		}
		if (!empty($post['pushoverretry']) && !is_numeric($post['pushoverretry'])) {
			$input_errors[] = gettext("Please enter valid notification retry interval in seconds.");
		} elseif (!empty($post['pushoverretry']) && ($post['pushoverretry'] < 30)) {
			$input_errors[] = gettext("Please enter valid notification retry interval with a minimum value of 30.");
		} else {
			config_set_path('notifications/pushover/retry', $post['pushoverretry']);
		}
		if (!empty($post['pushoverexpire']) && !is_numeric($post['pushoverexpire'])) {
			$input_errors[] = gettext("Please enter valid notification expiration time in seconds.");
		} elseif (!empty($post['pushoverexpire']) && ($post['pushoverretry'] > 10800)) {
			$input_errors[] = gettext("Please enter valid notification expiration time with maximum value of 10800 (3 hours).");
		} else {
			config_set_path('notifications/pushover/expire', $post['pushoverexpire']);
		}

		// Slack
		config_set_path('notifications/slack/enabled', (($post['enable_slack'] == "yes") ? true : false));
		config_set_path('notifications/slack/api', $post['slack_api']);
		config_set_path('notifications/slack/channel', $post['slack_channel']);

		if (preg_replace("/[^a-zA-Z0-9:\-]/", "", config_get_path('notifications/slack/api')) !== config_get_path('notifications/slack/api')) {
			$input_errors[] = gettext("The only special characters permitted in the Slack API string are - and :");
		}
		if (preg_replace("/[^a-z0-9\-_]/", "", config_get_path('notifications/slack/channel')) !== config_get_path('notifications/slack/channel')) {
			$input_errors[] = gettext("The Slack Channel name may only contain lowercase letters, numbers, hyphens, and underscores.");
		}
		if (strlen(config_get_path('notifications/slack/channel')) > 80) {
			$input_errors[] = gettext("The Slack Channel name must be 80 characters or less.");
		}

		if (!$input_errors && !$testsmtp && !$testtelegram && !$testpushover && !$testslack) {
			write_config("Notifications Advanced Settings saved");

			if (!$json) {
				pfSenseHeader("system_advanced_notifications.php");
				return;
			}
		}
	}

	if ($testsmtp) {
		// Send test message via smtp
		if (file_exists("/var/db/notices_lastmsg.txt")) {
			unlink("/var/db/notices_lastmsg.txt");
		}

		$test_result = notify_via_smtp(sprintf(gettext("This is a test message from %s. It is safe to ignore this message."), g_get('product_label')), true);
		if (empty($test_result)) {
			$test_result = gettext("SMTP testing e-mail successfully sent");
			$test_class = 'success';
		} else {
			$test_class = 'danger';
		}
	}

	if ($testtelegram) {
		// Send test message via telegram
		$test_result = notify_via_telegram(sprintf(gettext("This is a Telegram test message from %s. It is safe to ignore this message."), g_get('product_label')), true);
		if (empty($test_result)) {
			$test_result = gettext("Telegram testing message successfully sent");
			$test_class = 'success';
		} else {
			$test_class = 'danger';
		}
	}

	if ($testpushover) {
		// Send test message via pushover
		$test_result = notify_via_pushover(sprintf(gettext("This is a Pushover test message from %s. It is safe to ignore this message."), g_get('product_label')), true);
		if (empty($test_result)) {
			$test_result = gettext("Pushover testing message successfully sent");
			$test_class = 'success';
		} else {
			$test_class = 'danger';
		}
	}

	if ($testslack) {
		// Send test message via slack
		$test_result = notify_via_slack(sprintf(gettext("This is a Slack test message from %s. It is safe to ignore this message."), g_get('product_label')), true);
		if (empty($test_result)) {
			$test_result = gettext("Slack testing message successfully sent");
			$test_class = 'success';
		} else {
			$test_class = 'danger';
		}
	}

	$rv['pconfig'] = $post;
	$rv['test_result'] = $test_result;
	$rv['test_class'] = $test_class;
	$rv['input_errors'] = $input_errors;

	return $json ? json_encode($rv):$rv;
}

?>
