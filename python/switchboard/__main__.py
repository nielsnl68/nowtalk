import argparse  # https://www.w3schools.com/python/ref_module_argparse.asp
import atexit  # https://www.w3schools.com/python/ref_module_atexit.asp
import configparser  # https://www.w3schools.com/python/ref_module_configparser.asp
import os
from pprint import pprint  # https://www.w3schools.com/python/ref_module_pprint.asp


if os.name == 'nt':  # sys.platform == 'win32':
    from serial.tools.list_ports_windows import comports
elif os.name == 'posix':
    from serial.tools.list_ports_posix import comports


iterator = sorted(comports(include_links=False))
list = []
for n, (port, desc, hwid) in enumerate(iterator, 1):
  list.append(port)

config = {
    'commport': 'none',
    'baudrate': 921600,
    'database': './NowTalk.sqlite',
    'switchboardName': 'SwitchBoard',
    'callname': "computer",
    'dynamicExtIP': False,
    'externelIP' : "",
    'allowGuests': True,
    'webAddress': "*", #127.0.0.1
    'webPort': 1215,
    'allowNewDevice': True,
    'badgeTimeout': 60,
    'readsize': 0
}

def loadConfig(inifile) :
    global config
    try:
        if (inifile):
            ini = configparser.ConfigParser(defaults=config, allow_unnamed_section=True)
            ini.read(inifile)
            if (ini.has_section(configparser.UNNAMED_SECTION)):
                config = ini[configparser.UNNAMED_SECTION]
    except FileNotFoundError:
        print("The file does not exist.")


def bye():
    print("Goodbye!")
atexit.register(bye)

loadConfig('./NowTalk.ini')

parser = argparse.ArgumentParser(description='NowTalk Switchboard Tool', epilog="Copyright 2025, LumenSoft Nederland. ")
parser.add_argument('commport', type=str, help='The COM port to use (e.g., COM3 or /dev/ttyUSB0)', choices=list)
parser.add_argument('baudrate', type=int, help='Baud rate (921600)', nargs="?", default=config['baudrate'])

parser.add_argument('--ini', type=str, help='Alternate ini filename', metavar="...")

parser.add_argument('-n', '--name', type=str, help='Name of this switchboard', dest='switchboardName', metavar="...", default=config['switchboardName'])
parser.add_argument('-c', '--callname', type=str, help='Wake word to call command switchboard', metavar="...", default=config['callname'])
parser.add_argument('-t', '--timeout', type=str, help='Badge timeout in seconds.', dest='badgeTimeout', metavar="...", default=config['badgeTimeout'])
parser.add_argument('--db', type=str, help='Database filename', dest='database', metavar="...", default=config['database'])
parser.add_argument('--dynamicIP', action="store_true", help="Your internet provider has gives dynamic IP's", dest='dynamicExtIP', default=config['dynamicExtIP'])
parser.add_argument('--IP', type=str, help='Set your external IP address', dest='externelIP', metavar="...", default=config['externelIP'])
parser.add_argument('--webaddress', type=str, help='Set the webservers address', metavar="...", dest='webAddress', default=config['webAddress'])
parser.add_argument('--webport', type=str, help='Set the webservers port', metavar="...", dest='webPort', default=config['webPort'])
parser.add_argument('--readsize', type=int, help='internal read bufsize (default: 1024)', metavar="...", default=config['readsize'])

parser.add_argument('-s', '--silent', help="Don't print console messages on the screen.", action="store_true")
parser.add_argument('-w', '--write',  help='Write configuration settings to file', action="store_true")

parser.add_argument('-V','--version', action='version', version='%(prog)s 4.0')

args = parser.parse_args()
if (args.ini is not None):
    loadConfig(args.ini)
else:
    config = config | vars(args)


pprint(config)

if (args.commport == 'none'):
    parser.error ('No commport selected')
