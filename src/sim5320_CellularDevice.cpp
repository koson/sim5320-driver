#include "sim5320_CellularDevice.h"
#include "sim5320_CellularInformation.h"
#include "sim5320_CellularNetwork.h"
#include "sim5320_CellularPower.h"
using namespace sim5320;

#define SIM5320_DEFAULT_TIMEOUT 4000

SIM5320CellularDevice::SIM5320CellularDevice(events::EventQueue& queue)
    : AT_CellularDevice(queue)
    , _gps(NULL)
    , _gps_ref_count(0)
    , _ftp_client(NULL)
    , _ftp_client_ref_count(0)
{
    _default_timeout = SIM5320_DEFAULT_TIMEOUT;
}

SIM5320CellularDevice::~SIM5320CellularDevice()
{
}

SIM5320GPSDevice* SIM5320CellularDevice::open_gps(FileHandle* fh)
{
    if (!_gps) {
        ATHandler* at_handler = get_at_handler(fh);
        if (at_handler) {
            _gps = open_gps_impl(*at_handler);
        }
    }
    if (_gps) {
        _gps_ref_count++;
    }
    return _gps;
}

void SIM5320CellularDevice::close_gps()
{
    if (_gps) {
        _gps_ref_count--;
        if (_gps_ref_count == 0) {
            ATHandler* atHandler = &_gps->get_at_handler();
            delete _gps;
            _gps = NULL;
            release_at_handler(atHandler);
        }
    }
}

SIM5320FTPClient* SIM5320CellularDevice::open_ftp_client(FileHandle* fh)
{
    if (!_ftp_client) {
        ATHandler* at_handler = get_at_handler(fh);
        if (at_handler) {
            _ftp_client = open_ftp_client_impl(*at_handler);
        }
    }
    if (_ftp_client) {
        _ftp_client_ref_count++;
    }
    return _ftp_client;
}

void SIM5320CellularDevice::close_ftp_client()
{
    if (_ftp_client) {
        _ftp_client_ref_count--;
        if (_ftp_client_ref_count == 0) {
            ATHandler* atHandler = &_ftp_client->get_at_handler();
            delete _ftp_client;
            _ftp_client = NULL;
            release_at_handler(atHandler);
        }
    }
}

AT_CellularPower* SIM5320CellularDevice::open_power_impl(ATHandler& at)
{
    return new SIM5320CellularPower(at);
}

AT_CellularInformation* SIM5320CellularDevice::open_information_impl(ATHandler& at)
{
    return new SIM5320CellularInformation(at);
}

AT_CellularNetwork* SIM5320CellularDevice::open_network_impl(ATHandler& at)
{
    return new SIM5320CellularNetwork(at);
}

SIM5320GPSDevice* SIM5320CellularDevice::open_gps_impl(ATHandler& at)
{
    return new SIM5320GPSDevice(at);
}

SIM5320FTPClient* SIM5320CellularDevice::open_ftp_client_impl(ATHandler& at)
{
    return new SIM5320FTPClient(at);
}