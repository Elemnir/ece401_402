from django.shortcuts               import render, get_object_or_404
from django.http                    import (Http404, HttpResponse, 
                                            HttpResponseNotModified, 
                                            HttpResponseBadRequest)
from django.views.decorators.csrf   import csrf_exempt
from django.contrib.auth.decorators import login_required
from bencode                        import encode
from openbdr.models                 import (Account, Peer, Share, PeerID, 
                                            genPeerIDBatch)
from openbdr.forms                  import (PeerListRequestForm, 
                                            ShareUpdateForm, AddPeerForm, 
                                            AddPeerToShareForm, AddShareForm, 
                                            RemoveShareForm, RemovePeerForm,
                                            RemovePeerFromShareForm)

@login_required
def profile(request):
    """Allow a user or the config tool to manage access permissions"""
    pass


def get_peer_id():
    pid = PeerID.objects.filter(avail=True).first()
    
    if pid == None:
        genPeerIDBatch(10)
        pid = PeerID.objects.filter(avail=True).first()
    
    pid.avail = False
    pid.save()
    return pid.value


@login_required
def add_peer(request):
    if request.method == 'POST':
        f = AddPeerForm(request.POST)
        if f.is_valid():
            peer = f.save(commit = False)
            peer.peer_id = get_peer_id()
            peer.save()
            return HttpResponse(str(peer.peer_id), content_type='text/plain')
        else:
            return HttpResponseBadRequest()
    else:
        f = AddPeerForm()

    return render(request, 'add_peer.html', {'form':f})


@login_required
def remove_peer(request):
    if request.method == 'POST':
        f = RemovePeerForm(request.POST)
        if f.is_valid():
            try:
                peer = Peer.objects.get(peer_id=f.cleaned_data['peer_id'])
            except:
                return HttpResponseBadRequest()
            peer.delete()
            return HttpResponse('Success', content_type='text/plain')
        else:
            return HttpResponseBadRequest()
    else:
        f = RemovePeerForm()

    return render(request, 'remove_peer.html', {'form':f})


@login_required
def add_peer_to_share(request):
    if request.method == 'POST':
        f = AddPeerToShareForm(request.POST)
        if f.is_valid():
            share = Share.objects.get(pk=request.POST['share_id'])
            try:
                peer = Peer.objects.get(peer_id=f.cleaned_data['peer_id'])
            except:
                return HttpResponseBadRequest()
            share.peer_list.add(peer)
            return HttpResponse('Success', content_type='text/plain')
        else:
            return HttpResponseBadRequest()
    else:
        f = AddPeerToShareForm()

    return render(request, 'add_peer_to_share.html', {'form':f})
   

@login_required
def remove_peer_from_share(request):
    if request.method == 'POST':
        f = RemovePeerFromShareForm(request.POST)
        if f.is_valid():
            share = Share.objects.get(pk=request.POST['share_id'])
            try:
                peer = Peer.objects.get(peer_id=f.cleaned_data['peer_id'])
                share.peer_list.remove(peer)
            except:
                return HttpResponseBadRequest()

            return HttpResponse('Success', content_type='text/plain')
        else:
            return HttpResponseBadRequest()
    else:
        f = RemovePeerFromShareForm()

    return render(request, 'remove_peer_from_share.html', {'form':f})
   

@login_required
def add_share(request):
    if request.method == 'POST':
        f = AddShareForm(request.POST)
        if f.is_valid():
            share = f.save(commit=False)
            share.share_owner = Account.objects.get(user=request.user)
            share.save()
            return HttpResponse(str(share.pk), content_type='text/plain')
        else:
            return HttpResponseBadRequest()
    else:
        f = AddShareForm()

    return render(request, 'add_share.html', {'form':f})


@login_required
def remove_share(request):
    if request.method == 'POST':
        f = RemoveShareForm(request.POST)
        if f.is_valid():
            share = Share.objects.get(pk=request.POST['share_id'])
            share.peer_list.clear()
            share.delete()
            return HttpResponse('Success', content_type='text/plain')
        else:
            return HttpResponseBadRequest()
    else:
        f = RemoveShareForm()

    return render(request, 'remove_share.html', {'form':f})


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
    share = get_object_or_404(Share, pk=int(request.GET.get('share_id',0)))
    peer = get_object_or_404(Peer, peer_id=request.GET.get('peer_id',''))

    if peer not in share.peer_list.get_queryset():
        raise Http404()

    if share.info_hash == request.GET.get('info_hash',''):
        return HttpResponseNotModified()

    return HttpResponse(share.share_file, content_type="text/plain")


@csrf_exempt
def update_share(request):
    """Access point for Utilities to update the share"""
    if request.method != 'POST':
        return HttpResponseBadRequest()

    suf = ShareUpdateForm(request.POST, request.FILES)
    if suf.is_valid():
        # Update the share
        share = Share.objects.get(pk=request.POST['share_id'])
        share.info_hash = request.POST['info_hash']
        share.share_file = request.FILES['share_file']
        share.save()
        return HttpResponse('Update Successful', content_type="text/plain")

    return HttpResponseBadRequest()

