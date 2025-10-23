import configparser
from shutil import copyfile
from os.path import basename
import sys
import datetime
import os
Import("env")

DESTINY = "\\\\lumrasp8\\srv\\firmware\\"

def set_value_in_property_file(file_path, section, key, value):
    config = configparser.RawConfigParser()
    config.read(file_path)
    if (not config.has_section(section)):
        config.add_section(section)

    config.set(section, key, value)
    cfgfile = open(file_path, 'w')
    # use flag in case case you need to avoid white space.
    config.write(cfgfile, space_around_delimiters=False)
    cfgfile.close()



def publish_firmware(source, target, env):
    firmware_path = str(source[0])
    firmware_name = basename(firmware_path)

    VERSION = env.GetProjectOption("custom_version")
    VERSION = VERSION[0:VERSION.rindex('.')+1]

    VERSION_FILE = '.version'

    filter = env.GetProjectOption("src_filter")[0][2:-1]
    srcdir = env.get("PROJECTSRC_DIR")+os.sep + filter+os.sep

    if os.path.exists(srcdir+VERSION_FILE):
        FILE = open(srcdir+VERSION_FILE)
        VERSION_PATCH_NUMBER = FILE.readline()
        VERSION_PATCH_NUMBER = int(VERSION_PATCH_NUMBER)
    else:
        VERSION_PATCH_NUMBER = 0

    VERSION = VERSION + str(VERSION_PATCH_NUMBER)

    print("Uploading {0}. Version: {1}".format(firmware_name, VERSION))
    copyfile(firmware_path, DESTINY + firmware_name)

    package = env.GetProjectOption("custom_name")
    set_value_in_property_file(DESTINY+"firmware.ini", package, 'file', firmware_name)
    set_value_in_property_file(DESTINY+"firmware.ini", package, 'version', VERSION)
    set_value_in_property_file(DESTINY+"firmware.ini", package, 'date',datetime.datetime.now())
    
    print("The firmware has been successfuly copied!")


# Custom upload command and program name
env.Replace(UPLOADCMD=publish_firmware)
