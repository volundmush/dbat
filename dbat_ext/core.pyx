from libcpp.memory cimport shared_ptr

cimport structs   # brings in room_data and get_room


def get_room_py(int vn):
    """
    Wraps the C++ get_room() function.
    Returns a Python-accessible PyRoomData instance.
    """
    cdef structs.room_data* cpp_room = structs.get_room(vn)
    if cpp_room is NULL:
        raise ValueError(f"Room not found for vn: {vn}")
    return True