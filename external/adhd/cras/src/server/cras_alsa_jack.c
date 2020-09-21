/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <alsa/asoundlib.h>
#include <linux/input.h>
#include <regex.h>
#include <syslog.h>

#include "cras_alsa_jack.h"
#include "cras_alsa_mixer.h"
#include "cras_alsa_ucm.h"
#include "cras_system_state.h"
#include "cras_gpio_jack.h"
#include "cras_tm.h"
#include "cras_util.h"
#include "edid_utils.h"
#include "utlist.h"

static const unsigned int DISPLAY_INFO_RETRY_DELAY_MS = 200;
static const unsigned int DISPLAY_INFO_MAX_RETRIES = 10;
static const unsigned int DISPLAY_INFO_GPIO_MAX_RETRIES = 25;

/* Constants used to retrieve monitor name from ELD buffer. */
static const unsigned int ELD_MNL_MASK = 31;
static const unsigned int ELD_MNL_OFFSET = 4;
static const unsigned int ELD_MONITOR_NAME_OFFSET = 20;

/* Keeps an fd that is registered with system settings.  A list of fds must be
 * kept so that they can be removed when the jack list is destroyed. */
struct jack_poll_fd {
	int fd;
	struct jack_poll_fd *prev, *next;
};

/* cras_gpio_jack:  Describes headphone & microphone jack connected to GPIO
 *
 *   On Arm-based systems, the headphone & microphone jacks are
 *   connected to GPIOs which are plumbed through the /dev/input/event
 *   system.  For these jacks, the software is written to open the
 *   corresponding /dev/input/event file and monitor it for 'insert' &
 *   'remove' activity.
 *
 *   fd           : File descriptor corresponding to the /dev/input/event file.
 *
 *   switch_event : Indicates the type of the /dev/input/event file.
 *                  Either SW_HEADPHONE_INSERT, or SW_MICROPHONE_INSERT.
 *
 *   current_state: 0 -> device not plugged in
 *                  1 -> device plugged in
 *   device_name  : Device name extracted from /dev/input/event[0..9]+.
 *                  Allocated on heap; must free.
 */
struct cras_gpio_jack {
	int fd;
	unsigned switch_event;
	unsigned current_state;
	char *device_name;
};

/* Represents a single alsa Jack, e.g. "Headphone Jack" or "Mic Jack".
 *    is_gpio: 1 -> gpio switch (union field: gpio)
 *             0 -> Alsa 'jack' (union field: elem)
 *    elem - alsa hcontrol element for this jack, when is_gpio == 0.
 *    gpio - description of gpio-based jack, when is_gpio != 0.
 *    eld_control - mixer control for ELD info buffer.
 *    jack_list - list of jacks this belongs to.
 *    mixer_output - mixer output control used to control audio to this jack.
 *        This will be null for input jacks.
 *    mixer_input - mixer input control used to control audio to this jack.
 *        This will be null for output jacks.
 *    ucm_device - Name of the ucm device if found, otherwise, NULL.
 *    edid_file - File to read the EDID from (if available, HDMI only).
 *    display_info_timer - Timer used to poll display info for HDMI jacks.
 *    display_info_retries - Number of times to retry reading display info.
 */
struct cras_alsa_jack {
	unsigned is_gpio;	/* !0 -> 'gpio' valid
				 *  0 -> 'elem' valid
				 */
	union {
		snd_hctl_elem_t *elem;
		struct cras_gpio_jack gpio;
	};

	snd_hctl_elem_t *eld_control;
	struct cras_alsa_jack_list *jack_list;
	struct mixer_control *mixer_output;
	struct mixer_control *mixer_input;
	char *ucm_device;
	const char *dsp_name;
	const char* override_type_name;
	const char *edid_file;
	struct cras_timer *display_info_timer;
	unsigned int display_info_retries;
	struct cras_alsa_jack *prev, *next;
};

/* Contains all Jacks for a given device.
 *    hctl - alsa hcontrol for this device's card
 *         - not opened by the jack list.
 *    mixer - cras mixer for the card providing this device.
 *    ucm - CRAS use case manager if available.
 *    card_index - Index ALSA uses to refer to the card.  The X in "hw:X".
 *    card_name - The name of the card.
 *    device_index - Index ALSA uses to refer to the device.  The Y in "hw:X,Y".
 *    is_first_device - whether this device is the first device on the card.
 *    direction - Input or output.
 *    change_callback - function to call when the state of a jack changes.
 *    callback_data - data to pass back to the callback.
 *    jacks - list of jacks for this device.
 */
struct cras_alsa_jack_list {
	snd_hctl_t *hctl;
	struct cras_alsa_mixer *mixer;
	struct cras_use_case_mgr *ucm;
	unsigned int card_index;
	const char *card_name;
	size_t device_index;
	int is_first_device;
	enum CRAS_STREAM_DIRECTION direction;
	jack_state_change_callback *change_callback;
	void *callback_data;
	struct cras_alsa_jack *jacks;
};

/* Used to contain information needed while looking through GPIO jacks.
 *    jack_list - The current jack_list.
 *    section - An associated UCM section.
 *    result_jack - The resulting jack.
 *    rc - The return code for the operation.
 */
struct gpio_switch_list_data {
	struct cras_alsa_jack_list *jack_list;
	struct ucm_section *section;
	struct cras_alsa_jack *result_jack;
	int rc;
};

