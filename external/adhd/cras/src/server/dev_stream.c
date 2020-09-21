/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <syslog.h>

#include "audio_thread_log.h"
#include "byte_buffer.h"
#include "cras_fmt_conv.h"
#include "dev_stream.h"
#include "cras_audio_area.h"
#include "cras_mix.h"
#include "cras_shm.h"

/*
 * Sleep this much time past the buffer size to be sure at least
 * the buffer size is captured when the audio thread wakes up.
 */
static const unsigned int capture_extra_sleep_frames = 20;
/* Adjust device's sample rate by this step faster or slower. Used
 * to make sure multiple active device has stable buffer level.
 */
static const int coarse_rate_adjust_step = 3;

/*
 * Allow capture callback to fire this much earlier than the scheduled
 * next_cb_ts to avoid an extra wake of audio thread.
 */
static const struct timespec capture_callback_fuzz_ts = {
	.tv_sec = 0,
	.tv_nsec = 1000000, /* 1 ms. */
};

struct dev_stream *dev_stream_create(struct cras_rstream *stream,
				     unsigned int dev_id,
				     const struct cras_audio_format *dev_fmt,
				     void *dev_ptr,
				     struct timespec *cb_ts)
{
	struct dev_stream *out;
	struct cras_audio_format *stream_fmt = &stream->format;
	int rc = 0;
	unsigned int max_frames, dev_frames, buf_bytes;
	const struct cras_audio_format *ofmt;

	out = calloc(1, sizeof(*out));
	out->dev_id = dev_id;
	out->stream = stream;
	out->dev_rate = dev_fmt->frame_rate;

	if (stream->direction == CRAS_STREAM_OUTPUT) {
		max_frames = MAX(stream->buffer_frames,
				 cras_frames_at_rate(stream_fmt->frame_rate,
						     stream->buffer_frames,
						     dev_fmt->frame_rate));
		rc = config_format_converter(&out->conv,
					     stream->direction,
					     stream_fmt,
					     dev_fmt,
					     max_frames);
	} else {
		max_frames = MAX(stream->buffer_frames,
				 cras_frames_at_rate(dev_fmt->frame_rate,
						     stream->buffer_frames,
						     stream_fmt->frame_rate));
		rc = config_format_converter(&out->conv,
					     stream->direction,
					     dev_fmt,
					     stream_fmt,
					     max_frames);
	}
	if (rc) {
		free(out);
		return NULL;
	}

	ofmt = cras_fmt_conv_out_format(out->conv);

	dev_frames = (stream->direction == CRAS_STREAM_OUTPUT)
		? cras_fmt_conv_in_frames_to_out(out->conv,
						 stream->buffer_frames)
		: cras_fmt_conv_out_frames_to_in(out->conv,
						 stream->buffer_frames);

	out->conv_buffer_size_frames = 2 * MAX(dev_frames,
					       stream->buffer_frames);

	/* Create conversion buffer and area using the output format
	 * of the format converter. Note that this format might not be
	 * identical to stream_fmt for capture. */
	buf_bytes = out->conv_buffer_size_frames * cras_get_format_bytes(ofmt);
	out->conv_buffer = byte_buffer_create(buf_bytes);
	out->conv_area = cras_audio_area_create(ofmt->num_channels);

	cras_frames_to_time(cras_rstream_get_cb_threshold(stream),
			    stream_fmt->frame_rate,
			    &stream->sleep_interval_ts);
	stream->next_cb_ts = *cb_ts;

	if (stream->direction != CRAS_STREAM_OUTPUT) {
		struct timespec extra_sleep;

		cras_frames_to_time(capture_extra_sleep_frames,
				    stream->format.frame_rate, &extra_sleep);
		add_timespecs(&stream->next_cb_ts, &stream->sleep_interval_ts);
		add_timespecs(&stream->next_cb_ts, &extra_sleep);
	}

	cras_rstream_dev_attach(stream, dev_id, dev_ptr);

	return out;
}

