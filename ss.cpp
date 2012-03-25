#include "ss.h"

#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"
#include "XPLMPlugin.h"
#include "XPLMMenus.h"
#include "XPWidgets.h"
#include "XPStandardWidgets.h"

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <string.h>
#include <math.h>

XPLMDataRef ref_xpndr_mode      = NULL;
XPLMDataRef ref_xpndr_setting   = NULL;
XPLMDataRef ref_alt_agl         = NULL;
XPLMDataRef ref_grnd_spd        = NULL;

XPLMWindowID	debug_window = NULL;
int				clicked = 0;

int config_default_xpndr_setting;

char debug_string[255];

/* window callbacks for debugging */
void MyDrawWindowCallback(
                                   XPLMWindowID         inWindowID,    
                                   void *               inRefcon);    

void MyHandleKeyCallback(
                                   XPLMWindowID         inWindowID,    
                                   char                 inKey,    
                                   XPLMKeyFlags         inFlags,    
                                   char                 inVirtualKey,    
                                   void *               inRefcon,    
                                   int                  losingFocus);    

int MyHandleMouseClickCallback(
                                   XPLMWindowID         inWindowID,    
                                   int                  x,    
                                   int                  y,    
                                   XPLMMouseStatus      inMouse,    
                                   void *               inRefcon);    

void initConfig() {
    /* TODO: make this a config file */
    config_default_xpndr_setting = 1200;
}

float SmoothSailingCallback(
        float   inElapsedSinceLastCall,    
        float   inElapsedTimeSinceLastFlightLoop,    
        int     inCounter,    
        void    *inRefcon) {

    initXpndr();

    return CALLBACK_INTERVAL;
}

void initXpndr() {
    /* if the plane is stationary and altitude agl is 0, and the xpndr is off, turn it on */
    int xpndr_mode;

    xpndr_mode  = XPLMGetDatai( ref_xpndr_mode );

    /* Transponder mode (off=0 stdby=1 on=2 test=3) */
    if(     !xpndr_mode 
            && !floor( XPLMGetDataf( ref_alt_agl ) ) 
            && !floor( XPLMGetDataf( ref_grnd_spd ) ) 
       ) {
        XPLMSetDatai( ref_xpndr_mode, 2 );
        XPLMSetDatai( ref_xpndr_setting, config_default_xpndr_setting );
    }
}

PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc) {

    /* First set up our plugin info. */
    strcpy(outName, "SmoothSailing 0.1");
    strcpy(outSig, "Smooth Sailing by Colin Lieberman");
    strcpy(outDesc, "Simple weather and visibility controls for convenience");

    initConfig();

    ref_xpndr_mode      = XPLMFindDataRef("sim/cockpit/radios/transponder_mode");
    ref_xpndr_setting   = XPLMFindDataRef("sim/cockpit/radios/transponder_code");
    ref_alt_agl         = XPLMFindDataRef("sim/flightmodel/position/y_agl");
    ref_grnd_spd        = XPLMFindDataRef("sim/flightmodel/position/groundspeed");

    // * Register our callback for every loop. Positive intervals
    // * are in seconds, negative are the negative of sim frames.  Zero
    // * registers but does not schedule a callback for time.
    XPLMRegisterFlightLoopCallback(		
            SmoothSailingCallback,	// * Callback *
            CALLBACK_INTERVAL,          // * Interval -1 every loop*
            NULL);				        // * refcon not used. *

	if( DEBUG ) {
        debug_window = XPLMCreateWindow(
                        50, 600, 300, 200,			/* Area of the window. */
                        1,							/* Start visible. */
                        MyDrawWindowCallback,		/* Callbacks */
                        MyHandleKeyCallback,
                        MyHandleMouseClickCallback,
                        NULL);						/* Refcon - not used. */
    }

    return 1;
}

PLUGIN_API void	XPluginStop(void) {
    XPLMUnregisterFlightLoopCallback(SmoothSailingCallback, NULL);
	
    if( DEBUG ) {
        XPLMDestroyWindow(debug_window);
    }
}