/*
 * Local Helpers.
 */

#define BITS_PER_BYTE		(8)
#define BITS_PER_LONG		(sizeof(long) * BITS_PER_BYTE)
#define NBITS(x)		((((x) - 1) / BITS_PER_LONG) + 1)
#define OFF(x)			((x) % BITS_PER_LONG)
#define BIT(x)			(1UL << OFF(x))
#define LONG(x)			((x) / BITS_PER_LONG)
#define IS_BIT_SET(bit, array)	!!((array[LONG(bit)]) & (1UL << OFF(bit)))

static int sys_input_get_switch_state(int fd, unsigned sw, unsigned *state)
{
	unsigned long bits[NBITS(SW_CNT)];
	const unsigned long switch_no = sw;

	memset(bits, '\0', sizeof(bits));
	/* If switch event present & supported, get current state. */
	if (gpio_switch_eviocgbit(fd, bits, sizeof(bits)) < 0)
		return -EIO;

	if (IS_BIT_SET(switch_no, bits))
		if (gpio_switch_eviocgsw(fd, bits, sizeof(bits)) >= 0) {
			*state = IS_BIT_SET(switch_no, bits);
			return 0;
		}

	return -1;
}

static inline struct cras_alsa_jack *cras_alloc_jack(int is_gpio)
{
	struct cras_alsa_jack *jack = calloc(1, sizeof(*jack));
	if (jack == NULL)
		return NULL;
	jack->is_gpio = is_gpio;
	return jack;
}

static void cras_free_jack(struct cras_alsa_jack *jack,
			   int rm_select_fd)
{
	if (!jack)
		return;

	free(jack->ucm_device);
	free((void *)jack->edid_file);
	if (jack->display_info_timer)
		cras_tm_cancel_timer(cras_system_state_get_tm(),
				     jack->display_info_timer);

	if (jack->is_gpio) {
		free(jack->gpio.device_name);
		if (jack->gpio.fd >= 0) {
			if (rm_select_fd)
				cras_system_rm_select_fd(jack->gpio.fd);
			close(jack->gpio.fd);
		}
	}

	/*
	 * Remove the jack callback set on hctl. Otherwise, snd_hctl_close will
	 * trigger a callback while iodev might already be destroyed.
	 */
	if (!jack->is_gpio && jack->elem)
		snd_hctl_elem_set_callback(jack->elem, NULL);

	free((void *)jack->override_type_name);
	free((void *)jack->dsp_name);
	free(jack);
}

/* Gets the current plug state of the jack */
static int get_jack_current_state(struct cras_alsa_jack *jack)
{
	snd_ctl_elem_value_t *elem_value;

	if (jack->is_gpio)
		return jack->gpio.current_state;

	snd_ctl_elem_value_alloca(&elem_value);
	snd_hctl_elem_read(jack->elem, elem_value);

	return snd_ctl_elem_value_get_boolean(elem_value, 0);
}

static int read_jack_edid(const struct cras_alsa_jack *jack, uint8_t *edid)
{
	int fd, nread;

	fd = open(jack->edid_file, O_RDONLY);
	if (fd < 0)
		return -1;

	nread = read(fd, edid, EEDID_SIZE);
	close(fd);

	if (nread < EDID_SIZE || !edid_valid(edid))
		return -1;
	return 0;
}

static int check_jack_edid(struct cras_alsa_jack *jack)
{
	uint8_t edid[EEDID_SIZE];

	if (read_jack_edid(jack, edid))
		return -1;

	/* If the jack supports EDID, check that it supports audio, clearing
	 * the plugged state if it doesn't.
	 */
	if (!edid_lpcm_support(edid, edid[EDID_EXT_FLAG]))
		jack->gpio.current_state = 0;
	return 0;
}

static int get_jack_edid_monitor_name(const struct cras_alsa_jack *jack,
				      char *buf,
				      unsigned int buf_size)
{
	uint8_t edid[EEDID_SIZE];

	if (read_jack_edid(jack, edid))
		return -1;

	return edid_get_monitor_name(edid, buf, buf_size);
}

/* Checks the ELD control of the jack to see if the ELD buffer
 * is ready to read and report the plug status.
 */
static int check_jack_eld(struct cras_alsa_jack *jack)
{
	snd_ctl_elem_info_t *elem_info;
	snd_ctl_elem_info_alloca(&elem_info);

	/* Poll ELD control by getting the count of ELD buffer.
	 * When seeing zero buffer count, retry after a delay until
	 * it's ready or reached the max number of retries. */
	if (snd_hctl_elem_info(jack->eld_control, elem_info) != 0)
		return -1;
	if (snd_ctl_elem_info_get_count(elem_info) == 0)
		return -1;
	return 0;
}

static void display_info_delay_cb(struct cras_timer *timer, void *arg);

/* Callback function doing following things:
 * 1. Reset timer and update max number of retries.
 * 2. Check all conditions to see if it's okay or needed to
 *    report jack status directly. E.g. jack is unplugged or
 *    EDID is not ready for some reason.
 * 3. Check if max number of retries is reached and decide
 *    to set timer for next callback or report jack state.
 */
