from __future__     import print_function
from sys            import argv
from commtracker    import TrackerComm

t = TrackerComm("http://home.elemnir.com:8000")

user = 'test@test.com'
pswd = 'testpass'

if len(argv) >= 3:
    user = argv[1]
    pswd = argv[2]
    t.register_user(user, pswd)

s_id = t.add_share(user, pswd, 'test_share')    
p_id = t.add_peer(user, pswd, 'test_peer') 

print("Share_id: {} Peer_id: {}".format(s_id, p_id))

t.add_peer_to_share(user, pswd, p_id, s_id)

raw_input("Hit Enter to run remove_peer_from_share().")
t.remove_peer_from_share(user, pswd, p_id, s_id)

raw_input("Hit Enter to run remove_peer().")
t.remove_peer(user, pswd, p_id)

raw_input("Hit Enter to run remove_share().")
t.remove_share(user, pswd, s_id)

print("Test passed")
