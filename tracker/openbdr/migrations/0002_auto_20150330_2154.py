# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from django.db import models, migrations
import openbdr.models


class Migration(migrations.Migration):

    dependencies = [
        ('openbdr', '0001_initial'),
    ]

    operations = [
        migrations.RemoveField(
            model_name='sharehist',
            name='share',
        ),
        migrations.DeleteModel(
            name='ShareHist',
        ),
        migrations.AlterField(
            model_name='share',
            name='share_file',
            field=models.FileField(upload_to=openbdr.models.file_path, blank=True),
            preserve_default=True,
        ),
    ]