static inline void jack_state_change_cb(struct cras_alsa_jack *jack, int retry)
{
	struct cras_tm *tm = cras_system_state_get_tm();

	if (jack->display_info_timer) {
		cras_tm_cancel_timer(tm, jack->display_info_timer);
		jack->display_info_timer = NULL;
	}
	if (retry) {
		jack->display_info_retries =
				jack->is_gpio ? DISPLAY_INFO_GPIO_MAX_RETRIES
					      : DISPLAY_INFO_MAX_RETRIES;
	}

	if (!get_jack_current_state(jack))
		goto report_jack_state;

	/* If there is an edid file, check it.  If it is ready continue, if we
	 * need to try again later, return here as the timer has been armed and
	 * will check again later.
	 */
	if (jack->edid_file == NULL && jack->eld_control == NULL)
		goto report_jack_state;
	if (jack->edid_file && (check_jack_edid(jack) == 0))
		goto report_jack_state;
	if (jack->eld_control && (check_jack_eld(jack) == 0))
		goto report_jack_state;

	if (--jack->display_info_retries == 0) {
		if (jack->is_gpio)
			jack->gpio.current_state = 0;
		if (jack->edid_file)
			syslog(LOG_ERR, "Timeout to read EDID from %s",
			       jack->edid_file);
		goto report_jack_state;
	}

	jack->display_info_timer = cras_tm_create_timer(tm,
						DISPLAY_INFO_RETRY_DELAY_MS,
						display_info_delay_cb, jack);
	return;

report_jack_state:
	jack->jack_list->change_callback(jack,
					 get_jack_current_state(jack),
					 jack->jack_list->callback_data);
}

/* gpio_switch_initial_state
 *
 *   Determines the initial state of a gpio-based switch.
 */
static void gpio_switch_initial_state(struct cras_alsa_jack *jack)
{
	unsigned v;
	int r = sys_input_get_switch_state(jack->gpio.fd,
					   jack->gpio.switch_event, &v);
	jack->gpio.current_state = r == 0 ? v : 0;
	jack_state_change_cb(jack, 1);
}

/* Check if the input event is an audio switch event. */
static inline int is_audio_switch_event(const struct input_event *ev,
					int sw_code)
{
	return (ev->type == EV_SW && ev->code == sw_code);
}

/* Timer callback to read display info after a hotplug event for an HDMI jack.
 */
static void display_info_delay_cb(struct cras_timer *timer, void *arg)
{
	struct cras_alsa_jack *jack = (struct cras_alsa_jack *)arg;

	jack->display_info_timer = NULL;
	jack_state_change_cb(jack, 0);
}

/* gpio_switch_callback
 *
 *   This callback is invoked whenever the associated /dev/input/event
 *   file has data to read.  Perform autoswitching to / from the
 *   associated device when data is available.
 */
static void gpio_switch_callback(void *arg)
{
	struct cras_alsa_jack *jack = arg;
	int i;
	int r;
	struct input_event ev[64];

	r = gpio_switch_read(jack->gpio.fd, ev,
			     ARRAY_SIZE(ev) * sizeof(struct input_event));
	if (r < 0)
		return;

	for (i = 0; i < r / sizeof(struct input_event); ++i)
		if (is_audio_switch_event(&ev[i], jack->gpio.switch_event)) {
			jack->gpio.current_state = ev[i].value;

			jack_state_change_cb(jack, 1);
		}
}

/* Determines if the GPIO jack should be associated with the device of the
 * jack list. If the device name is not specified in UCM (common case),
 * assume it should be associated with the first input device or the first
 * output device on the card.
 */
static unsigned int gpio_jack_match_device(const struct cras_alsa_jack *jack,
			struct cras_alsa_jack_list* jack_list,
			const char *card_name,
			enum CRAS_STREAM_DIRECTION direction)
{
	const char* target_device_name = NULL;
	char current_device_name[CRAS_IODEV_NAME_BUFFER_SIZE];
	unsigned int rc;

	/* If the device name is not specified in UCM, assume it should be
	 * associated with device 0. */
	if (!jack_list->ucm || !jack->ucm_device)
		return jack_list->is_first_device;

	/* Look for device name specified in a device section of UCM. */
	target_device_name = ucm_get_device_name_for_dev(
				jack_list->ucm, jack->ucm_device, direction);

	if (!target_device_name)
		return jack_list->is_first_device;

	syslog(LOG_DEBUG, "Matching GPIO jack, target device name: %s, "
	       "current card name: %s, device index: %zu\n",
		target_device_name, card_name, jack_list->device_index);

	/* Device name of format "hw:<card_name>,<device_index>", should fit
	 * in the string of size CRAS_IODEV_NAME_BUFFER_SIZE.*/
	snprintf(current_device_name,
		 sizeof(current_device_name),
		 "hw:%s,%zu",
		 card_name,
		 jack_list->device_index);

	rc = !strcmp(current_device_name, target_device_name);
	free((void*)target_device_name);
	return rc;
}

static int create_jack_for_gpio(struct cras_alsa_jack_list *jack_list,
				const char *pathname,
				const char *dev_name,
				unsigned switch_event,
				struct cras_alsa_jack **out_jack)
{
	struct cras_alsa_jack *jack;
	unsigned long bits[NBITS(SW_CNT)];
	const char *card_name = jack_list->card_name;
	int r;

	if (!out_jack)
		return -EINVAL;
	*out_jack = NULL;

