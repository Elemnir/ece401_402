__author__ = 'Jeremy Rogers'

import configparser
import sys
import os
import commtracker
import getpass


class Directory:
    def __init__(self, name, path, id):
        self.directoryName = name
        self.directoryPath = path
        self.id = id


config = configparser.ConfigParser()

if len(sys.argv) > 2:
    print('usage: btfsConfig.py [filename]')
    sys.exit(1)
elif len(sys.argv) == 1:
    if os.path.isfile('tracker.conf'):
        config.read('tracker.conf')
        configfile = 'tracker.conf'
    else:
        print('No file specified, and tracker.conf does not exist, exiting')
        sys.exit(1)
elif len(sys.argv) == 2:
    if os.path.isfile(sys.argv[1]):
        config.read(sys.argv[1])
        configfile = sys.argv[1]
    else:
        print(sys.argv[1])
        print('Given file not found')
        sys.exit(1)

if 'daemon' not in config:
    print('No daemon section in config file')
    sys.exit(1)

if 'network' not in config:
    print('No network section in config file')
    sys.exit(1)

peer_id = config.get('daemon', 'PeerID', fallback='')

peername = config.get('daemon', 'PeerName', fallback='')

if peername == '':
    peername = input('Enter a name for this peer: ')
    config['daemon']['PeerName'] = peername

domain = config.get('network', 'TrackerDomain', fallback='')

if domain == '':
    domain = input('Enter the name of the domain: ')
    config['network']['TrackerDomain'] = domain

username = config.get('network', 'TrackerUsername', fallback='')

if username == '':
    username = input('Tracker Username: ')

password = getpass.getpass(prompt='Password for {0} on {1}: '.format(username, domain))
password_confirm = getpass.getpass(prompt='Confirm password: ')

if password != password_confirm:
    print('Password mismatch')
    sys.exit(1)

port = config.get('network', 'DaemonPort', fallback='auto')

directories = []

for section in config.sections():
    if section != 'daemon' and section != 'network':
        name = section

        path = config.get(section, 'DirectoryPath', fallback='')

        if path == '':
            print('No directory path specified for', section)
            sys.exit(1)

        identifier = config.get(section, 'ID', fallback='')

        directories.append(Directory(name, path, identifier))

print('Read in config file')
print('There are', len(directories), 'directories to sync')
print('The tracker domain name is', domain, 'with username', username)
print('The port is currently set to', port)

for directory in directories:
    print(directory.directoryName, directory.directoryPath)

ct = commtracker.TrackerComm(domain)

reg = input('Is the user already registered? (Y/n)')

if reg == 'y' or reg == 'Y':
    try:
        ct.register_user(username, password)
    except commtracker.CommFailure:
        print('Error in registering user, exiting')
        sys.exit(1)

if peer_id == '':
    try:
        peer_id = ct.add_peer(username, password, peername)
    except commtracker.CommFailure:
        print('Error in adding a peer, exiting')
        sys.exit(1)

config['daemon']['PeerID'] = peer_id

with open(configfile, 'w') as file:
    config.write(file)

for directories in directories:
    if directory.id == '':
        try:
            share_id = ct.add_share(username, password, directory.directoryName)
        except commtracker.CommFailure:
            print('Error in adding share {0}, exiting'.format(directory.directoryName))
            sys.exit(1)

        config[directory.directoryName]['ID'] = share_id
        with open(configfile, 'w') as file:
            config.write(file)

        try:
            ct.add_peer_to_share(username, password, peer_id, share_id)
        except commtracker.CommFailure:
            print('Error adding peer {0} (id = {1}) to share {2} (id = {3})'.format(peername, peer_id,
                                                                                    directory.directoryName, share_id))

        with open(configfile, 'w') as file:
            config.write(file)