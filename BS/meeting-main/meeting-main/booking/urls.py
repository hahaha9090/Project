from django.urls import path, include
from django.contrib import admin
from . import views
from . import api

urlpatterns = [
    # Django管理员
    path('admin/', admin.site.urls),
    
    # 用户认证
    path('login/', views.CustomLoginView.as_view(), name='login'), # Override default login
    path('accounts/', include('django.contrib.auth.urls')),
    path('register/', views.register, name='register'),
    path('dashboard/', views.dashboard_redirect, name='dashboard_redirect'),
    
    # 角色仪表盘
    path('student/dashboard/', views.student_dashboard, name='student_dashboard'),
    path('teacher/dashboard/', views.teacher_dashboard, name='teacher_dashboard'),
    path('admin/dashboard/', views.admin_dashboard, name='admin_dashboard'),
    
    # 业务功能
    path('rooms/<int:room_id>/', views.room_detail, name='room_detail'),
    path('my-reservations/', views.my_reservations, name='my_reservations'),
    path('profile/', views.profile_edit, name='profile_edit'),
    path('statistics/', views.my_statistics, name='my_statistics'),
    
    # 主页视图
    path('', views.index, name='index'),
    path('booking-admin/', views.admin, name='admin'),
    path('room_status/', views.room_status, name='room_status'),
    path('status_display/', views.status_display, name='status_display'),
    
    # API接口
    path('api/load_rooms/', api.load_rooms, name='load_rooms'),
    path('api/load_reservations/', api.load_reservations, name='load_reservations'),
    path('api/load_settings/', api.load_settings, name='load_settings'),
    path('api/save_rooms/', api.save_rooms, name='save_rooms'),
    path('api/save_reservations/', api.save_reservations, name='save_reservations'),
    path('api/cancel_reservation/', api.cancel_reservation, name='cancel_reservation'),
    path('api/save_settings/', api.save_settings, name='save_settings'),

]