void dev_stream_destroy(struct dev_stream *dev_stream)
{
	cras_rstream_dev_detach(dev_stream->stream, dev_stream->dev_id);
	if (dev_stream->conv) {
		cras_audio_area_destroy(dev_stream->conv_area);
		cras_fmt_conv_destroy(dev_stream->conv);
		byte_buffer_destroy(dev_stream->conv_buffer);
	}
	free(dev_stream);
}

void dev_stream_set_dev_rate(struct dev_stream *dev_stream,
			     unsigned int dev_rate,
			     double dev_rate_ratio,
			     double master_rate_ratio,
			     int coarse_rate_adjust)
{
	if (dev_stream->dev_id == dev_stream->stream->master_dev.dev_id) {
		cras_fmt_conv_set_linear_resample_rates(
				dev_stream->conv,
				dev_rate,
				dev_rate);
		cras_frames_to_time_precise(
			cras_rstream_get_cb_threshold(dev_stream->stream),
			dev_stream->stream->format.frame_rate * dev_rate_ratio,
			&dev_stream->stream->sleep_interval_ts);
	} else {
		double new_rate = dev_rate * dev_rate_ratio /
				master_rate_ratio +
				coarse_rate_adjust_step * coarse_rate_adjust;
		cras_fmt_conv_set_linear_resample_rates(
				dev_stream->conv,
				dev_rate,
				new_rate);
	}

}

int dev_stream_mix(struct dev_stream *dev_stream,
		   const struct cras_audio_format *fmt,
		   uint8_t *dst,
		   unsigned int num_to_write)
{
	struct cras_rstream *rstream = dev_stream->stream;
	uint8_t *src;
	uint8_t *target = dst;
	unsigned int fr_written, fr_read;
	unsigned int buffer_offset;
	int fr_in_buf;
	unsigned int num_samples;
	size_t frames = 0;
	unsigned int dev_frames;
	float mix_vol;

	fr_in_buf = dev_stream_playback_frames(dev_stream);
	if (fr_in_buf <= 0)
		return fr_in_buf;
	if (fr_in_buf < num_to_write)
		num_to_write = fr_in_buf;

	buffer_offset = cras_rstream_dev_offset(rstream, dev_stream->dev_id);

	/* Stream volume scaler. */
	mix_vol = cras_rstream_get_volume_scaler(dev_stream->stream);

	fr_written = 0;
	fr_read = 0;
	while (fr_written < num_to_write) {
		unsigned int read_frames;
		src = cras_rstream_get_readable_frames(
				rstream, buffer_offset + fr_read, &frames);
		if (frames == 0)
			break;
		if (cras_fmt_conversion_needed(dev_stream->conv)) {
			read_frames = frames;
			dev_frames = cras_fmt_conv_convert_frames(
					dev_stream->conv,
					src,
					dev_stream->conv_buffer->bytes,
					&read_frames,
					num_to_write - fr_written);
			src = dev_stream->conv_buffer->bytes;
		} else {
			dev_frames = MIN(frames, num_to_write - fr_written);
			read_frames = dev_frames;
		}
		num_samples = dev_frames * fmt->num_channels;
		cras_mix_add(fmt->format, target, src, num_samples, 1,
			     cras_rstream_get_mute(rstream), mix_vol);
		target += dev_frames * cras_get_format_bytes(fmt);
		fr_written += dev_frames;
		fr_read += read_frames;
	}

	cras_rstream_dev_offset_update(rstream, fr_read, dev_stream->dev_id);
	ATLOG(atlog, AUDIO_THREAD_DEV_STREAM_MIX,
				    fr_written, fr_read, 0);

	return fr_written;
}

