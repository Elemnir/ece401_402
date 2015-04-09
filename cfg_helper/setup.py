__author__ = 'Jeremy Rogers'

import configparser
import sys

config = configparser.ConfigParser()
config.optionxform = str

filename = input('Enter the name of the config file (default = \'tracker.conf\'): ')

if filename == '':
    filename = 'tracker.conf'

peername = input('Enter the name of this peer: ')
if peername == '':
    print('No peername given, exiting')
    sys.exit(1)

domain = input('Enter the tracker domain: ')

if domain == '':
    print('No domain given, exiting')
    sys.exit(1)

username = input('Enter the username for the tracker: ')

if username == '':
    print('No username given, exiting')
    sys.exit(1)

port = input('Enter the port for the daemon (default = \'auto\'): ')

if port == '':
    port = 'auto'

scanrate = input('How often do you want to scan for changes (in seconds)? (default = 60)')

if scanrate == '':
    scanrate = '60'

config['daemon'] = {'PeerName': peername,
                    'PeerID': '',
                    'ScanRate': scanrate}
config['network'] = {'TrackerDomain': domain,
                     'TrackerUsername': username,
                     'DaemonPort': port}

while True:
    directoryID = input('Enter a directory identifier, or enter nothing to exit: ')
    if directoryID == '':
        break

    prompt2 = str('Enter the directory location for ' + directoryID + ' (default = ~/' + directoryID + '): ')
    path = input(prompt2)

    if path == '':
        path = str('~/' + directoryID)

    config[directoryID] = {'DirectoryPath': path,
                           'ID': ''}


with open(filename, 'w') as configfile:
    config.write(configfile)
