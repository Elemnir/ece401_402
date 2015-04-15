from openbdr.models             import Peer, Share, Account
from django.contrib.auth.models import User
from django.core.files.base     import ContentFile
from django.core.urlresolvers   import reverse
from django.test                import TestCase, Client
from urllib                     import quote, unquote

VALID_PEER_ID_1 = '12345678901234567890'
VALID_PEER_ID_2 = '1234abcde01234567890'
INVALID_PEER_ID = 'abcdefghijklmnopqrst'

SHARE_INFO_HASH = u'qw#$&%uiopas&*^hjklz'
OUTDATED_HASH_1 = 'asdfasdfasdfasdfasdf'

#SHARE_INFO_HASH = u'\xe2\x16N\xc23&\x17\x9ccxxd\xdf\x05\xfdm\xd6\xbf\x81\xa4'

class TrackerTest(TestCase):
    def setUp(self):
        u  = User.objects.create_user('username', 'user@name.com', 'password')
        p1 = Peer.objects.create(peer_name='jeff', peer_id=VALID_PEER_ID_1, 
                peer_ip='1.1.1.1', peer_port='12345')
        sh = Share.objects.create(share_name='hello', info_hash=SHARE_INFO_HASH.upper().encode('utf-8').encode('hex'),
                share_owner=Account.objects.get(user=u))
        sh.save()
        sh.peer_list.add(p1)
        sh.save()

    def test(self):
        num_tests = 4
        c = Client()
        print "\nBeginning tests for \"/tracker/\""

        # Test for single peer
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
        self.assertTrue(VALID_PEER_ID_1 in r.content)
        print "Passed 1/{}: Test for single peer".format(num_tests)
        
        # Test for two peers
        p2 = Peer.objects.create(peer_name='bill', peer_id=VALID_PEER_ID_2, 
                peer_ip='2.2.2.2', peer_port='67890')
        sh = Share.objects.first()
        sh.peer_list.add(p2)

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
        self.assertTrue(VALID_PEER_ID_1 in r.content)
        self.assertTrue(VALID_PEER_ID_2 in r.content)
        print "Passed 2/{}: Test for two peers".format(num_tests)

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
        print "Passed 3/{}: Test for disallowed Peer Id".format(num_tests)

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
        print "Passed 4/{}: Test for unknown Info Hash".format(num_tests)


class ReadShareTest(TestCase):
    def setUp(self):
        u  = User.objects.create_user('username', 'user@name.com', 'password')
        p1 = Peer.objects.create(peer_name='jeff', peer_id=VALID_PEER_ID_1, 
                peer_ip='1.1.1.1', peer_port='12345')
        sh = Share.objects.create(share_name='hello', info_hash=SHARE_INFO_HASH.upper().encode('utf-8').encode('hex'),
                share_owner=Account.objects.get(user=u))
        sh.share_file.save('test.torrent', ContentFile('Test Content')) 
        sh.save()
        sh.peer_list.add(p1)
        sh.save()
        s2 = Share.objects.create(share_name='blah', info_hash=OUTDATED_HASH_1,
                share_owner=Account.objects.get(user=u))
        s2.peer_list.add(p1)
        s2.save()

    def test(self):
        num_tests = 7
        c = Client()
        sh = Share.objects.first()
        print "\nBeginning tests for \"/read_share/\""

        # Properly formatted request with Not Modified response
        r = c.get(reverse('openbdr_read_share'), {
                    'share_id'  : sh.pk,
                    'info_hash' : SHARE_INFO_HASH,
                    'peer_id'   : VALID_PEER_ID_1,
                })
        self.assertEquals(r.status_code, 304)
        print "Passed 1/{}: Proper format: Same Info Hash".format(num_tests)

        # Properly formatted request with differing info hash
        r = c.get(reverse('openbdr_read_share'), {
                    'share_id'  : sh.pk,
                    'info_hash' : OUTDATED_HASH_1,
                    'peer_id'   : VALID_PEER_ID_1,
                })
        self.assertEquals(r.status_code, 200)
        self.assertEquals(r.content, sh.share_file.read())
        print "Passed 2/{}: Proper format: Differing Info Hash".format(num_tests)

        # Properly formatted request with invalid peer id
        r = c.get(reverse('openbdr_read_share'), {
                    'share_id'  : sh.pk,
                    'info_hash' : OUTDATED_HASH_1,
                    'peer_id'   : INVALID_PEER_ID,
                })
        self.assertEquals(r.status_code, 404)
        print "Passed 3/{}: Proper format: Invalid Peer ID".format(num_tests)

        # Improperly formatted request with no share id
        r = c.get(reverse('openbdr_read_share'), {
                    'info_hash' : OUTDATED_HASH_1,
                    'peer_id'   : VALID_PEER_ID_1,
                })
        self.assertEquals(r.status_code, 404)
        print "Passed 4/{}: Improper format: No Share ID".format(num_tests)

        # Improperly formatted request with no peer id
        r = c.get(reverse('openbdr_read_share'), {
                    'share_id'  : sh.pk,
                    'info_hash' : OUTDATED_HASH_1,
                })
        self.assertEquals(r.status_code, 404)
        print "Passed 5/{}: Improper format: No Peer ID".format(num_tests)

        # Properly formatted request with no info hash
        r = c.get(reverse('openbdr_read_share'), {
                    'share_id'  : sh.pk,
                    'peer_id'   : VALID_PEER_ID_1,
                })
        self.assertEquals(r.status_code, 200)
        sh.share_file.open()
        self.assertEquals(r.content, sh.share_file.read())
        print "Passed 6/{}: Proper format: No Info Hash".format(num_tests)
        
        # Properly formatted request with no file field
        s2 = Share.objects.get(share_name="blah")
        r = c.get(reverse('openbdr_read_share'), {
                    'share_id'  : s2.pk,
                    'info_hash' : SHARE_INFO_HASH,
                    'peer_id'   : VALID_PEER_ID_1,
                })
        self.assertEquals(r.status_code, 204)
        print "Passed 7/{}: Proper format: No Share File".format(num_tests)


