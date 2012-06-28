/*
 * main.c
 *
 *  Author: andol li, andol@andol.info
 *  Created on: Jun 2012
 *  Description: gsthanddetect_so plugin application test
 */

#include <gst/gst.h>
#include <glib.h>

//msg process function
static gboolean bus_call(GstBus *bus,GstMessage *msg,gpointer data)
{
    GMainLoop *loop = (GMainLoop *) data;// main loop pointer, exit when gets EOS
    switch (GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(loop);
            break;
        case GST_MESSAGE_ERROR:
        {
		   gchar *debug;
		   GError *error;

		   gst_message_parse_error(msg,&error,&debug);
		   g_free(debug);
		   g_printerr("ERROR:%s\n",error->message);
		   g_error_free(error);
		   g_main_loop_quit(loop);
		   break;
        }
        default:
             break;
    }
    return TRUE;
}

int mp3player_main(int argc,char *argv[])
{
    GMainLoop *loop;
    //define all elements
    GstElement *pipeline,*source,*decoder,*sink;
    //define plugin element
    GstElement *handdetect;
    GstBus *bus;

    //init gst
    gst_init(&argc,&argv);

    //main loop, starts after executing g_main_loop_run
    loop = g_main_loop_new(NULL,FALSE);

    //check argument list
//    if(argc != 2)
//    {
//        g_printerr("Usage:%s <mp3 filename>\n",argv[0]);
//        return -1;
//    }

    //create pipeline and elements
    pipeline = gst_pipeline_new("audio-player");
    source = gst_element_factory_make("filesrc","file-source");
    decoder = gst_element_factory_make("mad","mad-decoder");
    sink = gst_element_factory_make("autoaudiosink","audio-output");
    handdetect = gst_element_factory_make("gsthanddetect", "hand-detect");

    //check element status
    if(!pipeline||!source||!decoder||!sink){
        g_printerr("One element could not be created.Exiting.\n");
        return -1;
    }

    //set 'source' location - the mp3 file location
    //g_object_set(G_OBJECT(source),"location",argv[1],NULL);
    g_object_set(G_OBJECT(source),"location","music.mp3",NULL);

    //get pipeline's msg bus
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus,bus_call,loop);
    gst_object_unref(bus);

    //add elements to the pipeline
    gst_bin_add_many(GST_BIN(pipeline),source,decoder,sink,NULL);

    //link elements added in the pipeline
    gst_element_link_many(source,decoder,sink,NULL);

    //start playing - change state
    gst_element_set_state(pipeline,GST_STATE_PLAYING);
    g_print("Running\n");

    //start loop
    g_main_loop_run(loop);

    g_print("Returned,stopping playback\n");
    gst_element_set_state(pipeline,GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));

    return 0;
}