	jack = cras_alloc_jack(1);
	if (jack == NULL)
		return -ENOMEM;

	jack->gpio.fd = gpio_switch_open(pathname);
	if (jack->gpio.fd == -1) {
		r = -EIO;
		goto error;
	}

	jack->gpio.switch_event = switch_event;
	jack->jack_list = jack_list;
	jack->gpio.device_name = strdup(dev_name);
	if (!jack->gpio.device_name) {
		r = -ENOMEM;
		goto error;
	}

	if (!strstr(jack->gpio.device_name, card_name) ||
	    (gpio_switch_eviocgbit(jack->gpio.fd, bits, sizeof(bits)) < 0) ||
	    !IS_BIT_SET(switch_event, bits)) {
		r = -EIO;
		goto error;
	}

	*out_jack = jack;
	return 0;

error:
	/* Not yet registered with system select. */
	cras_free_jack(jack, 0);
	return r;
}

/* Take ownership and finish setup of the jack.
 * Add the jack to the jack_list if everything goes well, or destroy it.
 */
static int cras_complete_gpio_jack(struct gpio_switch_list_data *data,
				   struct cras_alsa_jack *jack,
				   unsigned switch_event)
{
	struct cras_alsa_jack_list *jack_list = data->jack_list;
	enum CRAS_STREAM_DIRECTION direction = jack_list->direction;
	int r;

	if (jack->ucm_device) {
		jack->edid_file = ucm_get_edid_file_for_dev(jack_list->ucm,
							    jack->ucm_device);
		jack->dsp_name = ucm_get_dsp_name(
			jack->jack_list->ucm, jack->ucm_device, direction);
	}

	r = sys_input_get_switch_state(jack->gpio.fd, switch_event,
				       &jack->gpio.current_state);
	if (r < 0) {
		cras_free_jack(jack, 0);
		return -EIO;
	}
	r = cras_system_add_select_fd(jack->gpio.fd,
				      gpio_switch_callback, jack);
	if (r < 0) {
		/* Not yet registered with system select. */
		cras_free_jack(jack, 0);
		return r;
	}

	DL_APPEND(jack_list->jacks, jack);
	if (!data->result_jack)
		data->result_jack = jack;
	else if (data->section)
		syslog(LOG_ERR,
		       "More than one jack for SectionDevice '%s'.",
		       data->section->name);
	return 0;
}

/* open_and_monitor_gpio:
 *
 *   Opens a /dev/input/event file associated with a headphone /
 *   microphone jack and watches it for activity.
 *   Returns 0 when a jack has been successfully added.
 */
static int open_and_monitor_gpio(struct gpio_switch_list_data *data,
				 const char *pathname,
				 const char *dev_name,
				 unsigned switch_event)
{
	struct cras_alsa_jack *jack;
	struct cras_alsa_jack_list *jack_list = data->jack_list;
	const char *card_name = jack_list->card_name;
	enum CRAS_STREAM_DIRECTION direction = jack_list->direction;
	int r;

	r = create_jack_for_gpio(jack_list, pathname, dev_name,
				 switch_event, &jack);
	if (r != 0)
		return r;

	if (jack_list->ucm)
		jack->ucm_device =
			ucm_get_dev_for_jack(jack_list->ucm,
					     jack->gpio.device_name,
					     direction);

	if (!gpio_jack_match_device(jack, jack_list, card_name, direction)) {
		cras_free_jack(jack, 0);
		return -EIO;
	}

	if (direction == CRAS_STREAM_OUTPUT &&
	    (strstr(jack->gpio.device_name, "Headphone") ||
	     strstr(jack->gpio.device_name, "Headset")))
		jack->mixer_output = cras_alsa_mixer_get_output_matching_name(
			jack_list->mixer,
			"Headphone");
	else if (direction == CRAS_STREAM_OUTPUT &&
		 strstr(jack->gpio.device_name, "HDMI"))
		jack->mixer_output = cras_alsa_mixer_get_output_matching_name(
			jack_list->mixer,
			"HDMI");

	if (jack->ucm_device && direction == CRAS_STREAM_INPUT) {
		char *control_name;
		control_name = ucm_get_cap_control(jack->jack_list->ucm,
						   jack->ucm_device);
		if (control_name)
			jack->mixer_input =
				cras_alsa_mixer_get_input_matching_name(
					jack_list->mixer,
					control_name);
	}

	return cras_complete_gpio_jack(data, jack, switch_event);
}

static int open_and_monitor_gpio_with_section(
			struct gpio_switch_list_data *data,
			const char *pathname,
			unsigned switch_event)
{
	struct cras_alsa_jack *jack;
	struct cras_alsa_jack_list *jack_list = data->jack_list;
	struct ucm_section *section = data->section;
	enum CRAS_STREAM_DIRECTION direction = jack_list->direction;
	int r;

	r = create_jack_for_gpio(jack_list, pathname, section->jack_name,
				 switch_event, &jack);
	if (r != 0)
		return r;

	jack->ucm_device = strdup(section->name);
	if (!jack->ucm_device) {
		cras_free_jack(jack, 0);
		return -ENOMEM;
	}

	if (direction == CRAS_STREAM_OUTPUT)
		jack->mixer_output = cras_alsa_mixer_get_control_for_section(
						jack_list->mixer, section);
	else if (direction == CRAS_STREAM_INPUT)
		jack->mixer_input = cras_alsa_mixer_get_control_for_section(
						jack_list->mixer, section);

	return cras_complete_gpio_jack(data, jack, switch_event);
}

