######################################################################
# Communi
######################################################################

QT = core network gui testlib
CONFIG += communi
COMMUNI += core model util
CONFIG += testcase install_name no_testcase_installs
CONFIG -= app_bundle

include(../shared.pri)
