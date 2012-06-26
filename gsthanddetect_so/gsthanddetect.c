/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2012 andol <<user@hostname.org>>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-handdetect
 *
 * FIXME:operats hand gesture detection in video streams and images, and enable media operation e.g. play/stop/fast forward/back rewind.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! handdetect ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include "gsthanddetect.h"

GST_DEBUG_CATEGORY_STATIC (gst_handdetect_debug);
#define GST_CAT_DEFAULT gst_handdetect_debug

/* define the dir of haar file for hand detect */
#define HAAR_FILE "./fist.xml"

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_DISPLAY,
  PROP_PROFILE
};

/* the capabilities of the inputs and outputs.
 *
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw-rgb")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw-rgb")
    );

GST_BOILERPLATE (Gsthanddetect, gst_handdetect, GstElement, GST_TYPE_ELEMENT);

static void gst_handdetect_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_handdetect_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_handdetect_set_caps (GstPad * pad, GstCaps * caps);
static GstFlowReturn gst_handdetect_chain (GstPad * pad, GstBuffer * buf);

static void gst_handdetect_load_profile (Gsthanddetect * filter);

/* clean up - opencv images and parameters */
static void
gst_handdetect_finalise(GObject *obj)
{
	Gsthanddetect *filter = GST_HANDDETECT (obj);

	if (filter->cvImage) {
	    cvReleaseImage (&filter->cvImage);
	    cvReleaseImage (&filter->cvGray);
	  }

	g_free (filter->profile);

	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

/* GObject vmethod implementations */
static void
gst_handdetect_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_details_simple(element_class,
    "handdetect",
    "Filter/Effect/Video",
    "Performs hand detection on videos and images, providing detected positions via bus messages, and use the messages for media operation",
    "andol li <<andol@andol.info>>");

  gst_element_class_add_pad_template (element_class, gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (element_class, gst_static_pad_template_get (&sink_factory));
}

/* initialise the HANDDETECT class */
static void
gst_handdetect_class_init (GsthanddetectClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_handdetect_finalise);
  gobject_class->set_property = gst_handdetect_set_property;
  gobject_class->get_property = gst_handdetect_get_property;

  g_object_class_install_property (
		  gobject_class,
		  PROP_DISPLAY,
		  g_param_spec_boolean (
				  "display",
				  "Display",
				  "Sets whether the detected hands should be highlighted in the output",
          TRUE,
          G_PARAM_READWRITE));

  g_object_class_install_property (
		  gobject_class,
		  PROP_PROFILE,
		  g_param_spec_string (
				  "profile",
				  "Profile",
				  "Location of Haar cascade file to use for hand detection",
            HAAR_FILE,
            G_PARAM_READWRITE));
}

/* initialise the new element
 * instantiate pads and add them to element
 * set pad call-back functions
 * initialise instance structure
 */
static void
gst_handdetect_init (Gsthanddetect * filter, GsthanddetectClass * gclass)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_setcaps_function (
		  filter->sinkpad,
		  GST_DEBUG_FUNCPTR(gst_handdetect_set_caps));
  gst_pad_set_getcaps_function (
		  filter->sinkpad,
		  GST_DEBUG_FUNCPTR(gst_pad_proxy_getcaps));
  gst_pad_set_chain_function (
		  filter->sinkpad,
		  GST_DEBUG_FUNCPTR(gst_handdetect_chain));

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  gst_pad_set_getcaps_function (
		  filter->srcpad,
		  GST_DEBUG_FUNCPTR(gst_pad_proxy_getcaps));

  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->profile = g_strdup(HAAR_FILE);
  filter->display = TRUE;
  gst_handdetect_load_profile (filter);
}

static void
gst_handdetect_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  Gsthanddetect *filter = GST_HANDDETECT (object);

  switch (prop_id) {
  	  case PROP_PROFILE:
        g_free (filter->profile);
        filter->profile = g_value_dup_string (value);
        gst_handdetect_load_profile (filter);
        break;
      case PROP_DISPLAY:
        filter->display = g_value_get_boolean (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
  }
}

static void
gst_handdetect_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  Gsthanddetect *filter = GST_HANDDETECT (object);

  switch (prop_id) {
  	  case PROP_PROFILE:
        g_value_set_string (value, filter->profile);
        break;
      case PROP_DISPLAY:
        g_value_set_boolean (value, filter->display);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
  }
}

