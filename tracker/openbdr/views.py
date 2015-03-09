from django.shortcuts               import render
from django.http                    import Http404
from django.contrib.auth.decorators import login_required
from bencode                        import bencode
from openbdr.models                 import Account, Peer, Share
from openbdr.forms                  import PeerListRequestForm
@login_required
def profile(request):
    """Allow a user or the config tool to manage access permissions"""
    pass

def tracker(request):
    """Peer communication access point"""
    res = {}
    try:
        if request.method == 'GET':
            plrf = PeerListRequestForm(request.GET)
            if plrf.is_valid():
                qs = plrf.process()
                res['peers'] = [{
                    'peer id':i.peer_id, 
                    'ip':i.peer_ip, 
                    'port':i.peer_port
                    } for i in qs if i.is_online()
                ]
                res['interval'] = 55
            else:
                raise Http404()
    except Http404:
        res['failure reason'] = 'poorly formed request'

    resb = bencode(res)
    return HttpResponse(resb, content_type="text/plain")
