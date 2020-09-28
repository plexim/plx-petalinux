#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>

#include <QApplication>
#include <QHBoxLayout>
#include <QDebug>
#include <QLabel>
#include <QTimer>
#include <QLCDNumber>
#include <QSocketNotifier>
#include <QKeyEvent>

#include <metal/sys.h>

#include <rpumsg.h>
#include "display_backend_messages.h"


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


class ScreenWidget: public QWidget
{
public:
    ScreenWidget(QWidget *parent = nullptr)
    : QWidget(parent)
    {
        setFixedSize(256, 64);
        setStyleSheet("background-color:black;");
        show();

        auto *lcd = new QLCDNumber(this);
        lcd->show();
        lcd->setSegmentStyle(QLCDNumber::Flat);
        lcd->setDigitCount(5);

        QPalette pal = lcd->palette();
        pal.setColor(QPalette::WindowText, QColor(255, 255, 255));
        pal.setColor(QPalette::Background, QColor(0, 0, 0));
        lcd->setPalette(pal);

        auto *text1 = new QLabel("Test", this);
        text1->setStyleSheet("QLabel { background-color: black; color: white; font-family: \"Terminus\"}");
        text1->setAlignment(Qt::AlignCenter);

        auto *layout = new QHBoxLayout();
        setLayout(layout);
        layout->addWidget(lcd, 0, 0);
        layout->addWidget(text1);

        auto *timer = new QTimer(this);
        timer->setInterval(100);

        connect(timer, &QTimer::timeout, [lcd, text1] () {
            static int count = 0;
            QString text = QString::number(count++).rightJustified(lcd->digitCount(), '0');
            lcd->display(text);

            static int text_ds = +4;
            static int text_size = 12;
            auto font = text1->font();
            text_size += text_ds;
            if (text_size >= 32 || text_size <= 12) text_ds = -text_ds;
            font.setPixelSize(text_size);
            text1->setFont(font);
        });

        timer->start();
    }

protected:
    virtual void keyPressEvent(QKeyEvent *event) override
    {
        switch (event->key())
        {
        case Qt::Key_Backspace:
            exitApp();
            break;

        case Qt::Key_Return:
            toggleOnOff();
            break;

        case Qt::Key_Up:
            adjustContrast(+1);
            break;

        case Qt::Key_Down:
            adjustContrast(-1);
            break;
        }
    }

private:
    void exitApp()
    {
        for (auto w: findChildren<QWidget *>())
            w->hide();

        QTimer::singleShot(0, qApp, &QApplication::quit);
    }


    void toggleOnOff()
    {
        static bool display_on = true;

        display_on = !display_on;

        uint32_t msg_id;
        msg_id = display_on ? DISP_BACK_MSG_ON : DISP_BACK_MSG_OFF;
        rpumsg_post_message(&msg_id, 4);
        rpumsg_notify_remote();
    }


    void adjustContrast(int delta)
    {
        static int contrast = 15;

        contrast += delta;
        if (contrast < 0) contrast = 0;
        if (contrast > 15) contrast = 15;

        static uint8_t buffer[4 + sizeof(struct disp_back_msg_contrast_param)];

        uint32_t *msg_id = (uint32_t *)&buffer[0];
        struct disp_back_msg_contrast_param *par = (struct disp_back_msg_contrast_param *)&buffer[4];

        *msg_id = DISP_BACK_MSG_CONTRAST;
        par->contrast = contrast;
        rpumsg_post_message(&buffer[0], sizeof(buffer));
        rpumsg_notify_remote();
    }
};


int main(int argc, char *argv[])
{
    struct metal_init_params init_param = METAL_INIT_DEFAULTS;
    if (metal_init(&init_param) < 0)
    {
        qCritical() << "!!! Cannot initialize libmetal.\n";
        return -1;
    }

    atexit(metal_finish);

    struct rpumsg_params par = {
        .shm_dev_name = "3f000000.shm",
        .shm_msgbuf_offset = 0,
        .shm_msgbuf_size = 0x10000,
        .ipi_dev_name = "ff340000.ipi",
        .ipi_chn_remote = 1
    };

    if (rpumsg_initialize(&par) < 0)
    {
        qCritical() << "Cannot initialize rpumsg.\n";
        return -1;
    }


    QApplication app(argc, argv);


    if (start_rpu_firmware("display_cairo_test_r5.elf") < 0)
    {
        qCritical() << "Cannot start RPU firmware.\n";
        return -1;
    }

    std::cout << "Starting Qt demo; function of the RT Box buttons:\n"
              << "[Back]  : exit application;\n"
              << "[Enter] : turn display on/off;\n"
              << "[Up]    : increase contrast;\n"
              << "[Down]  : decrease contrast.\n";
    std::cout.flush();

    ScreenWidget screen;
    app.exec();

    usleep(100000);
    rpumsg_terminate();
    stop_rpu_firmware();
}