/* Monitor GPIO switches for this jack_list.
 * Args:
 *    data - Data for GPIO switch search.
 *    dev_path - Device full path.
 *    dev_name - Device name.
 * Returns:
 *    0 for success, or negative on error. Assumes success if no jack is
 *    found, or if the jack could not be accessed.
 */
static int gpio_switches_monitor_device(struct gpio_switch_list_data *data,
					const char *dev_path,
					const char *dev_name)
{
	static const int out_switches[] = {SW_HEADPHONE_INSERT,
					   SW_LINEOUT_INSERT};
	static const int in_switches[] = {SW_MICROPHONE_INSERT};
	int sw;
	const int *switches = out_switches;
	int num_switches = ARRAY_SIZE(out_switches);
	int success = 1;
	int rc = 0;

	if (data->section && data->section->jack_switch >= 0) {
		switches = &data->section->jack_switch;
		num_switches = 1;
	}
	else if (data->jack_list->direction == CRAS_STREAM_INPUT) {
		switches = in_switches;
		num_switches = ARRAY_SIZE(in_switches);
	}

	/* Assume that -EIO is returned for jacks that we shouldn't
	 * be looking at, but stop trying if we run into another
	 * type of error.
	 */
	for (sw = 0; (rc == 0 || rc == -EIO)
		     && sw < num_switches; sw++) {
		if (data->section)
			rc = open_and_monitor_gpio_with_section(
				data, dev_path, switches[sw]);
		else
			rc = open_and_monitor_gpio(
				data, dev_path, dev_name, switches[sw]);
		if (rc != 0 && rc != -EIO)
			success = 0;
	}

	if (success)
		return 0;
	return rc;
}

static int gpio_switch_list_with_section(const char *dev_path,
					 const char *dev_name,
					 void *arg)
{
	struct gpio_switch_list_data *data =
		(struct gpio_switch_list_data *)arg;

	if (strcmp(dev_name, data->section->jack_name)) {
		/* No match: continue searching. */
		return 0;
	}

	data->rc = gpio_switches_monitor_device(data, dev_path, dev_name);
	/* Found the only possible match: stop searching. */
	return 1;
}

/* Match the given jack name to the given regular expression.
 * Args:
 *    jack_name - The jack's name.
 *    re - Regular expression string.
 * Returns:
 *    Non-zero for success, or 0 for failure.
 */
static int jack_matches_regex(const char *jack_name, const char *re)
{
	regmatch_t m[1];
	regex_t regex;
	int rc;

	rc = regcomp(&regex, re, REG_EXTENDED);
	if (rc != 0) {
		syslog(LOG_ERR, "Failed to compile regular expression: %s", re);
		return 0;
	}

	rc = regexec(&regex, jack_name, ARRAY_SIZE(m), m, 0) == 0;
	regfree(&regex);
	return rc;
}

static int gpio_switch_list_by_matching(const char *dev_path,
					const char *dev_name,
					void *arg)
{
	struct gpio_switch_list_data *data =
		(struct gpio_switch_list_data *)arg;

	if (data->jack_list->direction == CRAS_STREAM_INPUT) {
		if (!jack_matches_regex(dev_name, "^.*Mic Jack$") &&
		    !jack_matches_regex(dev_name, "^.*Headset Jack$")) {
			/* Continue searching. */
			return 0;
		}
	}
	else if (data->jack_list->direction == CRAS_STREAM_OUTPUT) {
		if (!jack_matches_regex(dev_name, "^.*Headphone Jack$") &&
		    !jack_matches_regex(dev_name, "^.*Headset Jack$") &&
		    !jack_matches_regex(dev_name, "^.*HDMI Jack$")) {
			/* Continue searching. */
			return 0;
		}
	}

	data->rc = gpio_switches_monitor_device(data, dev_path, dev_name);
	/* Stop searching for failure. */
	return data->rc;
}

/* Find GPIO jacks for this jack_list.
 * Args:
 *    jack_list - Jack list to add to.
 *    section - UCM section.
 *    result_jack - Filled with a pointer to the resulting cras_alsa_jack.
 * Returns:
 *    0 for success, or negative on error. Assumes success if no jack is
 *    found, or if the jack could not be accessed.
 */
static int find_gpio_jacks(struct cras_alsa_jack_list *jack_list,
			   struct ucm_section *section,
			   struct cras_alsa_jack **result_jack)
{
	/* GPIO switches are on Arm-based machines, and are
	 * only associated with on-board devices.
	 */
	struct gpio_switch_list_data data;
	int rc;

	rc = wait_for_dev_input_access();
	if (rc != 0) {
		syslog(LOG_WARNING, "Could not access /dev/input/event0: %s",
		       strerror(rc));
		return 0;
	}

	data.jack_list = jack_list;
	data.section = section;
	data.result_jack = NULL;
	data.rc = 0;

	if (section)
		gpio_switch_list_for_each(
			gpio_switch_list_with_section, &data);
	else
		gpio_switch_list_for_each(
			gpio_switch_list_by_matching, &data);
	if (result_jack)
		*result_jack = data.result_jack;
	return data.rc;
}

