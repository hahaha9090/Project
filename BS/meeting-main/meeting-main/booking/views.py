from django.shortcuts import render, redirect, get_object_or_404
from django.http import HttpResponse, JsonResponse
from django.views.decorators.csrf import ensure_csrf_cookie
from django.contrib.auth.decorators import login_required
from django.contrib.auth import login as auth_login
from django.contrib.auth.models import User
from django.contrib.auth import views as auth_views
from django.contrib import messages
from .models import Room, Reservation, Seat, Equipment, Announcement, UserProfile
from .forms import RegisterForm

class CustomLoginView(auth_views.LoginView):
    template_name = 'registration/login.html'
    
    def form_valid(self, form):
        # Authenticate first (default behavior)
        auth_login(self.request, form.get_user())
        
        # Check if the selected role matches the user's role
        selected_role = self.request.POST.get('role')
        user = self.request.user
        
        # Superuser can bypass role check or login as any role (optional, here we treat as admin)
        if user.is_superuser:
            if selected_role != 'admin':
                # Optional: Force admin to admin dashboard even if they selected student?
                # For now, let's just let them in but warn or redirect appropriately.
                pass
        else:
            try:
                user_role = user.profile.role
                if selected_role != user_role:
                    messages.error(self.request, f"登录失败：您不是{dict(UserProfile.ROLE_CHOICES)[selected_role]}用户。")
                    # Logout and redirect back to login
                    from django.contrib.auth import logout
                    logout(self.request)
                    return redirect('login')
            except UserProfile.DoesNotExist:
                # If no profile, maybe valid for admin, but regular users should have one.
                # If selected role is not admin, deny.
                if selected_role != 'admin' and not user.is_staff:
                     messages.error(self.request, "登录失败：用户信息不完整。")
                     from django.contrib.auth import logout
                     logout(self.request)
                     return redirect('login')

        return redirect(self.get_success_url())

@ensure_csrf_cookie
def register(request):
    """用户注册视图"""
    if request.method == 'POST':
        form = RegisterForm(request.POST)
        if form.is_valid():
            form.save()
            messages.success(request, "注册成功，请登录。")
            return redirect('login')
    else:
        form = RegisterForm()
    return render(request, 'registration/register.html', {'form': form})

@login_required
def dashboard_redirect(request):
    """登录后跳转分发"""
    try:
        role = request.user.profile.role
    except:
        role = 'student' # 默认
        
    if request.user.is_superuser:
        return redirect('admin_dashboard')
    elif role == 'teacher':
        return redirect('teacher_dashboard')
    elif role == 'admin': # 自定义管理员角色
        return redirect('admin_dashboard')
    else:
        return redirect('student_dashboard')

@login_required
def student_dashboard(request):
    """学生端首页"""
    rooms = Room.objects.all()
    my_reservations = Reservation.objects.filter(user=request.user).order_by('-created_at')[:5]
    announcements = Announcement.objects.all()[:5]
    return render(request, "booking/student_dashboard.html", {
        "rooms": rooms, 
        "my_reservations": my_reservations,
        "announcements": announcements
    })

@login_required
def teacher_dashboard(request):
    """教师端首页"""
    # 教师管理的教室
    managed_rooms = Room.objects.filter(manager=request.user)
    # 这些教室的预约
    room_reservations = Reservation.objects.filter(room__in=managed_rooms).order_by('-created_at')
    
    return render(request, "booking/teacher_dashboard.html", {
        "managed_rooms": managed_rooms,
        "room_reservations": room_reservations
    })

@login_required
def admin_dashboard(request):
    """管理员首页"""
    if not request.user.is_staff and not request.user.profile.role == 'admin':
        return redirect('student_dashboard')
        
    stats = {
        'room_count': Room.objects.count(),
        'user_count': User.objects.count(),
        'reservation_count': Reservation.objects.count(),
        'equipment_count': Equipment.objects.count(),
    }
    return render(request, "booking/admin_dashboard.html", {"stats": stats})