PLUGIN_API void XPluginDisable(void) {
}

PLUGIN_API int XPluginEnable(void) {
    return 1;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, long inMessage, void *inParam){
}

/*
 * MyDrawingWindowCallback
 * 
 * This callback does the work of drawing our window once per sim cycle each time
 * it is needed.  It dynamically changes the text depending on the saved mouse
 * status.  Note that we don't have to tell X-Plane to redraw us when our text
 * changes; we are redrawn by the sim continuously.
 * 
 */
void MyDrawWindowCallback(
                                   XPLMWindowID         inWindowID,    
                                   void *               inRefcon)
{
	int		left, top, right, bottom;
	float	color[] = { 1.0, 1.0, 1.0 }; 	/* RGB White */
    int xpndr_mode;
    float alt_agl, grnd_spd;
    char  buffer[50];

    xpndr_mode  = XPLMGetDatai( ref_xpndr_mode );
    alt_agl     = XPLMGetDataf( ref_alt_agl );
    grnd_spd    = XPLMGetDataf( ref_grnd_spd );
	
	/* First we get the location of the window passed in to us. */
	XPLMGetWindowGeometry(inWindowID, &left, &top, &right, &bottom);
	
	/* We now use an XPLMGraphics routine to draw a translucent dark
	 * rectangle that is our window's shape. */
	XPLMDrawTranslucentDarkBox(left, top, right, bottom);
    
    /* Finally we draw the text into the window, also using XPLMGraphics
	 * routines.  The NULL indicates no word wrapping. */
	sprintf(buffer, "xpndr_mode: %d", xpndr_mode);
    XPLMDrawString(color, left + 5, top - 20, buffer, NULL, xplmFont_Basic);
	
	sprintf(buffer, "alt_agl: %d", (int)floor(alt_agl));
    XPLMDrawString(color, left + 5, top - 40, buffer, NULL, xplmFont_Basic);
	
	sprintf(buffer, "grnd_spd: %d", (int)floor(grnd_spd));
    XPLMDrawString(color, left + 5, top - 60, buffer, NULL, xplmFont_Basic);
    
    XPLMDrawString(color, left + 5, top - 80, debug_string, NULL, xplmFont_Basic);
		
}                                   

/*
 * MyHandleKeyCallback
 * 
 * Our key handling callback does nothing in this plugin.  This is ok; 
 * we simply don't use keyboard input.
 * 
 */
void MyHandleKeyCallback(
                                   XPLMWindowID         inWindowID,    
                                   char                 inKey,    
                                   XPLMKeyFlags         inFlags,    
                                   char                 inVirtualKey,    
                                   void *               inRefcon,    
                                   int                  losingFocus)
{
}                                   

/*
 * MyHandleMouseClickCallback
 * 
 * Our mouse click callback toggles the status of our mouse variable 
 * as the mouse is clicked.  We then update our text on the next sim 
 * cycle.
 * 
 */
int MyHandleMouseClickCallback(
                                   XPLMWindowID         inWindowID,    
                                   int                  x,    
                                   int                  y,    
                                   XPLMMouseStatus      inMouse,    
                                   void *               inRefcon)
{
	/* If we get a down or up, toggle our status click.  We will
	 * never get a down without an up if we accept the down. */
	if ((inMouse == xplm_MouseDown) || (inMouse == xplm_MouseUp))
		clicked = 1 - clicked;
	
	/* Returning 1 tells X-Plane that we 'accepted' the click; otherwise
	 * it would be passed to the next window behind us.  If we accept
	 * the click we get mouse moved and mouse up callbacks, if we don't
	 * we do not get any more callbacks.  It is worth noting that we 
	 * will receive mouse moved and mouse up even if the mouse is dragged
	 * out of our window's box as long as the click started in our window's 
	 * box. */
	return 1;
}                                      
