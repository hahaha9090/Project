
import os
import django
import sys

sys.path.append(os.getcwd())
os.environ.setdefault("DJANGO_SETTINGS_MODULE", "settings")
django.setup()

from booking.models import Room, Seat

def generate_seats():
    rooms = Room.objects.all()
    if not rooms.exists():
        print("No rooms found. Creating a default room...")
        room = Room.objects.create(
            name="第一自习室",
            category="self_study",
            capacity=30,
            description="安静明亮的自习环境，适合个人学习。"
        )
        rooms = [room]

    for room in rooms:
        print(f"Checking seats for room: {room.name}")
        if room.seats.count() > 0:
            print(f"  Room {room.name} already has {room.seats.count()} seats. Skipping.")
            continue
        
        # Generate seats (Rows A-E, Cols 1-6 => 30 seats)
        # Or based on capacity. Let's assume 6 columns.
        capacity = room.capacity if room.capacity else 30
        cols = 6
        rows = (capacity + cols - 1) // cols
        
        seats_created = 0
        row_labels = "ABCDEFGHIJ"
        
        print(f"  Generating {capacity} seats...")
        
        for r in range(rows):
            if r >= len(row_labels): break
            row_char = row_labels[r]
            for c in range(1, cols + 1):
                if seats_created >= capacity: break
                
                seat_number = f"{row_char}{c}"
                Seat.objects.create(
                    room=room,
                    number=seat_number,
                    is_active=True
                )
                seats_created += 1
                
        print(f"  Created {seats_created} seats for {room.name}.")

if __name__ == "__main__":
    generate_seats()
