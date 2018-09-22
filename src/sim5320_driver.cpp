#include "sim5320_driver.h"
#include <string.h>
using namespace sim5320;

const int SIM5320::SERIAL_BAUDRATE = 115200;
const int SIM5320::AT_BUFFER_SIZE = 256;
const int SIM5320::AT_TIMEOUT = 8000;
const char* SIM5320::AT_DELIMITER = "\r";

const char* SIM5320::GPS_ASSIST_SERVER_URL = "supl.google.com:7276";
const bool SIM5320::GPS_ASSIST_SERVER_SSL = false;

//
// ATCmdParser note: to match the whole string, it should ends with '\n'
//

SIM5320::SIM5320(UARTSerial* serial_ptr, PinName rts, PinName cts)
    : _serial_ptr(serial_ptr)
    , _parser(serial_ptr, AT_DELIMITER, AT_BUFFER_SIZE, AT_TIMEOUT)
    , _state(0x00)
    , _rts(rts)
    , _cts(cts)
{
    serial_ptr->set_baud(SERIAL_BAUDRATE);
    serial_ptr->set_format(8, UARTSerial::None, 1);
}

SIM5320::SIM5320(PinName tx, PinName rx, PinName rts, PinName cts)
    : _serial_ptr(new UARTSerial(tx, rx, SERIAL_BAUDRATE))
    , _parser(_serial_ptr, AT_DELIMITER, AT_BUFFER_SIZE, AT_TIMEOUT)
    , _state(0x00)
    , _rts(rts)
    , _cts(cts)
{
}

SIM5320::~SIM5320()
{
    if (_state & SIM5320::CLEANUP_SERIAL) {
        delete _serial_ptr;
    }
}

void SIM5320::debug_on(bool on)
{
    DriverLock(this);
    _parser.debug_on(on);
}

bool SIM5320::at_available()
{
    DriverLock(this);
    return _parser.send("AT") && _parser.recv("OK\n");
}

bool SIM5320::start_uart_hw_flow_ctrl()
{
    DriverLock(this);
    bool done = true;

    if (_rts != NC && _cts != NC) {
        _serial_ptr->set_flow_control(UARTSerial::RTSCTS, _rts, _cts);
        done = _parser.send("AT+IFC=2,2") && _parser.recv("OK\n");
    } else if (_rts != NC) {
        _serial_ptr->set_flow_control(UARTSerial::RTS, _rts);
        done = _parser.send("AT+IFC=2,0") && _parser.recv("OK\n");
    } else if (_cts != NC) {
        _serial_ptr->set_flow_control(UARTSerial::CTS, _cts);
        done = _parser.send("AT+IFC=0,2") && _parser.recv("OK\n");
    }
}

bool SIM5320::stop_uart_hw_flow_ctrl()
{
    DriverLock(this);
    bool done = true;

    if (_rts != NC || _cts != NC) {
        _serial_ptr->set_flow_control(SerialBase::Disabled, _rts, _cts);
        done = _parser.send("AT+IFC=0,2") && _parser.recv("OK\n");
    }

    return done;
}

int SIM5320::init()
{
    int code;
    bool done;
    DriverLock(this);

    // ensure that serial has correct settings
    _serial_ptr->set_baud(SERIAL_BAUDRATE);
    _serial_ptr->set_format(8, Serial::None, 1);

    // check AT command
    if (!at_available()) {
        return MBED_ERROR_CODE_INITIALIZATION_FAILED;
    }

    // disable echo
    done = _parser.send("ATE0") && _parser.recv("OK\n");
    if (!done) {
        return MBED_ERROR_CODE_INITIALIZATION_FAILED;
    }

    // check device model
    char modem_model[24];
    done = _parser.send("AT+CGMM") && _parser.recv("%23s\n", modem_model) && _parser.recv("OK\n");
    if (!done) {
        return MBED_ERROR_CODE_INITIALIZATION_FAILED;
    }
    if (strncmp(modem_model, "SIMCOM_SIM5320", 14) != 0) {
        return MBED_ERROR_CODE_INITIALIZATION_FAILED;
    }

    // switch device into low power mode, if
    if (!stop()) {
        return MBED_ERROR_CODE_INITIALIZATION_FAILED;
    }

    // initialize GPS
    code = gps_init();
    if (code) {
        return code;
    }

    return MBED_SUCCESS;
}

bool SIM5320::reset()
{
    DriverLock(this);

    bool done = _parser.send("AT+CRESET") && _parser.recv("OK\n");
    if (!done) {
        return false;
    }
    // wait reset
    for (int i = 0; i < 10; i++) {
        done = _parser.recv("START\n");
        if (done) {
            return true;
        }
    }

    return false;
}

void SIM5320::get_imei(char imei[])
{
    DriverLock(this);
    _parser.send("AT+CGSN") && _parser.recv("%17s\n", imei) && _parser.recv("OK\n");
}

bool SIM5320::start()
{
    DriverLock(this);
    return _parser.send("AT+CFUN=1") && _parser.recv("OK\n");
}

