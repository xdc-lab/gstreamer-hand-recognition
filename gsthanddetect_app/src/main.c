/*
 * main.c
 *
 *  Created on: 28 Jun 2012
 *  Author: andol li, andol@andol.info
 *  description: get videos from camera using gst
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch v4l2src ! video/x-raw-yuv, width=320, height=240 ! videoscale !
 * decodebin ! ffmpegcolorspace ! handdetect display=TRUE ! ffmpegcolorspace ! xvimagesink
 * ]|
 * </refsect2>
 */

#include <gst/gst.h>
#include <glib.h>

//elements
GstElement	*pipeline, *v4l2src, *videoscale, *decodebin,
			*ffmpegcolorspace_in, *ffmpegcolorspace_out,
			*handdetect, *xvimagesink;

static gboolean
bus_call(GstBus		*bus,
		GstMessage 	*msg,
		gpointer 	data)
{
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
		  break;}
		default:
		  break;
	}

	return TRUE;
}

static void
on_pad_add(GstElement	*element,
		GstPad			*pad,
		gpointer		data)
{
	gchar *name;
	name = gst_pad_get_name(pad);
	g_print("a new pad %s is created \n", name);
	g_free(name);

	GstPad *sinkpad;
	sinkpad = gst_element_get_pad(ffmpegcolorspace_in, "sink");
	gst_pad_link(pad, sinkpad);

	gst_object_unref(sinkpad);
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
	pipeline = gst_pipeline_new("video_player");
	if(!pipeline) {
		g_printerr("ERR: failed to init pipeline \n");
		return 0;
	}
	v4l2src = gst_element_factory_make("v4l2src", "video_source");
	if(!v4l2src) {
		g_printerr("ERR: failed to init v4l2src \n");
		return 0;
	}
	videoscale = gst_element_factory_make("videoscale", "video_scale");
	if(!videoscale){
		g_printerr("ERR: failed to init videoscale \n");
		return 0;
	}
	decodebin = gst_element_factory_make("decodebin2", "video_decodebin");
	if(!decodebin){
		g_printerr("ERR: failed to init decodebin \n");
		return 0;
	}
	ffmpegcolorspace_in = gst_element_factory_make("ffmpegcolorspace", "video_ffmpegcolorsapce_in");
	if(!ffmpegcolorspace_in){
		g_printerr("ERR: failed to init ffmpegcolorspace_in \n");
		return 0;
	}
	ffmpegcolorspace_out = gst_element_factory_make("ffmpegcolorspace", "video_ffmpegcolorsapce_out");
	if(!ffmpegcolorspace_out){
		g_printerr("ERR: failed to init ffmpegcolorspace_out \n");
		return 0;
	}
	handdetect = gst_element_factory_make("handdetect", "video_handdetect");
	if(!handdetect){
		g_printerr("ERR: failed to init handdetect \n");
		return 0;
	}
	xvimagesink = gst_element_factory_make("xvimagesink", "video_xvimagesink");
	if(!xvimagesink){
		g_printerr("ERR: failed to init xvimagesink \n");
		return 0;
	}
	//create caps for video streams
	caps = gst_caps_new_simple("video/x-raw-yuv",
			"width", G_TYPE_INT, 320,
			"height", G_TYPE_INT, 240,
			"framerate", GST_TYPE_FRACTION, 25,
			1, NULL);

	//set video camera parameters
	g_object_set(G_OBJECT(v4l2src), "device", video_device, NULL);
	//set handdetect plugin's parameters
	g_object_set(G_OBJECT(handdetect), "profile", "../fist.xml", NULL);
	g_object_set(G_OBJECT(handdetect), "display", TRUE, NULL);

	//add msg handler
	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	gst_bus_add_watch(bus, bus_call, loop);
	gst_object_unref(bus);

	//add elements to pipeline
	gst_bin_add_many(GST_BIN(pipeline),
			v4l2src,
			videoscale,
			decodebin,
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
	g_signal_connect(decodebin, "pad-added", G_CALLBACK(on_pad_add), ffmpegcolorspace_in);

	//if(!gst_element_link(v4l2src, videoscale)) g_printerr("ERR: gst_element_link 1 \n");
	if(!gst_element_link(videoscale, decodebin)) g_printerr("ERR: gst_element_link 2 \n");
	gst_element_link_pads(decodebin, "src", ffmpegcolorspace_in, "sink");
//	if(!gst_element_link(decodebin, ffmpegcolorspace_in)) g_printerr("ERR: gst_element_link 3 \n");
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

