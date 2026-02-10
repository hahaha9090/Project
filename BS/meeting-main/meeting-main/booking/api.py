from django.http import JsonResponse
from django.views.decorators.http import require_http_methods
from django.contrib.auth.decorators import login_required
from django.views.decorators.csrf import ensure_csrf_cookie
import json
from .models import Room, Reservation, Settings
from django.utils import timezone
import datetime
import requests
import logging

# 配置日志
logger = logging.getLogger(__name__)

# -----------------------------------------------------------------------------
# 辅助函数
# -----------------------------------------------------------------------------

def send_wechat_notification(reservation, action='新增'):
    """发送企业微信群机器人通知（保留原有逻辑）"""
    try:
        debug_setting = Settings.objects.filter(key='debug_mode').first()
        debug_mode = debug_setting and debug_setting.value.lower() == 'true'
        
        from django.conf import settings as django_settings
        webhook_setting = Settings.objects.filter(key='webhook_url').first()
        webhook_url = (webhook_setting.value.strip() if (webhook_setting and webhook_setting.value) else getattr(django_settings, 'DEFAULT_WEBHOOK_URL', '').strip())
        
        if not webhook_url:
            return False, "Webhook URL未配置"
        
        title_map = {
            '新增': '新增会议室预约通知',
            '修改': '会议室预约修改通知',
            '编辑': '会议室预约修改通知',
            '取消': '会议室预约取消通知'
        }
        title = title_map.get(action, '会议室预约通知')
        
        def escape_markdown_v2(text):
            if not text: return text
            special_chars = ['_', '*', '[', ']', '(', ')', '~', '`', '>', '#', '+', '-', '=', '|', '{', '}', '.', '!']
            for char in special_chars:
                text = text.replace(char, f'\\{char}')
            return text
        
        room_name = escape_markdown_v2(reservation.room.name)
        res_title = escape_markdown_v2(reservation.title)
        booker = escape_markdown_v2(reservation.booker)
        department = escape_markdown_v2(reservation.department or '未填写')
        
        dash_separator = "\\-"
        triple_dash = "\\-\\-\\-"
        date_format = reservation.date.strftime('%Y年%m月%d日')
        time_range = f"{reservation.start_time.strftime('%H:%M')} {dash_separator} {reservation.end_time.strftime('%H:%M')}"
        created_local = timezone.localtime(reservation.created_at)
        created_time = created_local.strftime('%Y-%m-%d %H:%M:%S').replace('-', '\\-')
        
        status_text = "已取消" if action == '取消' else "已预约"
        
        markdown_content = f"""# 📅 {title}
## 📋 会议详情
| **项目** | **内容** |
| :--- | :--- |
| **会议室** | {room_name} |
| **预约日期** | {date_format} |
| **会议时间** | {time_range} |
| **会议主题** | {res_title} |
| **预约人** | {booker} |
| **状态** | {status_text} |

{triple_dash}
> 📌 操作时间：{created_time}"""

        payload = {"msgtype": "markdown_v2", "markdown_v2": {"content": markdown_content}}
        headers = {'Content-Type': 'application/json; charset=utf-8'}
        
        response = requests.post(webhook_url, json=payload, headers=headers, timeout=5)
        if response.status_code == 200 and response.json().get('errcode') == 0:
            return True, "发送成功"
        return False, f"API错误: {response.text}"
            
    except Exception as e:
        logger.error(f"发送通知失败: {e}")
        return False, str(e)

# -----------------------------------------------------------------------------
# API 视图
# -----------------------------------------------------------------------------

@require_http_methods(["GET"])
def load_rooms(request):
    """加载会议室数据"""
    rooms = []
    for room in Room.objects.all():
        rooms.append({
            'id': str(room.id),
            'name': room.name,
            'capacity': room.capacity,
            'description': room.description or '',
            'equipment': room.equipment or '',
            'status': room.status
        })
    return JsonResponse(rooms, safe=False)

