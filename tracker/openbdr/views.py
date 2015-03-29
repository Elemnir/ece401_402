from django.shortcuts               import render, get_object_or_404
from django.http                    import (Http404, HttpResponse, 
                                            HttpResponseNotModified)
from django.contrib.auth.decorators import login_required
from bencode                        import bencode
from openbdr.models                 import Account, Peer, Share, ShareHist
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

def read_share(request):
    """Access point for Utilities to check for updates to the share file"""
    sh = get_object_or_404(ShareHist, info_hash=request.GET.get('info_hash',''))
    share = sh.share
    peer = get_object_or_404(Peer, peer_id=request.GET.get('peer_id',''))

    if peer not in share.peer_list:
        raise Http404()

    if share.info_hash == sh.info_hash:
        return HttpResponseNotModified()

    return HttpResponse(share.phile, content_type="text/plain")

@csrf_exempt
def update_share(request):
    """Access point for Utilities to update the share"""
    if request.method != 'POST':
        raise Http404()

    
