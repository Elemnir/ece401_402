from django.shortcuts               import render
from django.contrib.auth.decorators import login_required
from openbdr.models                 import Account, Peer, Share

@login_required
def profile(request):
    """Allow a user or the config tool to manage access permissions"""
    pass

def tracker(request):
    """Peer communication access point"""
    pass
