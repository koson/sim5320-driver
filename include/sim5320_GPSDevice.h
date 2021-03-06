﻿#ifndef SIM5320_GPSDEVICE_H
#define SIM5320_GPSDEVICE_H

#include "AT_CellularBase.h"
#include "mbed.h"

namespace sim5320 {

/**
 * GPS API of the SIM5320
 */
class SIM5320GPSDevice : public AT_CellularBase, private NonCopyable<SIM5320GPSDevice> {
public:
    SIM5320GPSDevice(ATHandler &at);
    virtual ~SIM5320GPSDevice();

    /**
     * Struct for GPS coordinates.
     */
    struct gps_coord_t {
        // current longitude
        float longitude;
        // current latitude
        float latitude;
        // current altitude
        float altitude;
        // current time
        time_t time;
    };

    enum Mode {
        STANDALONE_MODE = 0,
        UE_BASED_MODE = 1
    };

    /**
     * Run GPS.
     *
     * @return 0 on success, non-zero on failure
     */
    nsapi_error_t start(Mode gps_mode = STANDALONE_MODE);

    /**
     * Stop GPS.
     *
     * @return 0 on success, non-zero on failure
     */
    nsapi_error_t stop();

    /**
     * Check if GPS is run.
     *
     * @return 0 on success, non-zero on failure
     */
    nsapi_error_t is_active(bool &active);

    /**
     * Get current GPS mode.
     *
     * @return 0 on success, non-zero on failure
     */
    nsapi_error_t get_mode(Mode &mode);

    /**
     * Set desired GPS accuracy in meters.
     *
     * @param value
     * @return  0 on success, non-zero on failure
     */
    nsapi_error_t set_desired_accuracy(int value);

    /**
     * Get desired GPS accuracy in meters.
     *
     * @param value
     * @return  0 on success, non-zero on failure
     */
    nsapi_error_t get_desired_accuracy(int &value);

    /**
     * Get current coordinate.
     *
     * @param has_coordinates this value will be true if coordinates are available
     * @param coord structure that will be filled with coordinates
     * @return 0 on success, non-zero on failure
     */
    nsapi_error_t get_coord(bool &has_coordinates, gps_coord_t &coord);

protected:
    // GPS assist server settings
    const virtual char *get_assist_server_url();
    bool virtual use_assist_server_ssl();
};
}

#endif // SIM5320_GPSDEVICE_H
