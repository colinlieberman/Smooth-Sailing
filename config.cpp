#include "ss.h"

#include <fstream>

/* lines in the file + 1 */
#define EXPECTED_PREF_LINES 61

#define CONFIG_FILE_PATH "Resources/plugins/SmoothSailing/config.txt"
#define ERROR_PREFIX "SmoothSailing:"

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

extern char debug_string[255];

void initConfig() {
    string line, lines[ EXPECTED_PREF_LINES + 2 ];
    ifstream pref_file;
    int cur_line = 1, tmpi;
    float tmpf;


    /* set defaults */
    config_default_xpndr_setting = 1200;

    /* in meters - 40km =~ 25 sm */
    config_visibility_setting    = 40000;

    /* in meters - 800m =~ 2600 ft */
    config_min_cloud_base = 1500;

    /* time reset - time is measured in seconds since midnight -
     * config_late_time_seconds is the time (in seconds since midnight) at which we start rolling back
     * config_early_time_seconds is similar, but for too early in the morning
     * config_time_rollback_seconds is the number of seconds to roll back 
     * config_time_push_forward_seconds is the number of seconds to push forward */

    /* 4pm = 16 hours from midnight = 57600 seconds */
    /* use 4pm so there's a good 3 hours or so flight time available */
    config_late_time_seconds = 57600;

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

    pref_file.open( CONFIG_FILE_PATH );
    if( ! pref_file ) {
        fprintf( stderr, "%s unable to open file %s; using defaults", ERROR_PREFIX, CONFIG_FILE_PATH );
        return;
    }

    while( !pref_file.eof() )
    {
        if( cur_line > EXPECTED_PREF_LINES ) {
            /* this means we're still reading */
            fprintf( stderr, "%s preferences file %s in unexpeted format, using defaults", ERROR_PREFIX, CONFIG_FILE_PATH );
            return;
        }
        
        getline( pref_file, line );
        lines[ cur_line++ ] = line;
    }
    pref_file.close();

    /* check to make sure we have the expected number of lines -
     * cur line should be one greater than expected length because it
     * incremented at the bottom of the loop */
    if( cur_line != EXPECTED_PREF_LINES + 1 ) {
        fprintf( stderr, "%s preferences file %s in unexpeted format, using defaults", ERROR_PREFIX, CONFIG_FILE_PATH );
        return;
    }

    /* line 6: xpndr */
    line = lines[ 6 ];
    tmpi  = atoi( line.c_str() );
    if( tmpi >= 0 && tmpi <= 7777 ) {
        config_default_xpndr_setting = tmpi;
    }

    /* line 9: visibility */
    line = lines[ 9 ];
    tmpf  = atof( line.c_str() );
    if( tmpf >= 0 && tmpf <= 50000 ) {
        config_visibility_setting = tmpf;
    }

    /* line 12: min cloud base */
    line = lines[ 12 ];
    tmpf  = atof( line.c_str() );
    if( tmpf >= 0 && tmpf <= 100000 ) {
        config_min_cloud_base = tmpf;
    }

    /* line 23: late time seconds */
    line = lines[ 23 ];
    tmpf  = atof( line.c_str() );
    if( tmpf >= 0 && tmpf <= 86400 ) {
        config_late_time_seconds = tmpf;
    }

    /* line 28: early time seconds */
    line = lines[ 28 ];
    tmpf  = atof( line.c_str() );
    if( tmpf >= 0 && tmpf <= 86400 ) {
        config_early_time_seconds = tmpf;
    }

    /* line 34: rollback seconds */
    line = lines[ 34 ];
    tmpi  = atof( line.c_str() );
    if( tmpf >= 0 && tmpf <= 86400 ) {
        config_time_rollback_seconds = tmpi;
   }
 
    /* line 40: push forward seconds */
    line = lines[ 40 ];
    tmpi  = atof( line.c_str() );
    if( tmpf >= 0 && tmpf <= 86400 ) {
        config_time_push_forward_seconds = tmpi;
    }

    /* line 45: wind transition altitude */
    line = lines[ 45 ];
    tmpf  = atof( line.c_str() );
    if( tmpf >= 0 && tmpf <= 100000 ) {
        config_wind_transition_altitude = tmpf;
    }

    /* line 48: tailwind speed */
    line = lines[ 48 ];
    tmpf  = atof( line.c_str() );
    if( tmpf >= 0 && tmpf <= 200 ) {
        config_tailwind_speed = tmpf;
    }

    /* line 51: headwind speed */
    line = lines[ 51 ];
    tmpf  = atof( line.c_str() );
    if( tmpf >= 0 && tmpf <= 200 ) {
        config_headwind_speed = tmpf;
    }
    
    return;
}
