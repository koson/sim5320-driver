/**
 * Example of the SIM5320E usage with STM32F3Discovery board.
 *
 * This example shows network information.
 *
 * Requirements:
 *
 * - active SIM card
 *
 * Pin map:
 *
 * - PA_2 - UART TX (SIM5320E)
 * - PA_3 - UART RX (SIM5320E)
 */

#include "math.h"
#include "mbed.h"
#include "sim5320_driver.h"
#include "stdlib.h"
#include "time.h"

using namespace sim5320;

#define CHECK_RET_CODE(expr)                                                           \
    {                                                                                  \
        int err = expr;                                                                \
        if (err < 0) {                                                                 \
            char err_msg[64];                                                          \
            sprintf(err_msg, "Expression \"" #expr "\" failed (error code: %i)", err); \
            MBED_ERROR(MBED_MAKE_ERROR(MBED_MODULE_APPLICATION, err), err_msg);        \
        }                                                                              \
    }

DigitalOut led(LED2);

nsapi_error_t attach_to_network(SIM5320 *sim5320)
{
    nsapi_error_t err = 0;
    CellularNetwork::AttachStatus attach_status;
    CellularNetwork *cellular_network = sim5320->get_network();

    for (int i = 0; i < 10; i++) {
        err = cellular_network->set_attach();
        if (!err) {
            break;
        }
        wait_ms(1000);
    }
    if (err) {
        return err;
    }

    for (int i = 0; i < 30; i++) {
        cellular_network->get_attach(attach_status);
        if (attach_status == CellularNetwork::Attached) {
            return NSAPI_ERROR_OK;
        }
        wait_ms(1000);
    }
    return NSAPI_ERROR_TIMEOUT;
}

nsapi_error_t detach_from_network(SIM5320 *sim5320)
{
    CellularNetwork *cellular_network = sim5320->get_network();
    return cellular_network->detach();
}

static const char *get_nw_registering_mode_name(CellularNetwork::NWRegisteringMode mode)
{
    switch (mode) {
    case CellularNetwork::NWModeAutomatic:
        return "NWModeAutomatic";
    case CellularNetwork::NWModeManual:
        return "NWModeManual";
    case CellularNetwork::NWModeDeRegister:
        return "NWModeDeRegister";
    case CellularNetwork::NWModeSetOnly:
        return "NWModeSetOnly";
    case CellularNetwork::NWModeManualAutomatic:
        return "NWModeManualAutomatic";
    default:
        return "Unknown error";
    }
}

static const char *get_radio_access_technology_name(CellularNetwork::RadioAccessTechnology rat)
{
    switch (rat) {
    case CellularNetwork::RAT_GSM:
        return "RAT_GSM";
    case CellularNetwork::RAT_GSM_COMPACT:
        return "RAT_GSM_COMPACT";
    case CellularNetwork::RAT_UTRAN:
        return "RAT_UTRAN";
    case CellularNetwork::RAT_EGPRS:
        return "RAT_EGPRS";
    case CellularNetwork::RAT_HSDPA:
        return "RAT_HSDPA";
    case CellularNetwork::RAT_HSUPA:
        return "RAT_HSUPA";
    case CellularNetwork::RAT_HSDPA_HSUPA:
        return "RAT_HSDPA_HSUPA";
    case CellularNetwork::RAT_E_UTRAN:
        return "RAT_E_UTRAN";
    case mbed::CellularNetwork::RAT_CATM1:
        return "RAT_CATM1";
    case CellularNetwork::RAT_NB1:
        return "RAT_NB1";
    case CellularNetwork::RAT_UNKNOWN:
        return "RAT_UNKNOWN";
    case CellularNetwork::RAT_MAX:
        return "RAT_MAX";
    default:
        return "Unknown error";
    }
}

static const char *get_operator_status_name(CellularNetwork::operator_t::Status status)
{
    switch (status) {
    case CellularNetwork::operator_t::Unknown:
        return "Unknown";
    case CellularNetwork::operator_t::Available:
        return "Available";
    case CellularNetwork::operator_t::Current:
        return "Current";
    case CellularNetwork::operator_t::Forbiden:
        return "Forbiden";
    default:
        return "Unknown error";
    }
}

static const char *get_attach_status_name(CellularNetwork::AttachStatus status)
{
    switch (status) {
    case CellularNetwork::Detached:
        return "Detached";
    case CellularNetwork::Attached:
        return "Attached";
    default:
        return "Unknown error";
    }
}

static const char *get_reg_mode_name(CellularNetwork::RegistrationType type)
{

    switch (type) {
    case CellularNetwork::C_EREG:
        return "C_EREG";
    case CellularNetwork::C_GREG:
        return "C_GREG";
    case CellularNetwork::C_REG:
        return "C_REG";
    default:
        return "Unknown error";
    }
}

static const char *get_reg_status_name(CellularNetwork::RegistrationStatus status)
{
    switch (status) {
    case CellularNetwork::StatusNotAvailable:
        return "StatusNotAvailable";
    case CellularNetwork::NotRegistered:
        return "NotRegistered";
    case CellularNetwork::RegisteredHomeNetwork:
        return "RegisteredHomeNetwork";
    case CellularNetwork::SearchingNetwork:
        return "SearchingNetwork";
    case CellularNetwork::RegistrationDenied:
        return "RegistrationDenied";
    case CellularNetwork::Unknown:
        return "Unknown";
    case CellularNetwork::RegisteredRoaming:
        return "RegisteredRoaming";
    case CellularNetwork::RegisteredSMSOnlyHome:
        return "RegisteredSMSOnlyHome";
    case CellularNetwork::RegisteredSMSOnlyRoaming:
        return "RegisteredSMSOnlyRoaming";
    case CellularNetwork::AttachedEmergencyOnly:
        return "AttachedEmergencyOnly";
    case CellularNetwork::RegisteredCSFBNotPreferredHome:
        return "RegisteredCSFBNotPreferredHome";
    case CellularNetwork::RegisteredCSFBNotPreferredRoaming:
        return "RegisteredCSFBNotPreferredRoaming";
    case CellularNetwork::AlreadyRegistered:
        return "AlreadyRegistered";
    default:
        return "Unknown error";
    }
}

static void format_nw_operator(char *buf, CellularNetwork::operator_t *nw_operator_ptr)
{
    sprintf(buf, "%s/%s/%s - %s (%s)", nw_operator_ptr->op_long, nw_operator_ptr->op_short, nw_operator_ptr->op_num,
        get_radio_access_technology_name(nw_operator_ptr->op_rat), get_operator_status_name(nw_operator_ptr->op_status));
}

static void format_reg_params(char *buf, CellularNetwork::registration_params_t *params_ptr)
{
    sprintf(buf, "type: %s; status: %s; technology: %s",
        get_reg_mode_name(params_ptr->_type),
        get_reg_status_name(params_ptr->_status),
        get_radio_access_technology_name(params_ptr->_act));
}

int main()
{
    // create driver
    SIM5320 sim5320(PA_2, PA_3);
    // reset and initialize device
    CHECK_RET_CODE(sim5320.reset());
    CHECK_RET_CODE(sim5320.init());

    printf("Start ...\n");
    CHECK_RET_CODE(sim5320.request_to_start());

    // show current network information
    const size_t buf_size = 256;
    char buf[buf_size];
    CellularNetwork::AttachStatus attach_status;
    CellularNetwork *cellular_network = sim5320.get_network();

    // set credential
    //CHECK_RET_CODE(sim5320.get_device()->set_pin("1234"));
    // try to attach to network
    printf("Attach to network ...\n");
    CHECK_RET_CODE(attach_to_network(&sim5320));

    printf("Network information:\n");

    // show network registration mode
    CellularNetwork::NWRegisteringMode reg_mode;
    CHECK_RET_CODE(cellular_network->get_network_registering_mode(reg_mode));
    printf("  - registration mode: %s\n", get_nw_registering_mode_name(reg_mode));

    // show attach status
    CHECK_RET_CODE(cellular_network->get_attach(attach_status));
    printf("  - attach status: %s\n", get_attach_status_name(attach_status));

    // show context status
    bool context_status = cellular_network->is_active_context();
    printf("  - context: %s\n", context_status ? "activated" : "not activated");

    // show registration parameters (CREG)
    CellularNetwork::registration_params_t reg_param;
    CHECK_RET_CODE(cellular_network->get_registration_params(CellularNetwork::C_GREG, reg_param));
    format_reg_params(buf, &reg_param);
    printf("  - registration params (CGREG): %s\n", buf);

    // show signal quality
    int signal_rssi = -1;
    int signal_ber = -1;
    CHECK_RET_CODE(cellular_network->get_signal_quality(signal_rssi, &signal_ber));
    printf("  - signal rssi: %i, ber: %i\n", signal_rssi, signal_ber);

    // get list available operators
    CellularNetwork::operList_t nw_operator_list;
    CellularNetwork::operator_t *nw_operator_ptr;
    int nw_operator_n;
    CHECK_RET_CODE(cellular_network->scan_plmn(nw_operator_list, nw_operator_n));
    nw_operator_ptr = nw_operator_list.get_head();
    printf("  - operators:\n");
    while (nw_operator_ptr->next) {
        format_nw_operator(buf, nw_operator_ptr);
        printf("    - %s\n", buf);
        nw_operator_ptr = nw_operator_ptr->next;
    }

    // show operator parameters
    CellularNetwork::operator_names_list nw_operator_name_list;
    CellularNetwork::operator_names_t *nw_operator_name_ptr;
    CHECK_RET_CODE(cellular_network->get_operator_names(nw_operator_name_list));
    nw_operator_name_ptr = nw_operator_name_list.get_head();
    printf("  - operator names:\n");
    while (nw_operator_name_ptr->next) {
        printf("    - %s/%s\n", nw_operator_name_ptr->alpha, nw_operator_name_ptr->numeric);
        nw_operator_name_ptr = nw_operator_name_ptr->next;
    }

    // detach from network and stop modem
    CHECK_RET_CODE(detach_from_network(&sim5320));
    printf("Stop ...\n");
    CHECK_RET_CODE(sim5320.request_to_stop());
    printf("Complete!\n");

    while (1) {
        wait(0.5);
        led = !led;
    }
}
