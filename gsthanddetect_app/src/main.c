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
	GstElement *pipeline, *v4l2src, *v4l2sink;
	GstElement *handdetect, *ffmpegcolorspace, *vqueue; //test elements

	//caps
	GstCaps *caps;

	//init gst
	gst_init(&argc, &argv);

	//create elements
	pipeline = gst_pipeline_new("pipeline");
	if(!pipeline) {
		g_print("\nfailed to init pipeline!!!");
		return 0;
	}
	v4l2src = gst_element_factory_make("v4l2src", "video_source");
	if(!v4l2src) {
		g_print("\nfailed to init v4l2src!!!");
		return 0;
	}
	v4l2sink = gst_element_factory_make("autovideosink", "v4l2sink");
	if(!v4l2sink) {
		g_print("\nfailed to init v4l2sink!!!");
		return 0;
	}

	/* create TEST elements */
	ffmpegcolorspace = gst_element_factory_make("ffmpegcolorspace", "ffmpegcolorspace");
	if(!ffmpegcolorspace) {
		g_print("\nfailed to init ffmpegcolorspace!!!");
		return 0;
	}
	vqueue = gst_element_factory_make("queue", NULL);
	if(!vqueue){
		g_print("\nfaild to init vqueue!!!");
		return 0;
	}
	handdetect = gst_element_factory_make("handdetect", "handdetect");
	if(!handdetect){
		g_print("\nfailed to init handdetect");
		return 0;
	}

	//create caps for video streams
	caps = gst_caps_new_simple("video/x-raw-yuv",
			"width", G_TYPE_INT, 320,
			"height", G_TYPE_INT, 240,
			"framerate", GST_TYPE_FRACTION, 30,
			1, NULL);

	//set video camera parameters
	g_object_set(G_OBJECT(v4l2src), "device", video_device, NULL);

	//set handdetect plugin's parameters
	g_object_set(G_OBJECT(handdetect), "profile", "../fist.xml", NULL);
	g_object_set(G_OBJECT(handdetect), "display", TRUE, NULL);

	//add elements to pipeline
	gst_bin_add_many(GST_BIN(pipeline), v4l2src, handdetect, v4l2sink, NULL);

	//link these elements
	if(!gst_element_link_filtered( v4l2src, handdetect, caps)){
		g_print("\nv4l2src->handdetect caps NOT OK!!!");
		return 0;
	}
	if(!gst_element_link_filtered(handdetect, v4l2sink, caps)){
		g_print("\nhanddetect->v4l2sink caps NOT OK!!!");
	}
	if(!gst_element_link_many(v4l2src, handdetect, v4l2sink, NULL)){
		g_print("\nelement links NOT OK!!!");
	}

	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	//main loop
	g_main_loop_run(loop);

	//clean
	gst_object_unref(pipeline);
	gst_caps_unref(caps);

	return(0);
}

