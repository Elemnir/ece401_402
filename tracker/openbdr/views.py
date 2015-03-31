from django.shortcuts               import render, get_object_or_404
from django.http                    import (Http404, HttpResponse, 
                                            HttpResponseNotModified)
from django.views.decorators.csrf   import csrf_exempt
from django.contrib.auth.decorators import login_required
from bencode                        import encode
from openbdr.models                 import Account, Peer, Share
from openbdr.forms                  import (PeerListRequestForm, ShareUpdateForm,
                                            AddPeerForm, AddPeerToShareForm, 
                                            AddShareForm, RemoveShareForm, 
                                            RemovePeerFromShareForm, 
                                            RemovePeerForm)

@login_required
def profile(request):
    """Allow a user or the config tool to manage access permissions"""
    pass


@login_required
def add_peer(request):
    if request.method == 'POST':
        f = AddPeerForm(request.POST)
        if f.is_valid():
            f.save()
            return HttpResponse('Success', content_type='text/plain')
    else:
        f = AddPeerForm()

    return render('add_peer.html', {'form':f})


@login_required
def remove_peer(request):
    if request.method == 'POST':
        f = RemovePeerForm(request.POST)
        if f.is_valid():
            Peer = get_object_or_404(Peer, peer_id=f.cleaned_data['peer_id'])
            peer.delete()
            return HttpResponse('Success', content_type='text/plain')
    else:
        f = RemovePeerForm()

    return render('remove_peer.html', {'form':f})


@login_required
def add_peer_to_share(request):
    if request.method == 'POST':
        f = AddPeerToShareForm(request.POST)
        if f.is_valid():
            share = Share.objects.get(pk=f.cleaned_data['share_id'])
            try:
                peer = Peer.objects.get(peer_id=f.cleaned_data['peer_id'])
            except:
                peer = Peer.objects.create(
                    peer_name = f.cleaned_data.get('peer_name',''),
                    peer_id = f.cleaned_data['peer_id']
                )
                peer.save()
            share.peer_list.add(peer)
            return HttpResponse('Success', content_type='text/plain')
    else:
        f = AddPeerToShareForm()

    return render('add_peer_to_share.html', {'form':f})
   

@login_required
def remove_peer_from_share(request):
    if request.method == 'POST':
        f = RemovePeerFromShareForm(request.POST)
        if f.is_valid():
            share = Share.objects.get(pk=f.cleaned_data['share_id'])
            peer = get_object_or_404(Peer, peer_id=f.cleaned_data['peer_id'])
            share.peer_list.remove(peer)
            return HttpResponse('Success', content_type='text/plain')
    else:
        f = RemovePeerFromShareForm()

    return render('remove_peer_from_share.html', {'form':f})
   

@login_required
def add_share(request):
    if request.method == 'POST':
        f = AddShareForm(request.POST)
        if f.is_valid():
            f.save()
            return HttpResponse('Success', content_type='text/plain')
    else:
        f = AddShareForm()

    return render('add_share.html', {'form':f})


@login_required
def remove_share(request):
    if request.method == 'POST':
        f = RemoveShareForm(request.POST)
        if f.is_valid():
            share = Share.objects.get(pk=f.cleaned_data['share_id'])
            share.peer_list.clear()
            share.delete()
            return HttpResponse('Success', content_type='text/plain')
    else:
        f = RemoveShareForm()

    return render('remove_share.html', {'form':f})


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

    resb = encode(res)
    return HttpResponse(resb, content_type="text/plain")


def read_share(request):
    """Access point for Utilities to check for updates to the share file"""
    share = get_object_or_404(Share, pk=request.GET.get('share_id',''))
    peer = get_object_or_404(Peer, peer_id=request.GET.get('peer_id',''))

    if peer not in share.peer_list:
        raise Http404()

    if share.info_hash == request.GET.get('info_hash',''):
        return HttpResponseNotModified()

    return HttpResponse(share.phile, content_type="text/plain")


@csrf_exempt
def update_share(request):
    """Access point for Utilities to update the share"""
    if request.method != 'POST':
        raise Http404()

    suf = ShareUpdateForm(request.POST, request.FILES)
    if suf.is_valid():
        # Update the share
        share = Share.objects.get(pk=request.POST['share_id'])
        share.info_hash = request.POST['info_hash']
        share.phile = request.FILES['share_file']
        share.save()
        return HttpResponse('Update Successful', content_type="text/plain")

    raise Http404()

