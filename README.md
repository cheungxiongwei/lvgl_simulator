# lvgl 依赖
Python3

```
pip install pcpp
```

```
CMakeLists.txt

# copy lv_conf_template.h to project root directory
# rename lv_conf_template.h to lv_conf.h

# set LV_COLOR_DEPTH 32
# set LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB
# set LV_USE_OS LV_OS_WINDOWS
# set LV_USE_FS_STDIO 1
    LV_FS_STDIO_LETTER 'C'
    LV_FS_STDIO_PATH 'C'

set(LV_CONF_DIR ${CMAKE_CURRENT_LIST_DIR})
add_subdirectory(lvgl) 
```