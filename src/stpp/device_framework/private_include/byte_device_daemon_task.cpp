#include "byte_device_daemon_task.hpp"
#include "../byte_device.hpp"
#include <FreeRTOS.h>
#include <task.h>

void stpp::device_framework_internal::ByteDeviceDaemon(void *argument)
{
    device::ByteDevice *device = static_cast<device::ByteDevice *>(argument);

    device->Spin();
    vTaskDelete(nullptr);
}
