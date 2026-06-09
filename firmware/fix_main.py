#!/usr/bin/env python3
"""修复main_master_v3.c的构建问题"""

import re

with open("main_master_v3.c", "r") as f:
    content = f.read()

# 1. 添加stdio.h
if '#include <stdio.h>' not in content:
    content = content.replace('#include <string.h>', '#include <string.h>\n#include <stdio.h>')
    print("Added stdio.h")

# 2. 添加额外的头文件
old_includes = '#include "ui_manager.h"'
new_includes = '''#include "button_defs.h"
#include "irq_wrapper.h"
#include "stubs.h"
#include "ui_manager.h"
#include "timer_driver.h"'''

if 'button_defs.h' not in content:
    content = content.replace(old_includes, new_includes)
    print("Added extra headers")

# 3. 修复delay_ms调用 - 改为timer_delay_ms
content = content.replace('delay_ms(', 'timer_delay_ms(')
print("Fixed delay_ms calls")

with open("main_master_v3.c", "w") as f:
    f.write(content)

print("Fixed main_master_v3.c successfully!")