/* Copy from the captured buffer to the temporary format converted buffer. */
static unsigned int capture_with_fmt_conv(struct dev_stream *dev_stream,
					  const uint8_t *source_samples,
					  unsigned int num_frames)
{
	const struct cras_audio_format *source_format;
	const struct cras_audio_format *dst_format;
	uint8_t *buffer;
	unsigned int total_read = 0;
	unsigned int write_frames;
	unsigned int read_frames;
	unsigned int source_frame_bytes;
	unsigned int dst_frame_bytes;

	source_format = cras_fmt_conv_in_format(dev_stream->conv);
	source_frame_bytes = cras_get_format_bytes(source_format);
	dst_format = cras_fmt_conv_out_format(dev_stream->conv);
	dst_frame_bytes = cras_get_format_bytes(dst_format);

	dev_stream->conv_area->num_channels = dst_format->num_channels;

	while (total_read < num_frames) {
		buffer = buf_write_pointer_size(dev_stream->conv_buffer,
						&write_frames);
		write_frames /= dst_frame_bytes;
		if (write_frames == 0)
			break;

		read_frames = num_frames - total_read;
		write_frames = cras_fmt_conv_convert_frames(
				dev_stream->conv,
				source_samples,
				buffer,
				&read_frames,
				write_frames);
		total_read += read_frames;
		source_samples += read_frames * source_frame_bytes;
		buf_increment_write(dev_stream->conv_buffer,
				    write_frames * dst_frame_bytes);
	}

	return total_read;
}

/* Copy from the converted buffer to the stream shm.  These have the same format
 * at this point. */
static unsigned int capture_copy_converted_to_stream(
		struct dev_stream *dev_stream,
		struct cras_rstream *rstream,
		float software_gain_scaler)
{
	struct cras_audio_shm *shm;
	uint8_t *stream_samples;
	uint8_t *converted_samples;
	unsigned int num_frames;
	unsigned int total_written = 0;
	unsigned int write_frames;
	unsigned int frame_bytes;
	unsigned int offset;
	const struct cras_audio_format *fmt;

	shm = cras_rstream_input_shm(rstream);

	fmt = cras_fmt_conv_out_format(dev_stream->conv);
	frame_bytes = cras_get_format_bytes(fmt);

	offset = cras_rstream_dev_offset(rstream, dev_stream->dev_id);

	stream_samples = cras_shm_get_writeable_frames(
			shm,
			cras_rstream_get_cb_threshold(rstream),
			&rstream->audio_area->frames);
	num_frames = MIN(rstream->audio_area->frames - offset,
			 buf_queued_bytes(dev_stream->conv_buffer) /
							frame_bytes);

	ATLOG(atlog, AUDIO_THREAD_CONV_COPY,
				    shm->area->write_buf_idx,
				    rstream->audio_area->frames,
				    offset);

	while (total_written < num_frames) {
		converted_samples =
			buf_read_pointer_size(dev_stream->conv_buffer,
					      &write_frames);
		write_frames /= frame_bytes;
		write_frames = MIN(write_frames, num_frames - total_written);

		cras_audio_area_config_buf_pointers(dev_stream->conv_area,
						    fmt,
						    converted_samples);
		cras_audio_area_config_channels(dev_stream->conv_area, fmt);
		dev_stream->conv_area->frames = write_frames;

		cras_audio_area_config_buf_pointers(rstream->audio_area,
						    &rstream->format,
						    stream_samples);

		cras_audio_area_copy(rstream->audio_area, offset,
				     &rstream->format,
				     dev_stream->conv_area, 0,
				     software_gain_scaler);

		buf_increment_read(dev_stream->conv_buffer,
				   write_frames * frame_bytes);
		total_written += write_frames;
		cras_rstream_dev_offset_update(rstream, write_frames,
					       dev_stream->dev_id);
		offset = cras_rstream_dev_offset(rstream, dev_stream->dev_id);
	}

	ATLOG(atlog, AUDIO_THREAD_CAPTURE_WRITE,
				    rstream->stream_id,
				    total_written,
				    cras_shm_frames_written(shm));
	return total_written;
}

