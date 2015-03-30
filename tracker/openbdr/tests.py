from openbdr.models             import Peer, Share, Account
from django.contrib.auth.models import User
from django.core.files.base     import ContentFile
from django.core.urlresolvers   import reverse
from django.test                import TestCase, Client
from bencode                    import bencode

VALID_PEER_ID_1 = '12345678901234567890'
VALID_PEER_ID_2 = '1234abcde01234567890'
INVALID_PEER_ID = 'abcdefghijklmnopqrst'
SHARE_INFO_HASH = 'qwertyuiopasdfghjklz'

class TrackerTest(TestCase):
    def setUp(self):
        u  = User.objects.create_user('username', 'user@name.com', 'password')
        p1 = Peer.objects.create(peer_name='jeff', peer_id=VALID_PEER_ID_1, 
                peer_ip='1.1.1.1', peer_port='12345')
        sh = Share.objects.create(share_name='hello', info_hash=SHARE_INFO_HASH,
                share_owner=Account.objects.get(user=u))
        sh.save()
        sh.peer_list.add(p1)
        sh.save()

    def test(self):
        num_tests = 4
        c = Client()

        # Test for single peer content
        r = c.get(reverse('openbdr_tracker'), {
                    'info_hash' : SHARE_INFO_HASH,
                    'peer_id'   : VALID_PEER_ID_1,
                    'port'      : '12345',
                    'uploaded'  : '0',
                    'downloaded': '0',
                    'left'      : '0',
                    'ip'        : '1.1.1.1',
                })
        self.assertEquals(r.status_code, 200)
        self.assertTrue()
        print "Passed 1/{}".format(num_tests)
        
        # Test for two peer content
        p2 = Peer.objects.create(peer_name='bill', peer_id=VALID_PEER_ID_2, 
                peer_ip='2.2.2.2', peer_port='67890')
        Share.objects.get(pk=1).peer_list.add(p2)
        r = c.get(reverse('openbdr_tracker'), {
                    'info_hash' : SHARE_INFO_HASH,
                    'peer_id'   : VALID_PEER_ID_2,
                    'port'      : '67890',
                    'uploaded'  : '0',
                    'downloaded': '0',
                    'left'      : '0',
                    'ip'        : '2.2.2.2',
                })
        self.assertEquals(r.status_code, 200)
        print "Passed 2/{}".format(num_tests)

        # Test for disallowed peer id
        r = c.get(reverse('openbdr_tracker'), {
                    'info_hash' : SHARE_INFO_HASH,
                    'peer_id'   : INVALID_PEER_ID,
                    'port'      : '12345',
                    'uploaded'  : '0',
                    'downloaded': '0',
                    'left'      : '0',
                    'ip'        : '1.1.1.1',
                })
        self.assertTrue('failure reason' in r.content)
        print "Passed 3/{}".format(num_tests)

        # Test for unknown info_hash
        r = c.get(reverse('openbdr_tracker'), {
                    'info_hash' : 'asdf',
                    'peer_id'   : VALID_PEER_ID_2,
                    'port'      : '67890',
                    'uploaded'  : '0',
                    'downloaded': '0',
                    'left'      : '0',
                    'ip'        : '2.2.2.2',
                })
        self.assertTrue('failure reason' in r.content)
        print "Passed 4/{}".format(num_tests)
