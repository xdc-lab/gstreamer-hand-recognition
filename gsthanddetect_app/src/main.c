/*
 * main.c
 *
 *  Created on: 28 Jun 2012
 *  Author: andol li, andol@andol.info
 *  description: gsthanddetect plugin test application
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch autovideosrc ! ffmpegcolorspace ! "video/x-raw-rgb, width=320, height=240" ! \
 * videoscale ! handdetect ! ffmpegcolorspace ! xvimagesink
 * ]|
 * </refsect2>
 */

#include <gst/gst.h>
#include <glib.h>

//elements
GstElement	*pipeline, *v4l2src, *videoscale,
			*ffmpegcolorspace_in, *ffmpegcolorspace_out,
			*handdetect, *xvimagesink;

static gboolean
bus_call(GstBus		*bus,
		GstMessage 	*msg,
		gpointer 	data)
{
	g_print("-msg src:%s, type:%s\n",
			gst_object_get_name(msg->src),
			gst_message_type_get_name(msg->type));

	GMainLoop *loop = (GMainLoop *) data;

	switch (GST_MESSAGE_TYPE (msg)) {
		case GST_MESSAGE_EOS:
			g_print ("End of stream\n");
			g_main_loop_quit (loop);
			break;

		case GST_MESSAGE_ERROR: {
			gchar  *debug;
			GError *error;

			gst_message_parse_error (msg, &error, &debug);
			g_free (debug);
			g_printerr ("Error: %s\n", error->message);
			g_error_free (error);
			g_main_loop_quit (loop);
			break; }
		/* handdetect element msg processing */
		case GST_MESSAGE_ELEMENT:{
			if((gchar )gst_object_get_name(msg->src) == "video_handdetect")
				g_print("-video_handdetect msg detected\n");
			break;	}
		default:
		  break;
	}

	return TRUE;
}

gint
main(gint argc, gchar **argv){

	//main loop
	static GMainLoop *loop;
	loop = g_main_loop_new(NULL, FALSE);

	//video camera device
	gchar *video_device = "/dev/video0";

	//buses
	GstBus *bus;
	//caps
	GstCaps *caps;

	//init gst
	gst_init(&argc, &argv);

	//create elements
	if(!(pipeline = gst_pipeline_new("video_player"))){
		g_printerr("ERR: failed to init pipeline \n");
		return 0;
	}
	if(!(v4l2src = gst_element_factory_make("v4l2src", "video_source"))){
		g_printerr("ERR: failed to init v4l2src \n");
		return 0;
	}
	if(!(videoscale = gst_element_factory_make("videoscale", "video_scale"))){
		g_printerr("ERR: failed to init videoscale \n");
		return 0;
	}
	if(!(ffmpegcolorspace_in = gst_element_factory_make("ffmpegcolorspace", "video_ffmpegcolorsapce_in"))){
		g_printerr("ERR: failed to init ffmpegcolorspace_in \n");
		return 0;
	}
	if(!(handdetect = gst_element_factory_make("handdetect", "video_handdetect"))){
		g_printerr("ERR: failed to init handdetect \n");
		return 0;
	}
	if(!(ffmpegcolorspace_out = gst_element_factory_make("ffmpegcolorspace", "video_ffmpegcolorsapce_out"))){
		g_printerr("ERR: failed to init ffmpegcolorspace_out \n");
		return 0;
	}
	if(!(xvimagesink = gst_element_factory_make("xvimagesink", "video_xvimagesink"))){
		g_printerr("ERR: failed to init xvimagesink \n");
		return 0;
	}
	//create caps for video streams
	caps = gst_caps_from_string("video/x-raw-yuv, width=320, height=240, framerate=(fraction)15/1");

	//set video camera parameters
	g_object_set(G_OBJECT(v4l2src), "device", video_device, NULL);
	//set handdetect plugin's parameters
//	g_object_set(G_OBJECT(handdetect), "profile", "../fist.xml", NULL);
//	g_object_set(G_OBJECT(handdetect), "display", TRUE, NULL);

	//add msg handler to pipeline
	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	gst_bus_add_watch(bus, bus_call, loop);
	gst_object_unref(bus);

	//add elements to pipeline
	gst_bin_add_many(GST_BIN(pipeline),
			v4l2src,
			videoscale,
			ffmpegcolorspace_in,
			handdetect,
			ffmpegcolorspace_out,
			xvimagesink,
			NULL);

	//link these elements
	if(!gst_element_link_filtered( v4l2src, videoscale, caps)){
		g_printerr("ERR:v4l2src -> videoscale caps\n");
		return 0;
	}
	gst_caps_unref(caps);

//	if(!gst_element_link(v4l2src, videoscale)) g_printerr("ERR: gst_element_link 1 \n");
	if(!gst_element_link(videoscale, ffmpegcolorspace_in)) g_printerr("ERR: gst_element_link 3 \n");
	if(!gst_element_link(ffmpegcolorspace_in, handdetect)) g_printerr("ERR: gst_element_link 4 \n");
	if(!gst_element_link(handdetect, ffmpegcolorspace_out)) g_printerr("ERR: gst_element_link 5 \n");
	if(!gst_element_link(ffmpegcolorspace_out, xvimagesink)) g_printerr("ERR: gst_element_link 6 \n");

	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	//main loop
	g_main_loop_run(loop);

	//clean
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(GST_OBJECT(pipeline));

	return(0);
}