unsigned int dev_stream_capture(struct dev_stream *dev_stream,
			const struct cras_audio_area *area,
			unsigned int area_offset,
			float software_gain_scaler)
{
	struct cras_rstream *rstream = dev_stream->stream;
	struct cras_audio_shm *shm;
	uint8_t *stream_samples;
	unsigned int nread;

	/* Check if format conversion is needed. */
	if (cras_fmt_conversion_needed(dev_stream->conv)) {
		unsigned int format_bytes;

		format_bytes = cras_get_format_bytes(
				cras_fmt_conv_in_format(dev_stream->conv));
		nread = capture_with_fmt_conv(
			dev_stream,
			area->channels[0].buf + area_offset * format_bytes,
			area->frames - area_offset);
		capture_copy_converted_to_stream(dev_stream, rstream,
						 software_gain_scaler);
	} else {
		unsigned int offset =
			cras_rstream_dev_offset(rstream, dev_stream->dev_id);

		/* Set up the shm area and copy to it. */
		shm = cras_rstream_input_shm(rstream);
		stream_samples = cras_shm_get_writeable_frames(
				shm,
				cras_rstream_get_cb_threshold(rstream),
				&rstream->audio_area->frames);
		cras_audio_area_config_buf_pointers(rstream->audio_area,
						    &rstream->format,
						    stream_samples);

		nread = cras_audio_area_copy(rstream->audio_area, offset,
					     &rstream->format, area,
					     area_offset,
					     software_gain_scaler);

		ATLOG(atlog, AUDIO_THREAD_CAPTURE_WRITE,
					    rstream->stream_id,
					    nread,
					    cras_shm_frames_written(shm));
		cras_rstream_dev_offset_update(rstream, nread,
					       dev_stream->dev_id);
	}

	return nread;
}

int dev_stream_attached_devs(const struct dev_stream *dev_stream)
{
	return dev_stream->stream->num_attached_devs;
}

void dev_stream_update_frames(const struct dev_stream *dev_stream)
{
	cras_rstream_update_queued_frames(dev_stream->stream);
}

int dev_stream_playback_frames(const struct dev_stream *dev_stream)
{
	int frames;

	frames = cras_rstream_playable_frames(dev_stream->stream,
					      dev_stream->dev_id);
	if (frames < 0)
		return frames;

	if (!dev_stream->conv)
		return frames;

	return cras_fmt_conv_in_frames_to_out(dev_stream->conv, frames);
}

unsigned int dev_stream_cb_threshold(const struct dev_stream *dev_stream)
{
	const struct cras_rstream *rstream = dev_stream->stream;
	unsigned int cb_threshold = cras_rstream_get_cb_threshold(rstream);

	if (rstream->direction == CRAS_STREAM_OUTPUT)
		return cras_fmt_conv_in_frames_to_out(dev_stream->conv,
						      cb_threshold);
	else
		return cras_fmt_conv_out_frames_to_in(dev_stream->conv,
						      cb_threshold);
}

unsigned int dev_stream_capture_avail(const struct dev_stream *dev_stream)
{
	struct cras_audio_shm *shm;
	struct cras_rstream *rstream = dev_stream->stream;
	unsigned int frames_avail;
	unsigned int conv_buf_level;
	unsigned int format_bytes;
	unsigned int wlimit;
	unsigned int dev_offset =
		cras_rstream_dev_offset(rstream, dev_stream->dev_id);

	shm = cras_rstream_input_shm(rstream);

	wlimit = cras_rstream_get_max_write_frames(rstream);
	wlimit -= dev_offset;
	cras_shm_get_writeable_frames(shm, wlimit, &frames_avail);

	if (!dev_stream->conv)
		return frames_avail;

	format_bytes = cras_get_format_bytes(
			cras_fmt_conv_out_format(dev_stream->conv));

	/* Sample rate conversion may cause some sample left in conv_buffer
	 * take this buffer into account. */
	conv_buf_level = buf_queued_bytes(dev_stream->conv_buffer) /
			format_bytes;
	if (frames_avail < conv_buf_level)
		return 0;
	else
		frames_avail -= conv_buf_level;

	frames_avail = MIN(frames_avail,
			   buf_available_bytes(dev_stream->conv_buffer) /
					format_bytes);

	return cras_fmt_conv_out_frames_to_in(dev_stream->conv, frames_avail);
}

/* TODO(dgreid) remove this hack to reset the time if needed. */
static void check_next_wake_time(struct dev_stream *dev_stream)
{
	struct cras_rstream *rstream = dev_stream->stream;
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC_RAW, &now);
	if (timespec_after(&now, &rstream->next_cb_ts)) {
		rstream->next_cb_ts = now;
		add_timespecs(&rstream->next_cb_ts,
			      &rstream->sleep_interval_ts);
	}
}

int dev_stream_playback_update_rstream(struct dev_stream *dev_stream)
{
	cras_rstream_update_output_read_pointer(dev_stream->stream);
	return 0;
}

