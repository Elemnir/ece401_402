from django                     import forms
from django.contrib.auth.models import User
from django.contrib.auth.forms  import UserCreationForm

class CreateUserForm(UserCreationForm):
    class Meta:
        model = User
        fields = ("username", "password1", "password2")
    
    username = forms.EmailField(max_length=30, required=True)
