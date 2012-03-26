
#include "XPLMUtilities.h"
#include "XPLMMenus.h"
#include "XPWidgets.h"
#include "XPStandardWidgets.h"

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

void initXpndr( float );
void initConfig( void );
void setVisibility( void );
void setCloudBase( float, float );
void setLocalTime( float );
void setWind( float, float );