static int late_enough_for_capture_callback(struct dev_stream *dev_stream)
{
	struct timespec now;
	struct cras_rstream *rstream = dev_stream->stream;
	clock_gettime(CLOCK_MONOTONIC_RAW, &now);
	add_timespecs(&now, &capture_callback_fuzz_ts);
	return timespec_after(&now, &rstream->next_cb_ts);
}

int dev_stream_capture_update_rstream(struct dev_stream *dev_stream)
{
	struct cras_rstream *rstream = dev_stream->stream;
	unsigned int frames_ready = cras_rstream_get_cb_threshold(rstream);
	int rc;

	cras_rstream_update_input_write_pointer(rstream);

	/*
	 * For stream without BULK_AUDIO_OK flag, if it isn't time for
	 * this stream then skip it.
	 */
	if (!(rstream->flags & BULK_AUDIO_OK) &&
	    !late_enough_for_capture_callback(dev_stream))
		return 0;

	/* If there is not enough data for one callback, skip it. */
	if (!cras_rstream_input_level_met(rstream))
		return 0;

	/* Enough data for this stream. */
	if (rstream->flags & BULK_AUDIO_OK)
		frames_ready = cras_rstream_level(rstream);

	ATLOG(atlog, AUDIO_THREAD_CAPTURE_POST,
				    rstream->stream_id,
				    frames_ready,
				    rstream->shm.area->read_buf_idx);

	rc = cras_rstream_audio_ready(rstream, frames_ready);

	if (rc < 0)
		return rc;

	/* Update next callback time according to perfect schedule. */
	add_timespecs(&rstream->next_cb_ts,
		      &rstream->sleep_interval_ts);
	/* Reset schedule if the schedule is missed. */
	check_next_wake_time(dev_stream);

	return 0;
}

void cras_set_playback_timestamp(size_t frame_rate,
				 size_t frames,
				 struct cras_timespec *ts)
{
	cras_clock_gettime(CLOCK_MONOTONIC_RAW, ts);

	/* For playback, want now + samples left to be played.
	 * ts = time next written sample will be played to DAC,
	 */
	ts->tv_nsec += frames * 1000000000ULL / frame_rate;
	while (ts->tv_nsec > 1000000000ULL) {
		ts->tv_sec++;
		ts->tv_nsec -= 1000000000ULL;
	}
}

void cras_set_capture_timestamp(size_t frame_rate,
				size_t frames,
				struct cras_timespec *ts)
{
	long tmp;

	cras_clock_gettime(CLOCK_MONOTONIC_RAW, ts);

	/* For capture, now - samples left to be read.
	 * ts = time next sample to be read was captured at ADC.
	 */
	tmp = frames * (1000000000L / frame_rate);
	while (tmp > 1000000000L) {
		tmp -= 1000000000L;
		ts->tv_sec--;
	}
	if (ts->tv_nsec >= tmp)
		ts->tv_nsec -= tmp;
	else {
		tmp -= ts->tv_nsec;
		ts->tv_nsec = 1000000000L - tmp;
		ts->tv_sec--;
	}
}

void dev_stream_set_delay(const struct dev_stream *dev_stream,
			  unsigned int delay_frames)
{
	struct cras_rstream *rstream = dev_stream->stream;
	struct cras_audio_shm *shm;
	unsigned int stream_frames;

	if (rstream->direction == CRAS_STREAM_OUTPUT) {
		shm = cras_rstream_output_shm(rstream);
		stream_frames = cras_fmt_conv_out_frames_to_in(dev_stream->conv,
							       delay_frames);
		cras_set_playback_timestamp(rstream->format.frame_rate,
					    stream_frames +
						cras_shm_get_frames(shm),
					    &shm->area->ts);
	} else {
		shm = cras_rstream_input_shm(rstream);
		stream_frames = cras_fmt_conv_in_frames_to_out(dev_stream->conv,
							       delay_frames);
		if (cras_shm_frames_written(shm) == 0)
			cras_set_capture_timestamp(
					rstream->format.frame_rate,
					stream_frames,
					&shm->area->ts);
	}
}

