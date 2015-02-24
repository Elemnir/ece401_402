from django.conf.urls           import patterns, include, url

urlpatterns = patterns("openbdr.views",
    url(r"^profile/$",  "profile",  name='openbdr_profile'),
    url(r"^tracker/$",  "tracker",  name='openbdr_profile'),
)