@require_http_methods(["GET"])
def load_reservations(request):
    """加载预约数据"""
    reservations = []
    # 只显示未取消的预约，或者全部显示但标记状态
    # 为了日历显示正常，我们只返回非取消状态的，或者前端处理
    # 这里返回所有非取消的预约
    objs = Reservation.objects.exclude(status='cancelled')
    
    for res in objs:
        reservations.append({
            'id': res.id,
            'room': str(res.room.id),
            'date': res.date.isoformat(),
            'start': res.start_time.strftime('%H:%M'),
            'end': res.end_time.strftime('%H:%M'),
            'title': res.title,
            'booker': res.booker,
            'department': res.department or '',
            'status': res.status,
            'is_mine': request.user.is_authenticated and res.user == request.user,
            'room_name': res.room.name
        })
    return JsonResponse(reservations, safe=False)

@require_http_methods(["GET"])
def load_settings(request):
    """加载设置数据"""
    settings = {s.key: s.value for s in Settings.objects.all()}
    return JsonResponse(settings)

@require_http_methods(["POST"])
def save_rooms(request):
    """保存会议室数据 (仅管理员)"""
    if not request.user.is_staff:
        return JsonResponse({'success': False, 'error': '权限不足'}, status=403)
        
    try:
        data = json.loads(request.body)
        if not isinstance(data, list):
            return JsonResponse({'success': False, 'error': '数据格式错误'}, status=400)
            
        # 简单实现：遍历保存
        for room_data in data:
            room_id = room_data.get('id')
            defaults = {
                'name': room_data.get('name'),
                'capacity': int(room_data.get('capacity', 0)),
                'description': room_data.get('description', ''),
                'equipment': room_data.get('equipment', ''),
                'status': room_data.get('status', 'available')
            }
            
            if room_id and str(room_id).isdigit():
                Room.objects.update_or_create(id=int(room_id), defaults=defaults)
            else:
                Room.objects.create(**defaults)
                
        return JsonResponse({'success': True, 'message': '保存成功'})
    except Exception as e:
        return JsonResponse({'success': False, 'error': str(e)}, status=500)

