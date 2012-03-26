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
XPLMDataRef ref_alt_msl         = NULL;
XPLMDataRef ref_grnd_spd        = NULL;
XPLMDataRef ref_visibility      = NULL;

XPLMDataRef ref_cloud_base0     = NULL;
XPLMDataRef ref_cloud_base1     = NULL;
XPLMDataRef ref_cloud_base2     = NULL;

XPLMDataRef ref_cloud_tops0     = NULL;
XPLMDataRef ref_cloud_tops1     = NULL;
XPLMDataRef ref_cloud_tops2     = NULL;

XPLMDataRef ref_use_sys_time    = NULL;
XPLMDataRef ref_local_time      = NULL;
XPLMDataRef ref_zulu_time       = NULL;

XPLMDataRef ref_heading         = NULL;

XPLMDataRef ref_wind_speed0     = NULL;
XPLMDataRef ref_wind_speed1     = NULL;
XPLMDataRef ref_wind_speed2     = NULL;

XPLMDataRef ref_wind_direction  = NULL;
XPLMDataRef ref_wind_altitude   = NULL;

int wind_state = WIND_STATE_INITIAL;

XPLMWindowID	debug_window = NULL;
int				clicked = 0;

int config_default_xpndr_setting;
float config_visibility_setting;
float config_min_cloud_base;
float config_late_time_seconds;
float config_early_time_seconds;
int config_time_rollback_seconds;
int config_time_push_forward_seconds;
float config_wind_transition_altitude;
float config_tailwind_speed;
float config_headwind_speed;

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

    /* in meters - 40km =~ 25 sm */
    config_visibility_setting    = 40000;

    /* in meters - 800m =~ 2600 ft */
    config_min_cloud_base = 800;

    /* time reset - time is measured in seconds since midnight -
     * config_late_time_seconds is the time (in seconds since midnight) at which we start rolling back
     * config_early_time_seconds is similar, but for too early in the morning
     * config_time_rollback_seconds is the number of seconds to roll back 
     * config_time_push_forward_seconds is the number of seconds to push forward */
     

    /* 7pm = 19 hours from midnight = 68400 seconds */
    config_late_time_seconds = 68400;

    /* 6am = 6 hours = 21600 seconds */
    config_early_time_seconds = 21600;

    /* roll back 10 hours, or 3600 seconds */
    config_time_rollback_seconds = 36000;

    /* push forward 6 hours, or 21600 seconds */
    config_time_push_forward_seconds = 21600;

    /* 800 meters =~ 2600 ft */
    config_wind_transition_altitude = 800;

    config_tailwind_speed = 20;
    config_headwind_speed = 5;
}

float SmoothSailingCallback(
        float   inElapsedSinceLastCall,
        float   inElapsedTimeSinceLastFlightLoop,
        int     inCounter,
        void    *inRefcon) {

    /* get altitude - it's used in a number of places */
    float alt_agl = XPLMGetDataf( ref_alt_agl );
    float alt_msl = XPLMGetDataf( ref_alt_msl );

    initXpndr( alt_agl );
    setVisibility();
    setCloudBase( alt_agl, alt_msl );
    setWind( alt_agl, alt_msl );

    return CALLBACK_INTERVAL;
}

