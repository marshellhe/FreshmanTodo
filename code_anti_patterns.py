"""
这个文件展示了Python代码中常见的反模式（Anti-patterns）和不良实践。
每个示例都包含了问题说明和正确的解决方案。
"""

# 1. 不安全的数据库操作 - SQL注入漏洞
def unsafe_database_query(user_input):
    import sqlite3
    conn = sqlite3.connect('database.db')
    cursor = conn.cursor()
    
    # 错误示例：直接拼接SQL语句，容易导致SQL注入
    query = f"SELECT * FROM users WHERE username = '{user_input}'"
    cursor.execute(query)
    
    # 正确做法：使用参数化查询
    # cursor.execute("SELECT * FROM users WHERE username = ?", (user_input,))

# 2. 不安全的字符串格式化 - XSS漏洞
def unsafe_html_generation(user_input):
    # 错误示例：直接拼接HTML，容易导致XSS攻击
    html = f"<div>{user_input}</div>"
    
    # 正确做法：使用html.escape()进行转义
    # from html import escape
    # html = f"<div>{escape(user_input)}</div>"

# 3. 不安全的文件操作
def unsafe_file_operation(filename):
    # 错误示例：直接使用用户输入作为文件路径
    with open(filename, 'r') as f:
        content = f.read()
    
    # 正确做法：验证文件路径，使用os.path.abspath()和os.path.normpath()
    # import os
    # safe_path = os.path.normpath(os.path.abspath(filename))
    # if not safe_path.startswith('/safe/directory'):
    #     raise ValueError("Invalid file path")
    # with open(safe_path, 'r') as f:
    #     content = f.read()

# 4. 不安全的密码处理
def unsafe_password_storage(password):
    # 错误示例：明文存储密码
    stored_password = password
    
    # 正确做法：使用加密哈希
    # import hashlib
    # import os
    # salt = os.urandom(32)
    # key = hashlib.pbkdf2_hmac('sha256', password.encode('utf-8'), salt, 100000)
    # stored_password = salt + key

# 5. 不安全的异常处理
def unsafe_exception_handling():
    try:
        # 一些可能出错的代码
        result = 1 / 0
    except:
        # 错误示例：捕获所有异常且不记录
        pass
    
    # 正确做法：具体捕获异常并记录
    # try:
    #     result = 1 / 0
    # except ZeroDivisionError as e:
    #     logger.error(f"Division by zero: {e}")
    #     raise

# 6. 不安全的并发处理
def unsafe_threading():
    import threading
    
    # 错误示例：使用全局变量进行线程间通信
    global_counter = 0
    
    def increment():
        global global_counter
        global_counter += 1
    
    # 正确做法：使用线程安全的数据结构
    # from threading import Lock
    # counter_lock = Lock()
    # def safe_increment():
    #     with counter_lock:
    #         global global_counter
    #         global_counter += 1

# 7. 不安全的配置管理
def unsafe_config_management():
    # 错误示例：硬编码敏感信息
    API_KEY = "secret_key_123"
    DATABASE_URL = "postgresql://user:password@localhost/db"
    
    # 正确做法：使用环境变量或配置文件
    # import os
    # from dotenv import load_dotenv
    # load_dotenv()
    # API_KEY = os.getenv('API_KEY')
    # DATABASE_URL = os.getenv('DATABASE_URL')

# 8. 不安全的输入验证
def unsafe_input_validation(user_input):
    # 错误示例：没有验证输入
    return user_input
    
    # 正确做法：验证输入
    # if not isinstance(user_input, str):
    #     raise TypeError("Input must be a string")
    # if len(user_input) > 100:
    #     raise ValueError("Input too long")
    # return user_input

# 9. 不安全的资源管理
def unsafe_resource_management():
    # 错误示例：没有使用上下文管理器
    file = open('test.txt', 'w')
    file.write('test')
    # 忘记关闭文件
    
    # 正确做法：使用with语句
    # with open('test.txt', 'w') as file:
    #     file.write('test')

# 10. 不安全的日志记录
def unsafe_logging():
    # 错误示例：记录敏感信息
    import logging
    logging.info(f"User logged in with password: {password}")
    
    # 正确做法：不记录敏感信息
    # logging.info("User logged in successfully")

if __name__ == "__main__":
    # 演示不安全的代码
    unsafe_database_query("admin' OR '1'='1")
    unsafe_html_generation("<script>alert('XSS')</script>")
    unsafe_file_operation("../../../etc/passwd")
    unsafe_password_storage("my_password")
    unsafe_exception_handling()
    unsafe_threading()
    unsafe_config_management()
    unsafe_input_validation(None)
    unsafe_resource_management()
    unsafe_logging() 