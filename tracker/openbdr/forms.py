from django                 import forms
from django.http            import Http404
from django.shortcuts       import get_object_or_404
from openbdr.models         import Share, ShareHist, Peer

class PeerListRequestForm(forms.Form):
    info_hash   = forms.CharField(max_length=20)
    peer_id     = forms.CharField(max_length=20)
    port        = forms.IntegerField()
    uploaded    = forms.IntegerField()
    downloaded  = forms.IntegerField()
    left        = forms.IntegerField()
    ip          = forms.CharField(max_length=256, required=False)
    numwant     = forms.IntegerField(required=False)
    event       = forms.ChoiceField(required=False,
        choices=(('started','str'),('stopped','stp'),('completed','cmp'))
    )

    def clean(self):
        cd = super(PeerListRequestForm, self).clean()
        
        # Get the associated share and its list of authorized peers
        share = get_object_or_404(Share, info_hash=cd.get('info_hash',''))
        auth_peers = share.peer_list.get_queryset().values('peer_id')
        auth_peers = [str(d['peer_id']) for d in auth_peers]

        # Only allow peers in the authorized list
        if cd.get('peer_id','') not in auth_peers:
            raise Http404()

        return cd
        
    def process(self):
        cd = self.cleaned_data
        share = get_object_or_404(Share, info_hash=cd.get('info_hash',''))
        peer = get_object_or_404(Peer, peer_id=cd.get('peer_id',''))

        peer.peer_ip = cd.get('ip','')
        peer.peer_port = cd['port']
        peer.save()
        
        return [ i for i in share.peer_list.get_queryset() if i.is_online ]

class ShareUpdateForm(forms.Form):
    info_hash   = forms.CharField(max_length=20)
    share_id    = forms.IntegerField()
    peer_id     = forms.CharField(max_length=20)
    share_file  = forms.FileField()

    def clean(self):
        cd = super(PeerListRequestForm, self).clean()
        
        # Get the associated share and its list of authorized peers
        share = get_object_or_404(Share, id=cd.get('share_id',''))
        auth_peers = share.peer_list.get_queryset().values('peer_id')
        auth_peers = [str(d['peer_id']) for d in auth_peers]

        # Only allow peers in the authorized list
        if cd.get('peer_id','') not in auth_peers:
            raise Http404()

        return cd