/* Callback from alsa when a jack control changes.  This is registered with
 * snd_hctl_elem_set_callback in find_jack_controls and run by calling
 * snd_hctl_handle_events in alsa_control_event_pending below.
 * Args:
 *    elem - The ALSA control element that has changed.
 *    mask - unused.
 */
static int hctl_jack_cb(snd_hctl_elem_t *elem, unsigned int mask)
{
	const char *name;
	snd_ctl_elem_value_t *elem_value;
	struct cras_alsa_jack *jack;

	jack = snd_hctl_elem_get_callback_private(elem);
	if (jack == NULL) {
		syslog(LOG_ERR, "Invalid jack from control event.");
		return -EINVAL;
	}

	snd_ctl_elem_value_alloca(&elem_value);
	snd_hctl_elem_read(elem, elem_value);
	name = snd_hctl_elem_get_name(elem);

	syslog(LOG_DEBUG,
	       "Jack %s %s",
	       name,
	       snd_ctl_elem_value_get_boolean(elem_value, 0) ? "plugged"
							     : "unplugged");
	jack_state_change_cb(jack, 1);
	return 0;
}

/* Determines the device associated with this jack if any.  If the device cannot
 * be determined (common case), assume device 0. */
static unsigned int hctl_jack_device_index(const char *name)
{
	/* Look for the substring 'pcm=<device number>' in the element name. */
	static const char pcm_search[] = "pcm=";
	const char *substr;
	int device_index;

	substr = strstr(name, pcm_search);
	if (substr == NULL)
		return 0;
	substr += ARRAY_SIZE(pcm_search) - 1;
	if (*substr == '\0')
		return 0;
	device_index = atoi(substr);
	if (device_index < 0)
		return 0;
	return (unsigned int)device_index;
}

/* For non-gpio jack, check if it's of type hdmi/dp by
 * matching jack name. */
static int is_jack_hdmi_dp(const char *jack_name)
{
	static const char *hdmi_dp = "HDMI/DP";
	return strncmp(jack_name, hdmi_dp, strlen(hdmi_dp)) == 0;
}

/* Checks if the given control name is in the supplied list of possible jack
 * control base names. */
static int is_jack_control_in_list(const char * const *list,
				   unsigned int list_length,
				   const char *control_name)
{
	unsigned int i;

	for (i = 0; i < list_length; i++)
		if (strncmp(control_name, list[i], strlen(list[i])) == 0)
			return 1;
	return 0;
}

/* Looks for any JACK controls.  Monitors any found controls for changes and
 * decides to route based on plug/unlpug events. */
static int find_jack_controls(struct cras_alsa_jack_list *jack_list)
{
	snd_hctl_elem_t *elem;
	struct cras_alsa_jack *jack;
	const char *name;
	static const char * const output_jack_base_names[] = {
		"Headphone Jack",
		"Front Headphone Jack",
		"HDMI/DP",
		"Speaker Phantom Jack",
	};
	static const char * const input_jack_base_names[] = {
		"Mic Jack",
	};
	static const char eld_control_name[] = "ELD";
	const char * const *jack_names;
	unsigned int num_jack_names;
	char device_name[6];

	if (!jack_list->hctl) {
		syslog(LOG_WARNING, "Can't search hctl for jacks.");
		return 0;
	}

	if (jack_list->direction == CRAS_STREAM_OUTPUT) {
		jack_names = output_jack_base_names;
		num_jack_names = ARRAY_SIZE(output_jack_base_names);
	} else {
		jack_names = input_jack_base_names;
		num_jack_names = ARRAY_SIZE(input_jack_base_names);
	}

	for (elem = snd_hctl_first_elem(jack_list->hctl); elem != NULL;
			elem = snd_hctl_elem_next(elem)) {
		snd_ctl_elem_iface_t iface;

		iface = snd_hctl_elem_get_interface(elem);
		if (iface != SND_CTL_ELEM_IFACE_CARD)
			continue;
		name = snd_hctl_elem_get_name(elem);
		if (!is_jack_control_in_list(jack_names, num_jack_names, name))
			continue;
		if (hctl_jack_device_index(name) != jack_list->device_index)
			continue;

		jack = cras_alloc_jack(0);
		if (jack == NULL)
			return -ENOMEM;
		jack->elem = elem;
		jack->jack_list = jack_list;
		DL_APPEND(jack_list->jacks, jack);

		syslog(LOG_DEBUG, "Found Jack: %s for %s", name, device_name);
		snd_hctl_elem_set_callback(elem, hctl_jack_cb);
		snd_hctl_elem_set_callback_private(elem, jack);

		if (jack_list->direction == CRAS_STREAM_OUTPUT)
			jack->mixer_output =
				cras_alsa_mixer_get_output_matching_name(
					jack_list->mixer,
					name);
		if (jack_list->ucm)
			jack->ucm_device =
				ucm_get_dev_for_jack(jack_list->ucm, name,
						     jack_list->direction);

		if (jack->ucm_device && jack_list->direction == CRAS_STREAM_INPUT) {
			char *control_name;
			control_name = ucm_get_cap_control(jack->jack_list->ucm,
						       jack->ucm_device);
			if (control_name)
				jack->mixer_input =
					cras_alsa_mixer_get_input_matching_name(
						jack_list->mixer,
						control_name);
		}

		if (jack->ucm_device) {
			jack->dsp_name = ucm_get_dsp_name(
				jack->jack_list->ucm, jack->ucm_device,
				jack_list->direction);
			jack->override_type_name = ucm_get_override_type_name(
				jack->jack_list->ucm, jack->ucm_device);
		}
	}

	/* Look up ELD controls */
	DL_FOREACH(jack_list->jacks, jack) {
		if (jack->is_gpio || jack->eld_control)
			continue;
		name = snd_hctl_elem_get_name(jack->elem);
		if (!is_jack_hdmi_dp(name))
			continue;
		for (elem = snd_hctl_first_elem(jack_list->hctl); elem != NULL;
		     elem = snd_hctl_elem_next(elem)) {
			if (strcmp(snd_hctl_elem_get_name(elem),
				   eld_control_name))
				continue;
			if (snd_hctl_elem_get_device(elem)
			    != jack_list->device_index)
				continue;
			jack->eld_control = elem;
			break;
		}
	}

	return 0;
}