bool SIM5320::stop()
{
    DriverLock(this);
    return _parser.send("AT+CFUN=0") && _parser.recv("OK\n");
}

bool SIM5320::is_active()
{
    DriverLock(this);
    int mode;
    _parser.send("AT+CFUN?");
    _parser.recv("+CFUN: %d\n", &mode) && _parser.recv("OK\n");
    return mode != 0;
}

int SIM5320::gps_init()
{
    DriverLock(this);
    // stop GPS if it's run
    if (!gps_stop()) {
        return MBED_ERROR_CODE_INITIALIZATION_FAILED;
    }
    // set AGPS url
    if (!(_parser.send("AT+CGPSURL=\"%s\"", GPS_ASSIST_SERVER_URL) && _parser.recv("OK\n"))) {
        return MBED_ERROR_CODE_INITIALIZATION_FAILED;
    }
    // set AGPS ssl
    if (!(_parser.send("AT+CGPSSSL=%d", GPS_ASSIST_SERVER_SSL ? 1 : 0) && _parser.recv("OK\n"))) {
        return MBED_ERROR_CODE_INITIALIZATION_FAILED;
    }
    // disable automatic (AT+CGPSAUTO) AGPS start
    if (!(_parser.send("AT+CGPSAUTO=0") && _parser.recv("OK\n"))) {
        return MBED_ERROR_CODE_INITIALIZATION_FAILED;
    }
    // set position mode (AT+CGPSPMD) to 127
    if (!(_parser.send("AT+CGPSPMD=127") && _parser.recv("OK\n"))) {
        return MBED_ERROR_CODE_INITIALIZATION_FAILED;
    }
    // ensure switch to standalone mode automatically
    if (!(_parser.send("AT+CGPSMSB=1") && _parser.recv("OK\n"))) {
        return MBED_ERROR_CODE_INITIALIZATION_FAILED;
    }

    return MBED_SUCCESS;
}

bool SIM5320::gps_start(SIM5320::GPSMode gps_mode)
{
    DriverLock(this);
    int mode;
    if (gps_mode == SIM5320::GPS_STANDALONE_MODE) {
        mode = 1;
    } else {
        mode = 2;
    }
    return _parser.send("AT+CGPS=1,%d", mode) && _parser.recv("OK\n");
}

bool sim5320::SIM5320::gps_stop()
{
    DriverLock(this);
    return _parser.send("AT+CGPS=0") && _parser.recv("OK\n");
}

bool sim5320::SIM5320::gps_is_active()
{
    DriverLock(this);
    int on_flag = 0;
    int mode;
    _parser.send("AT+CGPS?") && _parser.recv("+CGPS:%d,%d\n", &on_flag, &mode) && _parser.recv("OK\n");
    return on_flag;
}

SIM5320::GPSMode SIM5320::get_gps_mode()
{
    DriverLock(this);
    int on_flag;
    int mode = 2;
    _parser.send("AT+CGPS?") && _parser.recv("+CGPS:%d,%d\n", &on_flag, &mode) && _parser.recv("OK\n");
    return mode == 1 ? GPS_STANDALONE_MODE : GPS_UE_BASED_MODE;
}

bool sim5320::SIM5320::gps_get_coord(sim5320::gps_coord_t* coord)
{
    DriverLock(this);
    // read coordinates
    char coord_str[64];
    bool done = _parser.send("AT+CGPSINFO") && _parser.recv("+CGPSINFO:%63s\n", coord_str) && _parser.recv("OK\n");
    if (!done) {
        return false;
    }
    // parse coordinate string
    // example: 3113.343286,N,12121.234064,E,250311,072809.3,44.1,0.0,0
    int lat_deg, lon_deg;
    float lat_min, lon_min;
    char lat_ind, lon_ind;
    tm gps_tm;
    float tm_sec;
    float alt;

    int res = sscanf(coord_str, "%2d%f,%c,%3d%f,%c,%2d%2d%2d,%2d%2d%f,%f",
        &lat_deg, &lat_min, &lat_ind,
        &lon_deg, &lon_min, &lon_ind,
        &gps_tm.tm_mday, &gps_tm.tm_mon, &gps_tm.tm_year,
        &gps_tm.tm_hour, &gps_tm.tm_min, &tm_sec,
        &alt);

    if (res != 13) {
        return false;
    }

    // calculate longitude/latitude
    float lat = lat_deg + lat_min / 60;
    lat = lat_ind == 'S' ? -lat : lat;
    float lon = lon_deg + lon_min / 60;
    lon = lon_ind == 'W' ? -lon : lon;
    coord->latitude = lat;
    coord->longitude = lon;
    coord->altitude = alt;

    // calculate time
    gps_tm.tm_sec = (int)tm_sec;
    gps_tm.tm_year += 100;
    gps_tm.tm_mon -= 1;
    coord->time = mktime(&gps_tm);

    return true;
}

SIM5320::DriverLock::DriverLock(SIM5320* instance_ptr)
    : _instance_ptr(instance_ptr)
{
    instance_ptr->_mutex.lock();
}

SIM5320::DriverLock::~DriverLock()
{
    _instance_ptr->_mutex.unlock();
}
