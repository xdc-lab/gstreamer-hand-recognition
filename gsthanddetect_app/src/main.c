/*
 * main.c
 *
 *  Author: andol li, andol@andol.info
 *  Created on: Jun 2012
 *  Description: gsthanddetect_so plugin application test
 */

#include <gst/gst.h>


static gboolean
bus_call (GstBus *bus, GstMessage *msg, gpointer data)
{
	GMainLoop *loop = data;

	switch (GST_MESSAGE_TYPE (msg)) {
		case GST_MESSAGE_EOS:
			g_print ("End-of-stream\n");
			g_main_loop_quit (loop);
			break;
		case GST_MESSAGE_ERROR: {
			gchar *debug = NULL;
			GError *err = NULL;
			gst_message_parse_error (msg, &err, &debug);
			g_print ("Error: %s\n", err->message);
			g_error_free (err);

			if (debug) {
				g_print ("Debug details: %s\n", debug);
				g_free (debug);
			}

			g_main_loop_quit (loop);
			break;
		}
		default:
		break;
	}
	return TRUE;
}

gint
main (gint argc, gchar *argv[])
{
	GstStateChangeReturn ret;
	GstElement *pipeline, *filesrc, *decoder, *filter, *sink, *handdetect;
	GstElement *convert1, *convert2, *resample;
	GMainLoop *loop;
	GstBus *bus;

	/* initialise gst */
	gst_init (&argc, &argv);

	loop = g_main_loop_new (NULL, FALSE);
	if (argc != 2) {
		g_print ("Usage: %s <mp3 filename>\n", argv[0]);
		return 01;
	}

	/* create pipeline element */
	pipeline = gst_pipeline_new ("my_pipeline");

	/* watch for messages on the pipeline’s bus (note that this will only
	* work like this when a GLib main loop is running) */
	bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	gst_bus_add_watch (bus, bus_call, loop);
	gst_object_unref (bus);

	filesrc = gst_element_factory_make ("filesrc", "my_filesource");
	decoder = gst_element_factory_make ("mad", "my_decoder");
	handdetect = gst_element_factory_make("Gsthanddetect", "my_handdetect");

	/* putting an audioconvert element here to convert the output of the
	* decoder into a format that my_filter can handle (we are assuming it
	* will handle any sample rate here though) */
	convert1 = gst_element_factory_make ("audioconvert", "audioconvert1");

	/* use "identity" here for a filter that does nothing */
	filter = gst_element_factory_make ("Gsthanddetect", "my_filter");

	/* there should always be audioconvert and audioresample elements before
	* the audio sink, since the capabilities of the audio sink usually vary
	* depending on the environment (output used, sound card, driver etc.) */
	convert2 = gst_element_factory_make ("audioconvert", "audioconvert2");
	resample = gst_element_factory_make ("audioresample", "audioresample");
	sink = gst_element_factory_make ("osssink", "audiosink");

	if (!sink || !decoder) {
		g_print ("Decoder or output could not be found - check your install\n");
		return -1;
	} else if (!convert1 || !convert2 || !resample) {
		g_print ("Could not create audioconvert or audioresample element, "
		"check your installation\n");
		return -1;
	} else if (!filter) {
		g_print ("Your self-written filter could not be found. Make sure it "
		"is installed correctly in $(libdir)/gstreamer-0.10/ or "
		"~/.gstreamer-0.10/plugins/ and that gst-inspect-0.10 lists it. "
		"If it doesn’t, check with ’GST_DEBUG=*:2 gst-inspect-0.10’ for "
		"the reason why it is not being loaded.");
		return -1;
	}

	g_object_set (G_OBJECT (filesrc), "location", argv[1], NULL);

	gst_bin_add_many (GST_BIN (pipeline), filesrc, decoder, convert1, filter, convert2, resample, sink, NULL);

	/* link everything together */
	if (!gst_element_link_many (filesrc, decoder, convert1, filter, convert2, resample, sink, NULL)) {
		g_print ("Failed to link one or more elements!\n");
		return -1;
	}

	/* run */
	ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		GstMessage *msg;
		g_print ("Failed to start up pipeline!\n");

		/* check if there is an error message with details on the bus */
		msg = gst_bus_poll (bus, GST_MESSAGE_ERROR, 0);

		if (msg) {
			GError *err = NULL;
			gst_message_parse_error (msg, &err, NULL);
			g_print ("ERROR: %s\n", err->message);
			g_error_free (err);
			gst_message_unref (msg);
		}
	return -1;
	}

	g_main_loop_run (loop);

	/* clean up */
	gst_element_set_state (pipeline, GST_STATE_NULL);
	gst_object_unref (pipeline);

	return 0;
}
