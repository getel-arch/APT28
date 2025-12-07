#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "base64.h"

char* get_location_info() {
    GEOID geoId = GetUserGeoID(GEOCLASS_NATION);
    if (geoId == GEOID_NOT_AVAILABLE) {
        char* error_msg = "Unable to retrieve the region information.";
        return base64_encode((unsigned char*)error_msg, strlen(error_msg));
    }

    WCHAR regionName[100];
    int result = GetGeoInfoW(geoId, GEO_FRIENDLYNAME, regionName, sizeof(regionName) / sizeof(WCHAR), 0);
    if (result == 0) {
        char* error_msg = "Unable to retrieve the region name.";
        return base64_encode((unsigned char*)error_msg, strlen(error_msg));
    }

    // Convert wide char to multi-byte string
    char regionNameMB[200];
    snprintf(regionNameMB, sizeof(regionNameMB), "Region: ");
    
    int len = WideCharToMultiByte(CP_UTF8, 0, regionName, -1, 
                                   regionNameMB + strlen(regionNameMB), 
                                   sizeof(regionNameMB) - strlen(regionNameMB), 
                                   NULL, NULL);
    
    if (len == 0) {
        char* error_msg = "Unable to convert region name.";
        return base64_encode((unsigned char*)error_msg, strlen(error_msg));
    }

    // Encode to base64
    return base64_encode((unsigned char*)regionNameMB, strlen(regionNameMB));
}
