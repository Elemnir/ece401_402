from __future__ import print_function
from httplib2   import Http
from urllib     import urlencode
from bs4        import BeautifulSoup as BS

class CommFailure(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

class TrackerComm(object):
    
    def __init__(self, domain=""):
        self.h = Http()
        self.domain = str(domain)

    def register_user(self):
        url = '/accounts/register/'
        res, con = self.h.request(self.domain+url, "GET")
        
        headers = {'Content-type': 'application/x-www-form-urlencoded'}
        
        if res.status != 200:
            raise CommFailure('Request Failed')
        
        headers['Cookie'] = res['set-cookie']
        csrf = BS(con).find('input', attrs = {
                'name' : "csrfmiddlewaretoken", 'type' : 'hidden'
            })['value']
        
        username = str(raw_input("Email Address: "))
        password = str(raw_input("Password: "))
        passconf = str(raw_input("Password (Again): "))

        if password != passconf:
            raise CommFailure('Password Mismatch')

        data = {
            "csrfmiddlewaretoken" : csrf, 
            "username" : username,
            "password1" : password,
            "password2" : password
        }
        res, con = self.h.request(self.domain+url, "POST", 
                headers=headers, body=urlencode(data))

        if res.status != 301:
            raise CommFailure('Invalid Input')

        print('User added!')
