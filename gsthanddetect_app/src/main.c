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

	//video camera device
	gchar *video_device = "/dev/video0";

	//elements
	GstElement *pipeline, *v4l2src, *ffmpegcolorspace, *vqueue, *vsink;

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

