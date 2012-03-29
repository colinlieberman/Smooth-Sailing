
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
using namespace std;

#include <string.h>
#include <math.h>

#define DEBUG 0
#define CALLBACK_INTERVAL -1

/* abbreviations used in wind state:
 *      A - above
 *      B - bellow
 *
 *      T - transition
 *      P - persistent
 *
 *      eg AT: above-transition
 */
#define WIND_STATE_INITIAL  0
#define WIND_STATE_AT       1
#define WIND_STATE_AP       2
#define WIND_STATE_BT       3
#define WIND_STATE_BP       4

#define WIND_TRANSITION_STEPS   1000

#define OPP_HEADING heading > 180 ? heading - 180 : heading + 180

#define TRANSITION_TO_BT { \
                wind_state          = WIND_STATE_BT; \
 \
                origin_speed        = XPLMGetDataf( ref_wind_speed0 ); \
                origin_direction    = OPP_HEADING; \
                target_direction    = heading; \
                /* because the configuration might change, test which direction without assuming */ \
                speed_interval      = config_headwind_speed > config_tailwind_speed  \
                                        ? ( config_headwind_speed - config_tailwind_speed ) / WIND_TRANSITION_STEPS \
                                        : ( config_tailwind_speed - config_headwind_speed ) / WIND_TRANSITION_STEPS; \
     \
                /* don't assume it's 180 because might transition to below before completing above transition */ \
                direction_interval = origin_direction > target_direction  \
                                        ? ( origin_direction - target_direction ) / WIND_TRANSITION_STEPS \
                                        : ( target_direction - origin_direction ) / WIND_TRANSITION_STEPS; \
 \
                transition_steps    = 0; \
    }

#define TRANSITION_TO_AT { \
                wind_state          = WIND_STATE_AT; \
 \
                /* from initial the only transition is to the AT state, so we want to work towards a \
                 * 20kt tail wind */ \
                origin_speed        = XPLMGetDataf( ref_wind_speed0 ); \
                speed_interval      = config_tailwind_speed > origin_speed \
                                        ? ( config_tailwind_speed - origin_speed ) / WIND_TRANSITION_STEPS \
                                        : ( origin_speed - config_tailwind_speed ) / WIND_TRANSITION_STEPS; \
 \
                origin_direction    = XPLMGetDataf( ref_wind_direction0 ); \
                target_direction    = OPP_HEADING; \
                direction_interval  = ( origin_direction > target_direction ) \
                                        ? ( origin_direction - target_direction ) / WIND_TRANSITION_STEPS \
                                        : ( target_direction - origin_direction ) / WIND_TRANSITION_STEPS; \
 \
                transition_steps    = 0; \
    }

void initXpndr( void );
void initConfig( void );
void setVisibility( void );
void setCloudBase( float, float );
void setWind( float, float );
void resetTime( void );
void forceWind( float, float );
void setTurbulence( float );
float getHighestCloudAlt( void );

