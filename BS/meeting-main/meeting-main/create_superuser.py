
import os
import django
import sys

# Add the project root to sys.path
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

os.environ.setdefault("DJANGO_SETTINGS_MODULE", "settings")
django.setup()

from django.contrib.auth.models import User
from booking.models import UserProfile

username = 'admin'
email = 'admin@example.com'
password = 'admin'

if not User.objects.filter(username=username).exists():
    print(f"Creating superuser {username}...")
    user = User.objects.create_superuser(username, email, password)
    # Also create a UserProfile for the admin to ensure role checks pass
    if not hasattr(user, 'profile'):
        UserProfile.objects.create(user=user, role='admin', department='Admin Office')
    print(f"Superuser created. Username: {username}, Password: {password}")
else:
    print(f"Superuser {username} already exists.")
    user = User.objects.get(username=username)
    # Ensure profile exists
    if not hasattr(user, 'profile'):
        UserProfile.objects.create(user=user, role='admin', department='Admin Office')
        print("Created missing UserProfile for admin.")
