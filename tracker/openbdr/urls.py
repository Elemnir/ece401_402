from django.conf.urls           import patterns, include, url

urlpatterns = patterns("openbdr.views",
    url(r"^profile/$",      "profile",      name='openbdr_profile'),
    url(r"^add_peer/$",     "add_peer",     name='openbdr_add_peer'),
    url(r"^remove_peer/$",  "remove_peer",  name='openbdr_remove_peer'),
    url(r"^add_peer_to_share/$",        "add_peer_to_share",     
        name='openbdr_add_peer_to_share'),
    url(r"^remove_peer_from_share/$",   "remove_peer_from_share",  
        name='openbdr_remove_peer_from_share'),
    url(r"^add_share/$",    "add_share",    name='openbdr_add_share'),
    url(r"^remove_share/$", "remove_share", name='openbdr_remove_share'),
    url(r"^tracker/$",      "tracker",      name='openbdr_tracker'),
    url(r"^read_share/$",   "read_share",   name='openbdr_read_share'),
    url(r"^update_share/$", "update_share", name='openbdr_update_share'),
)
