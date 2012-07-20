/*
 * main.c
 *
 *  Created on: Jun 2012
 *  Author: Andol Li, andol@andol.info
 *  description: gsthanddetect plugin test application - hand gestures to control media playing
 *
 */

#include <gst/gst.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>


/* elements */
GstElement	*pipeline, *v4l2src, *videoscale,
			*ffmpegcolorspace_in, *ffmpegcolorspace_out,
			*handdetect, *xvimagesink,
			*playbin,
			*filesrc, *demuxer, *audiodecoder, *audiosink, *alsasink;

/* message processing function */
static GstBusSyncHandler
bus_sync_handler(GstBus *bus,
		GstMessage *message,
		GstPipeline *pipeline)
{
	if(GST_MESSAGE_TYPE (message) != GST_MESSAGE_ELEMENT )
		return GST_BUS_PASS;
	if(!gst_structure_has_name(message->structure, "detected_hand_info"))
		return GST_BUS_PASS;

	const GstStructure *structure = message->structure;
		if(structure && strcmp(gst_structure_get_name(structure), "detected_hand_info") == 0){
			/* print message type and structure name */
			g_print("%s{%s}\n",	gst_message_type_get_name(message->type), gst_structure_get_name(structure));
			/* read/print msg structure names/values */
			int i;
			for(i = 0; i < gst_structure_n_fields(structure); i++){
				const gchar *name = gst_structure_nth_field_name(structure, i);
				GType type = gst_structure_get_field_type(structure, name);
				const GValue *value = gst_structure_get_value(structure, name);
				type == G_TYPE_STRING ?
						g_print("%s[%s]{%s}\n", name, g_type_name(type), g_value_get_string(value)) :
						g_print("%s[%s]{%d}\n", name, g_type_name(type), g_value_get_uint(value));
			}
			g_print("\n");

			/* manage pipeline's media playing state */
			if(GST_STATE(pipeline) == GST_STATE_PLAYING)
				gst_element_set_state(pipeline, GST_STATE_PAUSED);
			else
				gst_element_set_state(pipeline, GST_STATE_PLAYING);
		}

	gst_message_unref(message);
	return GST_BUS_DROP;
}

/* process the pipeline bus msgs */
//static gboolean
//bus_call(GstBus		*bus,
//		GstMessage 	*msg,
//		gpointer 	data)
//{
//	/*g_print("-msg src:%s, type:%s\n",
//	 *		gst_object_get_name(msg->src),
//	 *		gst_message_type_get_name(msg->type));
//	 */
//
//	GMainLoop *loop = (GMainLoop *) data;
//
//	switch (GST_MESSAGE_TYPE (msg)) {
//		case GST_MESSAGE_EOS:
//			g_print ("End of stream\n");
//			g_main_loop_quit (loop);
//			break;
//
//		case GST_MESSAGE_ERROR: {
//			gchar  *debug;
//			GError *error;
//
//			gst_message_parse_error (msg, &error, &debug);
//			g_free (debug);
//			g_printerr ("Error: %s\n", error->message);
//			g_error_free (error);
//			g_main_loop_quit (loop);
//			break; }
//		/* handdetect element msg processing */
//		case GST_MESSAGE_ELEMENT:{
//			break;	}
//		case GST_MESSAGE_STATE_CHANGED:{
//			/* print pipeline's state changes */
//			GstState old_state, new_state;
//			gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
//			g_print ("[%s]: %s -> %s\n", GST_OBJECT_NAME (msg->src),
//					gst_element_state_get_name (old_state),
//					gst_element_state_get_name (new_state));
//			break;	}
//		default:
//			break;
//	}
//
//	return TRUE;
//}

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
	if(!(pipeline = gst_pipeline_new("video_player")))
		g_error("ERR: failed to init pipeline \n");
	if(!(filesrc = gst_element_factory_make("filesrc", "video_filesource")))
		g_error("ERR: failed to init filesrc \n");
	if(!(v4l2src = gst_element_factory_make("v4l2src", "video_source")))
		g_error("ERR: failed to init v4l2src \n");
	if(!(videoscale = gst_element_factory_make("videoscale", "video_scale")))
		g_error("ERR: failed to init videoscale \n");
	if(!(ffmpegcolorspace_in = gst_element_factory_make("ffmpegcolorspace", "video_ffmpegcolorsapce_in")))
		g_error("ERR: failed to init ffmpegcolorspace_in \n");
	if(!(handdetect = gst_element_factory_make("handdetect", "video_handdetect")))
		g_error("ERR: failed to init handdetect \n");
	if(!(ffmpegcolorspace_out = gst_element_factory_make("ffmpegcolorspace", "video_ffmpegcolorsapce_out")))
		g_error("ERR: failed to init ffmpegcolorspace_out \n");
	if(!(xvimagesink = gst_element_factory_make("xvimagesink", "video_xvimagesink")))
		g_error("ERR: failed to init xvimagesink \n");

	//create caps for video streams
	caps = gst_caps_from_string("video/x-raw-yuv, width=320, height=240, framerate=(fraction)30/1");

	//set video camera parameters
	g_object_set(G_OBJECT(v4l2src), "device", video_device, NULL);
	//set handdetect plugin's parameters
	//g_object_set(G_OBJECT(handdetect), "profile", "../fist.xml", NULL);
	//g_object_set(G_OBJECT(handdetect), "display", TRUE, NULL);

	//add msg handler to pipeline
	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	//gst_bus_add_watch(bus, bus_call, loop);
	gst_bus_set_sync_handler(bus, (GstBusSyncHandler) bus_sync_handler, pipeline);
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

	gst_element_link_many(videoscale,
			ffmpegcolorspace_in,
			handdetect,
			ffmpegcolorspace_out,
			xvimagesink,
			NULL);

	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	//main loop
	g_main_loop_run(loop);

	//clean
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(GST_OBJECT(pipeline));

	return(0);
}

