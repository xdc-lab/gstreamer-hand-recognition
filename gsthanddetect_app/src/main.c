/*
 * main.c
 *
 *  Author: andol li
 *  Created on: 11 Jun 2012
 *  Description: gst plugin application test
 */


#include <stdio.h>
#include <gst/base/gstpushsrc.h>
#include "../gsthanddetect.h"
#include <opencv/cxcore.h>
#include <opencv/highgui.h>

static gboolean
bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
	GMainLoop *loop = data;

	switch(GST_MESSAGE_TYPE(msg))
	{
	case GST_MESSAGE_EOS:
		g_print("end of stream\n");
		g_main_loop_quit(loop);
		break;

	case GST_MESSAGE_ERROR:
		{
			gchar *debug;
			GError *err;

			gst_message_parse_error(msg, &err, &debug);
			g_free(&debug);

			g_print("error: %s\n", err->message);
			g_error_free(err);

			g_main_loop_quit(loop);
			break;
		}

	default:
		break;
	}

	return TRUE;
}

gint main(gint argc, gchar* argv[])
{
	//define opencv variables
	IplImage *src, resize_src;
	cvNamedWindow("src", 1);
	cvShowImage("src", src);


	GstElement *pipeline, *filesrc, *decode, *filter, *sink, *v4l2src;
	GMainLoop *loop;

	/*create element*/
	pipeline = gst_pipeline_new("myPipeline");
	gst_bus_add_watch(gst_pipeline_get_bus(GST_PIPELINE(pipeline)), bus_call, loop);

	/*init*/
	gst_init(&argc, &argv);
	loop = g_main_loop_new(NULL, FALSE);

	/*test codes*/
	GsthanddetectClass *handdetect;

	/*clean up*/
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(GST_OBJECT(pipeline));

	/*end application*/
	return 0;
}
