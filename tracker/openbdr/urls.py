from django.conf.urls           import patterns, include, url

urlpatterns = patterns("openbdr.views",
    url(r"^profile/$",      "profile",      name='openbdr_profile'),
    url(r"^tracker/$",      "tracker",      name='openbdr_tracker'),
    url(r"^read_share/$",   "read_share",   name='openbdr_read_share'),
    url(r"^update_share/$", "update_share", name='openbdr_update_share'),
)
