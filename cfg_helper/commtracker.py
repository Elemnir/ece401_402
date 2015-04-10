from __future__ import print_function
from httplib2 import Http
from urllib import urlencode
from bs4 import BeautifulSoup as BS


class CommFailure(Exception):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)


class TrackerComm(object):
    def __init__(self, domain=""):
        self.h = Http()
        self.domain = str(domain)

    def register_user(self, user="", pswd=""):
        url = '/accounts/register/'
        res, con = self.h.request(self.domain + url, "GET")

        headers = {'Content-type': 'application/x-www-form-urlencoded'}

        if res.status != 200:
            raise CommFailure('Request Failed')

        headers['Cookie'] = res['set-cookie']
        csrf = BS(con).find('input', attrs={
            'name': "csrfmiddlewaretoken", 'type': 'hidden'
        })['value']

        data = {
            "csrfmiddlewaretoken": csrf,
            "username": user,
            "password1": pswd,
            "password2": pswd
        }
        res, con = self.h.request(self.domain + url, "POST",
                                  headers=headers, body=urlencode(data))

        if res.status != 302:
            raise CommFailure('Invalid Input')

        return res.get('set-cookie', None)

    def login_user(self, user="", pswd=""):
        """Logs in the given user and returns the cookies"""
        url = '/accounts/login/'
        res, con = self.h.request(self.domain + url, "GET")

        headers = {'Content-type': 'application/x-www-form-urlencoded'}

        if res.status != 200:
            raise CommFailure('Request Failed')

        headers['Cookie'] = res['set-cookie']
        csrf = BS(con).find('input', attrs={
            'name': "csrfmiddlewaretoken", 'type': 'hidden'
        })['value']

        data = {
            "csrfmiddlewaretoken": csrf,
            "username": user,
            "password": pswd,
        }
        res, con = self.h.request(self.domain + url, "POST",
                                  headers=headers, body=urlencode(data))

        if res.status != 302:
            raise CommFailure('Validation Failure')

        return res.get('set-cookie', None)

    def add_share(self, user="", pswd="", share=""):
        """Adds a share of the given name and returns its assigned ID"""
        url = '/add_share/'
        headers = {'Content-type': 'application/x-www-form-urlencoded'}

        # Login the user first
        headers['Cookie'] = self.login_user(user, pswd)

        # Request the page
        res, con = self.h.request(self.domain + url, "GET", headers=headers)
        if res.status != 200:
            raise CommFailure('Request Failed')

        # Find the csrf token and construct the response
        csrf = BS(con).find('input', attrs={
            'name': "csrfmiddlewaretoken", 'type': 'hidden'
        })['value']

        data = {
            "csrfmiddlewaretoken": csrf,
            "share_name": share
        }
        res, con = self.h.request(self.domain + url, "POST",
                                  headers=headers, body=urlencode(data))

        if res.status != 200:
            raise CommFailure('Validation Failure')

        return int(con)

    def add_peer(self, user="", pswd="", peer=""):
        """Adds a peer of the given name and returns its assigned ID"""
        url = '/add_peer/'
        headers = {'Content-type': 'application/x-www-form-urlencoded'}

        # Login the user first
        headers['Cookie'] = self.login_user(user, pswd)

        # Request the page
        res, con = self.h.request(self.domain + url, "GET", headers=headers)
        if res.status != 200:
            raise CommFailure('Request Failed')

        # Find the csrf token and construct the response
        csrf = BS(con).find('input', attrs={
            'name': "csrfmiddlewaretoken", 'type': 'hidden'
        })['value']

        data = {
            "csrfmiddlewaretoken": csrf,
            "peer_name": peer
        }
        res, con = self.h.request(self.domain + url, "POST",
                                  headers=headers, body=urlencode(data))

        if res.status != 200:
            raise CommFailure('Validation Failure')

        return str(con)

    def add_peer_to_share(self, user="", pswd="", p_id="", s_id=0):
        """Adds a peer of the given id to the authorized list of the share of 
            the given id"""
        url = '/add_peer_to_share/'
        headers = {'Content-type': 'application/x-www-form-urlencoded'}

        # Login the user first
        headers['Cookie'] = self.login_user(user, pswd)

        # Request the page
        res, con = self.h.request(self.domain + url, "GET", headers=headers)
        if res.status != 200:
            raise CommFailure('Request Failed')

        # Find the csrf token and construct the response
        csrf = BS(con).find('input', attrs={
            'name': "csrfmiddlewaretoken", 'type': 'hidden'
        })['value']

        data = {
            "csrfmiddlewaretoken": csrf,
            "peer_id": p_id,
            "share_id": s_id
        }
        res, con = self.h.request(self.domain + url, "POST",
                                  headers=headers, body=urlencode(data))

        if res.status != 200:
            raise CommFailure('Validation Failure')

    def remove_peer(self, user="", pswd="", p_id=""):
        """Removes a peer of the given id"""
        url = '/remove_peer/'
        headers = {'Content-type': 'application/x-www-form-urlencoded'}

        # Login the user first
        headers['Cookie'] = self.login_user(user, pswd)

        # Request the page
        res, con = self.h.request(self.domain + url, "GET", headers=headers)
        if res.status != 200:
            raise CommFailure('Request Failed')

        # Find the csrf token and construct the response
        csrf = BS(con).find('input', attrs={
            'name': "csrfmiddlewaretoken", 'type': 'hidden'
        })['value']

        data = {
            "csrfmiddlewaretoken": csrf,
            "peer_id": p_id
        }
        res, con = self.h.request(self.domain + url, "POST",
                                  headers=headers, body=urlencode(data))

        if res.status != 200:
            raise CommFailure('Validation Failure')

    def remove_share(self, user="", pswd="", s_id=0):
        """Removes a share of the given id"""
        url = '/remove_share/'
        headers = {'Content-type': 'application/x-www-form-urlencoded'}

        # Login the user first
        headers['Cookie'] = self.login_user(user, pswd)

        # Request the page
        res, con = self.h.request(self.domain + url, "GET", headers=headers)
        if res.status != 200:
            raise CommFailure('Request Failed')

        # Find the csrf token and construct the response
        csrf = BS(con).find('input', attrs={
            'name': "csrfmiddlewaretoken", 'type': 'hidden'
        })['value']

        data = {
            "csrfmiddlewaretoken": csrf,
            "share_id": s_id
        }
        res, con = self.h.request(self.domain + url, "POST",
                                  headers=headers, body=urlencode(data))

        if res.status != 200:
            raise CommFailure('Validation Failure')

    def remove_peer_from_share(self, user="", pswd="", p_id="", s_id=0):
        """Removes a peer of the given id from the authorized list of the share 
            of the given id"""
        url = '/remove_peer_from_share/'
        headers = {'Content-type': 'application/x-www-form-urlencoded'}

        # Login the user first
        headers['Cookie'] = self.login_user(user, pswd)

        # Request the page
        res, con = self.h.request(self.domain + url, "GET", headers=headers)
        if res.status != 200:
            raise CommFailure('Request Failed')

        # Find the csrf token and construct the response
        csrf = BS(con).find('input', attrs={
            'name': "csrfmiddlewaretoken", 'type': 'hidden'
        })['value']

        data = {
            "csrfmiddlewaretoken": csrf,
            "peer_id": p_id,
            "share_id": s_id
        }
        res, con = self.h.request(self.domain + url, "POST",
                                  headers=headers, body=urlencode(data))

        if res.status != 200:
            raise CommFailure('Validation Failure')
 
