SRC_URI += "file://user_2018-08-21-09-30-00.cfg \
            file://user_2018-08-22-08-02-00.cfg \
            file://user_2018-09-05-12-33-00.cfg \
            file://user_2018-09-05-13-55-00.cfg \
            file://user_2019-03-05-15-53-00.cfg \
            file://user_2019-03-05-16-38-00.cfg \
            file://user_2019-03-06-10-59-00.cfg \
            file://user_2019-03-06-11-21-00.cfg \
            file://user_2019-03-06-12-15-00.cfg \
            file://user_2019-03-07-13-54-00.cfg \
            file://user_2019-03-13-13-48-00.cfg \
            file://user_2019-03-16-21-46-00.cfg \
            file://user_2019-03-18-16-27-00.cfg \
            file://user_2019-03-26-10-16-00.cfg \
            file://user_2019-03-26-12-20-00.cfg \
            file://user_2019-03-26-12-52-00.cfg \
            file://user_2019-04-02-15-59-00.cfg \
            file://user_2019-04-02-16-07-00.cfg \
            file://user_2019-06-06-12-27-00.cfg \
            file://user_2019-06-20-09-21-00.cfg \
            file://user_2019-07-01-11-39-00.cfg \
            file://user_2019-07-25-11-58-00.cfg \
            file://user_2019-08-15-11-21-00.cfg \
            file://user_2019-08-20-14-42-00.cfg \
            file://user_2019-08-26-15-57-00.cfg \
            file://user_2019-08-28-14-47-00.cfg \
            file://user_2019-08-28-15-08-00.cfg \
            file://user_2019-08-28-16-06-00.cfg \
            file://user_2019-08-28-16-11-00.cfg \
            file://user_2019-08-28-16-34-00.cfg \
            file://user_2019-08-29-13-58-00.cfg \
            file://user_2019-08-29-15-00-00.cfg \
            file://user_2019-11-05-10-58-00.cfg \
            file://user_2020-01-23-08-38-00.cfg \
            file://user_2020-02-11-15-08-00.cfg \
            file://user_2020-04-14-15-14-00.cfg \
            file://user_2020-09-14-14-49-00.cfg \
            "

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI += "file://cadence_mdio1.patch"
SRC_URI += "file://cadence_mdio2.patch"
SRC_URI += "file://0001-Fix-spidev-compatibility.patch"
SRC_URI += "file://0001-Allow-normal-access-to-mapped-PCI-shmem.patch"

