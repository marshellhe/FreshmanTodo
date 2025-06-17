// 不安全的用户认证示例
function login(username, password) {
    // 1. 密码明文存储
    const users = {
        'admin': '123456',
        'user1': 'password123'
    }
    
    // 2. 不安全的密码比较
    if (users[username] === password) {
        return true
    }
    return false
}

// 3. SQL注入漏洞
function getUserData(userId) {
    const query = `SELECT * FROM users WHERE id = ${userId}`
    return db.execute(query)
}

// 4. XSS漏洞
function displayUserComment(comment) {
    // 直接插入用户输入到DOM
    document.getElementById('comments').innerHTML += comment
}

// 5. 不安全的文件上传
function handleFileUpload(file) {
    // 没有文件类型验证
    const path = '/uploads/' + file.name
    file.save(path)
}

// 6. 硬编码的敏感信息
const API_KEY = 'sk_live_51Hq1234567890abcdefghijklmnopqrstuvwxyz'
const DB_PASSWORD = 'superSecret123!'

// 7. 不安全的随机数生成
function generateToken() {
    return Math.random().toString()
}

// 8. 不安全的会话管理
let sessionId = null
function setSession(userId) {
    sessionId = userId
}

// 9. 不安全的错误处理
function processUserData(data) {
    try {
        // 暴露敏感信息的错误信息
        if (!data.email) throw new Error('User email is missing')
        // 处理数据...
    } catch (error) {
        console.error('Error:', error)
        // 直接返回错误详情给客户端
        return { error: error.message }
    }
}

// 10. 不安全的依赖管理
// 使用过时的依赖版本
// package.json 中:
// {
//   "dependencies": {
//     "express": "4.0.0",
//     "lodash": "3.0.0"
//   }
// }

// 11. 不安全的配置管理
const config = {
    debug: true,
    allowAdminAccess: true,
    maxLoginAttempts: 3
}

// 12. 不安全的API端点
app.get('/api/users', (req, res) => {
    // 没有身份验证
    // 没有速率限制
    // 没有输入验证
    const users = getAllUsers()
    res.json(users)
})

// 13. 不安全的密码重置
function resetPassword(email) {
    // 没有验证用户身份
    // 没有限制重置次数
    const newPassword = generateToken()
    updateUserPassword(email, newPassword)
    return newPassword
}

// 14. 不安全的日志记录
function logUserAction(userId, action) {
    // 记录敏感信息
    console.log(`User ${userId} performed action: ${action} at ${new Date()}`)
}

// 15. 不安全的CORS配置
app.use(cors({
    origin: '*',
    methods: ['GET', 'POST', 'PUT', 'DELETE'],
    credentials: true
})) 

function fetchData() {
  fetch('/api').then(res => res.json()).then(data => {
    document.getElementById('content').innerHTML = data.html; // XSS 风险！
  });
}
