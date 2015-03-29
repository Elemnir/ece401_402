# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from django.db import models, migrations
from django.conf import settings


class Migration(migrations.Migration):

    dependencies = [
        migrations.swappable_dependency(settings.AUTH_USER_MODEL),
    ]

    operations = [
        migrations.CreateModel(
            name='Account',
            fields=[
                ('id', models.AutoField(verbose_name='ID', serialize=False, auto_created=True, primary_key=True)),
                ('user', models.ForeignKey(to=settings.AUTH_USER_MODEL, unique=True)),
            ],
            options={
            },
            bases=(models.Model,),
        ),
        migrations.CreateModel(
            name='Peer',
            fields=[
                ('id', models.AutoField(verbose_name='ID', serialize=False, auto_created=True, primary_key=True)),
                ('peer_name', models.CharField(max_length=256, blank=True)),
                ('peer_id', models.CharField(max_length=20)),
                ('peer_ip', models.CharField(max_length=256)),
                ('peer_port', models.IntegerField()),
                ('last_seen', models.DateTimeField(auto_now=True)),
            ],
            options={
            },
            bases=(models.Model,),
        ),
        migrations.CreateModel(
            name='Share',
            fields=[
                ('id', models.AutoField(verbose_name='ID', serialize=False, auto_created=True, primary_key=True)),
                ('share_name', models.CharField(max_length=256, blank=True)),
                ('info_hash', models.CharField(max_length=20)),
                ('share_file', models.FileField(upload_to=b'')),
                ('peer_list', models.ManyToManyField(to='openbdr.Peer', blank=True)),
                ('share_owner', models.ForeignKey(to='openbdr.Account')),
            ],
            options={
            },
            bases=(models.Model,),
        ),
        migrations.CreateModel(
            name='ShareHist',
            fields=[
                ('id', models.AutoField(verbose_name='ID', serialize=False, auto_created=True, primary_key=True)),
                ('info_hash', models.CharField(max_length=20)),
                ('created', models.DateTimeField(auto_now_add=True)),
                ('share', models.ForeignKey(to='openbdr.Share')),
            ],
            options={
            },
            bases=(models.Model,),
        ),
    ]
