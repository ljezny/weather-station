#ifndef DEBUG
#define DEBUG 1
#endif

#ifndef DEMO
#define DEMO 0
#endif

#ifndef OTA_ENABLED
#define OTA_ENABLED 0
#endif

#ifndef VERSION_NUMBER
#define VERSION_NUMBER (0)
#endif

// Maximum wakeup time before forcing deep sleep (3 minutes)
#define MAX_WAKEUP_TIME_MS (1000 * 60 * 3)

// WiFi retry settings
#define MAX_WIFI_FAILURES_BEFORE_SETUP 3
#define WIFI_RETRY_SLEEP_SECONDS (5 * 60)

// Display 10.2" (GDEM102T91)
#define DISPLAY_102 1
