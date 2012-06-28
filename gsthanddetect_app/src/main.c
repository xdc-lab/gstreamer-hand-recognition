/*
 * main.c
 *
 *  Created on: 28 Jun 2012
 *  Author: andol li, andol@andol.info
 *  description: get videos from camera using gst
 */

#include <gst/gst.h>
#include <glib.h>

gint
main(gint argc, gchar **argv){

	//main loop
	static GMainLoop *loop;
	loop = g_main_loop_new(NULL, FALSE);

<<<<<<< HEAD
	//video camera device
	gchar *video_device = "/dev/video0";

	//elements
	GstElement *pipeline, *v4l2src, *ffmpegcolorspace, *vqueue, *vsink;
=======
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
>>>>>>> origin/dev

	//caps
	GstCaps *caps;

	//init gst
	gst_init(&argc, &argv);

	//create elements
	pipeline = gst_pipeline_new("pipeline");
	v4l2src = gst_element_factory_make("v4l2src", "video_source");
	ffmpegcolorspace = gst_element_factory_make("ffmpegcolorspace", "ffmpegcolorspace");
	vqueue = gst_element_factory_make("queue", NULL);
	vsink = gst_element_factory_make("xvimagesink", "vsink");

	//create caps for video streams
	caps = gst_caps_new_simple("video/x-raw-yuv",
			"width", G_TYPE_INT, 320,
			"height", G_TYPE_INT, 240,
			"framerate", GST_TYPE_FRACTION, 20,
			1, NULL);

	//set video camera parameters
	g_object_set(G_OBJECT(v4l2src), "device", video_device, NULL);

	//add elements to pipeline
	gst_bin_add_many(GST_BIN(pipeline), v4l2src, vsink, NULL);

	//link these elements
	if(!gst_element_link_filtered( v4l2src, vsink, caps)){
		printf("\ncaps not negotiatable!!!");
		return 0;
	}
	if(!gst_element_link_many(v4l2src, vsink, NULL)){
		printf("\nelement link unsuccessfull!!!");
	}

	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	//main loop
	g_main_loop_run(loop);

	//cleand
	gst_object_unref(pipeline);
	gst_caps_unref(caps);

	return(0);

}