/*
 * Exported Interface.
 */

int cras_alsa_jack_list_find_jacks_by_name_matching(
	struct cras_alsa_jack_list *jack_list)
{
	int rc;

	rc = find_jack_controls(jack_list);
	if (rc != 0)
		return rc;

	return find_gpio_jacks(jack_list, NULL, NULL);
}

static int find_hctl_jack_for_section(
		struct cras_alsa_jack_list *jack_list,
		struct ucm_section *section,
		struct cras_alsa_jack **result_jack)
{
	static const char eld_control_name[] = "ELD";
	snd_hctl_elem_t *elem;
	snd_ctl_elem_id_t *elem_id;
	struct cras_alsa_jack *jack;

	if (!jack_list->hctl) {
		syslog(LOG_WARNING, "Can't search hctl for jacks.");
		return -ENODEV;
	}

	snd_ctl_elem_id_alloca(&elem_id);
	snd_ctl_elem_id_clear(elem_id);
	snd_ctl_elem_id_set_interface(elem_id, SND_CTL_ELEM_IFACE_CARD);
	snd_ctl_elem_id_set_device(elem_id, jack_list->device_index);
	snd_ctl_elem_id_set_name(elem_id, section->jack_name);
	elem = snd_hctl_find_elem(jack_list->hctl, elem_id);
	if (!elem)
		return -ENOENT;

	syslog(LOG_DEBUG, "Found Jack: %s for %s",
	       section->jack_name, section->name);

	jack = cras_alloc_jack(0);
	if (jack == NULL)
		return -ENOMEM;
	jack->elem = elem;
	jack->jack_list = jack_list;

	jack->ucm_device = strdup(section->name);
	if (!jack->ucm_device) {
		free(jack);
		return -ENOMEM;
	}
	if (jack_list->direction == CRAS_STREAM_OUTPUT)
		jack->mixer_output = cras_alsa_mixer_get_control_for_section(
					jack_list->mixer, section);
	else if (jack_list->direction == CRAS_STREAM_INPUT)
		jack->mixer_input = cras_alsa_mixer_get_control_for_section(
					jack_list->mixer, section);

	jack->dsp_name = ucm_get_dsp_name(
		jack->jack_list->ucm, jack->ucm_device,
		jack_list->direction);

	snd_hctl_elem_set_callback(elem, hctl_jack_cb);
	snd_hctl_elem_set_callback_private(elem, jack);
	DL_APPEND(jack_list->jacks, jack);
	if (result_jack)
		*result_jack = jack;

	if (!strcmp(jack->ucm_device, "HDMI") ||
	    !strcmp(jack->ucm_device, "DP"))
		return 0;

	/* Look up ELD control. */
	snd_ctl_elem_id_set_name(elem_id, eld_control_name);
	elem = snd_hctl_find_elem(jack_list->hctl, elem_id);
	if (elem)
		jack->eld_control = elem;
	return 0;
}

/*
 * Exported Interface.
 */

int cras_alsa_jack_list_add_jack_for_section(
	struct cras_alsa_jack_list *jack_list,
	struct ucm_section *ucm_section,
	struct cras_alsa_jack **result_jack)
{
	if (result_jack)
		*result_jack = NULL;
	if (!ucm_section)
		return -EINVAL;

	if (!ucm_section->jack_name) {
		/* No jacks defined for this device. */
		return 0;
	}

	if (!ucm_section->jack_type) {
		syslog(LOG_ERR,
		       "Must specify the JackType for jack '%s' in '%s'.",
		       ucm_section->jack_name, ucm_section->name);
		return -EINVAL;
	}

	if (!strcmp(ucm_section->jack_type, "hctl")) {
		return find_hctl_jack_for_section(
				jack_list, ucm_section, result_jack);
	} else if (!strcmp(ucm_section->jack_type, "gpio")) {
		return find_gpio_jacks(jack_list, ucm_section, result_jack);
	} else {
		syslog(LOG_ERR,
		       "Invalid JackType '%s' in '%s'.",
		       ucm_section->jack_type, ucm_section->name);
		return -EINVAL;
	}
}

