# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from django.db import models, migrations


class Migration(migrations.Migration):

    dependencies = [
        ('openbdr', '0002_auto_20150330_2154'),
    ]

    operations = [
        migrations.CreateModel(
            name='PeerID',
            fields=[
                ('id', models.AutoField(verbose_name='ID', serialize=False, auto_created=True, primary_key=True)),
                ('value', models.CharField(unique=True, max_length=20)),
                ('avail', models.BooleanField(default=True)),
            ],
            options={
            },
            bases=(models.Model,),
        ),
    ]