void setWind( float alt_agl, float alt_msl ) {
    int wind_speed;
    float wind_direction, target_speed;

    static int transition_steps = 0;
    static float direction_interval, speed_interval, origin_direction, target_direction, origin_speed;

    float heading = XPLMGetDataf( ref_heading );

    /* wind alt should always be the plane's alt */
    XPLMSetDatai( ref_wind_altitude, alt_msl );

    switch( wind_state ) {
        case WIND_STATE_INITIAL:
            /* don't change the weather, just check to see if we're reading for a state change */
            if( alt_agl > config_wind_transition_altitude ) {
                wind_state          = WIND_STATE_AT;

                /* from initial the only transition is to the AT state, so we want to work towards a
                 * 20kt tail wind */
                origin_speed        = XPLMGetDataf( ref_wind_speed0 );
                speed_interval      = config_tailwind_speed > origin_speed
                                        ? ( config_tailwind_speed - origin_speed ) / WIND_TRANSITION_STEPS
                                        : ( origin_speed - config_tailwind_speed ) / WIND_TRANSITION_STEPS;

                origin_direction    = XPLMGetDataf( ref_wind_direction );
                target_direction    = heading > 180 ? heading - 180 : heading + 180;
                direction_interval  = ( origin_direction > target_direction )
                                        ? ( origin_direction - target_direction ) / WIND_TRANSITION_STEPS
                                        : ( target_direction - origin_direction ) / WIND_TRANSITION_STEPS;

                transition_steps    = 0;
            }
            break;

        case WIND_STATE_AT:
            /* in the transition state, we want to move direction to 180 degrees from heading,
             * and move the speed to 20kts */
            
            transition_steps++;

            target_speed = config_tailwind_speed > origin_speed 
                                            ? origin_speed + ( speed_interval * transition_steps )
                                            : origin_speed - ( speed_interval * transition_steps );


            /* set all three layers so the speed is predictable as you climb */
            XPLMSetDataf( ref_wind_speed0, target_speed);
            XPLMSetDataf( ref_wind_speed1, target_speed);
            XPLMSetDataf( ref_wind_speed2, target_speed);

            XPLMSetDataf( ref_wind_direction, origin_direction > target_direction
                                                ? origin_direction - ( direction_interval * transition_steps )
                                                : origin_direction + ( direction_interval * transition_steps )
                        );

            if( transition_steps >= WIND_TRANSITION_STEPS ) {
                wind_state = WIND_STATE_AP;
            }

            break;
    
        case WIND_STATE_AP:
            /* persistant state - preserve tailwind */
            XPLMSetDataf( ref_wind_speed0, config_tailwind_speed );
            XPLMSetDataf( ref_wind_speed1, config_tailwind_speed );
            XPLMSetDataf( ref_wind_speed2, config_tailwind_speed );
            XPLMSetDataf( ref_wind_direction, heading > 180 ? heading - 180 : heading + 180 ); 
    }
}

void setCloudBase( float alt_agl, float alt_msl ) {

    float cloud_base_msl = XPLMGetDataf( ref_cloud_base0 );
    float ground_level   = alt_msl - alt_agl;
    float cloud_base_agl = cloud_base_msl - ground_level;

    /* find the difference betwee the lowest cloud layer and what we want,
     * then apply that difference to all cloud layers */
    float delta = config_min_cloud_base - cloud_base_agl;

    if( delta > 0 ) {
        XPLMSetDataf( ref_cloud_base0, cloud_base_msl + delta );
        XPLMSetDataf( ref_cloud_tops0, XPLMGetDataf( ref_cloud_tops0 ) + delta );

        XPLMSetDataf( ref_cloud_base1, XPLMGetDataf( ref_cloud_base1 ) + delta );
        XPLMSetDataf( ref_cloud_tops1, XPLMGetDataf( ref_cloud_tops1 ) + delta );

        XPLMSetDataf( ref_cloud_base2, XPLMGetDataf( ref_cloud_base2 ) + delta );
        XPLMSetDataf( ref_cloud_tops2, XPLMGetDataf( ref_cloud_tops2 ) + delta );
    }
}

void setVisibility() {
    XPLMSetDataf( ref_visibility, config_visibility_setting );
}

void initXpndr(float alt_agl) {
    /* if the plane is stationary and altitude agl is 0, and the xpndr is off, turn it on */
    int xpndr_mode;

    xpndr_mode  = XPLMGetDatai( ref_xpndr_mode );

    /* Transponder mode (off=0 stdby=1 on=2 test=3) */
    if(     !xpndr_mode
            && !floor( alt_agl )
            && !floor( XPLMGetDataf( ref_grnd_spd ) )
       ) {
        XPLMSetDatai( ref_xpndr_mode, 2 );
        XPLMSetDatai( ref_xpndr_setting, config_default_xpndr_setting );
    }
}

