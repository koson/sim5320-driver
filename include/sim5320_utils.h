#ifndef SIM5320_UTILS_H
#define SIM5320_UTILS_H
#include "CellularLog.h"
#include "mbed.h"
namespace sim5320 {
/**
 * Helper function that to return first found error code.
 *
 * @param err_1
 * @param err_2
 * @return
 */
static nsapi_error_t any_error(nsapi_error_t err_1, nsapi_error_t err_2)
{
    if (err_1) {
        return err_1;
    }
    if (err_2) {
        return err_2;
    }
    return 0;
}

#define SIM5320_UNLOCK_RETURN_IF_ERROR(at_handler_ptr) \
    if (at_handler_ptr->get_last_error()) {            \
        return at_handler_ptr->unlock_return_error();  \
    }
}
#endif // SIM5320_UTILS_H
