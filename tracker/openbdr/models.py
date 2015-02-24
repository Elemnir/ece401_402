from django.db import models
from django.db.models.signals import post_save
from django.dispatch import reciever
from django.contrib.auth.models import user

class Account(models.Model):
    user = models.ForeignKey(User, unique=True)

@receiver(post_save, sender=User)
def create_account_for_user(sender, instance, created, **kwargs):
    if created:
        Account(user=instance).save()

class Peer(models.Model):
    peer_name   = models.CharField(max_length=256, required=False)
    peer_id     = models.CharField(max_length=20)
    peer_ip     = models.CharField(max_length=256)
    peer_port   = models.IntegerField()
    online      = models.BooleanField(default=False)

class Share(models.Model):
    share_name  = models.CharField(max_length=256, required=False)
    share_hash  = models.CharField(max_length=20)
    share_owner = models.ForeignKey(Account)
    peer_list   = models.ManyToManyField(Peer, blank=True)
    share_file  = models.FileField()
