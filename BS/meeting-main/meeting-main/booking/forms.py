from django import forms
from django.contrib.auth.models import User
from django.contrib.auth.forms import UserCreationForm
from .models import UserProfile, StudentInfo, TeacherInfo

class RegisterForm(UserCreationForm):
    # Exclude 'admin' from registration choices
    ROLE_CHOICES_REGISTER = [
        ('student', '学生'),
        ('teacher', '教师'),
    ]
    role = forms.ChoiceField(choices=ROLE_CHOICES_REGISTER, label="注册角色")
    real_name = forms.CharField(max_length=100, label="真实姓名", help_text="请输入您的真实姓名进行核验")
    student_id = forms.CharField(max_length=20, label="学号/工号", help_text="请输入您的学号或工号")
    department = forms.CharField(max_length=100, label="院系", required=False, help_text="选填，若为空将自动从信息库填充")

    class Meta(UserCreationForm.Meta):
        model = User
        fields = UserCreationForm.Meta.fields + ('email',)

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # Adjust field order for better UX
        self.order_fields(['role', 'real_name', 'student_id', 'department', 'username', 'email', 'password1', 'password2'])

    def clean(self):
        cleaned_data = super().clean()
        role = cleaned_data.get('role')
        name = cleaned_data.get('real_name')
        sid = cleaned_data.get('student_id')

        if not role or not name or not sid:
            return cleaned_data

        if role == 'student':
            # Check if StudentInfo exists
            try:
                info = StudentInfo.objects.get(student_id=sid, name=name)
                # Auto-fill department if not provided
                if not cleaned_data.get('department'):
                    cleaned_data['department'] = info.department
            except StudentInfo.DoesNotExist:
                raise forms.ValidationError("学生身份核验失败：学号与姓名不匹配或不存在于信息库中。")
        
        elif role == 'teacher':
             # Check if TeacherInfo exists
            try:
                info = TeacherInfo.objects.get(teacher_id=sid, name=name)
                 # Auto-fill department if not provided
                if not cleaned_data.get('department'):
                    cleaned_data['department'] = info.department
            except TeacherInfo.DoesNotExist:
                raise forms.ValidationError("教师身份核验失败：工号与姓名不匹配或不存在于信息库中。")
        
        # Check if this ID has already been registered by another user
        if UserProfile.objects.filter(student_id=sid, role=role).exists():
             raise forms.ValidationError("该学号/工号已被注册。")

        return cleaned_data

    def save(self, commit=True):
        user = super().save(commit=False)
        # Store first_name/last_name from real_name (simple split)
        user.first_name = self.cleaned_data['real_name']
        if commit:
            user.save()
            UserProfile.objects.create(
                user=user,
                role=self.cleaned_data['role'],
                student_id=self.cleaned_data['student_id'],
                department=self.cleaned_data['department']
            )
        return user
