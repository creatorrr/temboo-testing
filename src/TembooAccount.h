/*
IMPORTANT NOTE about TembooAccount.h

TembooAccount.h contains your Temboo account information and must be included
alongside your sketch. To do so, make a new tab in Arduino, call it TembooAccount.h,
and copy this content into it.
*/

#define TEMBOO_ACCOUNT "diwank"  // Your Temboo account name
#define TEMBOO_APP_KEY_NAME "myFirstApp"  // Your Temboo app key name
#define TEMBOO_APP_KEY "7XIwOSAEWOc53RXRkvqVUT6Y1obusCc3"  // Your Temboo app key

#define GSM_PIN_NUMBER ""
#define GPRS_APN "airtelgprs.com"
#define GPRS_LOGIN ""
#define GPRS_PASSWORD ""

#define WIFI_SSID "SSIP"
#define WEP_KEY "laddumasti1233"
#define WEP_KEY_INDEX 1

/*
The same TembooAccount.h file settings can be used for all Temboo SDK sketches.
Keeping your account information in a separate file means you can share the
main .ino file without worrying that you forgot to delete your credentials.
*/