void resetTime() {
    float local_time_seconds, new_zulu_seconds;
    
    /* in order for the clock to be set right, we need to make sure we're using
     * system time initially to get local time, then turn off use system time before
     * changing the time */
    XPLMSetDatai( ref_use_sys_time, 1 );
    
    local_time_seconds = XPLMGetDataf( ref_local_time );

    if( local_time_seconds > config_late_time_seconds ) {
        new_zulu_seconds = XPLMGetDataf( ref_zulu_time ) - config_time_rollback_seconds;
        XPLMSetDataf( ref_zulu_time, new_zulu_seconds );
    }
    else if( local_time_seconds < config_early_time_seconds ) {
        new_zulu_seconds = XPLMGetDataf( ref_zulu_time ) + config_time_push_forward_seconds;
        XPLMSetDataf( ref_zulu_time, new_zulu_seconds );
    }
        
    XPLMSetDatai( ref_use_sys_time, 0 );
}

PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc) {

    /* First set up our plugin info. */
    strcpy(outName, "SmoothSailing 0.1");
    strcpy(outSig, "Smooth Sailing by Colin Lieberman");
    strcpy(outDesc, "Simple weather and visibility controls for convenience");

    initConfig();

    /* datarefs used for the transponder setting & checks to determine if i want to set it */
    ref_xpndr_mode      = XPLMFindDataRef("sim/cockpit/radios/transponder_mode");
    ref_xpndr_setting   = XPLMFindDataRef("sim/cockpit/radios/transponder_code");
    ref_alt_agl         = XPLMFindDataRef("sim/flightmodel/position/y_agl");
    ref_grnd_spd        = XPLMFindDataRef("sim/flightmodel/position/groundspeed");

    /* dataref for visibility */
    ref_visibility      = XPLMFindDataRef("sim/weather/visibility_reported_m");

    /* datarefs for cloud base */
    ref_cloud_base0     = XPLMFindDataRef("sim/weather/cloud_base_msl_m[0]");
    ref_cloud_base1     = XPLMFindDataRef("sim/weather/cloud_base_msl_m[1]");
    ref_cloud_base2     = XPLMFindDataRef("sim/weather/cloud_base_msl_m[2]");

    ref_cloud_tops0     = XPLMFindDataRef("sim/weather/cloud_tops_msl_m[0]");
    ref_cloud_tops1     = XPLMFindDataRef("sim/weather/cloud_tops_msl_m[1]");
    ref_cloud_tops2     = XPLMFindDataRef("sim/weather/cloud_tops_msl_m[2]");

    ref_alt_msl         = XPLMFindDataRef("sim/flightmodel/position/elevation");

    /* datarefs for setting time */
    ref_use_sys_time    = XPLMFindDataRef("sim/time/use_system_time");

    ref_local_time      = XPLMFindDataRef("sim/time/local_time_sec");
    ref_zulu_time       = XPLMFindDataRef("sim/time/zulu_time_sec");

    /* datarefs for wind */
    ref_heading         = XPLMFindDataRef("sim/flightmodel/position/psi");
    ref_wind_speed0     = XPLMFindDataRef("sim/weather/wind_speed_kt[0]");
    ref_wind_speed1     = XPLMFindDataRef("sim/weather/wind_speed_kt[1]");
    ref_wind_speed2     = XPLMFindDataRef("sim/weather/wind_speed_kt[2]");
    ref_wind_direction  = XPLMFindDataRef("sim/weather/wind_direction_degt[0]");
    ref_wind_altitude   = XPLMFindDataRef("sim/weather/wind_altitude_msl_m[0]");

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
    
    if( inMessage >= XPLM_MSG_AIRPORT_LOADED || inMessage == XPLM_MSG_PLANE_LOADED ) {
        /* anything that looks like the situation was reset, reset the clock */ 
        resetTime();
    }
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
