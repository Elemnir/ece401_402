from django.conf.urls           import patterns, include, url
from django.contrib.auth.views  import login, logout

urlpatterns = patterns("accounts.views",
    url(r"^login/$",    login,      name='accounts_login'),
    url(r"^logout/$",   logout,     name='accounts_logout'),
    url(r"^register/$", "register", name='accounts_register'),
)