int dev_stream_can_fetch(struct dev_stream *dev_stream)
{
	struct cras_rstream *rstream = dev_stream->stream;
	struct cras_audio_shm *shm;

	shm = cras_rstream_output_shm(rstream);

	/* Don't fetch if the previous request hasn't got response. */
	return !cras_shm_callback_pending(shm) &&
	       cras_shm_is_buffer_available(shm);
}

int dev_stream_request_playback_samples(struct dev_stream *dev_stream,
					const struct timespec *now)
{
	struct cras_rstream *rstream = dev_stream->stream;
	int rc;

	rc = cras_rstream_request_audio(dev_stream->stream, now);
	if (rc < 0)
		return rc;

	add_timespecs(&rstream->next_cb_ts,
		      &rstream->sleep_interval_ts);
	check_next_wake_time(dev_stream);
	cras_shm_set_callback_pending(cras_rstream_output_shm(rstream), 1);

	return 0;
}

int dev_stream_poll_stream_fd(const struct dev_stream *dev_stream)
{
	const struct cras_rstream *stream = dev_stream->stream;

	if (!stream_uses_output(stream) ||
	    !cras_shm_callback_pending(&stream->shm) ||
	    cras_rstream_get_is_draining(stream))
		return -1;

	return stream->fd;
}

/*
 * Needed frames from this device such that written frames in shm meets
 * cb_threshold.
 */
static int get_input_needed_frames(struct dev_stream *dev_stream,
				   unsigned int curr_level)
{
	struct cras_rstream *rstream = dev_stream->stream;
	unsigned int rstream_level = cras_rstream_level(rstream);
	unsigned int dev_offset = cras_rstream_dev_offset(
			rstream, dev_stream->dev_id);
	unsigned int needed_for_stream;

	/*
	 * rstream_level + def_offset is the number of frames written to shm
	 * from this device.
	 */
	if (rstream_level + dev_offset > rstream->cb_threshold) {
		/* Enough frames from this device for this stream. */
		return 0;
	}

	/*
	 * Needed number of frames in shm such that written frames in shm meets
	 * cb_threshold.
	 */
	needed_for_stream = rstream->cb_threshold - rstream_level - dev_offset;

	/* Convert the number of frames from stream format to device format. */
	return cras_fmt_conv_out_frames_to_in(dev_stream->conv,
					      needed_for_stream);

}

/*
 * Gets proper wake up time for an input stream. It considers both
 * time for samples to reach one callback level, and the time for next callback.
 */
static int get_input_wake_time(struct dev_stream *dev_stream,
			       unsigned int curr_level,
			       struct timespec *level_tstamp,
			       struct timespec *wake_time_out)
{
	struct cras_rstream *rstream = dev_stream->stream;
	struct timespec time_for_sample;
	int needed_frames_from_device;

	needed_frames_from_device = get_input_needed_frames(
			dev_stream, curr_level);

	/*
	 * If current frames in the device can provide needed amount for stream,
	 * there is no need to wait.
	 */
	if (curr_level >= needed_frames_from_device)
		needed_frames_from_device = 0;

	else
		needed_frames_from_device -= curr_level;

	cras_frames_to_time(needed_frames_from_device,
			    dev_stream->dev_rate,
			    &time_for_sample);

	add_timespecs(&time_for_sample, level_tstamp);

	/* Select the time that is later so both sample and time conditions
	 * are met. */
	if (timespec_after(&time_for_sample, &rstream->next_cb_ts)) {
		*wake_time_out =  time_for_sample;
	} else {
		*wake_time_out =  rstream->next_cb_ts;
	}

	return 0;
}

int dev_stream_wake_time(struct dev_stream *dev_stream,
			 unsigned int curr_level,
			 struct timespec *level_tstamp,
			 struct timespec *wake_time_out)
{
	if (dev_stream->stream->direction == CRAS_STREAM_OUTPUT) {
		/*
                 * TODO(cychiang) Implement the method for output stream.
		 * The logic should be similar to what
		 * get_next_stream_wake_from_list in audio_thread.c is doing.
		 */
		return -EINVAL;
	}

	return get_input_wake_time(dev_stream, curr_level, level_tstamp,
				   wake_time_out);
}
