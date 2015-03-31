from django.http                import HttpResponseRedirect
from django.shortcuts           import render
from django.contrib.auth        import authenticate, login
from django.contrib.auth.models import User
from accounts.forms             import CreateUserForm

def register(request):
    """Register an account with the tracker"""
    if request.method == 'POST':
        form = CreateUserForm(request.POST)
        if form.is_valid():
            user = form.save()
            user = authenticate(username=form.cleaned_data['username'], 
                password=form.cleaned_data['password2'])
            login(request,user)
            return HttpResponseRedirect('/profile/')
    else:
        form = CreateUserForm()

    return render(request, 'register.html', {
        'form' : form,
    })
