#include <unistd.h>
#include <cassert>
#include <sstream>
#include <string>
#include <random>

#include <sys/poll.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>

#include <cairomm/cairomm.h>

#include <rpumsg.h>
#include "display_backend_messages.h"

#include "cairo_demo.h"


static Cairo::RefPtr<Cairo::ImageSurface> create_surface(void *data)
{
    int xres = 256;
    int yres = 64;
    int bpp = 16;
    auto stride = xres * bpp/8;
    assert(stride == Cairo::ImageSurface::format_stride_for_width(Cairo::Format::FORMAT_RGB16_565, xres));

    auto surface = Cairo::ImageSurface::create(
        reinterpret_cast<unsigned char *>(data),
        Cairo::Format::FORMAT_RGB16_565,
        xres,
        yres,
        stride
    );

    return surface;
}


static void draw_cpu_bar(Cairo::RefPtr<Cairo::Context> cr, int ypos, int ncpu, double load)
{
    cr->set_source_rgb(1, 1, 1);
    cr->move_to(2, ypos);
    cr->show_text(std::string("CPU") + std::to_string(ncpu));

    double barx = 35.5;
    double bary = ypos-8-0.5;
    double barw = 180;
    double barh = 8;

    //cr->set_source_rgb(0.6, 0.6, 0.6);
    auto grad = Cairo::LinearGradient::create(barx, bary, barx+barw, bary);
    grad->add_color_stop_rgb(0.0, 0.32, 0.32, 0.32);
    grad->add_color_stop_rgb(1.0, 1.0, 1.0, 1.0);
    cr->set_source(grad);

    cr->rectangle(barx, bary, std::floor(barw*load)+0.5, barh);
    cr->fill();

    cr->set_line_width(1);
    cr->set_source_rgb(1.0, 1.0, 1.0);
    cr->rectangle(barx, bary, barw, barh);
    cr->stroke();

    std::ostringstream str_load;
    str_load.precision(2);
    str_load << std::fixed << load;
    cr->move_to(226, ypos);
    cr->show_text(str_load.str());
}


static void draw_frame(Cairo::RefPtr<Cairo::Context> &cr)
{
    cr->set_source_rgb(0, 0, 0);
    cr->paint();

    cr->set_source_rgb(1, 1, 1);
    cr->move_to(2, 14);
    cr->show_text("Model: test_model");

    cr->set_source_rgb(1, 1, 1);
    cr->move_to(186, 14);
    cr->show_text("Step: 10 µs");

    static std::normal_distribution<double> rnd(0.0, 0.01);
    static std::default_random_engine re;

    static double l1 = 0.4;
    static double l2 = 0.0;
    static double l3 = 0.7;

    l1 += rnd(re);
    l2 += rnd(re);
    l3 += rnd(re);

    l1 = std::max(0.0, std::min(l1, 1.0));
    l2 = std::max(0.0, std::min(l2, 1.0));
    l3 = std::max(0.0, std::min(l3, 1.0));

    draw_cpu_bar(cr, 32, 1, l1);
    draw_cpu_bar(cr, 46, 2, l2);
    draw_cpu_bar(cr, 60, 3, l3);
}


static uint8_t rgb565_to_g16(uint16_t pixel)
{
        uint16_t b = pixel & 0x1f;
        uint16_t g = (pixel & (0x3f << 5)) >> 5;
        uint16_t r = (pixel & (0x1f << (5 + 6))) >> (5 + 6);

        pixel = (299 * r + 587 * g + 114 * b) / 195;
        if (pixel > 255)
                pixel = 255;

        return (uint8_t)pixel / 16;
}



static void update_display(void *disp_buffer)
{
    uint16_t *buf16 = reinterpret_cast<uint16_t *>(disp_buffer);

    static uint8_t buffer[4 + sizeof(struct disp_back_msg_update_param) + 8192];
    uint8_t *ptr = &buffer[0];

    uint32_t *msg_id = (uint32_t *)ptr;
    ptr += 4;

    struct disp_back_msg_update_param *par = (struct disp_back_msg_update_param *)ptr;
    ptr += sizeof(struct disp_back_msg_update_param);

    uint8_t *data = (uint8_t *)ptr;

    *msg_id = DISP_BACK_MSG_UPDATE;

    par->bytes_per_row = 128;
    par->top = 0;
    par->left = 0;
    par->bottom = 63;
    par->right = 255;

    for (int i = 0; i < 8192; ++i)
    {
        uint8_t ph = rgb565_to_g16(buf16[i*2 + 0]);
        uint8_t pl = rgb565_to_g16(buf16[i*2 + 1]);

        data[i] = (ph << 4) | pl;
    }

    rpumsg_post_message(&buffer[0], sizeof(buffer));
    rpumsg_notify_remote();
}


static int handle_input(struct input_event &ev)
{
    static int display_on = 1;
    static int display_contrast = 15;

    if (ev.type == EV_KEY && ev.value > 0)
    {
        if (ev.code == KEY_BACKSPACE)
        {
            return -1;
        }
        else if (ev.code == KEY_ENTER)
        {
            display_on = !display_on;

            uint32_t msg_id;
            msg_id = display_on ? DISP_BACK_MSG_ON : DISP_BACK_MSG_OFF;
            rpumsg_post_message(&msg_id, 4);
            rpumsg_notify_remote();
        }
        else if (ev.code == KEY_UP || ev.code == KEY_DOWN)
        {
            if (ev.code == KEY_UP)
                display_contrast = std::min(15, display_contrast + 1);
            else
                display_contrast = std::max(0, display_contrast - 1);

            static uint8_t buffer[4 + sizeof(struct disp_back_msg_contrast_param)];

            uint32_t *msg_id = (uint32_t *)&buffer[0];
            struct disp_back_msg_contrast_param *par = (struct disp_back_msg_contrast_param *)&buffer[4];

            *msg_id = DISP_BACK_MSG_CONTRAST;
            par->contrast = display_contrast;
            rpumsg_post_message(&buffer[0], sizeof(buffer));
            rpumsg_notify_remote();
        }
    }

    return 0;
}


void do_demo(void *displ_buffer, int buttons_fd)
{
    auto surface = create_surface(displ_buffer);
    auto cr = Cairo::Context::create(surface);

    auto font = Cairo::ToyFontFace::create(
        "terminus",
        Cairo::FONT_SLANT_NORMAL,
        Cairo::FONT_WEIGHT_NORMAL
    );
    cr->set_font_face(font);
    cr->set_font_size(12.0);

    struct pollfd poll_fds[1];
    poll_fds[0].fd = buttons_fd;
    poll_fds[0].events = POLLIN;

    while (true)
    {
        draw_frame(cr);
        update_display(displ_buffer);

        if (poll(poll_fds, 1, 100) > 0)
        {
            if (poll_fds[0].revents | POLLIN)
            {
                struct input_event ev;
                if (read(buttons_fd, &ev, sizeof(struct input_event)) > 0)
                    if (handle_input(ev) < 0)
                        break;
            }
        }
    }

    cr->set_source_rgb(0, 0, 0);
    cr->paint();
    update_display(displ_buffer);

    usleep(1000);

    uint32_t msg_id;
    msg_id = DISP_BACK_MSG_SHUTDOWN;
    rpumsg_post_message(&msg_id, 4);
    rpumsg_notify_remote();

    usleep(1000);
}