class UpdateShareTest(TestCase):
    def setUp(self):
        u  = User.objects.create_user('username', 'user@name.com', 'password')
        p1 = Peer.objects.create(peer_name='jeff', peer_id=VALID_PEER_ID_1, 
                peer_ip='1.1.1.1', peer_port='12345')
        sh = Share.objects.create(share_name='hello', info_hash=OUTDATED_HASH_1,
                share_owner=Account.objects.get(user=u))
        sh.share_file.save('test.torrent', ContentFile('Test Content')) 
        sh.save()
        sh.peer_list.add(p1)
        sh.save()

    def test(self):
        num_tests = 5
        c = Client()
        sh = Share.objects.first()
        print "\nBeginning tests for \"/update_share/\""

        # Properly formatted update with authorized peer
        with open('test.torrent','w') as f:
            f.write('Different Content')
        
        with open('test.torrent','r') as f:
            r = c.post(reverse('openbdr_update_share'), {
                        'share_id'  : sh.pk,
                        'info_hash' : SHARE_INFO_HASH,
                        'peer_id'   : VALID_PEER_ID_1,
                        'share_file': f
                    })
        self.assertEquals(r.status_code, 200)
        print "Passed 1/{}: Proper format: Valid peer".format(num_tests)

        # Properly formatted update with unauthorized peer
        with open('test.torrent','w') as f:
            f.write('Different Content')
        
        with open('test.torrent','r') as f:
            r = c.post(reverse('openbdr_update_share'), {
                        'share_id'  : sh.pk,
                        'info_hash' : SHARE_INFO_HASH,
                        'peer_id'   : INVALID_PEER_ID,
                        'share_file': f
                    })
        self.assertEquals(r.status_code, 404)
        print "Passed 2/{}: Proper format: Invalid peer".format(num_tests)

        # Invalid request method
        r = c.get(reverse('openbdr_update_share'), {
                        'share_id'  : sh.pk,
                        'info_hash' : SHARE_INFO_HASH,
                        'peer_id'   : VALID_PEER_ID_1,
                    })
        self.assertEquals(r.status_code, 400)
        print "Passed 3/{}: Invalid request method".format(num_tests)

        # Imroperly formatted update with no file attached
        r = c.post(reverse('openbdr_update_share'), {
                    'share_id'  : sh.pk,
                    'info_hash' : SHARE_INFO_HASH,
                    'peer_id'   : VALID_PEER_ID_1
                })
        self.assertEquals(r.status_code, 400)
        print "Passed 4/{}: Improper format: No file uploaded".format(num_tests)

        # Improperly formatted update with unauthorized peer
        with open('test.torrent','w') as f:
            f.write('Different Content')
        
        with open('test.torrent','r') as f:
            r = c.post(reverse('openbdr_update_share'), {
                        'share_id'  : sh.pk,
                        'peer_id'   : INVALID_PEER_ID,
                        'share_file': f
                    })
        self.assertEquals(r.status_code, 404)
        print "Passed 5/{}: Improper format: No Info Hash and Invalid peer".format(num_tests)


