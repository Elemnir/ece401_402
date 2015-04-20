__author__ = 'Jeremy Rogers'

import ConfigParser
import sys
import os
import commtracker
import getpass


def setdomain(conf):
    domain = input('Tracker Domain: ')
    if 'network' not in conf:
        conf['network'] = {}
    conf['network']['TrackerDomain'] = domain


def setuser(conf):
    username = input('Tracker Username: ')
    if 'network' not in conf:
        conf['network'] = {}
    conf['network']['TrackerUsername'] = username


def setrate(conf):
    rate = input('Scanning Rate: ')
    if 'daemon' not in conf:
        conf['daemon'] = {}
    conf['daemon']['ScanRate'] = rate


def setport(conf):
    port = input('Tracker Port: ')
    if 'network' not in conf:
        conf['network'] = {}
    conf['network']['DaemonPort'] = port


def registeruser(conf):
    username = conf.get('network', 'TrackerUsername', fallback='')
    if username == '':
        print('No username in config file. Run \'python3 openbdr_config.py setuser\' to set one.')
        sys.exit(1)

    domain = conf.get('network', 'DomainName', fallback='')
    if domain == '':
        print('No domain name in config file. Run \'python3 openbdr_config.py\' setdomain to set one.')
        sys.exit(1)

    ct = commtracker.TrackerComm(domain)

    password = getpass.getpass(prompt='Password for {0} on {1}: '.format(username, domain))
    password_confirm = getpass.getpass(prompt='Confirm password: ')

    if password != password_confirm:
        print('Password mismatch')
        sys.exit(1)

    try:
        ct.register_user(username, password)
    except commtracker.CommFailure as e:
        print(e.__str__)
        raise

    return password


def setpeer(conf):
    peername = input('Enter the name of this peer: ')
    if 'daemon' not in conf:
        conf['daemon'] = {}
    conf['daemon']['PeerName'] = peername


def registerpeer(conf, password):
    peername = conf.get('daemon', 'PeerName', fallback='')
    if peername == '':
        print('No peer name in config file. Run \'python3 openbdr_config.py setpeer\' to set one.')
        sys.exit(1)

    domain = conf.get('network', 'TrackerDomain', fallback='')
    if domain == '':
        print('No domain name in config file. Run \'python3 openbdr_config.py setdomain\' to set one.')
        sys.exit(1)

    username = conf.get('network', 'TrackerUsername', fallback='')

    if username == '':
        print('No Username specified for the tracker. Run \'python3 openbdr_config.py setusername\' to set one.')
        sys.exit(1)

    ct = commtracker.TrackerComm(domain)

    try:
        pid = ct.add_peer(username, password, peername)
    except commtracker.CommFailure as e:
        print(e.__str__)
        raise

    if 'daemon' not in conf:
        conf['daemon'] = {}
    conf['daemon']['PeerID'] = pid


def addshare(conf, password):
    domain = conf.get('network', 'TrackerDomain', fallback='')

    if domain == '':
        print('No domain name specified for the tracker. Run \'python3 openbdr_config.py setdomain\' to set one.')

    username = conf.get('network', 'TrackerUsername', fallback='')

    if username == '':
        print('No username specified for the tracker. Run \'python3 openbdr_config.py setuser\' to set one.')

    peer_id = conf.get('daemon', 'PeerID', fallback='')
    if peer_id == '':
        print('The peer is unregistered to the tracker. Run \'python3 openbdr_config.py registerpeer\' to register.')

    ct = commtracker.TrackerComm(domain)

    directoryid = input('Directory identifier: ')
    directorypath = input('Directory Path: ')

    try:
        share_id = ct.add_share(username, password, directoryid)
        ct.add_peer_to_share(username, password, peer_id, share_id)
    except commtracker.CommFailure as e:
        print(e.__str__)
        raise

    if directoryid not in conf:
        conf[directoryid] = {}
    conf[directoryid]['DirectoryPath'] = directorypath
    conf[directoryid]['ID'] = share_id


def setup(conf):
    setdomain(conf)
    setport(conf)
    setuser(conf)
    setpeer(conf)
    setrate(conf)
    pword = registeruser(conf)
    registerpeer(conf, pword)
    print('Setup complete')
    print('Add shares with \'python3 openbdr_config.py addshare\'')


config = ConfigParser.ConfigParser()

if not os.path.exists('~/.btfs'):
    os.makedirs('~/.btfs')

if not os.path.isdir('~/.btfs'):
    print('~/.btfs is a file, and not a directory. Delete this file and start over')
    sys.exit(1)

if os.path.exists('~/.btfs/openbdr.conf'):
    config.read('~/.btfs/openbdr.conf')

if len(sys.argv) != 2:
    print('usage: python3 openbdr_config.py (setup/setuser/setdomain/setrate/setpeer/setport/registeruser/registerpeer/'
          'addshare)')
    sys.exit(1)

valid = False

if sys.argv[1] == 'setup':
    setup(config)
    valid = True

if sys.argv[1] == 'setuser':
    setuser(config)
    valid = True

if sys.argv[1] == 'setdomain':
    setdomain(config)
    valid = True

if sys.argv[1] == 'setrate':
    setrate(config)
    valid = True

if sys.argv[1] == 'setpeer':
    setpeer(config)
    valid = True

if sys.argv[1] == 'setport':
    setport(config)
    valid = True

if sys.argv[1] == 'registeruser':
    registeruser(config)
    valid = True

if sys.argv[1] == 'registerpeer':
    pw = getpass.getpass(prompt='Tracker Password: ')
    registerpeer(config, pw)
    valid = True

if sys.argv[1] == 'addshare':
    pw = getpass.getpass(prompt='Tracker Password: ')
    addshare(config, pw)
    valid = True

if not valid:
    print('usage: python3 openbdr_config.py (setup/setuser/setdomain/setrate/setpeer/setport/registeruser/registerpeer/'
          'addshare)')
    sys.exit(1)

with open('~/.btfs/openbdr.conf', 'w') as f:
    config.write(f)