struct cras_alsa_jack_list *cras_alsa_jack_list_create(
		unsigned int card_index,
		const char *card_name,
		unsigned int device_index,
		int is_first_device,
		struct cras_alsa_mixer *mixer,
		struct cras_use_case_mgr *ucm,
		snd_hctl_t *hctl,
		enum CRAS_STREAM_DIRECTION direction,
		jack_state_change_callback *cb,
		void *cb_data)
{
	struct cras_alsa_jack_list *jack_list;

	if (direction != CRAS_STREAM_INPUT && direction != CRAS_STREAM_OUTPUT)
		return NULL;

	jack_list = (struct cras_alsa_jack_list *)calloc(1, sizeof(*jack_list));
	if (jack_list == NULL)
		return NULL;

	jack_list->change_callback = cb;
	jack_list->callback_data = cb_data;
	jack_list->mixer = mixer;
	jack_list->ucm = ucm;
	jack_list->hctl = hctl;
	jack_list->card_index = card_index;
	jack_list->card_name = card_name;
	jack_list->device_index = device_index;
	jack_list->is_first_device = is_first_device;
	jack_list->direction = direction;

	return jack_list;
}

void cras_alsa_jack_list_destroy(struct cras_alsa_jack_list *jack_list)
{
	struct cras_alsa_jack *jack;

	if (jack_list == NULL)
		return;
	DL_FOREACH(jack_list->jacks, jack) {
		DL_DELETE(jack_list->jacks, jack);
		cras_free_jack(jack, 1);
	}
	free(jack_list);
}

int cras_alsa_jack_list_has_hctl_jacks(struct cras_alsa_jack_list *jack_list)
{
	struct cras_alsa_jack *jack;

	if (!jack_list)
		return 0;
	DL_FOREACH(jack_list->jacks, jack) {
		if (!jack->is_gpio)
			return 1;
	}
	return 0;
}

struct mixer_control *cras_alsa_jack_get_mixer_output(
		const struct cras_alsa_jack *jack)
{
	if (jack == NULL)
		return NULL;
	return jack->mixer_output;
}

struct mixer_control *cras_alsa_jack_get_mixer_input(
		const struct cras_alsa_jack *jack)
{
	return jack ? jack->mixer_input : NULL;
}

void cras_alsa_jack_list_report(const struct cras_alsa_jack_list *jack_list)
{
	struct cras_alsa_jack *jack;

	if (jack_list == NULL)
		return;

	DL_FOREACH(jack_list->jacks, jack)
		if (jack->is_gpio)
			gpio_switch_initial_state(jack);
		else
			hctl_jack_cb(jack->elem, 0);
}

const char *cras_alsa_jack_get_name(const struct cras_alsa_jack *jack)
{
	if (jack == NULL)
		return NULL;
	if (jack->is_gpio)
		return jack->gpio.device_name;
	return snd_hctl_elem_get_name(jack->elem);
}

const char *cras_alsa_jack_get_ucm_device(const struct cras_alsa_jack *jack)
{
	return jack->ucm_device;
}

void cras_alsa_jack_update_monitor_name(const struct cras_alsa_jack *jack,
					char *name_buf,
					unsigned int buf_size)
{
	snd_ctl_elem_value_t *elem_value;
	snd_ctl_elem_info_t *elem_info;
	const char *buf = NULL;
	int count;
	int mnl = 0;

	if (!jack->eld_control) {
		if (jack->edid_file)
			get_jack_edid_monitor_name(jack, name_buf, buf_size);
		return;
	}

	snd_ctl_elem_info_alloca(&elem_info);
	if (snd_hctl_elem_info(jack->eld_control, elem_info) < 0)
		goto fallback_jack_name;

	count = snd_ctl_elem_info_get_count(elem_info);
	if (count <= ELD_MNL_OFFSET)
		goto fallback_jack_name;

	snd_ctl_elem_value_alloca(&elem_value);
	if (snd_hctl_elem_read(jack->eld_control, elem_value) < 0)
		goto fallback_jack_name;

	buf = (const char *)snd_ctl_elem_value_get_bytes(elem_value);
	mnl = buf[ELD_MNL_OFFSET] & ELD_MNL_MASK;

	if (count < ELD_MONITOR_NAME_OFFSET + mnl)
		goto fallback_jack_name;

	/* Note that monitor name string does not contain terminate character.
	 * Check monitor name length with name buffer size.
	 */
	if (mnl >= buf_size)
		mnl = buf_size - 1;
	strncpy(name_buf, buf + ELD_MONITOR_NAME_OFFSET, mnl);
	name_buf[mnl] = '\0';

	return;

fallback_jack_name:
	buf = cras_alsa_jack_get_name(jack);
	strncpy(name_buf, buf, buf_size - 1);

	return;
}

void cras_alsa_jack_update_node_type(const struct cras_alsa_jack *jack,
				     enum CRAS_NODE_TYPE *type)
{
	if (!jack->override_type_name)
		return;
	if (!strcmp(jack->override_type_name, "Internal Speaker"))
		*type = CRAS_NODE_TYPE_INTERNAL_SPEAKER;
	return;
}

const char *cras_alsa_jack_get_dsp_name(const struct cras_alsa_jack *jack)
{
	if (jack == NULL)
		return NULL;
	return jack->dsp_name;
}

void cras_alsa_jack_enable_ucm(const struct cras_alsa_jack *jack, int enable)
{
	if (jack && jack->ucm_device)
		ucm_set_enabled(jack->jack_list->ucm, jack->ucm_device, enable);
}
