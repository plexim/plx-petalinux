#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>
#include <memory>
#include <cstdint>
#include <string>

#include <metal/sys.h>
#include <rpumsg.h>

#include "cairo_demo.h"



static int start_rpu_firmware(const char *fname)
{
    std::string firmware_name(fname);
    auto firmware_path = std::string("/lib/firmware/") + firmware_name;

    if (access(firmware_path.c_str(), F_OK) < 0)
    {
        std::cerr << firmware_path << " not found.\n";
        return -1;
    }

    int fd;
    fd = open("/sys/class/remoteproc/remoteproc0/firmware", O_WRONLY|O_SYNC);
    write(fd, firmware_name.c_str(), firmware_name.length());
    close(fd);

    fd = open("/sys/class/remoteproc/remoteproc0/state", O_WRONLY|O_SYNC);
    std::string cmd("start");
    write(fd, cmd.c_str(), cmd.length());
    close(fd);

    usleep(10000);

    return 0;
}


static void stop_rpu_firmware()
{
    int fd = open("/sys/class/remoteproc/remoteproc0/state", O_WRONLY|O_SYNC);
    std::string cmd("stop");
    write(fd, cmd.c_str(), cmd.length());
    close(fd);
}


int main(int, char **)
{   
    struct metal_init_params init_param = METAL_INIT_DEFAULTS;
    if (metal_init(&init_param) < 0)
    {
        std::cerr << "Cannot initialize libmetal.\n";
        return -1;
    }

    struct rpumsg_params par = {
        .shm_dev_name = "3f000000.shm",
        .shm_msgbuf_offset = 0,
        .shm_msgbuf_size = 0x10000,
        .ipi_dev_name = "ff340000.ipi",
        .ipi_chn_remote = 1
    };
    if (rpumsg_initialize(&par) < 0)
    {
        std::cerr << "Cannot initialize rpumsg.\n";
        return -1;
    }

    int buttons_fd = open("/dev/input/by-id/display-buttons", O_RDONLY);
    if (buttons_fd < 0)
    {
        std::cerr << "Cannot open buttons input device.\n";
        return -1;
    }

    if (start_rpu_firmware("display_cairo_test_r5.elf") < 0)
    {
        std::cerr << "Cannot start RPU firmware.\n";
        return -1;
    }

    std::unique_ptr<unsigned char> displ_buffer;
    displ_buffer.reset(new unsigned char[32768]);


    std::cout << "Starting cairo demo; function of the RT Box buttons:\n"
              << "[Back]  : exit application;\n"
              << "[Enter] : turn display on/off;\n"
              << "[Up]    : increase contrast;\n"
              << "[Down]  : decrease contrast.\n";
    std::cout.flush();

    do_demo(displ_buffer.get(), buttons_fd);


    usleep(100000);
    stop_rpu_firmware();
    rpumsg_terminate();
    metal_finish();

    return 0;
}
