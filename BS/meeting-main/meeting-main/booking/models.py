from django.db import models
from django.contrib.auth.models import User

class Room(models.Model):
    """自习室模型"""
    ROOM_TYPES = [
        ('self_study', '自习用教室'),
        ('computer', '电脑自习室'),
        ('book', '图书自习室'),
    ]
    
    name = models.CharField(max_length=100, verbose_name="自习室名称")
    category = models.CharField(max_length=20, choices=ROOM_TYPES, default='self_study', verbose_name="类型")
    capacity = models.IntegerField(verbose_name="容量")
    description = models.TextField(blank=True, null=True, verbose_name="描述")
    manager = models.ForeignKey(User, on_delete=models.SET_NULL, null=True, blank=True, related_name='managed_rooms', verbose_name="管理员(教师)")
    status = models.CharField(max_length=20, choices=[
        ('available', '可用'),
        ('maintenance', '维护中'),
        ('unavailable', '不可用')
    ], default='available', verbose_name="状态")

    def __str__(self):
        return f"{self.name} ({self.get_category_display()})"
    
    class Meta:
        verbose_name = "自习室"
        verbose_name_plural = "自习室"

class Seat(models.Model):
    """座位模型"""
    room = models.ForeignKey(Room, on_delete=models.CASCADE, related_name='seats', verbose_name="所属自习室")
    number = models.CharField(max_length=20, verbose_name="座位号")
    is_active = models.BooleanField(default=True, verbose_name="是否可用")
    
    def __str__(self):
        return f"{self.room.name} - {self.number}"
        
    class Meta:
        verbose_name = "座位"
        verbose_name_plural = "座位"
        ordering = ['number']

class Equipment(models.Model):
    """设备模型"""
    room = models.ForeignKey(Room, on_delete=models.CASCADE, related_name='equipments', verbose_name="所属自习室")
    name = models.CharField(max_length=100, verbose_name="设备名称")
    description = models.TextField(blank=True, verbose_name="描述")
    is_active = models.BooleanField(default=True, verbose_name="是否可用")
    
    def __str__(self):
        return f"{self.room.name} - {self.name}"
        
    class Meta:
        verbose_name = "设备"
        verbose_name_plural = "设备"

class Announcement(models.Model):
    """公告模型"""
    title = models.CharField(max_length=200, verbose_name="标题")
    content = models.TextField(verbose_name="内容")
    created_at = models.DateTimeField(auto_now_add=True, verbose_name="发布时间")
    author = models.ForeignKey(User, on_delete=models.CASCADE, verbose_name="发布人")
    
    class Meta:
        verbose_name = "公告"
        verbose_name_plural = "公告"
        ordering = ['-created_at']

class Reservation(models.Model):
    """预约模型"""
    user = models.ForeignKey(User, on_delete=models.CASCADE, verbose_name="预约用户", null=True, blank=True)
    room = models.ForeignKey(Room, on_delete=models.CASCADE, verbose_name="自习室")
    seat = models.ForeignKey(Seat, on_delete=models.SET_NULL, null=True, blank=True, verbose_name="座位")
    equipment = models.ForeignKey(Equipment, on_delete=models.SET_NULL, null=True, blank=True, verbose_name="设备")
    
    date = models.DateField(verbose_name="日期")
    start_time = models.TimeField(verbose_name="开始时间")
    end_time = models.TimeField(verbose_name="结束时间")
    title = models.CharField(max_length=255, verbose_name="用途/主题")
    booker = models.CharField(max_length=100, verbose_name="预约人姓名") # 保留作为显示名
    department = models.CharField(max_length=100, blank=True, null=True, verbose_name="院系/班级")
    
    status = models.CharField(max_length=20, choices=[
        ('pending', '待审核'),
        ('approved', '已通过'),
        ('rejected', '已驳回'),
        ('cancelled', '已取消')
    ], default='pending', verbose_name="状态")
    created_at = models.DateTimeField(auto_now_add=True, verbose_name="创建时间")

    def __str__(self):
        target = self.seat or self.equipment or self.room
        return f"{self.title} - {target} ({self.date})"
    
    class Meta:
        verbose_name = "预约记录"
        verbose_name_plural = "预约记录"

class UserProfile(models.Model):
    """用户扩展信息"""
    ROLE_CHOICES = [
        ('student', '学生'),
        ('teacher', '教师'),
        ('admin', '管理员'),
    ]
    user = models.OneToOneField(User, on_delete=models.CASCADE, related_name='profile')
    role = models.CharField(max_length=20, choices=ROLE_CHOICES, default='student', verbose_name="角色")
    student_id = models.CharField(max_length=20, blank=True, null=True, verbose_name="学号/工号")
    department = models.CharField(max_length=100, blank=True, null=True, verbose_name="院系")
    
    def __str__(self):
        return f"{self.user.username} - {self.get_role_display()}"

class Settings(models.Model):
    """系统设置模型"""
    key = models.CharField(max_length=100, unique=True, verbose_name="键")
    value = models.TextField(blank=True, null=True, verbose_name="值")

    def __str__(self):
        return self.key
    
    class Meta:
        verbose_name = "系统设置"
        verbose_name_plural = "系统设置"

class StudentInfo(models.Model):
    """学生信息白名单"""
    name = models.CharField(max_length=100, verbose_name="姓名")
    student_id = models.CharField(max_length=20, unique=True, verbose_name="学号")
    department = models.CharField(max_length=100, verbose_name="院系")

    def __str__(self):
        return f"{self.name} ({self.student_id})"

    class Meta:
        verbose_name = "学生信息库"
        verbose_name_plural = "学生信息库"

class TeacherInfo(models.Model):
    """教师信息白名单"""
    name = models.CharField(max_length=100, verbose_name="姓名")
    teacher_id = models.CharField(max_length=20, unique=True, verbose_name="工号")
    department = models.CharField(max_length=100, verbose_name="院系")

    def __str__(self):
        return f"{self.name} ({self.teacher_id})"

    class Meta:
        verbose_name = "教师信息库"
        verbose_name_plural = "教师信息库"
