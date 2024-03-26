SRC_URI += "file://user_2020-08-17-13-03-00.cfg \
            file://user_2020-08-17-14-09-00.cfg \
            file://user_2020-08-17-17-27-00.cfg \
            file://user_2020-08-17-19-45-00.cfg \
            file://user_2020-08-20-08-57-00.cfg \
            file://user_2020-08-29-17-12-00.cfg \
            file://user_2021-03-29-08-45-00.cfg \
            file://user_2022-11-25-11-20-00.cfg \
            "

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI += "file://0001-Don-t-re-add-CPU-to-Linux-just-stop-it.patch"