@require_http_methods(["POST"])
def save_reservations(request):
    """保存预约数据 (新版适配座位预约)"""
    if not request.user.is_authenticated:
        return JsonResponse({'success': False, 'error': '请先登录'}, status=401)
        
    try:
        data = json.loads(request.body)
        
        # New logic: Handle single seat reservation
        if 'seat_id' in data:
            room_id = data.get('room_id')
            seat_id = data.get('seat_id')
            date_str = data.get('date')
            start_str = data.get('start_time')
            end_str = data.get('end_time')
            title = data.get('title', '自习')
            
            if not all([room_id, seat_id, date_str, start_str, end_str]):
                return JsonResponse({'status': 'error', 'message': '缺少必要参数'}, status=400)
                
            room = Room.objects.get(id=room_id)
            seat = Seat.objects.get(id=seat_id)
            date_obj = datetime.date.fromisoformat(date_str)
            start_time = datetime.datetime.strptime(start_str, '%H:%M').time()
            end_time = datetime.datetime.strptime(end_str, '%H:%M').time()
            
            # Check for conflict
            conflict = Reservation.objects.filter(
                seat=seat, # Check specific seat
                date=date_obj,
                status__in=['approved', 'pending'],
                start_time__lt=end_time,
                end_time__gt=start_time
            )
            
            if conflict.exists():
                return JsonResponse({'status': 'error', 'message': f'该座位在 {start_str}-{end_str} 时段已被预约'}, status=400)
                
            # Create
            Reservation.objects.create(
                user=request.user,
                room=room,
                seat=seat,
                date=date_obj,
                start_time=start_time,
                end_time=end_time,
                title=title,
                booker=request.user.first_name or request.user.username,
                status='approved'
            )
            
            return JsonResponse({'status': 'success', 'message': '预约成功'})
            
        # Legacy logic below...
        
        # 兼容处理：支持单条或列表
        if isinstance(data, dict) and 'reservations' in data:
            items = data['reservations']
        elif isinstance(data, list):
            items = data
        else:
            items = [data] # 尝试当做单条处理
            
        saved_count = 0
        
        for item in items:
            # 基础校验
            if not item.get('title') or not item.get('date'):
                continue
                
            room_id = item.get('room')
            room = Room.objects.get(id=room_id)
            
            date_obj = datetime.date.fromisoformat(item.get('date'))
            start_time = datetime.datetime.strptime(item.get('start_time') or item.get('start'), '%H:%M').time()
            end_time = datetime.datetime.strptime(item.get('end_time') or item.get('end'), '%H:%M').time()
            
            # 检查时间冲突 (简单的后端校验)
            conflict = Reservation.objects.filter(
                room=room,
                date=date_obj,
                status__in=['approved', 'pending'], # 只检查有效预约
                start_time__lt=end_time,
                end_time__gt=start_time
            )
            
            res_id = item.get('id')
            if res_id:
                conflict = conflict.exclude(id=res_id)
                
            if conflict.exists():
                return JsonResponse({'success': False, 'error': f'时间段冲突: {start_time}-{end_time}'}, status=400)

            # 准备数据
            defaults = {
                'room': room,
                'date': date_obj,
                'start_time': start_time,
                'end_time': end_time,
                'title': item.get('title'),
                'booker': item.get('booker') or request.user.first_name or request.user.username,
                'department': item.get('department', ''),
                'user': request.user,
                'status': 'approved' # 简化演示，直接通过
            }
            
            if res_id:
                # 更新
                try:
                    res = Reservation.objects.get(id=res_id)
                    # 只能修改自己的预约，如果不是自己的则跳过（兼容全量提交的前端）
                    if res.user != request.user and not request.user.is_staff:
                        continue
                    
                    for k, v in defaults.items():
                        setattr(res, k, v)
                    res.save()
                    send_wechat_notification(res, '修改')
                except Reservation.DoesNotExist:
                    continue # ID不存在则跳过
            else:
                # 创建
                res = Reservation.objects.create(**defaults)
                send_wechat_notification(res, '新增')
            
            saved_count += 1
            
        return JsonResponse({'success': True, 'message': f'成功保存 {saved_count} 条预约'})
        
    except Exception as e:
        logger.error(f"保存失败: {e}")
        return JsonResponse({'success': False, 'error': str(e)}, status=500)

@require_http_methods(["POST"])
def cancel_reservation(request):
    """取消预约"""
    if not request.user.is_authenticated:
        return JsonResponse({'status': 'error', 'message': '请先登录'}, status=401)
        
    try:
        data = json.loads(request.body)
        res_id = data.get('id')
        
        res = Reservation.objects.get(id=res_id)
        
        # 鉴权
        if res.user != request.user and not request.user.is_staff:
            return JsonResponse({'status': 'error', 'message': '无权操作'}, status=403)
            
        res.status = 'cancelled'
        res.save()
        
        send_wechat_notification(res, '取消')
        
        return JsonResponse({'status': 'success'})
    except Reservation.DoesNotExist:
        return JsonResponse({'status': 'error', 'message': '预约不存在'}, status=404)
    except Exception as e:
        return JsonResponse({'status': 'error', 'message': str(e)}, status=500)

@require_http_methods(["POST"])
def save_settings(request):
    if not request.user.is_staff:
        return JsonResponse({'success': False, 'error': '权限不足'}, status=403)
    # ... (保持原逻辑简化版)
    try:
        data = json.loads(request.body)
        for key, value in data.items():
            if key != 'settings': # 过滤掉可能的外层包裹
                Settings.objects.update_or_create(key=key, defaults={'value': value})
        return JsonResponse({'success': True})
    except Exception as e:
        return JsonResponse({'success': False, 'error': str(e)}, status=500)
