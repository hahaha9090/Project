from django.contrib import admin
from django.contrib.auth.admin import UserAdmin as BaseUserAdmin
from django.contrib.auth.models import User
from .models import Room, Seat, Equipment, Reservation, Announcement, UserProfile, Settings, StudentInfo, TeacherInfo

@admin.register(StudentInfo)
class StudentInfoAdmin(admin.ModelAdmin):
    list_display = ('name', 'student_id', 'department')
    search_fields = ('name', 'student_id')

@admin.register(TeacherInfo)
class TeacherInfoAdmin(admin.ModelAdmin):
    list_display = ('name', 'teacher_id', 'department')
    search_fields = ('name', 'teacher_id')

# Define an inline admin descriptor for UserProfile model
class UserProfileInline(admin.StackedInline):
    model = UserProfile
    can_delete = False
    verbose_name_plural = '用户扩展信息'

# Define a new User admin
class UserAdmin(BaseUserAdmin):
    inlines = (UserProfileInline,)

# Re-register UserAdmin
admin.site.unregister(User)
admin.site.register(User, UserAdmin)

@admin.register(Room)
class RoomAdmin(admin.ModelAdmin):
    list_display = ('name', 'category', 'capacity', 'manager', 'status')
    list_filter = ('category', 'status')
    search_fields = ('name', 'description')

@admin.register(Seat)
class SeatAdmin(admin.ModelAdmin):
    list_display = ('room', 'number', 'is_active')
    list_filter = ('room', 'is_active')

@admin.register(Equipment)
class EquipmentAdmin(admin.ModelAdmin):
    list_display = ('name', 'room', 'is_active')
    list_filter = ('room', 'is_active')

@admin.register(Reservation)
class ReservationAdmin(admin.ModelAdmin):
    list_display = ('title', 'user', 'room', 'seat', 'equipment', 'date', 'start_time', 'end_time', 'status')
    list_filter = ('status', 'date', 'room')
    search_fields = ('title', 'booker', 'user__username')

@admin.register(Announcement)
class AnnouncementAdmin(admin.ModelAdmin):
    list_display = ('title', 'author', 'created_at')
    list_filter = ('created_at',)

@admin.register(Settings)
class SettingsAdmin(admin.ModelAdmin):
    list_display = ('key', 'value')
