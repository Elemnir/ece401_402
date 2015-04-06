# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from django.db import models, migrations


class Migration(migrations.Migration):

    dependencies = [
        ('openbdr', '0003_peerid'),
    ]

    operations = [
        migrations.AlterField(
            model_name='peer',
            name='peer_port',
            field=models.IntegerField(default=-1, blank=True),
            preserve_default=True,
        ),
        migrations.AlterField(
            model_name='share',
            name='info_hash',
            field=models.CharField(max_length=40),
            preserve_default=True,
        ),
    ]