/* GstElement vmethod implementations */
/* this function handles the link with other elements */
static gboolean
gst_handdetect_set_caps (GstPad * pad, GstCaps * caps)
{
  Gsthanddetect *filter;
  GstPad *otherpad;
  gint width, height;
  GstStructure *structure;

  filter = GST_HANDDETECT (gst_pad_get_parent (pad));
  structure = gst_caps_get_structure(caps, 0);
  gst_structure_get_int(structure, "width", &width);
  gst_structure_get_int(structure, "height", &height);

  filter->cvImage = cvCreateImage (cvSize(width, height), IPL_DEPTH_8U, 3);
  filter->scvImage = cvCreateImage (cvSize(320, 240), IPL_DEPTH_8U, 3);
  filter->cvGray = cvCreateImage (cvSize(filter->scvImage->width, filter->scvImage->height), IPL_DEPTH_8U, 1);
  filter->cvStorage = cvCreateMemStorage (0);

  otherpad = (pad == filter->srcpad) ? filter->sinkpad : filter->srcpad;
  gst_object_unref (filter);

  return gst_pad_set_caps (otherpad, caps);
}

/* chain function
 * this function does the actual processing 'of hand detect and display'
 */
static GstFlowReturn
gst_handdetect_chain (GstPad * pad, GstBuffer * buf)
{
  Gsthanddetect *filter;
  CvSeq *hands;
  int i;

  filter = GST_HANDDETECT (GST_OBJECT_PARENT (pad));
  filter->cvImage->imageData = (char *) GST_BUFFER_DATA (buf);

  /* resize if the image size is too large */
  if(filter->cvImage->width > 320 || filter->cvImage->height > 240)
	  cvResize(filter->cvImage, filter->scvImage, 0);
  cvCvtColor(filter->scvImage, filter->cvGray, CV_RGB2GRAY);

  if(filter->cvCascade){
	  /* detect hands */
	  hands = cvHaarDetectObjects (
			  filter->cvGray,
			  filter->cvCascade,
			  filter->cvStorage,
			  1.1,
			  2,
			  0,
			  cvSize(10,10),
			  cvSize(50, 50));

	  /* if hands detected, get the buffer ready */
	  if(filter->display && hands && hands->total > 0){
		  buf = gst_buffer_make_writable(buf);
	  }

	  /* go through all hand detect results */
	  for(i = 0; i < (hands ? hands->total : 0); i++){
		  /* read a hand detect result */
		  CvRect *r = (CvRect *) cvGetSeqElem(hands, i);

		  /* define a structure to contain the result */
		  GstStructure *s = gst_structure_new(
				  "hand",
				  "x", G_TYPE_UINT, r->x,
				  "y", G_TYPE_UINT, r->y,
				  "width", G_TYPE_UINT, r->width,
				  "height", G_TYPE_UINT, r->height,
				  NULL);

		  /* set up new message element */
		  GstMessage *m = gst_message_new_element(GST_OBJECT(filter), s);
		  /* post a msg on the filter element's GstBus */
		  gst_element_post_message(GST_ELEMENT(filter), m);

		  /* draw out the circle on detected hands */
		  if(filter->display){
			  CvPoint center;
			  int radius;
			  center.x = cvRound((r->x + r->width * 0.5));
			  center.y = cvRound((r->y + r->height * 0.5));
			  radius = cvRound((r->width + r->height) * 0.25);
			  cvCircle(filter->cvImage, center, radius, CV_RGB(255, 32, 32), 3, 8, 0);
		  }
	  }

  }

  /* just push out the incoming buffer without touching it */
  return gst_pad_push (filter->srcpad, buf);
}

static void
gst_handdetect_load_profile(Gsthanddetect *filter){
	filter->cvCascade =
			(CvHaarClassifierCascade *) cvLoad(filter->profile, 0, 0, 0);
	if(! filter->cvCascade){
		GST_WARNING("could not load haar classifier cascade: %s.", filter->profile);
	}
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
handdetect_init (GstPlugin * handdetect)
{
  /* debug category for filtering log messages
   *
   * exchange the string 'Template handdetect' with your description
   */
  GST_DEBUG_CATEGORY_INIT (
		  gst_handdetect_debug,
		  "handdetect",
		  0,
		  "performs hand detect on videos and images, providing detected positions via bus messages for media operation.");

  return gst_element_register (
		  handdetect,
		  "handdetect",
		  GST_RANK_NONE,
		  GST_TYPE_HANDDETECT);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "gst_handdetect"
#endif

/* gstreamer looks for this structure to register handdetects
 *
 * exchange the string 'Template handdetect' with your handdetect description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "handdetect",
    "Template handdetect",
    handdetect_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)