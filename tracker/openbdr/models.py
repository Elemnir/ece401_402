from datetime                   import timedelta
from django.utils               import timezone
from django.db                  import models
from django.db.models.signals   import post_save
from django.dispatch            import receiver
from django.contrib.auth.models import User

class Account(models.Model):
    user = models.ForeignKey(User, unique=True)

@receiver(post_save, sender=User)
def create_account_for_user(sender, instance, created, **kwargs):
    if created:
        Account(user=instance).save()

class Peer(models.Model):
    peer_name   = models.CharField(max_length=256, blank=True)
    peer_id     = models.CharField(max_length=20)
    peer_ip     = models.CharField(max_length=256)
    peer_port   = models.IntegerField()
    last_seen   = models.DateTimeField(auto_now=True)
    
    def is_online(self):
        return self.last_seen >= timezone.now() - timedelta(minutes=1)

def file_path(share, filename):
    return '{}/{}'.format(share.share_owner.id, share.info_hash)

class Share(models.Model):
    share_name  = models.CharField(max_length=256, blank=True)
    info_hash   = models.CharField(max_length=20)
    share_owner = models.ForeignKey(Account)
    peer_list   = models.ManyToManyField(Peer, blank=True)
    share_file  = models.FileField(upload_to=file_path, blank=True)
