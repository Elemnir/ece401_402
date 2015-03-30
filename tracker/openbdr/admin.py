from django.contrib import admin
from openbdr.models import Account, Peer, Share

class AccountAdmin(admin.ModelAdmin):
    class Meta:
        model = Account
admin.site.register(Account, AccountAdmin)


class PeerAdmin(admin.ModelAdmin):
    class Meta:
        model = Peer
    fieldsets = [('Peer', {'fields': (
        'id', 'peer_name', 'peer_id',
        'peer_ip', 'peer_port',
    )})]
    list_display = ('peer_name', 'peer_id', 'peer_ip')
admin.site.register(Peer, PeerAdmin)


class ShareAdmin(admin.ModelAdmin):
    class Meta:
        model = Share
    fieldsets = [('Share', {'fields': (
        'id', 'share_name', 'info_hash',
        'share_owner', 'peer_list', 'share_file'
    )})]
    list_display = ('share_name', 'info_hash', 
        'share_owner'
    )
admin.site.register(Share, ShareAdmin)
