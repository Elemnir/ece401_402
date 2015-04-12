# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from django.db import models, migrations


class Migration(migrations.Migration):

    dependencies = [
        ('openbdr', '0004_auto_20150406_0205'),
    ]

    operations = [
        migrations.AlterField(
            model_name='share',
            name='info_hash',
            field=models.CharField(max_length=100),
            preserve_default=True,
        ),
    ]