@login_required
def room_detail(request, room_id):
    """自习室详情与预约"""
    room = get_object_or_404(Room, id=room_id)
    seats = room.seats.all()
    equipments = room.equipments.all()
    
    # Get selected date from query params or default to today
    from django.utils import timezone
    import datetime
    
    selected_date_str = request.GET.get('date')
    selected_start_str = request.GET.get('start_time')
    selected_end_str = request.GET.get('end_time')

    if selected_date_str:
        try:
            selected_date = datetime.datetime.strptime(selected_date_str, '%Y-%m-%d').date()
        except ValueError:
            selected_date = timezone.now().date()
    else:
        selected_date = timezone.now().date()
        
    # Get reservations for this room and date to check availability
    reservations = Reservation.objects.filter(
        room=room,
        date=selected_date,
        status__in=['pending', 'approved']
    )
    
    # Calculate Seat Status based on selected time range (if provided)
    # If time range is provided, check if seat is occupied in that range.
    # If no time range, check if seat has ANY reservation (partial occupation).
    
    seat_status = {} # seat_id: is_occupied (bool) or status string
    
    if selected_start_str and selected_end_str:
        try:
            filter_start = datetime.datetime.strptime(selected_start_str, '%H:%M').time()
            filter_end = datetime.datetime.strptime(selected_end_str, '%H:%M').time()
            
            # Check overlap for each seat
            for seat in seats:
                is_occupied = False
                seat_res = reservations.filter(seat=seat)
                for res in seat_res:
                    # Overlap logic: (StartA < EndB) and (EndA > StartB)
                    if res.start_time < filter_end and res.end_time > filter_start:
                        is_occupied = True
                        break
                seat_status[seat.id] = is_occupied
        except ValueError:
            pass # Invalid time format
    else:
        # Default: check if seat has any reservation today
        for seat in seats:
             # Just check if ID exists in reservation list
             # Optimization: get list of seat IDs
             seat_status[seat.id] = reservations.filter(seat=seat).exists()

    return render(request, "booking/room_detail.html", {
        "room": room,
        "seats": seats,
        "equipments": equipments,
        "selected_date": selected_date,
        "selected_start": selected_start_str,
        "selected_end": selected_end_str,
        "reservations": reservations,
        "seat_status": seat_status,
    })

@login_required
def my_reservations(request):
    """我的预约"""
    reservations = Reservation.objects.filter(user=request.user).order_by('-created_at')
    return render(request, "booking/my_reservations.html", {"reservations": reservations})

# -----------------------------------------------------------------------------
# 兼容旧视图 (重定向或保留)
# -----------------------------------------------------------------------------
@login_required
def profile_edit(request):
    """编辑个人信息"""
    if request.method == 'POST':
        user = request.user
        user.first_name = request.POST.get('real_name')
        user.email = request.POST.get('email')
        user.save()
        messages.success(request, '个人信息更新成功！')
        return redirect('profile_edit')
    
    return render(request, "booking/profile_edit.html")

@login_required
def my_statistics(request):
    """我的学习统计"""
    from django.db.models import Sum, Count
    import datetime
    
    user = request.user
    reservations = Reservation.objects.filter(user=user, status='approved')
    
    total_hours = 0
    # Simple estimation (assuming hourly slots) - in reality calculate delta
    total_count = reservations.count()
    
    # Calculate hours for each reservation
    for res in reservations:
        # datetime.combine
        start = datetime.datetime.combine(datetime.date.min, res.start_time)
        end = datetime.datetime.combine(datetime.date.min, res.end_time)
        diff = end - start
        total_hours += diff.total_seconds() / 3600
        
    total_hours = round(total_hours, 1)
    
    return render(request, "booking/my_statistics.html", {
        "total_count": total_count,
        "total_hours": total_hours,
        "reservations": reservations
    })

@ensure_csrf_cookie
def index(request):
    if request.user.is_authenticated:
        return redirect('dashboard_redirect')
    return redirect('login')

@ensure_csrf_cookie
def admin(request):
    return redirect('admin_dashboard')

@ensure_csrf_cookie
def room_status(request):
    return render(request, "booking/room_status.html")

@ensure_csrf_cookie
def status_display(request):
    rooms = Room.objects.all()
    return render(request, "booking/status_display.html", {"rooms": rooms})
