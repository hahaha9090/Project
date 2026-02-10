
import os
import django
import sys

sys.path.append(os.getcwd())
os.environ.setdefault("DJANGO_SETTINGS_MODULE", "settings")
django.setup()

from django.contrib.auth.models import User
from booking.models import UserProfile, StudentInfo, TeacherInfo

def create_user(username, password, role, real_name, id_num, department):
    # Ensure whitelist info exists
    if role == 'student':
        StudentInfo.objects.get_or_create(
            student_id=id_num,
            defaults={'name': real_name, 'department': department}
        )
    elif role == 'teacher':
        TeacherInfo.objects.get_or_create(
            teacher_id=id_num,
            defaults={'name': real_name, 'department': department}
        )

    # Create or update user
    user, created = User.objects.get_or_create(username=username)
    user.set_password(password)
    user.first_name = real_name
    user.email = f"{username}@example.com"
    user.save()

    # Create or update profile
    if not hasattr(user, 'profile'):
        UserProfile.objects.create(user=user, role=role, student_id=id_num, department=department)
    else:
        user.profile.role = role
        user.profile.student_id = id_num
        user.profile.department = department
        user.profile.save()
    
    print(f"User {username} ({role}) ready.")

# Create Admin (ensure profile)
admin_user, _ = User.objects.get_or_create(username='admin')
if not admin_user.check_password('admin'):
    admin_user.set_password('admin')
    admin_user.save()
if not hasattr(admin_user, 'profile'):
    UserProfile.objects.create(user=admin_user, role='admin', department='Admin Office')
print("User admin (admin) ready.")

# Create Student
create_user('student', 'student123', 'student', '张三', '2023001', '计算机学院')

# Create Teacher
create_user('teacher', 'teacher123', 'teacher', '李四', 'T001', '软件工程系')
