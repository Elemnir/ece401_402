from __future__     import print_function
from sys            import argv
from commtracker    import TrackerComm

t = TrackerComm("http://home.elemnir.com:8000")

user = 'test@test.com'
pswd = 'testpass'
share = 'test_share'
peer = 'test_peer'

if len(argv) >= 3:
    user = argv[1]
    pswd = argv[2]
    print("Registering user {}... ".format(user), end='')
    t.register_user(user, pswd)
    print("Done.")


print("Adding share {}... ".format(share), end='')
s_id = t.add_share(user, pswd, share) 
print("Done.")

print("Adding peer {}... ".format(peer), end='')
p_id = t.add_peer(user, pswd, peer) 
print("Done.")

print("Share_id: {} Peer_id: {}".format(s_id, p_id))

print("Adding peer to share... ", end='')
t.add_peer_to_share(user, pswd, p_id, s_id)
print("Done.")

raw_input("Hit Enter to run remove_peer_from_share().")
t.remove_peer_from_share(user, pswd, p_id, s_id)

raw_input("Hit Enter to run remove_peer().")
t.remove_peer(user, pswd, p_id)

raw_input("Hit Enter to run remove_share().")
t.remove_share(user, pswd, s_id)

print("Test passed")
